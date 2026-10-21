"""Prepare local production artifacts only. Never flashes, tags, or publishes."""
from __future__ import annotations
import argparse
import datetime
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import subprocess
import zipfile
import neurocube as bridge

ROOT = bridge.ROOT


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()


def digest(file):
    return hashlib.sha256(file.read_bytes()).hexdigest()


def build(directory, native, env, log):
    cmake = bridge.TOOLS / 'venv/Scripts/cmake.exe'
    args = [cmake, '--fresh', '-S', ROOT, '-B', directory, '-G', 'Ninja',
            '-DCMAKE_BUILD_TYPE=Release']
    if native:
        compiler = next((bridge.TOOLS/'llvm').glob('*/bin'))
        args += [f'-DCMAKE_C_COMPILER={compiler/"clang.exe"}',
                 f'-DCMAKE_CXX_COMPILER={compiler/"clang++.exe"}']
    else:
        args += ['-DNEUROCUBE_PICO=ON', '-DNEUROCUBE_DEBUG_INPUT=OFF',
                 f'-DPICO_TOOLCHAIN_PATH={bridge.TOOLS/"arm"}',
                 f'-Dpicotool_DIR={bridge.TOOLS/"picotool/picotool"}']
    with log.open('w', encoding='utf-8') as stream:
        for command in [args, [cmake, '--build', directory, '--target',
                               'neurocube_sim' if native else 'neurocube', '-j', '4']]:
            try:
                bridge.run(command, env=env, stdout=stream, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError:
                print(f'Build failed; inspect {log}', flush=True)
                raise
    print(f'Built {directory.name}', flush=True)


def archive(path, entries, epoch):
    # Stable file order, timestamps and permissions, independent of checkout time.
    stamp = datetime.datetime.fromtimestamp(max(epoch, 315532800), datetime.timezone.utc)
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as output:
        for name, source in sorted(entries.items()):
            info = zipfile.ZipInfo(name, stamp.timetuple()[:6])
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            output.writestr(info, source.read_bytes())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'artifacts/release-1.0.0')
    parser.add_argument('--verify-reproducible', action='store_true')
    args = parser.parse_args()
    if git('status', '--porcelain', '--untracked-files=normal'):
        raise bridge.BridgeError('Commit the candidate first; worktree must be clean')
    source = git('rev-parse', 'HEAD')
    version = re.search(r'project\(neurocube VERSION ([0-9.]+)',
                        (ROOT/'CMakeLists.txt').read_text()).group(1)
    if version != '1.0.0':
        raise bridge.BridgeError('This frozen release procedure prepares 1.0.0 only')
    # A release metadata change must not silently alter the accepted gameplay/HAL.
    frozen = []
    for directory in ('src/domain', 'src/application', 'src/games', 'src/presentation', 'include'):
        frozen += [f for f in (ROOT/directory).rglob('*') if f.is_file()]
    frozen += [ROOT/'src/infrastructure'/name for name in
               ('audio.cpp', 'board.cpp', 'display_power.cpp')]
    for file in frozen:
        name = file.relative_to(ROOT).as_posix()
        old = subprocess.check_output(['git', 'show', 'v0.3.5-rc1:'+name], cwd=ROOT)
        # Git stores LF; Windows checkout may have CRLF.
        if old.replace(b'\r\n', b'\n') != file.read_bytes().replace(b'\r\n', b'\n'):
            raise bridge.BridgeError('Frozen functionality changed: '+name)
    out = args.output.resolve()
    if not out.is_relative_to((ROOT/'artifacts').resolve()):
        raise bridge.BridgeError('Release output must stay under ignored artifacts/')
    out.mkdir(parents=True, exist_ok=True)
    env = bridge.tool_environment()
    epoch = int(env['SOURCE_DATE_EPOCH'])
    builds = {}
    for native in (False, True):
        kind = 'native' if native else 'pico'
        first = out / ('build-'+kind+'-a')
        build(first, native, env, out/(kind+'-a.log'))
        names = ['neurocube_sim.exe'] if native else ['neurocube.uf2', 'neurocube.elf', 'neurocube.bin']
        if args.verify_reproducible:
            second = out / ('build-'+kind+'-b')
            build(second, native, env, out/(kind+'-b.log'))
            for name in names:
                if digest(first/name) != digest(second/name):
                    raise bridge.BridgeError('Independent builds differ: '+name)
        builds[kind] = first
    for file in [builds['pico']/('neurocube.'+ext) for ext in ('uf2','elf','bin')] + [builds['native']/'neurocube_sim.exe']:
        if re.search(rb'(?i)[A-Z]:[\\/](?:Users|git)[\\/]', file.read_bytes()):
            raise bridge.BridgeError('Developer path remains in release binary: '+file.name)
    if git('rev-parse', 'HEAD') != source or git('status', '--porcelain', '--untracked-files=normal'):
        raise bridge.BridgeError('Candidate changed during build')
    cache = (builds['pico']/'CMakeCache.txt').read_text()
    if 'NEUROCUBE_DEBUG_INPUT:BOOL=OFF' not in cache:
        raise bridge.BridgeError('Production debug injection must be OFF')
    uf2 = bridge.validate_uf2(builds['pico']/'neurocube.uf2')
    dist = out/'dist'; dist.mkdir(exist_ok=True)
    allowed = {f'neurocube-{version}.{x}' for x in ('uf2','elf','bin','zip')}
    allowed |= {f'neurocube-{version}-windows.zip',f'neurocube-{version}-source.zip','manifest.json','SHA256SUMS.txt'}
    if any(file.name not in allowed for file in dist.iterdir()):
        raise bridge.BridgeError('Unexpected file in dist; use a fresh output directory')
    # Package committed canonical bytes, not checkout-dependent CRLF conversions.
    canonical=out/'canonical-source'; canonical.mkdir(exist_ok=True)
    source_zip=subprocess.check_output(['git','archive','--format=zip',source],cwd=ROOT)
    entries={}
    with zipfile.ZipFile(io.BytesIO(source_zip)) as archived:
        for info in archived.infolist():
            if info.is_dir(): continue
            target=canonical/info.filename
            if not target.resolve().is_relative_to(canonical.resolve()):
                raise bridge.BridgeError('Unsafe source archive path')
            target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(archived.read(info)); entries[info.filename]=target
    for suffix in ('uf2', 'elf', 'bin'):
        name = f'neurocube-{version}.{suffix}'
        shutil.copy2(builds['pico']/('neurocube.'+suffix), dist/name)
        entries[name] = dist/name
    entries['neurocube_sim.exe'] = builds['native']/'neurocube_sim.exe'
    for file in (canonical/'assets').rglob('*'):
        if file.is_file(): entries['licenses/'+file.relative_to(canonical/'assets').as_posix()] = file
    # Preserve the exact nonstandard notice; root MIT must not replace it.
    notice = (canonical/'third_party/waveshare_audio/es8311.c').read_bytes().split(b'#include', 1)[0]
    (out/'ES8311-file-notice.txt').write_bytes(notice)
    entries['licenses/ES8311-file-notice.txt'] = out/'ES8311-file-notice.txt'
    entries['licenses/ES8311-upstream-LICENSE.txt'] = canonical/'third_party/waveshare_audio/LICENSE'
    manifest = {
        'version':version, 'source_commit':source, 'source_date_epoch':epoch,
        'status':'PREPARED_NOT_PUBLISHED',
        'public_readiness':'BLOCKED: ES8311 file-level licensing unresolved; physical acceptance pending',
        'debug_injection':False, 'frozen_files_compared_to_0_3_5':len(frozen),
        'independent_builds_identical':args.verify_reproducible, 'uf2':uf2,
        'files':{name:{'size':file.stat().st_size,'sha256':digest(file)} for name,file in sorted(entries.items())}}
    (dist/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    entries['manifest.json'] = dist/'manifest.json'
    archive(dist/f'neurocube-{version}.zip', entries, epoch)
    windows = {name:file for name,file in entries.items() if name not in
               (f'neurocube-{version}.uf2',f'neurocube-{version}.elf',f'neurocube-{version}.bin','manifest.json')}
    windows_manifest = dict(manifest)
    windows_manifest['files'] = {name:{'size':file.stat().st_size,'sha256':digest(file)} for name,file in sorted(windows.items())}
    (out/'windows-manifest.json').write_text(json.dumps(windows_manifest,indent=2)+'\n')
    windows['manifest.json'] = out/'windows-manifest.json'
    archive(dist/f'neurocube-{version}-windows.zip', windows, epoch)
    subprocess.run(['git','archive','--format=zip','--prefix=neurocube-'+version+'/',
                    '--output='+str(dist/f'neurocube-{version}-source.zip'),source],cwd=ROOT,check=True)
    if args.verify_reproducible:
        for name, payload in [(f'neurocube-{version}.zip', entries),
                              (f'neurocube-{version}-windows.zip', windows)]:
            repeated=out/(name+'.repeated')
            archive(repeated,payload,epoch)
            if digest(repeated)!=digest(dist/name):
                raise bridge.BridgeError('Archive is not reproducible: '+name)
        repeated=out/'source.zip.repeated'
        subprocess.run(['git','archive','--format=zip','--prefix=neurocube-'+version+'/',
                        '--output='+str(repeated),source],cwd=ROOT,check=True)
        if digest(repeated)!=digest(dist/f'neurocube-{version}-source.zip'):
            raise bridge.BridgeError('Source archive is not reproducible')
    sums = ''.join(digest(file)+'  '+file.name+'\n' for file in sorted(dist.iterdir())
                   if file.is_file() and file.name!='SHA256SUMS.txt')
    (dist/'SHA256SUMS.txt').write_text(sums, encoding='ascii')
    print(json.dumps({'prepared':str(dist),'source_commit':source,'version':version,
                      'uf2':uf2,'reproducible':args.verify_reproducible}),flush=True)


if __name__ == '__main__':
    main()

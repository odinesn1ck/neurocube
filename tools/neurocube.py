"""NeuroCube bridge. No port is opened during detect. Flash always requires --allow-flash."""
from __future__ import annotations
import argparse
import contextlib
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
TOOLS = Path(os.environ.get('NEUROCUBE_TOOLS', Path.home() / 'neurocube-tools'))
DEFAULT_SERIAL = '419CB819454EA93D'
STORAGE_START = 0x10FFE000
FAMILY_ARM_S = 0xE48BFF59

class BridgeError(RuntimeError):
    pass

def emit(event, **data):
    print(json.dumps({'event': event, **data}, ensure_ascii=False), flush=True)

def run(args, **kwargs):
    return subprocess.run([str(x) for x in args], check=True, **kwargs)

def ports():
    from serial.tools.list_ports import comports
    return [p for p in comports() if p.vid == 0x2E8A and p.pid == 0x0009]

def select_port(serial):
    matches = [p for p in ports() if p.serial_number == serial]
    if len(matches) != 1:
        raise BridgeError(f'Expected one CDC for serial {serial}; found {len(matches)}')
    return matches[0]

def pnp():
    if os.name != 'nt':
        raise BridgeError('PnP identity checks currently require Windows')
    script = r"""$ErrorActionPreference='Stop'; $nodes=@(Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match '^USB\\VID_2E8A&PID_(0009|000F)\\' }); $rows=@(foreach($n in $nodes){ $loc=(Get-PnpDeviceProperty -InstanceId $n.InstanceId -KeyName DEVPKEY_Device_LocationPaths).Data; [pscustomobject]@{instance=$n.InstanceId;location=@($loc)[0];status=$n.Status} }); ConvertTo-Json -InputObject $rows -Compress"""
    r = run(['powershell', '-NoProfile', '-Command', script], capture_output=True, text=True)
    return json.loads(r.stdout or '[]')

def choose_node(nodes, serial, pid):
    candidates = [n for n in nodes if f'PID_{pid}\\' in n['instance'].upper()
                  and n['instance'].split('\\')[-1] == serial]
    if len(candidates) != 1:
        raise BridgeError(f'Expected one USB root for {serial} PID {pid}; found {len(candidates)}')
    return candidates[0]

@contextlib.contextmanager
def exclusive(serial):
    # OS releases the lock after crashes. One bridge owns both CDC and flash.
    safe = hashlib.sha256(serial.encode()).hexdigest()[:20]
    with open(Path(tempfile.gettempdir()) / f'neurocube-{safe}.lock', 'a+b') as f:
        f.seek(0, 2)
        if not f.tell():
            f.write(b'0'); f.flush()
        f.seek(0)
        if os.name == 'nt':
            import msvcrt
            try:
                msvcrt.locking(f.fileno(), msvcrt.LK_NBLCK, 1)
            except OSError as e:
                raise BridgeError('Another bridge owns this device') from e
        else:
            import fcntl
            try:
                fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except OSError as e:
                raise BridgeError('Another bridge owns this device') from e
        try:
            yield
        finally:
            if os.name == 'nt':
                f.seek(0); msvcrt.locking(f.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(f, fcntl.LOCK_UN)

class Connection:
    def __init__(self, serial, timeout=5):
        self.serial = serial
        self.timeout = timeout
        self.port = None
        self.lock = None
        self._rx = bytearray()
    def __enter__(self):
        import serial
        self.lock = exclusive(self.serial)
        self.lock.__enter__()
        try:
            selected = select_port(self.serial)
            self.port = serial.Serial(port=None, baudrate=115200, timeout=.2, write_timeout=2)
            self.port.dtr = True
            self.port.rts = False
            self.port.port = selected.device
            self.port.open()
            self.port.reset_input_buffer()
            return self
        except Exception:
            self.__exit__(*sys.exc_info())
            raise
    def __exit__(self, *args):
        if self.port:
            self.port.close()
        if self.lock:
            self.lock.__exit__(*args)
    def read(self, deadline=None):
        deadline = deadline or time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            newline=self._rx.find(b'\n')
            if newline>=0:
                line=bytes(self._rx[:newline]);del self._rx[:newline+1]
                try:obj=json.loads(line)
                except (ValueError,UnicodeDecodeError):continue
                if isinstance(obj,dict) and obj.get('v')==1:return obj
                continue
            if len(self._rx)>4096:raise BridgeError('Protocol line exceeds limit')
            waiting=getattr(self.port,'in_waiting',0)
            self._rx.extend(self.port.read(max(1,min(waiting,4096))))
        raise BridgeError('Timeout: firmware may not support NeuroCube protocol')
    def request(self, command):
        if '\n' in command or '\r' in command:
            raise BridgeError('Invalid protocol command')
        self.port.write(('NC1 ' + command + '\n').encode('ascii'))
        self.port.flush()
        return self.read()
    def hello(self):
        msg = self.request('hello')
        if msg.get('type') != 'hello' or msg.get('product') != 'NeuroCube' or msg.get('serial') != self.serial:
            raise BridgeError('Firmware identity mismatch or unsupported protocol')
        return msg

def validate_uf2(path):
    data = Path(path).read_bytes()
    if not data or len(data) % 512:
        raise BridgeError('Invalid UF2 length')
    total = len(data)//512
    seen = set(); addresses = set(); workaround = 0
    normal_count = None
    for i in range(total):
        block = data[i*512:(i+1)*512]
        m0,m1,flags,address,size,index,count,family = struct.unpack_from('<8I',block)
        if (m0,m1,struct.unpack_from('<I',block,508)[0]) != (0x0A324655,0x9E5D5157,0x0AB16F30):
            raise BridgeError('UF2 magic mismatch')
        if (flags, family, address, size, index, count) == (0xA000, 0xE48BFF57, 0x100FFF00, 256, 0, 2):
            if struct.unpack_from('<I', block, 288)[0] != 0x9957E304 or block[32:288] != bytes([0xEF])*256:
                raise BridgeError('Invalid RP2350-E10 workaround block')
            workaround += 1
            if workaround > 1: raise BridgeError('Duplicate workaround block')
            continue
        if flags != 0x2000 or family != FAMILY_ARM_S or size != 256 or address%256:
            raise BridgeError('Expected plain RP2350 ARM Secure UF2 blocks')
        if normal_count is None: normal_count = count
        if count != normal_count or index >= count or index in seen:
            raise BridgeError('UF2 block sequence invalid')
        if not (0x10000000 <= address < STORAGE_START) or address+size > STORAGE_START or address in addresses or 0x100FF000 <= address < 0x10100000:
            raise BridgeError('UF2 overlaps records, is out of flash range, or has duplicate addresses')
        seen.add(index); addresses.add(address)
    if normal_count is None or len(seen) != normal_count or normal_count + workaround != total:
        raise BridgeError("Incomplete UF2 image")
    if 0x10000000 not in addresses:
        raise BridgeError('UF2 has no flash start block')
    return {'sha256':hashlib.sha256(data).hexdigest(),'blocks':total,'size':len(data)}

def picotool():
    exe = TOOLS/'picotool/picotool/picotool.exe'
    if not exe.exists():
        raise BridgeError('picotool missing; run tools/bootstrap.ps1')
    return exe

def tool_environment():
    env=os.environ.copy()
    bins=[str(TOOLS/'venv/Scripts'),str(TOOLS/'arm/bin')]
    native=list((TOOLS/'llvm').glob('*/bin'))
    if native:
        bins.append(str(native[0]))
    env['PATH']=os.pathsep.join(bins+[env.get('PATH','')])
    env['PICO_SDK_PATH']=str(TOOLS/'pico-sdk')
    return env

def build(native=False,debug=False):
    env=tool_environment()
    cmake=TOOLS/'venv/Scripts/cmake.exe'
    out=ROOT/('build-native' if native else 'build-pico-debug' if debug else 'build-pico')
    args=[cmake,'--fresh','-S',ROOT,'-B',out,'-G','Ninja','-DCMAKE_BUILD_TYPE=Debug' if native else '-DCMAKE_BUILD_TYPE=Release']
    if native:
        bins=list((TOOLS/'llvm').glob('*/bin'))
        if not bins:raise BridgeError('LLVM-MinGW missing')
        args += [f'-DCMAKE_C_COMPILER={bins[0]/"clang.exe"}',f'-DCMAKE_CXX_COMPILER={bins[0]/"clang++.exe"}']
    else:
        args += ['-DNEUROCUBE_PICO=ON',f'-DPICO_TOOLCHAIN_PATH={TOOLS/"arm"}',f'-Dpicotool_DIR={TOOLS/"picotool/picotool"}',f'-DNEUROCUBE_DEBUG_INPUT={"ON" if debug else "OFF"}']
    run(args,env=env);run([cmake,'--build',out,'-j','4'],env=env)
    if native:
        run([TOOLS/'venv/Scripts/ctest.exe','--test-dir',out,'--output-on-failure'],env=env)
    else:
        emit('uf2_built',path=str(out/'neurocube.uf2'),**validate_uf2(out/'neurocube.uf2'))

def flash(path,serial,allowed):
    if not allowed:raise BridgeError('Flash requires --allow-flash and prior user authorization')
    image=validate_uf2(path);emit('image_validated',**image)
    with exclusive(serial):
        nodes=pnp()
        # Accept an already booted, exact-serial BOOTSEL device for recovery.
        boot=[n for n in nodes if 'PID_000F\\' in n['instance'].upper() and n['instance'].split('\\')[-1]==serial]
        if boot:
            original=choose_node(nodes,serial,'000F')
        else:
            original=choose_node(nodes,serial,'0009')
            run([picotool(),'reboot','-u','--ser',serial,'-f'])
        deadline=time.monotonic()+15
        while True:
            nodes=pnp()
            matches=[n for n in nodes if 'PID_000F\\' in n['instance'].upper() and n['location']==original['location'] and n['status']=='OK']
            if len(matches)==1:break
            if time.monotonic()>deadline:raise BridgeError('BOOTSEL identity/topology check failed; no write attempted')
            time.sleep(.3)
        boot_serial=matches[0]['instance'].split('\\')[-1]
        emit('bootsel_matched',application_serial=serial,boot_serial=boot_serial,location=matches[0]['location'])
        run([picotool(),'info','-a','--ser',boot_serial])
        run([picotool(),'load','-v',Path(path).resolve(),'--ser',boot_serial])
        run([picotool(),'reboot','--ser',boot_serial])
    deadline=time.monotonic()+20
    while True:
        try:
            with Connection(serial) as c:
                msg=c.hello();emit('firmware_connected',**msg)
            return
        except (BridgeError,OSError):
            if time.monotonic()>deadline:raise BridgeError('Write verified, but runtime handshake failed; use recovery instructions')
            time.sleep(.5)

def screenshot(c,path):
    first=c.request('screenshot')
    if first.get('type')!='frame_begin':raise BridgeError('Framebuffer unsupported')
    if (first.get('width'),first.get('height'),first.get('format'))!=(240,240,'rgb565be_hex'):raise BridgeError('Unexpected framebuffer format')
    rows={};deadline=time.monotonic()+30
    while True:
        msg=c.read(deadline)
        if msg.get('type')=='frame_end':break
        if msg.get('type')!='frame_row':raise BridgeError('Unexpected frame packet')
        y=msg.get('y');data=bytes.fromhex(msg.get('data',''))
        if not isinstance(y,int) or not 0<=y<240 or y in rows or len(data)!=480:raise BridgeError('Invalid framebuffer row')
        rows[y]=data
    if len(rows)!=240:raise BridgeError('Incomplete frame')
    from PIL import Image
    image=Image.new('RGB',(240,240))
    for y in range(240):
        for x in range(240):
            p=int.from_bytes(rows[y][2*x:2*x+2],'big');image.putpixel((x,y),(((p>>11)&31)*255//31,((p>>5)&63)*255//63,(p&31)*255//31))
    image.save(path);emit('framebuffer_saved',path=str(path),source='firmware framebuffer; not a photo of LCD')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial',default=DEFAULT_SERIAL)
    sub=parser.add_subparsers(dest='cmd',required=True)
    sub.add_parser('detect');sub.add_parser('info');sub.add_parser('selftest');sub.add_parser('records');sub.add_parser('power');sub.add_parser('settings')
    b=sub.add_parser('build');b.add_argument('--native',action='store_true');b.add_argument('--debug',action='store_true')
    f=sub.add_parser('flash');f.add_argument('image',type=Path);f.add_argument('--allow-flash',action='store_true')
    l=sub.add_parser('logs');l.add_argument('--seconds',type=float,default=10)
    s=sub.add_parser('screenshot');s.add_argument('output',type=Path)
    args=parser.parse_args()
    if args.cmd=='detect':
        emit('detected',cdc=[{'port':p.device,'vid':p.vid,'pid':p.pid,'serial':p.serial_number,'location':p.location} for p in ports()],usb=pnp());return
    if args.cmd=='build':build(args.native,args.debug);return
    if args.cmd=='flash':flash(args.image,args.serial,args.allow_flash);return
    if args.cmd=='logs':
        deadline=time.monotonic()+args.seconds
        while time.monotonic()<deadline:
            try:
                with Connection(args.serial) as c:
                    c.hello()
                    while time.monotonic()<deadline:
                        emit('telemetry',**c.request('status'));time.sleep(.5)
            except (BridgeError,OSError) as e:
                emit('reconnecting',reason=str(e));time.sleep(.5)
        return
    with Connection(args.serial) as c:
        hello=c.hello()
        if args.cmd=='info':emit('hello',**hello);emit('status',**c.request('status'))
        elif args.cmd=='selftest':
            result=c.request('selftest');emit('selftest',**result)
            if result.get('type')!='selftest' or not all(result.get(k) is True for k in ('imu_identity','imu_sample','touch_identity','lcd_spi_sent')):raise BridgeError('Hardware electronic selftest did not pass')
        elif args.cmd in ('records','power','settings'):emit(args.cmd,**c.request(args.cmd))
        elif args.cmd=='screenshot':screenshot(c,args.output)

if __name__=='__main__':
    try:main()
    except (BridgeError,subprocess.CalledProcessError,OSError,ValueError) as error:
        emit('error',message=str(error));sys.exit(1)

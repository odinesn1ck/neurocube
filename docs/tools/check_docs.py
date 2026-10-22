"""Dependency-free check of repository Markdown destinations, anchors and structure."""
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[2]
REQUIRED = ('README.md', 'AGENTS.md', 'CHANGELOG.md', 'LICENSE', 'CONTRIBUTING.md', 'THIRD_PARTY_LICENSES.md', 'docs/product-vision.md',
            'docs/documentation-policy.md', 'docs/architecture.md', 'docs/hardware.md',
            'docs/backlog.md', 'docs/progress.md', 'docs/tasks/README.md', 'docs/evidence/README.md')


def prose(text):
    return re.sub(r'```.*?```', '', text, flags=re.S)


def anchors(path):
    result = set()
    counts = {}
    for line in prose(path.read_text(encoding='utf-8-sig')).splitlines():
        if not re.match(r'^#{1,6}\s', line):
            continue
        title = re.sub(r'^#+\s*', '', line).strip().strip('#').strip()
        title = re.sub(r'\[([^]]+)\]\([^)]*\)', r'\1', title)
        slug = re.sub(r'[^\w\- ]', '', title.lower()).replace(' ', '-')
        count = counts.get(slug, 0)
        counts[slug] = count + 1
        result.add(slug + (f'-{count}' if count else ''))
    return result


def main():
    errors = []
    links = 0
    files = sorted(ROOT.glob('*.md')) + sorted((ROOT/'docs').rglob('*.md'))
    for required in REQUIRED:
        if not (ROOT/required).is_file():
            errors.append('Missing required document: '+required)
    for path in files:
        text = prose(path.read_text(encoding='utf-8-sig'))
        for target in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)', text):
            target = target.strip().split(' "', 1)[0].strip('<>')
            parsed = urlsplit(target)
            if parsed.scheme or target.startswith('//'):
                continue
            links += 1
            dest = (path.parent/unquote(parsed.path)).resolve() if parsed.path else path
            if not dest.is_relative_to(ROOT):
                errors.append(f'{path.relative_to(ROOT)}: destination outside repository: {target}')
            elif not dest.exists():
                errors.append(f'{path.relative_to(ROOT)}: missing destination: {target}')
            elif parsed.fragment and dest.suffix.lower() == '.md' and unquote(parsed.fragment) not in anchors(dest):
                errors.append(f'{path.relative_to(ROOT)}: missing anchor: {target}')
    for name in ('README.md', 'docs/product-vision.md'):
        text = prose((ROOT/name).read_text(encoding='utf-8'))
        if re.search(r'(?m)^.*(?:P0\b|VERIFIED_ON_HARDWARE|COMPILED_ONLY|/pull/\d+)', text):
            errors.append(name+': operational status or PR in strategic document')
    if errors:
        print('\n'.join(errors))
        return 1
    print(f'PASS: {len(files)} Markdown files, {links} local links/anchors, {len(REQUIRED)} required documents')
    return 0


if __name__ == '__main__':
    sys.exit(main())

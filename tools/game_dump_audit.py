"""Read-only audit of extracted Wii game executables, selectors and ARK v6 indexes."""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def selector_names(path):
    data = path.read_bytes()
    start, size, names = struct.unpack_from('>III', data, 0x40)
    if size % 16 or start + size > len(data) or names >= len(data):
        raise ValueError('Invalid RSO export table')
    result = []
    for offset in range(start, start + size, 16):
        pos = names + struct.unpack_from('>I', data, offset)[0]
        end = data.index(0, pos)
        result.append(data[pos:end].decode('ascii'))
    return sorted(result)


def archive_names(path):
    encrypted = path.read_bytes()
    if len(encrypted) > 32 * 1024 * 1024:
        raise ValueError('Archive index exceeds audit size limit')
    seed = abs(struct.unpack_from('<i', encrypted)[0]) or 1
    data = bytearray()
    for byte in encrypted[4:]:
        value = (seed % 127773) * 16807 - (seed // 127773) * 2836
        seed = value if value > 0 else value + 2147483647
        data.append(byte ^ (seed & 255))
    pos = 0

    def take(size):
        nonlocal pos
        if size < 0 or pos + size > len(data):
            raise ValueError('Truncated archive index')
        value = bytes(data[pos:pos + size])
        pos += size
        return value

    def integer():
        return struct.unpack('<I', take(4))[0]

    if integer() != 6 or integer() != 1:
        raise ValueError('Unsupported archive or GUID revision')
    take(16)
    count = integer()
    if integer() != count:
        raise ValueError('Inconsistent archive size count')
    take(count * 4)
    if integer() != count:
        raise ValueError('Inconsistent archive filename count')
    for _ in range(count):
        take(integer()).decode('ascii')
    if integer() != count:
        raise ValueError('Inconsistent archive priority count')
    take(count * 4)
    heap = take(integer())
    table = [integer() for _ in range(integer())]
    entries = integer()
    paths = []
    for _ in range(entries):
        _, name, directory, _, _ = struct.unpack('<QIIII', take(24))
        def string(index):
            offset = table[index]
            return heap[offset:heap.index(0, offset)].decode('ascii')
        paths.append('/'.join(filter(None, (string(directory), string(name)))))
    if pos != len(data):
        raise ValueError('Unexpected trailing archive index bytes')
    return paths


def audit(root, reference=None):
    if not root.is_dir():
        raise ValueError('Extracted game folder does not exist: ' + str(root))
    files = sorted(p for p in root.rglob('*') if p.is_file())
    symbol_extensions = {'.elf', '.map', '.pdb', '.sym', '.dbg'}
    report = {'root': str(root.resolve()), 'executables': [], 'selectors': [],
              'loose_symbol_candidates': [], 'archive_indexes': []}
    for path in files:
        relative = path.relative_to(root).as_posix()
        if path.name == 'main.dol':
            report['executables'].append({'path': relative, 'bytes': path.stat().st_size,
                'sha1': hashlib.sha1(path.read_bytes()).hexdigest()})
        if path.suffix.lower() in symbol_extensions:
            report['loose_symbol_candidates'].append(relative)
        if path.suffix.lower() == '.sel':
            names = selector_names(path)
            item = {'path': relative, 'export_count': len(names)}
            if reference:
                known = set(selector_names(reference))
                item.update(new_names=sorted(set(names) - known),
                            missing_names=sorted(known - set(names)))
            report['selectors'].append(item)
        if path.name == 'main_wii.hdr':
            names = archive_names(path)
            report['archive_indexes'].append({'path': relative, 'entry_count': len(names),
                'symbol_candidates': [n for n in names if Path(n).suffix.lower() in symbol_extensions]})
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--reference-selector', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = json.dumps(audit(args.root, args.reference_selector), indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(result, encoding='utf-8')
    print(result)

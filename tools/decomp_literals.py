"""Prepare a reviewed literal-order trial from original DTK disassembly.

This writes a manifest, never source code or matching-status changes. Literal
metadata does not reconstruct missing function behavior. Use source_trial's
object, regression, and executable checks before keeping the proposal.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

from decomp_runner import ROOT, write_json


def pool_literals(assembly):
    blocks = re.findall(
        r'^\.obj "@stringBase0"[^\n]*\n(.*?)^\.endobj "@stringBase0"',
        assembly, re.M | re.S,
    )
    if len(blocks) != 1:
        raise ValueError('Expected exactly one original @stringBase0 object')
    literals = []
    for line in blocks[0].splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith('#'):
            continue
        match = re.fullmatch(r'\.string ("(?:[^"\\]|\\.)*")', stripped)
        if not match:
            raise ValueError('Pool contains unsupported assembly; review it manually')
        literals.append(match[1])
    if not literals:
        raise ValueError('Original pool contains no strings')
    return literals


def prepare_manifest(unit, version, symbol, root=ROOT, allow_existing_forceactive=False):
    if version not in ('SZBE69', 'SZBE69_B8'):
        raise ValueError('Unsupported game version')
    project = json.loads((root / 'objdiff.json').read_text(encoding='utf-8'))
    matches = [item for item in project['units'] if item['name'] == unit]
    if len(matches) != 1:
        raise ValueError('Expected one configured unit')
    item = matches[0]
    target = Path(item['target_path'].replace('\\', '/'))
    prefix = Path('build') / version / 'obj'
    if not target.is_relative_to(prefix):
        raise ValueError('Configured objects belong to a different version')
    assembly_path = root / 'build' / version / 'asm' / target.relative_to(prefix).with_suffix('.s')
    source_path = (root / item['metadata']['source_path']).resolve()
    if not source_path.is_relative_to((root / 'src').resolve()):
        raise ValueError('Configured source is outside src')
    source = source_path.read_text(encoding='utf-8')
    if 'DECOMP_FORCEACTIVE(' in source and not allow_existing_forceactive:
        raise ValueError('Source already has literal forcing; review the existing metadata')
    assembly = assembly_path.read_text(encoding='utf-8')
    functions = [name.strip('"') for name in re.findall(
        r'^\.fn (.+?), (?:global|weak|local)$', assembly, re.M)]
    if symbol not in functions:
        raise ValueError('Target is absent from the original named functions')
    literals = pool_literals(assembly)
    anchor = source.splitlines()[0]
    if not anchor.startswith('#include ') or source.count(anchor) != 1:
        raise ValueError('Source needs a unique leading include anchor')
    module = 'LiteralPool' + re.sub(r'\W', '_', source_path.stem)
    include = '' if '#include "decomp.h"' in source or '#include <decomp.h>' in source else '\n#include "decomp.h"'
    replacement = (
        anchor + include + '\n\n#if defined(VERSION_' + version + ')\n'
        '// Preserve original literal ordering while incomplete methods remain.\n'
        'DECOMP_FORCEACTIVE(' + module + ',\n    ' + ',\n    '.join(literals)
        + ')\n#endif'
    )
    trial = source.replace(anchor, replacement, 1)
    end_line = trial[:trial.index(')\n#endif', trial.index('DECOMP_FORCEACTIVE(')) + 1].count('\n') + 1
    helper = f'FORCEACTIVE{module}{end_line}__Fv'
    return {
        'unit': unit, 'symbol': symbol,
        'files': [{'path': source_path.relative_to(root.resolve()).as_posix(),
                   'replacements': [{'old': anchor, 'new': replacement}]}],
        'evidence': {'version': version, 'literal_count': len(literals),
                     'original_assembly': assembly_path.relative_to(root).as_posix(),
                     'original_assembly_sha256': hashlib.sha256(assembly_path.read_bytes()).hexdigest(),
                     'review_helper': helper,
                     'scope': 'literal metadata only; missing behavior stays incomplete'},
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--unit', required=True)
    parser.add_argument('--symbol', required=True)
    parser.add_argument('--version', default='SZBE69_B8')
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    manifest = prepare_manifest(args.unit, args.version, args.symbol)
    write_json(args.output, manifest)
    print(json.dumps({'manifest': str(args.output), **manifest['evidence']}, indent=2))


if __name__ == '__main__':
    main()

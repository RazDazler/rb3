"""Generate bounded declaration-order trials from an explicitly reviewed template.

This writes a manifest only. The existing transactional runner performs compilation,
objdiff nonregression checks and executable verification before retaining any trial.
It does not infer whether changing a template preserves program behavior.
"""
import argparse
import itertools
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
TYPES = {'bool', 'int', 'unsigned int', 'float', 'double'}


def generate(spec, source, limit=120):
    declarations = spec['declarations']
    if not 2 <= len(declarations) <= 6 or not 1 <= limit <= 720:
        raise ValueError('Use 2-6 locals and a limit of 1-720 candidates')
    names = [item['name'] for item in declarations]
    if len(set(names)) != len(names):
        raise ValueError('Local names must be unique')
    for item in declarations:
        if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', item['name']) or item['type'] not in TYPES:
            raise ValueError('Use plain local identifiers and supported primitive types')
    old, template = spec['old'], spec['template']
    if not old or source.count(old) != 1 or template.count('{declarations}') != 1:
        raise ValueError('Original block must be unique; template needs one declaration placeholder')
    variants = []
    for order in itertools.islice(itertools.permutations(declarations), limit):
        text = '\n'.join('    ' + item['type'] + ' ' + item['name'] + ';' for item in order)
        candidate = template.replace('{declarations}', text)
        variants.append({'name': 'declaration-order-' + ','.join(item['name'] for item in order),
                         'replacements': [{'old': old, 'new': candidate}]})
    return {'unit': spec['unit'], 'symbol': spec['symbol'], 'variants': variants}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('spec', type=Path, help='Reviewed JSON spec: unit, symbol, old, template, declarations')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--limit', type=int, default=120)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / 'build/decomp') or output.suffix != '.json':
        parser.error('Write manifests under ignored build/decomp with a .json extension')
    spec = json.loads(args.spec.read_text(encoding='utf-8'))
    project = json.loads((ROOT / 'objdiff.json').read_text(encoding='utf-8'))
    units = [unit for unit in project['units'] if unit['name'] == spec['unit']]
    if len(units) != 1:
        parser.error('Select exactly one configured objdiff unit')
    source = (ROOT / units[0]['metadata']['source_path']).resolve()
    if not source.is_relative_to(ROOT / 'src') or not source.is_file():
        parser.error('Select an existing source unit')
    manifest = generate(spec, source.read_text(encoding='utf-8'), args.limit)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'candidates': len(manifest['variants']), 'manifest': str(output),
                      'source_edits': False, 'template_requires_semantic_review': True}))


if __name__ == '__main__':
    main()

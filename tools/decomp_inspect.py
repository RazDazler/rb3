"""Inspect an objdiff JSON function without dumping its entire disassembly."""
import argparse
import json
from pathlib import Path


def inspect_function(diff, name):
    symbols = [symbol for section in diff['left']['sections']
               if section.get('kind') == 'SECTION_TEXT'
               for symbol in section.get('symbols', [])]
    matches = [symbol for symbol in symbols if name == symbol['symbol']['name']]
    if not matches:
        matches = [symbol for symbol in symbols if name in symbol['symbol']['name']]
    if len(matches) != 1:
        raise ValueError(f'Expected one original text symbol, found {len(matches)}')
    original = matches[0]
    target = original.get('target')
    if target is None:
        raise ValueError('Original function has no compiled counterpart')
    compiled = diff['right']['sections'][target.get('section_index', 0)]['symbols'][target.get('symbol_index', 0)]
    left, right = original.get('instructions', []), compiled.get('instructions', [])
    if len(left) != len(right):
        raise ValueError('Expected objdiff aligned instruction rows')
    instructions, relocations = [], []
    for index, (a, b) in enumerate(zip(left, right)):
        if not a.get('diff_kind') and not b.get('diff_kind'):
            continue
        x, y = a.get('instruction', {}), b.get('instruction', {})
        row = {'row': index, 'original_address': x.get('address'),
               'original': x.get('formatted'), 'compiled': y.get('formatted'),
               'original_relocation': x.get('relocation'),
               'compiled_relocation': y.get('relocation')}
        (relocations if x.get('formatted') == y.get('formatted') else instructions).append(row)
    return {'original_symbol': original['symbol']['name'],
            'compiled_symbol': compiled['symbol']['name'],
            'match_percent': original.get('match_percent'),
            'instruction_differences': instructions,
            'same_instruction_relocation_differences': relocations}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('diff', type=Path)
    parser.add_argument('symbol', help='unique original symbol name or substring')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = inspect_function(json.loads(args.diff.read_text(encoding='utf-8')), args.symbol)
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(f"{result['original_symbol']}: {result['match_percent']}%")
    print(f"Different instruction text: {len(result['instruction_differences'])}; "
          f"same instruction text with differing relocations: {len(result['same_instruction_relocation_differences'])}")
    for row in result['instruction_differences']:
        print(f"Row {row['row']}, address {row['original_address']}: {row['original']} -> {row['compiled']}")

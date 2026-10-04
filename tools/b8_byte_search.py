"""Enumerate reviewed B8 byte operations; retain only native verified gains.

All inputs are read in Int(1), Int(2) order. These forms derive from the
original named PowerPC functions, including op11's XOR (not addition) and
op12's operand & 7 shift. This is a finite search, not generated host code.
"""
import itertools
import hashlib
import json
import sys
from pathlib import Path
from types import SimpleNamespace

from decomp_runner import ROOT, Runner, digest, write_json
from decomp_variants import run_variants, scores
from local_decomp_agent import body_span, native_lock


def variants(number, old):
    result = []
    for dtype, layout, constructor, shift_first in itertools.product(
        ('u32', 'unsigned long'), range(4), range(3), (False, True)
    ):
        shift = '(operand & 7)' if number in (5, 11, 12) else '(operand == 0)'
        expression = '!u8(w)' if number == 4 else 'u8(~w)' if number == 5 else 'u8(w)'
        lines = ['u32 operand = msg->Int(1);', 'u32 w = msg->Int(2);']
        if shift_first:
            lines.append('u32 shift = ' + shift + ';')
            shift = 'shift'
        if layout == 0:
            lines += [dtype + ' ret = ' + expression + ';', 'ret |= ret << 8;']
        elif layout == 1:
            lines += [dtype + ' ret = ' + expression + ';', 'ret = (ret << 8) | ret;']
        elif layout == 2:
            lines += [dtype + ' byte = ' + expression + ';', dtype + ' ret = byte << 8;', 'ret |= byte;']
        else:
            lines += [dtype + ' byte = ' + expression + ';', dtype + ' ret = byte | (byte << 8);']
        lines.append('ret >>= ' + shift + ';')
        if number in (10, 11):
            value = 'u8(ret ^ operand)'
        elif number in (12, 13):
            value = 'u8(ret + operand)'
        else:
            value = 'u8(ret)'
        returns = [value, 'DataNode(' + value + ')', 'DataNode(kDataInt, ' + value + ')']
        lines.append('return ' + returns[constructor] + ';')
        body = '{\n    ' + '\n    '.join(lines) + '\n}'
        result.append({'name': f'{dtype.replace(" ", "-")}-{layout}-{constructor}-{int(shift_first)}',
                       'replacements': [{'old': old, 'new': body}]})
    return result


def main():
    numbers = [int(n) for n in sys.argv[1:]] or [3, 4, 5, 10, 11, 12, 13]
    if any(n not in (3, 4, 5, 10, 11, 12, 13) for n in numbers):
        raise ValueError('Only reviewed byte rotations are supported')
    source = ROOT / 'src/system/synth/ByteGrinder.cpp'
    state = ROOT / 'build/local-agent/byte-search'
    state.mkdir(parents=True, exist_ok=True)
    runner = Runner(SimpleNamespace(version='SZBE69_B8', minutes=120, timeout=180,
        jobs=4, stop_file=state / 'STOP', quota_file=ROOT / 'build/local-agent/session-usage.json',
        allow_new_helper=[], limit=48))
    cache_path = state / 'cache.json'
    cache = json.loads(cache_path.read_text()) if cache_path.exists() else {}
    def cache_key():
        units = json.loads((ROOT / 'objdiff.json').read_text())['units']
        unit = next(u for u in units if u['name'] == 'main/system/synth/ByteGrinder')
        return hashlib.sha256(json.dumps([digest(Path(__file__)), digest(runner.config),
            digest(ROOT / unit['base_path']), digest(ROOT / unit['target_path'])]).encode()).hexdigest()
    with native_lock(runner):
        runner.recover()
        ok, log = runner.build('byte-search-cache-baseline')
        if not ok:
            raise RuntimeError(f'Cache baseline build failed: {log}')
        original_key = cache_key()
    completed = set(cache.get('completed', [])) if cache.get('key') == original_key else set()
    for number in numbers:
        if number in completed:
            print(f'op{number}: unchanged verified search cached', flush=True)
            continue
        with native_lock(runner):
            if runner.stop_reason():
                break
            rows = runner.compare('main/system/synth/ByteGrinder')
            if scores(rows)[f'op{number}__FP9DataArray'][0] == 100:
                completed.add(number)
                print(f'op{number}: already fully matched', flush=True)
                continue
            text = source.read_text()
            start, end = body_span(text, f'DataNode op{number}(DataArray *msg)')
            manifest = {'unit': 'main/system/synth/ByteGrinder', 'symbol': f'op{number}__FP9DataArray',
                        'variants': variants(number, text[start:end])}
            write_json(state / f'op{number}-manifest.json', manifest)
            result = run_variants(runner, manifest)
        write_json(state / f'op{number}-result.json', result)
        print(json.dumps({k: v for k, v in result.items() if k != 'attempts'}), flush=True)
        if result['stop_reason'] in ('variants_exhausted', 'target_fully_matched'):
            completed.add(number)
        if runner.stop_reason():
            break
    with native_lock(runner):
        write_json(cache_path, {'key': cache_key(), 'completed': sorted(completed)})


if __name__ == '__main__':
    main()

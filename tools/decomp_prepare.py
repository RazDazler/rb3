"""Finite B8 downtime preparation: comparisons and review artifacts, never source trials.

No cloud calls, source replacements, configuration adoption, or journal recovery.
Run a coherent native build once, compare each selected unit once, cache by actual
input hashes, then leave compact packets for the next decompilation session.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time
from types import SimpleNamespace

from decomp_runner import ROOT, Runner, candidates, digest, exclusive_lock, sha1, write_bytes, write_json
from decomp_inspect import inspect_function
from local_decomp_pipeline import keep_awake

VERSION = 'SZBE69_B8'
STATE = ROOT / 'build/decomp/preparation' / VERSION
POLICY = 1


def protected_snapshot(root):
    """Detect any changed, added or removed game source/header/configuration."""
    return {p.relative_to(root).as_posix(): digest(p)
            for directory in ('src', 'include', 'config')
            for p in (root / directory).rglob('*') if p.is_file()}


def checked_project(project, root=ROOT):
    units = {}
    version_root = (root / 'build' / VERSION).resolve()
    for item in project['units']:
        for key in ('target_path', 'base_path'):
            if not item.get(key):
                continue  # Original-only units have no configured compiled source.
            path = (root / item[key]).resolve()
            if not path.is_relative_to(version_root):
                raise ValueError('objdiff project must exclusively target B8')
        source = (root / item.get('metadata', {}).get('source_path', '')).resolve()
        if all(item.get(key) for key in ('target_path', 'base_path')) and source.is_relative_to((root / 'src').resolve()) and source.is_file():
            if item['name'] in units:
                raise ValueError('Duplicate unit name')
            units[item['name']] = item
    return units


def ranked_units(report, units, min_score, max_size, include_unmatched=False):
    rows = []
    for unit in report['units']:
        if unit['name'] not in units:
            continue
        functions = [f for f in unit.get('functions', [])
                     if 0 < int(f.get('size', 0)) <= max_size
                     and (min_score <= f.get('fuzzy_match_percent', 0) < 100
                          or include_unmatched and f.get('fuzzy_match_percent', 0) == 0)]
        if functions:
            rows.append({'unit': unit['name'], 'functions': functions,
                         'priority': max(f.get('fuzzy_match_percent', 0) for f in functions)})
    return sorted(rows, key=lambda r: (-r['priority'], r['unit']))


def unit_fingerprint(unit, config_hash, tool_hash):
    value = {'policy': POLICY, 'config': config_hash, 'tools': tool_hash,
             'source': digest(ROOT / unit['metadata']['source_path']),
             'target': digest(ROOT / unit['target_path']),
             'compiled': digest(ROOT / unit['base_path']), 'unit': unit}
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def prior_experiments(paths):
    """Compact historical hints, not proof that an unchanged trial was exhausted."""
    by_unit = {}
    for path in paths:
        ledger = json.loads(path.read_text(encoding='utf-8'))
        for category in ('accepted_changes', 'rejected_experiments'):
            for row in ledger.get(category, []):
                result = row.get('result') or {}
                if not isinstance(result, dict):
                    continue
                unit = result.get('unit') or row.get('unit')
                if not unit:
                    continue
                by_unit.setdefault(unit, []).append({
                    'ledger': str(path), 'category': category,
                    'candidate': row.get('candidate', row.get('symbol', 'unnamed experiment')),
                    'symbol': result.get('symbol', row.get('symbol')),
                    'reason': result.get('reason', result.get('stop_reason', row.get('reason'))),
                    'before': result.get('before'), 'after': result.get('after'),
                    'variants_tried': len(result.get('attempts', [])),
                    'caution': 'Historical evidence; source and build context may have changed.'})
    return by_unit


def relocation_name(instruction):
    return instruction.get('relocation', {}).get('target', {}).get('symbol', {}).get('name')


def classification(inspection):
    rows = inspection['instruction_differences'] + inspection['same_instruction_relocation_differences']
    if not rows:
        return 'strict_match' if inspection.get('match_percent') == 100 else 'unresolved_score'
    if all(all((r.get(k) or {}).get('target', {}).get('symbol', {}).get('name') == '@stringBase0'
               for k in ('original_relocation', 'compiled_relocation')) for r in rows):
        return 'string_metadata'
    if not inspection['instruction_differences']:
        return 'relocations_or_symbol_ownership'
    # Register-only spelling is an orientation hint, not proof of equal behavior.
    normalize = lambda s: re.sub(r'\b(?:r|f)\d+\b', 'REG', s or '')
    if not inspection['same_instruction_relocation_differences'] and all(
            normalize(r['original']) == normalize(r['compiled']) for r in rows):
        return 'register_allocation_hint'
    return 'instruction_or_behavior'


def symbol_pairs(diff, name):
    symbol = next(s for sec in diff['left']['sections'] if sec.get('kind') == 'SECTION_TEXT'
                  for s in sec.get('symbols', []) if s['symbol']['name'] == name)
    target = symbol.get('target')
    compiled = None if target is None else diff['right']['sections'][target.get('section_index', 0)]['symbols'][target.get('symbol_index', 0)]
    return symbol, compiled


def map_evidence(path, symbols):
    """Index exact mangled names once; retain original ownership and line numbers."""
    result = {name: [] for name in symbols}
    if path is None or not path.is_file():
        return result
    pattern = re.compile(r'^\s*[0-9a-fA-F]{8}\s+[0-9a-fA-F]{6,8}\s+[0-9a-fA-F]{8}\s+[0-9a-fA-F]{8}\s+(?:\d+\s+)?(\S+)')
    with path.open(encoding='utf-8', errors='replace') as stream:
        for number, line in enumerate(stream, 1):
            match = pattern.match(line)
            if match and match[1] in result and len(result[match[1]]) < 8:
                result[match[1]].append({'line': number, 'text': line.strip()})
    return result


def assembly_function(path, name):
    lines = path.read_text(encoding='utf-8').splitlines()
    starts = [i for i, line in enumerate(lines)
              if re.fullmatch(r'\.fn\s+"?' + re.escape(name) + r'"?,\s*\w+', line)]
    if len(starts) != 1:
        raise ValueError('Expected one original assembly function')
    start = starts[0]
    end = next(i for i in range(start + 1, len(lines)) if lines[i].startswith('.endfn '))
    if end - start > 600:
        raise ValueError('Assembly packet is too large')
    text = '\n'.join(re.sub(r'/\*.*?\*/\s*', '', line) for line in lines[start:end + 1])
    # DTK directives -> standalone GNU-as syntax understood by m2c.
    text = re.sub(r'(?m)^\.fn\s+"?([^",]+)"?,\s*\w+', r'.global "\1"\n"\1":', text)
    text = re.sub(r'(?m)^\.endfn.*', '', text)
    return '.text\n' + text + '\n'


def make_packet(diff, function, unit, fingerprint, map_rows):
    name = function['name']
    original, compiled = symbol_pairs(diff, name)
    aligned = []
    left = original.get('instructions', [])
    right = compiled.get('instructions', []) if compiled else []
    for index in range(max(len(left), len(right))):
        a = left[index] if index < len(left) else {}
        b = right[index] if index < len(right) else {}
        x, y = a.get('instruction', {}), b.get('instruction', {})
        aligned.append({'row': index, 'original': x.get('formatted'), 'compiled': y.get('formatted'),
                        'original_relocation': relocation_name(x), 'compiled_relocation': relocation_name(y),
                        'different': a.get('diff_kind', 'DIFF_NONE') != 'DIFF_NONE'
                                     or b.get('diff_kind', 'DIFF_NONE') != 'DIFF_NONE'})
    try:
        inspection = inspect_function(diff, name)
        category = classification(inspection)
        differences = len(inspection['instruction_differences']) + len(inspection['same_instruction_relocation_differences'])
    except ValueError:
        inspection, category, differences = None, 'missing_compiled_function', len(left)
    # This extracts observed facts; it does not infer types or aliasing guarantees.
    calls = [{'row': row['row'], 'text': row['original'], 'symbol': row['original_relocation']}
             for row in aligned if re.match(r'^(?:bl|bctrl|bctr|b)\s', row['original'] or '')
             or row['original'] in ('bctrl', 'bctr')]
    memory = [{'row': row['row'], 'text': row['original']}
              for row in aligned if re.match(r'^(?:l(?:bz|hz|ha|wz|fs|fd)|st(?:b|h|w|fs|fd))\w*\s', row['original'] or '')]
    return {'schema': POLICY, 'version': VERSION, 'unit': unit['name'], 'symbol': name,
            'demangled_name': function.get('metadata', {}).get('demangled_name'),
            'bytes': int(function['size']), 'report_score': function.get('fuzzy_match_percent', 0),
            'strict_score': original.get('match_percent'), 'classification': category,
            'difference_rows': differences, 'source_path': unit['metadata']['source_path'],
            'fingerprint': fingerprint, 'compiler': unit.get('scratch', {}).get('compiler'),
            'compiler_flags': unit.get('scratch', {}).get('c_flags'),
            'debug_map': map_rows, 'aligned_rows': aligned,
            'observed_calls_and_tail_branches': calls, 'observed_memory_instructions': memory,
            'limitations': ['Map evidence establishes symbols/ownership, not local variable types.',
                            'Classification is triage; matching and semantic review remain necessary.',
                            'Any pseudocode is an unreviewed interpretation, never adopted source.']}


def m2c_review(script, assembly, destination, timeout):
    start = time.perf_counter()
    log = destination.with_suffix('.m2c.txt')
    with log.open('w', encoding='utf-8') as stream:
        result = subprocess.run([sys.executable, str(script), '-t', 'ppc-mwcc-c++', '--no-cache',
                                 str(assembly)], cwd=destination.parent, stdout=stream,
                                stderr=subprocess.STDOUT, timeout=timeout)
    return {'exit_code': result.returncode, 'seconds': round(time.perf_counter() - start, 4),
            'artifact': str(log), 'status': 'unreviewed_pseudocode' if result.returncode == 0 else 'tool_failed'}


def handoff(summary):
    lines = ['# B8 preparation handoff', '',
             'Game source/configuration were not edited. No proposed code is adopted.', '',
             f"Status: {summary['status']}; elapsed {summary['elapsed_seconds']:.2f}s; "
             f"units compared {summary['units_compared']}, cache hits {summary['cache_hits']}.", '',
             'Read a packet before opening a large objdiff JSON. Strict matches need no reconstruction; '
             'check whole-unit data/layout before linking. String metadata needs the literal tool; '
             'relocation-only cases need symbol/section ownership evidence. Register hints need semantic review.', '',
             '| Category | Unit / symbol | Strict score | Review packet |', '|---|---|---:|---|']
    priority = {'string_metadata': 0, 'register_allocation_hint': 1,
                'relocations_or_symbol_ownership': 2, 'missing_compiled_function': 3,
                'instruction_or_behavior': 4, 'unresolved_score': 5, 'strict_match': 6}
    ordered = sorted(summary['packets'], key=lambda r: (priority.get(r['classification'], 9), r['difference_rows'], r['bytes']))
    for row in ordered[:24]:
        lines.append(f"| {row['classification']} | {row['unit']} / `{row['symbol']}` | {row['strict_score']} | `{row['packet']}` |")
    lines += ['', 'Showing up to 24 triage candidates. All packets and category counts are in latest.json.', '',
              f"Source-link candidates with report code/data at 100%: {len(summary.get('source_link_candidates', []))}. "
              'See latest.json for bytes and prior failures; report scores do not prove exact link layout.', '',
              'Changed inputs invalidate cached comparisons automatically. Before using an older packet, '
              'rerun preparation and confirm it remains in latest.json. STOP ends the next unit; '
              'the current bounded native command is allowed to finish. No model or cloud usage is needed.']
    return '\n'.join(lines) + '\n'


def run(args):
    STATE.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    deadline = started + args.minutes * 60
    runner = Runner(SimpleNamespace(version=VERSION, minutes=args.minutes, timeout=args.timeout,
                                    jobs=args.jobs, stop_file=STATE / 'STOP', quota_file=None))
    summary = {'started_at': time.time(), 'status': 'running', 'units_compared': 0, 'cache_hits': 0,
               'packets': [], 'errors': [], 'native_seconds': 0.0, 'm2c_seconds': 0.0,
               'mode': 'artifact_only_no_source_trials', 'version': VERSION}
    def save():
        summary['elapsed_seconds'] = round(time.monotonic() - started, 4)
        summary['classification_counts'] = dict(Counter(r['classification'] for r in summary['packets']))
        write_json(STATE / 'latest.json', summary)
        (STATE / 'HANDOFF.md').write_text(handoff(summary), encoding='utf-8')
    def stopped():
        return 'stop_file' if (STATE / 'STOP').exists() else 'deadline' if time.monotonic() >= deadline else None
    before = None
    try:
        with exclusive_lock(STATE / 'worker.lock'), exclusive_lock(runner.lock_path):
            if stopped():
                summary['status'] = stopped()
                return summary
            # Recovery writes source. This runner must refuse it rather than perform it.
            if runner.journal.exists():
                raise RuntimeError('Pending source trial: stop and recover with decomp_runner before preparation')
            units = checked_project(json.loads((ROOT / 'objdiff.json').read_text()), ROOT)
            before = protected_snapshot(ROOT)
            if sha1(runner.original) != runner.expected:
                raise RuntimeError('Original B8 executable checksum mismatch')
            stamp = time.perf_counter()
            code, log = runner.command([runner.ninja, '-j', str(args.jobs), f'build/{VERSION}/progress.json'], 'prepare-sync')
            if code or not runner.output.is_file() or sha1(runner.output) != runner.expected:
                raise RuntimeError(f'Native B8 sync failed: {log}')
            report = runner.refresh()
            summary['native_seconds'] += time.perf_counter() - stamp
            summary['measures'] = report['measures']
            summary['verified_dol_sha1'] = sha1(runner.output)
            ranked = ranked_units(report, units, args.min_score, args.max_size, args.include_unmatched)
            if args.unit:
                ranked = [r for r in ranked if r['unit'] in args.unit]
            all_symbols = {f['name'] for r in ranked for f in r['functions']}
            original_map = ROOT / 'orig/SZBE69_B8/files/band_r_wii.map'
            maps = map_evidence(original_map, all_symbols)
            history = prior_experiments(sorted((ROOT / 'doc').glob('DECOMP_B8*SESSION*.json')))
            history_hash = hashlib.sha256(json.dumps(history, sort_keys=True).encode()).hexdigest()
            summary['debug_map_path'] = str(original_map) if original_map.is_file() else None
            summary['debug_map_sha256'] = digest(original_map) if original_map.is_file() else None
            config_hash = digest(runner.config)
            tool_hash = [digest(Path(__file__)), digest(ROOT / 'tools/decomp_rtti.py'), digest(Path(runner.objdiff))]
            entries = {p: v for lib in json.loads(runner.config.read_text()).values() for p, v in lib['objects'].items()}
            summary['source_link_candidates'] = [
                {'unit': r['unit'], 'bytes': int(r['measures']['total_code']),
                 'prior_experiments': history.get(r['unit'], [])[-6:]}
                for r in candidates(report, entries)]
            summary['candidate_units'] = len(ranked)
            m2c_hash = None if not args.m2c else hashlib.sha256(json.dumps([
                (p.relative_to(args.m2c.parent).as_posix(), digest(p))
                for p in sorted(args.m2c.parent.rglob('*.py'))]).encode()).hexdigest()
            for rank in ranked:
                if stopped():
                    summary['status'] = stopped()
                    break
                if summary['units_compared'] >= args.limit:
                    summary['status'] = 'comparison_limit'
                    break
                unit = units[rank['unit']]
                try:
                    fingerprint = unit_fingerprint(unit, config_hash, tool_hash)
                    directory = STATE / 'units' / hashlib.sha256(unit['name'].encode()).hexdigest()[:16] / fingerprint
                    directory.mkdir(parents=True, exist_ok=True)
                    asm_relative = Path(unit['metadata']['source_path']).relative_to('src').with_suffix('.s')
                    asm_path = ROOT / 'build' / VERSION / 'asm' / asm_relative
                    selector = hashlib.sha256(json.dumps([
                        rank['functions'], summary['debug_map_sha256'], history_hash,
                        m2c_hash, digest(asm_path) if args.m2c and asm_path.is_file() else None],
                        sort_keys=True).encode()).hexdigest()
                    catalog_path = directory / ('catalog-' + selector + '.json')
                    if catalog_path.is_file():
                        catalog = json.loads(catalog_path.read_text())
                        if all((directory / relative).resolve().is_relative_to(directory.resolve())
                               and (directory / relative).is_file()
                               and digest(directory / relative) == expected
                               for relative, expected in catalog.get('artifact_hashes', {}).items()) and catalog.get('artifact_hashes'):
                            summary['cache_hits'] += 1
                            summary['packets'].extend(catalog['packets'])
                            save()
                            continue  # Avoid parsing a whole-unit diff on unchanged runs.
                    packet_start = len(summary['packets'])
                    diff_path = directory / 'comparison.json'
                    if diff_path.is_file():
                        summary['cache_hits'] += 1
                    else:
                        stamp = time.perf_counter()
                        runner.compare(unit['name'])
                        summary['native_seconds'] += time.perf_counter() - stamp
                        generated = runner.directory / (re.sub(r'[^\w-]', '_', unit['name']) + '-diff.json')
                        # Compact cached JSON saves disk and parse work; full native
                        # evidence remains in the normal runner directory.
                        write_bytes(diff_path, (json.dumps(json.loads(generated.read_text()),
                                                           separators=(',', ':')) + '\n').encode())
                        summary['units_compared'] += 1
                    diff = json.loads(diff_path.read_text())
                    for function in rank['functions']:
                        packet = make_packet(diff, function, unit, fingerprint, maps[function['name']])
                        packet['prior_experiments'] = [r for r in history.get(unit['name'], [])
                                                       if r['symbol'] in (None, function['name'])][-6:]
                        packet_id = hashlib.sha256(function['name'].encode()).hexdigest()[:16]
                        packet_path = directory / (packet_id + '.json')
                        if args.m2c:
                            assembly = directory / (packet_id + '.s')
                            try:
                                text = assembly_function(asm_path, function['name'])
                                if not assembly.is_file() or assembly.read_text(encoding='utf-8') != text:
                                    assembly.write_text(text, encoding='utf-8')
                                previous = json.loads(packet_path.read_text()) if packet_path.is_file() else {}
                                key = [m2c_hash, digest(assembly)]
                                if previous.get('m2c_key') == key:
                                    packet['m2c'] = previous['m2c']
                                else:
                                    if stopped():
                                        break
                                    packet['m2c'] = m2c_review(args.m2c, assembly, packet_path,
                                                               min(args.timeout, max(0.1, deadline - time.monotonic())))
                                    summary['m2c_seconds'] += packet['m2c']['seconds']
                                packet['m2c_key'] = key
                            except (ValueError, StopIteration, subprocess.TimeoutExpired) as error:
                                packet['m2c'] = {'status': 'unavailable', 'reason': str(error)}
                        write_json(packet_path, packet)
                        summary['packets'].append({k: packet[k] for k in ('unit', 'symbol', 'bytes', 'strict_score', 'classification', 'difference_rows')}
                                                | {'packet': str(packet_path), 'packet_bytes': packet_path.stat().st_size})
                    if not stopped():
                        artifacts = {}
                        for row in summary['packets'][packet_start:]:
                            path = Path(row['packet'])
                            for artifact in (path, path.with_suffix('.s'), path.with_suffix('.m2c.txt')):
                                if artifact.is_file():
                                    artifacts[artifact.relative_to(directory).as_posix()] = digest(artifact)
                        write_json(catalog_path, {'packets': summary['packets'][packet_start:],
                                                  'artifact_hashes': artifacts})
                    save()
                    print(json.dumps({'unit': unit['name'], 'compared': summary['units_compared'],
                                      'cache_hits': summary['cache_hits'], 'packets': len(summary['packets'])}), flush=True)
                except (ValueError, KeyError, StopIteration) as error:
                    summary['errors'].append({'unit': unit['name'], 'error': str(error)})
            else:
                summary['status'] = 'queue_complete'
    except BaseException as error:
        summary['status'] = 'failed'
        summary['error'] = str(error)
        raise
    finally:
        if before is not None:
            after = protected_snapshot(ROOT)
            changed = sorted(k for k in before.keys() | after.keys() if before.get(k) != after.get(k))
            summary['protected_files_checked'] = len(before)
            summary['protected_files_changed'] = changed
            if changed:
                summary['status'] = 'failed_source_changed'
            summary['protected_snapshot_sha256'] = hashlib.sha256(json.dumps(before, sort_keys=True).encode()).hexdigest()
        save()
    if summary.get('protected_files_changed'):
        raise RuntimeError('Source/configuration changed during preparation; inspect latest.json. Nothing was restored or adopted.')
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--minutes', type=float, default=60)
    parser.add_argument('--limit', type=int, default=64, help='maximum fresh unit comparisons; cached units do not consume this budget')
    parser.add_argument('--timeout', type=float, default=90)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--max-size', type=int, default=256)
    parser.add_argument('--min-score', type=float, default=70)
    parser.add_argument('--unit', action='append', default=[])
    parser.add_argument('--include-unmatched', action='store_true', help='also prepare small missing/zero-score functions')
    parser.add_argument('--m2c', type=Path, help='optional local m2c.py; unreviewed pseudocode goes only into build artifacts')
    args = parser.parse_args()
    if min(args.minutes, args.limit, args.timeout, args.jobs, args.max_size) <= 0 or not 0 <= args.min_score < 100:
        parser.error('Positive limits and 0 <= min-score < 100 are required')
    if args.m2c:
        args.m2c = args.m2c.resolve()
        if not args.m2c.is_relative_to(ROOT / 'build') or args.m2c.name != 'm2c.py' or not args.m2c.is_file():
            parser.error('m2c.py must already exist inside repository build/')
    with keep_awake():
        summary = run(args)
    print(json.dumps({k: v for k, v in summary.items() if k not in ('packets', 'source_link_candidates')}, indent=2))


if __name__ == '__main__':
    main()

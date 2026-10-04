"""Finite B8 string-offset restoration; never reconstructs missing behavior.

Only targets with remaining string-pool relocation differences are tried.
All literals come from original DTK assembly, not an LLM. Existing forcing
macros stay intact. Native object/global checks and exact DOL verification
decide retention. Results and metadata-only gains are recorded separately.
"""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace

from decomp_runner import ROOT, Runner, write_json
from decomp_inspect import inspect_function
from decomp_literals import prepare_manifest
from decomp_source_trial import run_trial
from local_decomp_agent import native_lock


def string_offsets_only(inspection):
    rows = inspection['instruction_differences'] + inspection['same_instruction_relocation_differences']
    if not rows:
        return False
    for row in rows:
        for key in ('original_relocation', 'compiled_relocation'):
            relocation = row.get(key) or {}
            if relocation.get('target', {}).get('symbol', {}).get('name') != '@stringBase0':
                return False
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--start', type=int, default=0)
    parser.add_argument('--limit', type=int, default=24)
    parser.add_argument('--min-score', type=float, default=98,
                        help='minimum report score when generating a new candidate list')
    parser.add_argument('--candidates', type=Path,
                        default=ROOT / 'build/local-agent/literal-candidates.json')
    parser.add_argument('--ledger', type=Path, help='optional existing session JSON ledger')
    args = parser.parse_args()
    if args.start < 0 or args.limit < 1:
        parser.error('start must be nonnegative and limit positive')
    if not 0 <= args.min_score < 100:
        parser.error('min-score must be at least zero and below 100')
    candidate_file = args.candidates.resolve()
    if not candidate_file.is_relative_to(ROOT / 'build/local-agent'):
        parser.error('candidate lists must stay under build/local-agent')
    if not candidate_file.exists():
        report = json.loads((ROOT / 'build/SZBE69_B8/report.json').read_text())
        ranked = []
        for unit in report['units']:
            if not unit['name'].startswith('main/'):
                continue
            source = ROOT / 'src' / (unit['name'][5:] + '.cpp')
            if not source.is_file() or 'LiteralPool' in source.read_text(errors='replace'):
                continue
            functions = [f for f in unit.get('functions', [])
                         if args.min_score <= f.get('fuzzy_match_percent', 100) < 100 and int(f['size']) >= 100]
            if functions:
                ranked.append((unit['name'], max(functions, key=lambda f: f['fuzzy_match_percent'])['name']))
        write_json(candidate_file, ranked)
    candidates = json.loads(candidate_file.read_text())
    if args.ledger is not None:
        args.ledger = args.ledger.resolve()
        if not args.ledger.is_relative_to(ROOT) or not args.ledger.is_file():
            parser.error('ledger must be an existing initialized session file in the repository')
    runner = Runner(SimpleNamespace(version='SZBE69_B8', minutes=120, timeout=180,
        jobs=4, stop_file=ROOT / 'build/local-agent/literal-search/STOP',
        quota_file=ROOT / 'build/local-agent/session-usage.json',
        allow_new_helper=[], adopt_unit=False, review_only=False))
    suffix = '' if candidate_file.name == 'literal-candidates.json' else '-' + candidate_file.stem
    destination = ROOT / ('build/local-agent/literal-search' + suffix)
    destination.mkdir(parents=True, exist_ok=True)
    for index, (unit, symbol) in enumerate(candidates):
        if index < args.start or index >= args.start + args.limit:
            continue
        item = {'index': index, 'unit': unit, 'symbol': symbol, 'kind': 'literal metadata only'}
        try:
            with native_lock(runner):
                if runner.stop_reason():
                    break
                runner.recover()
                runner.compare(unit)
                diff = runner.directory / (unit.replace('/', '_') + '-diff.json')
                comparison = json.loads(diff.read_text())
                inspection = inspect_function(comparison, symbol)
                if not string_offsets_only(inspection):
                    for section in comparison['left']['sections']:
                        if section.get('kind') != 'SECTION_TEXT':
                            continue
                        for candidate in section.get('symbols', []):
                            if not 90 <= candidate.get('match_percent', 0) < 100 or candidate.get('target') is None:
                                continue
                            possible = inspect_function(comparison, candidate['symbol']['name'])
                            if string_offsets_only(possible):
                                inspection = possible
                                symbol = candidate['symbol']['name']
                                item['symbol'] = symbol
                                break
                        if string_offsets_only(inspection):
                            break
                if not string_offsets_only(inspection):
                    item['skip_reason'] = 'not exclusively string offsets'
                else:
                    manifest = prepare_manifest(unit, 'SZBE69_B8', symbol,
                        allow_existing_forceactive=True)
                    write_json(destination / f'{index:03d}-manifest.json', manifest)
                    runner.args.allow_new_helper = [manifest['evidence']['review_helper']]
                    item['result'] = run_trial(runner, manifest)
                    if args.ledger is not None:
                        data = json.loads(args.ledger.read_text())
                        category = 'accepted_changes' if item['result'].get('retained') else 'rejected_experiments'
                        data[category].append(item)
                        write_json(args.ledger, data)
        except ValueError as error:
            item['skip_reason'] = str(error)
        write_json(destination / f'{index:03d}-result.json', item)
        result = item.get('result', {})
        print(json.dumps({'index': index, 'unit': unit, 'skip': item.get('skip_reason'),
            'retained': result.get('retained'), 'gains': result.get('measure_gains'),
            'reason': result.get('reason')}), flush=True)


if __name__ == '__main__':
    main()

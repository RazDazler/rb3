"""Record one reviewed B8 trial in a session ledger, retaining existing guards."""
import argparse
import json
from pathlib import Path
import time
from types import SimpleNamespace

from decomp_runner import ROOT, Runner, exclusive_lock, write_json
from decomp_source_trial import run_trial
from decomp_variants import run_variants
from decomp_literals import prepare_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kind', choices=['literal', 'source', 'variants'])
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--ledger', type=Path, required=True)
    parser.add_argument('--symbol')
    parser.add_argument('--note', required=True)
    parser.add_argument('--classification', default='behavior')
    parser.add_argument('--adopt', action='store_true')
    args = parser.parse_args()
    ledger = args.ledger.resolve()
    if not ledger.is_relative_to(ROOT / 'doc') or not ledger.is_file():
        parser.error('Use an existing repository doc ledger')
    session = json.loads(ledger.read_text(encoding='utf-8'))
    if session.get('target') != 'SZBE69_B8' or session.get('status') != 'active':
        parser.error('Only an active B8 session may receive trials')
    manifest = (prepare_manifest(args.manifest, 'SZBE69_B8', args.symbol, allow_existing_forceactive=True)
                if args.kind == 'literal' else json.loads(args.manifest.read_text(encoding='utf-8')))
    output = ROOT / 'build/decomp/session-trials' / str(time.time_ns())
    output.mkdir(parents=True)
    write_json(output / 'manifest.json', manifest)
    runner = Runner(SimpleNamespace(version='SZBE69_B8', minutes=30, timeout=90, jobs=4,
                    stop_file=ROOT / 'build/decomp/session-trials/STOP',
                    quota_file=ROOT / 'build/local-agent/session-usage.json',
                    limit=len(manifest.get('variants', [])) or 1,
                    allow_new_helper=[manifest['evidence']['review_helper']] if args.kind == 'literal'
                                     else manifest.get('allowed_helpers', []),
                    adopt_unit=args.adopt, review_only=False))
    start = time.time()
    with exclusive_lock(runner.lock_path):
        before = json.loads(runner.report_path.read_text())['measures']
        result = run_variants(runner, manifest) if args.kind == 'variants' else run_trial(runner, manifest)
        after = json.loads(runner.report_path.read_text())['measures']
    retained = bool(result.get('accepted_variant')) if args.kind == 'variants' else result.get('retained', False)
    entry = {'candidate': args.note, 'method': args.kind, 'classification': args.classification,
             'manifest': output.joinpath('manifest.json').relative_to(ROOT).as_posix(),
             'started_at': start, 'elapsed_seconds': round(time.time() - start, 2),
             'result': {k: v for k, v in result.items() if k != 'attempts'},
             'variants_tried': len(result.get('attempts', [])),
             'gains': {k: int(after[k]) - int(before[k]) for k in
                       ('matched_code', 'matched_functions', 'complete_code', 'complete_units',
                        'matched_data', 'complete_data')}}
    write_json(output / 'result.json', result)
    session['accepted_changes' if retained else 'rejected_experiments'].append(entry)
    if retained:
        session['retained_partial_reconstructions'] = [x for x in session['retained_partial_reconstructions']
                                                     if (x['unit'], x['symbol']) != (manifest['unit'], manifest['symbol'])]
        if result.get('after', [100])[0] < 100:
            session['retained_partial_reconstructions'].append({'unit': manifest['unit'], 'symbol': manifest['symbol'],
                                                              'after': result.get('after'), 'note': args.note})
    write_json(ledger, session)
    print(json.dumps({'candidate': args.note, 'retained': retained, 'before': result.get('before'),
                      'after': result.get('after'), 'reason': result.get('reason', result.get('stop_reason')),
                      'gains': entry['gains'], 'result_path': str(output / 'result.json')}, indent=2))


if __name__ == '__main__':
    main()

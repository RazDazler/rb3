"""Render a verified B8 session ledger and update the cross-session comparison."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path

from decomp_runner import ROOT, write_json


def report(ledger, finish=False):
    session = json.loads(ledger.read_text(encoding='utf-8'))
    checkpoint = session['latest_checkpoint']
    gains, measures = checkpoint['gains'], checkpoint['measures']
    usage = session['latest_usage']
    if finish:
        if min(100 - usage['primary_used_percent'], 100 - usage['weekly_used_percent']) > 1:
            raise ValueError('Usage stopping condition has not been reached')
        session.update(status='complete', stop_reason='critical_account_usage',
                       ended_at=checkpoint['at'],
                       ended_utc=datetime.fromtimestamp(checkpoint['at'], timezone.utc).isoformat())
    partials = session.get('retained_partial_reconstructions', [])
    elapsed = checkpoint['elapsed_seconds']
    session['partial_summary'] = {'retained_records': len(partials)}
    session_id = ledger.stem.lower()
    row = {'session_id': session_id, 'status': session['status'], 'model': session['model_label'],
           'target': session['target'], 'elapsed_seconds': elapsed, 'gains': gains,
           'upstream_gains_excluded': session.get('upstream_gains', {}),
           'partial_summary': session['partial_summary'],
           'account_usage_start': session['initial_usage'], 'account_usage_end': usage,
           'account_usage_consumed_percentage_points': {
               w: checkpoint['account_usage'][w]['consumed_percentage_points'] for w in ('primary', 'weekly')},
           'refill_observed': any(not checkpoint['account_usage'][w]['same_window'] for w in ('primary', 'weekly')),
           'verified_dol_sha1': checkpoint['verified_dol_sha1'],
           'ledger': ledger.relative_to(ROOT).as_posix(),
           'report': ledger.with_suffix('.md').relative_to(ROOT).as_posix(),
           'observed_throughput': {'matched_code_bytes_per_hour': round(gains['matched_code'] * 3600 / elapsed, 2),
                                   'matched_functions_per_hour': round(gains['matched_functions'] * 3600 / elapsed, 2)},
           'note': 'Elapsed and usage include setup/integration. Target mix and rounded shared-account percentages prevent causal model conclusions. Endpoint consumption is unavailable across refills; usage snapshots are retained.'}
    comparison = ROOT / 'doc/DECOMP_MODEL_COMPARISON_2026-10-02.json'
    data = json.loads(comparison.read_text(encoding='utf-8'))
    data['sessions'] = [x for x in data['sessions'] if x.get('session_id') != session_id] + [row]
    write_json(comparison, data)
    write_json(ledger, session)
    table = '\n'.join('| ' + label + ' | ' + str(session['baseline'][key]) + ' | ' + str(measures[key]) +
                      ' | ' + str(gains[key]) + ' |' for key, label in (
                          ('matched_code', 'Matched code bytes'), ('matched_functions', 'Matched functions'),
                          ('complete_code', 'Source-linked code bytes'), ('complete_units', 'Source-linked units'),
                          ('matched_data', 'Matched data bytes'), ('complete_data', 'Source-linked data bytes')))
    partial_text = '\n'.join('- `' + x['symbol'] + '` (' + x['unit'] + '): ' + str(x['after'][0]) + '% - ' + x['note']
                             for x in partials) or 'None retained in this session.'
    remaining = '\n'.join('- ' + x for x in session.get('remaining_issues', [])) or 'See trial outcomes in the ledger.'
    text = f'''# B8 Ghidra-assisted session

Status: {session['status']}. Model/effort: {session['model_label']}.
Elapsed: {elapsed / 60:.2f} minutes, including repository recovery, syncing and local tool setup.
Retail remains reference-only; local-model source editing remains stopped.

| Measure | Start after upstream sync | Latest | Session gain |
| --- | ---: | ---: | ---: |
{table}

Matched code: {measures['matched_code_percent']:.6f}%; source-linked code: {measures['complete_code_percent']:.6f}%.
Upstream gains excluded: {json.dumps(session.get('upstream_gains', {}))}.
Original/rebuilt B8 DOL SHA-1: `{checkpoint['verified_dol_sha1']}`.
Strict target-unit function/data nonregression and global match/link measures guard retained trials; exact executable verification does not imply unfinished source-linked units are complete.

Usage began at {session['initial_usage']['primary_used_percent']}% current-window / {session['initial_usage']['weekly_used_percent']}% weekly used;
latest is {usage['primary_used_percent']}% / {usage['weekly_used_percent']}% used.
Consumed percentage points: {json.dumps(row['account_usage_consumed_percentage_points'])}.
Refill observed: {row['refill_observed']}. These are rounded account-wide readings, not billing-token counts.
Across refills endpoint consumption is unavailable; the ledger keeps snapshots instead of inventing totals.

Retained partial reconstructions ({len(partials)}):

{partial_text}

Remaining issues:

{remaining}

Git checkpoints and Ghidra audit/provenance are recorded in the JSON ledger. Source commits explicitly credit ChatGPT; private inputs, native tools and generated pseudocode remain ignored. Ghidra drafts require semantic and assembly review before trials. Different targets and setup overhead prevent treating this as a controlled model benchmark.
'''
    ledger.with_suffix('.md').write_text(text, encoding='utf-8')
    print(json.dumps(row, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('ledger', type=Path)
    parser.add_argument('--finish', action='store_true')
    args = parser.parse_args()
    ledger = args.ledger.resolve()
    if not ledger.is_relative_to(ROOT / 'doc') or not ledger.is_file():
        parser.error('Use an existing repository doc ledger')
    report(ledger, args.finish)


if __name__ == '__main__':
    main()

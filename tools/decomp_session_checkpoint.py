"""Save comparable local session statistics after exact native verification.

Account percentages must be supplied by Codex; this tool makes no cloud calls.
Completed local-model runs are collected separately from retained source gains.
"""
import argparse
import json
import os
import subprocess
import time
from pathlib import Path

from decomp_runner import ROOT, Runner, exclusive_lock, sha1, write_json


def usage_delta(initial, current):
    result = {}
    for window in ('primary', 'weekly'):
        used = window + '_used_percent'
        reset = window + '_resets_at'
        if used not in current:
            continue
        # A refill makes endpoint subtraction invalid; never report negative use.
        same_window = (reset in current and reset in initial
                       and current[reset] == initial[reset]
                       and current[used] >= initial[used])
        result[window] = {
            'remaining_percent': max(0, 100 - current[used]),
            'consumed_percentage_points': current[used] - initial[used] if same_window else None,
            'same_window': same_window,
        }
    return result


def start_preparation():
    """Launch the finite artifact-only queue after the native lock is released."""
    if os.name != 'nt':
        raise ValueError('The background launch wrapper currently supports Windows only')
    try:
        result = subprocess.run(['powershell', '-NoProfile', '-File', str(ROOT / 'tools/decomp_prepare.ps1'),
                                 '-Action', 'Run', '-Minutes', '60', '-Limit', '1024', '-MinScore', '0'],
                                cwd=ROOT, capture_output=True, text=True, timeout=60,
                                creationflags=subprocess.CREATE_NO_WINDOW)
    except subprocess.TimeoutExpired:
        # The wrapper may already have launched its child. Preserve the verified
        # checkpoint and require checking worker status rather than launching twice.
        return {'status': 'launcher_timeout_check_worker_status',
                'mode': 'artifact_only_no_cloud_or_model_calls'}
    return {'exit_code': result.returncode, 'stdout': result.stdout.strip(),
            'stderr': result.stderr.strip(), 'mode': 'artifact_only_no_cloud_or_model_calls'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('ledger', type=Path)
    parser.add_argument('--primary-used', type=int, required=True)
    parser.add_argument('--weekly-used', type=int, required=True)
    parser.add_argument('--primary-reset', type=int)
    parser.add_argument('--weekly-reset', type=int)
    parser.add_argument('--prepare-downtime', action='store_true',
                        help='after saving the verified checkpoint, launch a finite read-only B8 preparation job')
    args = parser.parse_args()
    ledger = args.ledger.resolve()
    if not ledger.is_relative_to(ROOT / 'doc') or not ledger.is_file():
        parser.error('Select an existing repository doc ledger')
    if not all(0 <= n <= 100 for n in (args.primary_used, args.weekly_used)):
        parser.error('Usage must be between 0 and 100 percent')
    session = json.loads(ledger.read_text(encoding='utf-8'))
    version = session['target']
    if version != 'SZBE69_B8':
        parser.error('This checkpoint verifies the B8 session only')
    runner = Runner(argparse.Namespace(version=version, minutes=20, timeout=180,
                                      jobs=4, stop_file=None, quota_file=None))
    with exclusive_lock(runner.lock_path):
        runner.recover()
        ok, log = runner.build('session-checkpoint')
        if not ok or sha1(runner.output) != runner.expected:
            raise RuntimeError(f'Checkpoint must preserve the exact B8 executable: {log}')
        measures = runner.refresh()['measures']
    current = {'primary_used_percent': args.primary_used,
               'weekly_used_percent': args.weekly_used}
    for window in ('primary', 'weekly'):
        value = getattr(args, window + '_reset')
        if value is not None:
            current[window + '_resets_at'] = value
    prior = session.get('latest_checkpoint', {})
    checkpoint = {
        'at': time.time(), 'measures': measures,
        'gains': {key: int(measures[key]) - int(session['baseline'][key])
                  for key in ('matched_code', 'matched_functions', 'complete_code',
                              'complete_units', 'matched_data', 'complete_data')},
        'elapsed_seconds': round(time.time() - session['started_at'], 2),
        'account_usage': usage_delta(session['initial_usage'], current),
        'verified_dol_sha1': sha1(runner.output),
    }
    if 'matched_gain_breakdown' in prior:
        # This classification requires human semantic review, not inference.
        checkpoint['matched_gain_breakdown'] = prior['matched_gain_breakdown']
        checkpoint['breakdown_is_reviewed_checkpoint_only'] = True
    runs = []
    for path in (ROOT / 'build/local-agent/runs').glob('*/summary.json'):
        run = json.loads(path.read_text(encoding='utf-8'))
        if run.get('started_at', 0) < session['started_at'] or not run.get('stop_reason'):
            continue
        runs.append({key: run.get(key) for key in ('session', 'local_prompt_tokens',
                    'local_output_tokens', 'verified_proposals', 'elapsed_seconds', 'stop_reason')}
                    | {'attempts': len(run.get('attempts', []))})
    session.update(latest_checkpoint=checkpoint, latest_usage=current,
                   local_helper_runs=sorted(runs, key=lambda row: row['session']))
    write_json(ledger, session)
    if args.prepare_downtime:
        checkpoint['downtime_preparation'] = start_preparation()
        write_json(ledger, session)
    print(json.dumps(checkpoint, indent=2))


if __name__ == '__main__':
    main()

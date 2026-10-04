"""Finite local maintenance/search/model worker; no Codex or cloud API calls."""
import argparse
import contextlib
import json
import os
import subprocess
import sys
import time

from decomp_runner import ROOT, write_json


@contextlib.contextmanager
def keep_awake():
    """Finite process-scoped sleep prevention; do not change power settings."""
    if os.name != 'nt':
        yield
        return
    import ctypes
    set_state = ctypes.windll.kernel32.SetThreadExecutionState
    set_state.argtypes = [ctypes.c_uint]
    set_state.restype = ctypes.c_uint
    active = bool(set_state(0x80000001))  # CONTINUOUS | SYSTEM_REQUIRED; display may turn off.
    if not active:
        print('Could not prevent sleep; keep Windows awake manually.', flush=True)
    try:
        yield
    finally:
        if active:
            set_state(0x80000000)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--minutes', type=float, default=120)
    parser.add_argument('--attempts-per-task', type=int, default=8)
    parser.add_argument('--limit', type=int, default=32)
    args = parser.parse_args()
    if min(args.minutes, args.attempts_per_task, args.limit) <= 0:
        parser.error('Limits must be positive')
    start = time.time()
    deadline = time.monotonic() + args.minutes * 60
    state = ROOT / 'build/local-agent'
    summary = {'started_at': start, 'steps': []}
    # Existing native runners own their journals and hold the shared build lock.
    # Do not force-kill them at the overall deadline: cleanup reserves matter.
    steps = [
        ('library-audit', ['tools/library_audit.py', '--version', 'SZBE69_B8', '--output-dir', str(state / 'audit')], 10),
        ('deterministic-search', ['tools/local_decomp_search.py', 'run', '--minutes', '15', '--limit', '40'], 900),
        ('source-link-trials', ['tools/decomp_runner.py', 'run', '--version', 'SZBE69_B8', '--minutes', '15', '--limit', '4', '--stop-file', str(state / 'STOP')], 900),
        ('local-model', ['tools/local_decomp_agent.py', '--attempts-per-task', str(args.attempts_per_task), '--limit', str(args.limit)], None),
    ]
    state.mkdir(parents=True, exist_ok=True)
    for name, argv, cap in steps:
        remaining = deadline - time.monotonic()
        if (state / 'STOP').exists() or remaining < 375:
            summary['stop_reason'] = 'stop_file_or_deadline_reserve'
            break
        if cap:
            if '--minutes' in argv:
                argv[argv.index('--minutes') + 1] = str(min(cap, remaining) / 60)
        else:
            argv += ['--minutes', str(remaining / 60)]
        step_start = time.time()
        print(f'Starting local step: {name}', flush=True)
        result = subprocess.run([sys.executable] + argv, cwd=ROOT)
        summary['steps'].append({'step': name, 'exit_code': result.returncode,
                                 'elapsed_seconds': round(time.time() - step_start, 2)})
        summary['elapsed_seconds'] = round(time.time() - start, 2)
        write_json(state / 'pipeline-latest.json', summary)
        if result.returncode:
            summary['stop_reason'] = 'step_failed_' + name
            break
    else:
        summary['stop_reason'] = 'finite_pipeline_completed'
    write_json(state / 'pipeline-latest.json', summary)
    print(json.dumps(summary, indent=2), flush=True)
    if summary['stop_reason'].startswith('step_failed_'):
        sys.exit(1)


if __name__ == '__main__':
    with keep_awake():
        main()

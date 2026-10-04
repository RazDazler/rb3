"""Verify a coordinated source/header edit, with recovery and full DOL checks.

Manifest: {"unit": "main/...", "symbol": "original symbol", "files": [
  {"path": "src/...", "replacements": [{"old": "unique text", "new": "text"}]}]}
Unlike the fast variant runner, this rebuilds all dependencies for each trial.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import time

from decomp_runner import ROOT, Runner, digest, exclusive_lock, matching_entry, replace_entry, sha1, write_bytes, write_json
from decomp_variants import apply_replacements, improvement, scores
from dol_compare import compare_dols, split_ranges
from decomp_inspect import inspect_function
import re


def prepare_sources(files):
    prepared = []
    seen = set()
    for item in files:
        path = (ROOT / item['path']).resolve()
        if not path.is_relative_to((ROOT / 'src').resolve()) or not path.is_file():
            raise ValueError('Trial sources must be existing files under src')
        if path in seen:
            raise ValueError('A trial source is listed more than once')
        seen.add(path)
        original = path.read_bytes()
        prepared.append((path, original, apply_replacements(original, item['replacements'])))
    if not prepared:
        raise ValueError('Trial has no source edits')
    return prepared


def run_trial(runner, manifest):
    runner.recover()
    reason = runner.stop_reason()
    if reason:
        return {'accepted': False, 'stop_reason': reason}
    prepared = prepare_sources(manifest['files'])
    ok, log = runner.build('source-trial-baseline')
    if not ok or sha1(runner.original) != runner.expected:
        raise RuntimeError(f'Baseline executable verification failed: {log}')
    baseline = runner.refresh()['measures']
    initial = runner.compare(manifest['unit'])
    if manifest['symbol'] not in scores(initial):
        raise ValueError('Target original symbol is absent')
    trial_config = runner.config.read_bytes()
    adopt = getattr(runner.args, 'adopt_unit', False)
    if adopt:
        project = json.loads((ROOT / 'objdiff.json').read_text())
        unit = next(u for u in project['units'] if u['name'] == manifest['unit'])
        source = unit['metadata']['source_path'].replace('\\', '/')
        if not source.startswith('src/'):
            raise ValueError('Adoption requires a configured source unit')
        source = source[4:]
        entries = [lib['objects'][source] for lib in json.loads(trial_config).values()
                   if source in lib['objects']]
        if len(entries) != 1:
            raise ValueError('Adoption requires exactly one object entry')
        trial_config = replace_entry(trial_config, source, matching_entry(entries[0], []))
    prefix = runner.directory / str(time.time_ns())
    config_backup = Path(str(prefix) + '-objects-backup.json')
    dol_backup = Path(str(prefix) + '-main-backup.dol')
    shutil.copyfile(runner.config, config_backup)
    shutil.copyfile(runner.output, dol_backup)
    pending = {'config_backup': str(config_backup), 'dol_backup': str(dol_backup),
               'original_config_sha256': digest(config_backup),
               'trial_config_sha256': hashlib.sha256(trial_config).hexdigest(), 'sources': []}
    for index, (path, original, trial) in enumerate(prepared):
        backup = Path(str(prefix) + f'-source-{index}.bin')
        backup.write_bytes(original)
        pending['sources'].append({'path': str(path), 'backup': str(backup),
                                  'original_sha256': digest(backup),
                                  'allowed_sha256': [hashlib.sha256(x).hexdigest()
                                                     for x in (original, trial)]})
    write_json(runner.journal, pending)
    result = {'unit': manifest['unit'], 'symbol': manifest['symbol'], 'accepted': False}
    try:
        for path, _, trial in prepared:
            write_bytes(path, trial)
        if adopt:
            write_bytes(runner.config, trial_config)
        elf = runner.output.with_suffix('.elf')
        previous_elf_time = elf.stat().st_mtime_ns if elf.exists() else None
        ok, log = runner.build('source-trial')
        result.update(build_verified=ok, log=str(log))
        if not ok and runner.output.exists() and sha1(runner.output) != runner.expected:
            focus = split_ranges(runner.config.parent / 'splits.txt', source) if adopt else ()
            physical = Path(str(prefix) + '-dol-diff.json')
            write_json(physical, compare_dols(runner.original, runner.output, focus))
            result['physical_diff'] = str(physical)
            if elf.exists() and elf.stat().st_mtime_ns != previous_elf_time:
                _, diagnostic = runner.command([runner.dtk, 'dol', 'diff',
                                                str(runner.config.parent / 'config.yml'),
                                                str(elf)], 'source-layout-diff')
                result['layout_log'] = str(diagnostic)
        if ok:
            after = runner.refresh()['measures']
            comparison = runner.compare(manifest['unit'])
            comparison_path = Path(str(prefix) + '-object-summary.json')
            write_json(comparison_path, comparison)
            result['object_summary'] = str(comparison_path)
            diff_path = runner.directory / (re.sub(r'[^\w-]', '_', manifest['unit']) + '-diff.json')
            try:
                result['target_inspection'] = inspect_function(
                    json.loads(diff_path.read_text(encoding='utf-8')), manifest['symbol'])
            except ValueError as error:
                result['inspection_unavailable'] = str(error)
            improved, detail = improvement(initial, comparison, manifest['symbol'],
                                           runner.args.allow_new_helper,
                                           require_target_improvement=not adopt)
            regressions = [key for key in ('matched_code', 'matched_functions',
                                          'complete_code', 'matched_data')
                           if int(after.get(key, 0)) < int(baseline.get(key, 0))]
            result.update(before=scores(initial)[manifest['symbol']],
                          after=scores(comparison)[manifest['symbol']], reason=detail,
                          global_regressions=regressions,
                          measure_gains={key: int(after.get(key, 0)) - int(baseline.get(key, 0))
                                         for key in ('matched_code', 'matched_functions',
                                                     'complete_code', 'matched_data')})
            linked_gain = int(after.get('complete_code', 0)) > int(baseline.get('complete_code', 0))
            result['accepted'] = improved and not regressions and (not adopt or linked_gain)
            result['source_adopted'] = adopt and result['accepted']
        # Unattended model proposals are verified, then restored for semantic review.
        # Keep the journal until recovery so review-only trials are transactional too.
        result['retained'] = result['accepted'] and not getattr(runner.args, 'review_only', False)
        if result['retained']:
            runner.journal.unlink()
    finally:
        if runner.journal.exists():
            runner.recover()
    result['verified_dol_sha1'] = sha1(runner.output)
    write_json(Path(str(prefix) + '-source-trial.json'), result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--version', default='SZBE69_B8')
    parser.add_argument('--minutes', type=float, default=20)
    parser.add_argument('--timeout', type=float, default=180)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--quota-file', type=Path)
    parser.add_argument('--stop-file', type=Path)
    parser.add_argument('--allow-new-helper', action='append', default=[])
    parser.add_argument('--adopt-unit', action='store_true',
                        help='also source-link this unit; requires increased verified linked code')
    parser.add_argument('--review-only', action='store_true',
                        help='verify the proposal, then transactionally restore sources for semantic review')
    args = parser.parse_args()
    if min(args.minutes, args.timeout, args.jobs) <= 0:
        parser.error('Numeric limits must be positive')
    runner = Runner(args)
    manifest = json.loads(args.manifest.read_text(encoding='utf-8'))
    with exclusive_lock(runner.lock_path):
        result = run_trial(runner, manifest)
    print(json.dumps({key: value for key, value in result.items()
                      if key != 'target_inspection'}, indent=2))


if __name__ == '__main__':
    main()

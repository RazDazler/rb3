"""Compile explicit source variants, reject regressions, retain only verified improvements.

Manifest: {"unit": "main/...", "symbol": "mangled name", "variants": [
  {"name": "candidate", "replacements": [{"old": "unique text", "new": "replacement"}]}]}
All variants are independent edits against the initial source, not cumulative edits.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
from decomp_runner import ROOT, Runner, exclusive_lock, sha1, write_bytes, write_json
from decomp_inspect import inspect_function


def apply_replacements(original, replacements):
    newline = b"\r\n" if b"\r\n" in original else b"\n"
    result = original
    for replacement in replacements:
        old, new = (replacement[key].replace("\r\n", "\n").encode("utf-8").replace(b"\n", newline)
                    for key in ("old", "new"))
        if not old:
            raise ValueError("Each replacement must match exactly once")
        # Some inherited source files mix CRLF and LF. Match newline spelling
        # flexibly, while preserving all bytes outside the unique edited span.
        pattern = b"\r?\n".join(re.escape(part) for part in old.replace(b"\r\n", b"\n").split(b"\n"))
        matches = list(re.finditer(pattern, result))
        if len(matches) != 1:
            raise ValueError("Each replacement must match exactly once")
        match = matches[0]
        result = result[:match.start()] + new + result[match.end():]
    return result


def scores(rows):
    return {row["name"]: (float(row.get("match_percent", 0)), -sum(row.get("differences", {}).values()))
            for row in rows if row.get("side", "original") == "original"}


def improvement(baseline, candidate, symbol, allowed_helpers=(), require_target_improvement=True):
    before, after = scores(baseline), scores(candidate)
    if symbol not in before or symbol not in after or after[symbol] < before[symbol]:
        return False, "target_did_not_improve"
    if require_target_improvement and after[symbol] == before[symbol]:
        return False, "target_did_not_improve"
    regressed = [name for name, score in before.items() if after.get(name, (-1, 0)) < score]
    if regressed:
        return False, "other_functions_regressed: " + ", ".join(regressed[:5])
    def helpers(rows):
        result = Counter()
        for row in rows:
            if row.get("side") != "compiled" or not row.get("unpaired") or row["name"] in allowed_helpers:
                continue
            # Decomp forcing macros embed __LINE__; moving lines renames the same helper.
            name = re.sub(r'^(FORCEACTIVE|FORCEFUNC|FORCELITERAL|FORCEBLOCK)([^0-9]+)\d+(__)', r'\1\2#\3', row["name"])
            result[(name, row.get("bytes", 0))] += 1
        return result
    added_helpers = helpers(candidate) - helpers(baseline)
    if added_helpers:
        names = []
        for row in candidate:
            if row.get("side") != "compiled" or not row.get("unpaired"):
                continue
            normalized = re.sub(r'^(FORCEACTIVE|FORCEFUNC|FORCELITERAL|FORCEBLOCK)([^0-9]+)\d+(__)', r'\1\2#\3', row["name"])
            if (normalized, row.get("bytes", 0)) in added_helpers:
                names.append(row["name"])
        return False, "new_unpaired_helpers: " + ", ".join(sorted(names))
    return True, "improved_without_function_regressions"


def run_variants(runner, manifest):
    runner.recover()
    reason = runner.stop_reason()
    if reason:
        return {"stop_reason": reason, "attempts": []}
    if not runner.output.exists() or sha1(runner.output) != runner.expected or sha1(runner.original) != runner.expected:
        raise RuntimeError("Original and rebuilt DOL must match before trials")
    ok, log = runner.build("variant-baseline")
    if not ok:
        raise RuntimeError(f"Baseline build failed: {log}")
    baseline_report = runner.refresh()
    units = {u["name"]: u for u in json.loads((ROOT / "objdiff.json").read_text())["units"]}
    metadata = units[manifest["unit"]]
    # Trusted manifests may vary a shared header while comparing its configured
    # consumer. Final full dependency rebuild and global checks still apply.
    source = (ROOT / manifest.get("trial_source", metadata["metadata"]["source_path"])).resolve()
    if not source.is_relative_to((ROOT / "src").resolve()) or not source.is_file():
        raise ValueError("Variant unit must have a source file under src")
    target = str(Path(metadata["base_path"]).as_posix())
    original = source.read_bytes()
    # Validate every edit before any mutation.
    trials = [(v["name"], apply_replacements(original, v["replacements"])) for v in manifest["variants"]]
    if len({name for name, _ in trials}) != len(trials):
        raise ValueError("Variant names must be unique")
    initial = runner.compare(manifest["unit"])
    if manifest["symbol"] not in scores(initial):
        raise ValueError("Target symbol is absent from original object")
    source_backup = runner.directory / "source-before-trial.bin"
    config_backup = runner.directory / "objects-before-trial.json"
    dol_backup = runner.directory / "main-before-trial.dol"
    source_backup.write_bytes(original)
    config_backup.write_bytes(runner.config.read_bytes())
    shutil.copy2(runner.output, dol_backup)
    config_hash = hashlib.sha256(config_backup.read_bytes()).hexdigest()
    pending = {"unit": manifest["unit"], "config_backup": str(config_backup), "dol_backup": str(dol_backup),
               "original_config_sha256": config_hash, "trial_config_sha256": config_hash,
               "sources": [{"path": str(source), "backup": str(source_backup),
                            "original_sha256": hashlib.sha256(original).hexdigest(),
                            "allowed_sha256": [hashlib.sha256(original).hexdigest()]}]}
    best, best_rows, best_name = original, initial, None
    attempts = []
    started = time.time()
    completed = False
    reason = "variants_exhausted"
    try:
        for name, trial in trials[:runner.args.limit]:
            reason = runner.stop_reason()
            if reason:
                break
            pending["sources"][0]["allowed_sha256"] = list({hashlib.sha256(data).hexdigest() for data in (original, best, source.read_bytes(), trial)})
            write_json(runner.journal, pending)
            write_bytes(source, trial)
            code, log = runner.command([runner.ninja, "-j", str(runner.args.jobs), target], "variant-compile")
            row = {"name": name, "compiled": code == 0, "log": str(log)}
            if code == 0:
                comparison = runner.compare(manifest["unit"])
                diff_path = runner.directory / (re.sub(r"[^\w-]", "_", manifest["unit"]) + "-diff.json")
                try:
                    row["target_inspection"] = inspect_function(
                        json.loads(diff_path.read_text(encoding="utf-8")), manifest["symbol"]
                    )
                except ValueError as error:
                    row["inspection_unavailable"] = str(error)
                improved, detail = improvement(best_rows, comparison, manifest["symbol"], runner.args.allow_new_helper)
                row.update(improved=improved, reason=detail, target_score=scores(comparison).get(manifest["symbol"]))
                if improved:
                    best, best_rows, best_name = trial, comparison, name
            attempts.append(row)
            write_json(runner.directory / "variant-checkpoint.json", {"unit": manifest["unit"], "attempts": attempts, "best": best_name})
            if best_name and scores(best_rows)[manifest["symbol"]][0] == 100:
                reason = "target_fully_matched"
                break
            print(f'{name}: {row.get("reason", "compile_failed")}', flush=True)
        if runner.journal.exists():
            pending["sources"][0]["allowed_sha256"] = list({hashlib.sha256(data).hexdigest() for data in (original, best, source.read_bytes())})
            write_json(runner.journal, pending)
            write_bytes(source, best)
            ok, log = runner.build("variant-final")
            if not ok:
                raise RuntimeError(f"Final executable verification failed: {log}")
            final_report = runner.refresh()
            for key in ("matched_code", "matched_functions", "complete_code", "matched_data"):
                if int(final_report["measures"].get(key, 0)) < int(baseline_report["measures"].get(key, 0)):
                    raise RuntimeError(f"Global {key} regressed; rolling back")
            final_rows = runner.compare(manifest["unit"])
            if best_name and not improvement(initial, final_rows, manifest["symbol"], runner.args.allow_new_helper)[0]:
                raise RuntimeError("Final object comparison did not preserve the improvement")
            completed = True
            runner.journal.unlink()
    finally:
        if not completed and runner.journal.exists():
            runner.recover()
    result = {"unit": manifest["unit"], "symbol": manifest["symbol"], "started_at": started,
              "stop_reason": reason or "variants_exhausted", "accepted_variant": best_name if completed else None,
              "before": scores(initial)[manifest["symbol"]], "after": scores(best_rows)[manifest["symbol"]],
              "attempts": attempts, "verified_dol_sha1": sha1(runner.output)}
    write_json(runner.directory / f"{time.time_ns()}-variants.json", result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--version", default="SZBE69")
    parser.add_argument("--minutes", type=float, default=30)
    parser.add_argument("--timeout", type=float, default=120)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--limit", type=int, default=100)
    parser.add_argument("--quota-file", type=Path)
    parser.add_argument("--stop-file", type=Path)
    parser.add_argument("--allow-new-helper", action="append", default=[], metavar="SYMBOL",
                        help="allow an explicitly reviewed extra compiler helper; executable and regression checks still apply")
    args = parser.parse_args()
    if min(args.minutes, args.timeout, args.jobs, args.limit) <= 0:
        parser.error("All numeric limits must be positive")
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    runner = Runner(args)
    with exclusive_lock(runner.lock_path):
        result = run_variants(runner, manifest)
    print(json.dumps({k: v for k, v in result.items() if k != "attempts"}, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, RuntimeError, OSError, subprocess.TimeoutExpired) as error:
        print(f"Stopped: {error}", file=sys.stderr)
        sys.exit(1)

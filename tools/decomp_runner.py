"""Local, transactional objdiff comparisons and source-linking trials (stdlib only)."""
from __future__ import annotations

import argparse
import contextlib
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import time
from dol_compare import compare_dols, split_ranges
from decomp_rtti import comparison_mappings

ROOT = Path(__file__).resolve().parents[1]
VARIANTS = {"default": [], "ipa-off": ["-ipa off"],
            "align4": ["-func_align 4"], "align16": ["-func_align 16"]}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def sha1(path):
    return hashlib.sha1(Path(path).read_bytes()).hexdigest()


def write_bytes(path, data):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + ".runner-tmp")
    temporary.write_bytes(data)
    temporary.replace(path)


def write_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def replace_entry(raw, name, value):
    """Replace only one JSON value, preserving all other bytes and CRLF style."""
    text = raw.decode("utf-8")
    match = re.search(r'(?m)^([ \t]*)' + re.escape(json.dumps(name)) + r':\s*', text)
    if not match:
        raise ValueError(f"Missing object entry: {name}")
    _, end = json.JSONDecoder().raw_decode(text, match.end())
    newline = "\r\n" if "\r\n" in text else "\n"
    rendered = json.dumps(value, indent=4).replace("\n", newline + match[1])
    return (text[:match.end()] + rendered + text[end:]).encode("utf-8")


def matching_entry(entry, flags):
    result = copy.deepcopy(entry) if isinstance(entry, dict) else {"status": entry}
    result["status"] = "Matching"
    if flags:
        result["extra_cflags"] = result.get("extra_cflags", []) + flags
    return result


def candidates(report, entries, unit=None):
    result = []
    for item in report.get("units", []):
        meta, measures = item.get("metadata", {}), item.get("measures", {})
        source = meta.get("source_path", "").replace("\\", "/")
        name = source[4:] if source.startswith("src/") else ""
        if name not in entries or meta.get("complete"):
            continue
        if unit and unit not in (name, item["name"]):
            continue
        data_percent = measures.get("matched_data_percent", 0) if int(measures.get("total_data", 0)) else 100
        if not unit and (measures.get("matched_code_percent") != 100 or data_percent != 100):
            continue
        if int(measures.get("total_code", 0)):
            result.append({"unit": item["name"], "source": name, "measures": measures})
    return sorted(result, key=lambda x: (int(x["measures"]["total_code"]), x["unit"]))


def rank_functions(report, max_size=256):
    rows = []
    for unit in report.get("units", []):
        if not unit.get("metadata", {}).get("source_path"):
            continue
        for function in unit.get("functions", []):
            size = int(function.get("size", 0))
            score = function.get("fuzzy_match_percent", 0)
            if 0 < size <= max_size and 70 <= score < 100:
                rows.append({"unit": unit["name"], "function": function["name"],
                             "bytes": size, "fuzzy_percent": score})
    return sorted(rows, key=lambda x: (-x["fuzzy_percent"], x["bytes"], x["function"]))


def summarize_diff(data):
    """Retain instruction/relocation differences without dumping whole disassembly."""
    rows = []
    for section in data.get("left", {}).get("sections", []):
        for item in section.get("symbols", []):
            if "match_percent" not in item:
                if item.get("instructions"):
                    rows.append({"name": item["symbol"]["name"], "bytes": int(item["symbol"].get("size", 0)),
                                 "side": "original", "unpaired": True, "exact_comparison": False})
                continue
            counts, relocations = {}, []
            for line in item.get("instructions", []):
                kind = line.get("diff_kind")
                if kind and kind != "DIFF_NONE":
                    counts[kind] = counts.get(kind, 0) + 1
                    target = line.get("instruction", {}).get("relocation", {}).get("target", {})
                    symbol = target.get("symbol", {}).get("name")
                    if symbol and symbol not in relocations:
                        relocations.append(symbol)
            symbol = item["symbol"]
            rows.append({"name": symbol["name"], "bytes": int(symbol.get("size", 0)),
                         "match_percent": item["match_percent"], "differences": counts,
                         "target_relocations": relocations,
                         "side": "original", "exact_comparison": item["match_percent"] == 100 and not counts})
    for section in data.get("right", {}).get("sections", []):
        for item in section.get("symbols", []):
            if "match_percent" not in item and item.get("instructions"):
                rows.append({"name": item["symbol"]["name"], "bytes": int(item["symbol"].get("size", 0)),
                             "side": "compiled", "unpaired": True, "exact_comparison": False})
    return rows


def failure_summary(log):
    if "computed checksum" in log or "main.dol: FAILED" in log:
        return {"kind": "executable_mismatch", "next_step": "Inspect layout and relocation diagnostics."}
    duplicate = re.findall(r"multiply-defined: '([^']+)'", log)
    if duplicate:
        return {"kind": "duplicate_symbols", "symbols": sorted(set(duplicate)),
                "next_step": "Check original section ownership before source linking."}
    undefined = re.findall(r"undefined: '([^']+)'", log)
    if undefined:
        return {"kind": "undefined_symbols", "symbols": sorted(set(undefined)),
                "next_step": "Check exported source symbols, inlining and original section ownership."}
    return {"kind": "build_failure", "next_step": "Inspect the saved compiler/linker log."}


def remaining_usage(data):
    if "remaining_percent" in data:
        value = float(data["remaining_percent"])
        if not 0 <= value <= 100:
            raise ValueError("remaining_percent must be between 0 and 100")
        return value
    limits = data.get("rateLimitsByLimitId", {}).values() or [data.get("rateLimits", data)]
    percentages = [100 - float(window["usedPercent"]) for limit in limits
                   for key in ("primary", "secondary")
                   if (window := limit.get(key)) and window.get("usedPercent") is not None]
    if not percentages:
        raise ValueError("Quota file has no recognized usage measurement")
    return min(percentages)


@contextlib.contextmanager
def exclusive_lock(path):
    """OS releases the advisory lock after crashes; the marker file may remain."""
    with Path(path).open("a+b") as handle:
        handle.seek(0)
        if not handle.read(1):
            handle.write(b"0")
            handle.flush()
        handle.seek(0)
        if os.name == "nt":
            import msvcrt
            msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        try:
            yield
        finally:
            if os.name == "nt":
                handle.seek(0)
                msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(handle.fileno(), fcntl.LOCK_UN)


class Runner:
    def __init__(self, args):
        self.args = args
        if not re.fullmatch(r"[A-Za-z0-9_]+", args.version):
            raise ValueError("Invalid version")
        self.config = ROOT / "config" / args.version / "objects.json"
        settings = (self.config.parent / "config.yml").read_text()
        pinned_hash = re.search(r"(?m)^hash:\s*([0-9a-fA-F]{40})\s*$", settings)
        if pinned_hash:
            self.expected = pinned_hash[1].lower()
        else:
            # Older supported configurations pin the executable in build.sha1.
            rows = re.findall(r"(?m)^([0-9a-fA-F]{40})\s+\*?(.+?)\s*$",
                              (self.config.parent / "build.sha1").read_text())
            hashes = [value.lower() for value, path in rows
                      if path.replace("\\", "/") == f"build/{args.version}/main.dol"]
            if len(hashes) != 1:
                raise ValueError("Expected exactly one pinned executable checksum")
            self.expected = hashes[0]
        self.original = ROOT / re.search(r"(?m)^object:\s*(.+)$", settings)[1].strip()
        self.directory = ROOT / "build" / "decomp" / args.version
        self.directory.mkdir(parents=True, exist_ok=True)
        # Versions share build.ninja, objdiff.json and source files.
        self.lock_path = self.directory.parent / "runner.lock"
        self.journal = self.directory / "pending.json"
        self.history_file = self.directory / "trials.json"
        self.output = ROOT / "build" / args.version / "main.dol"
        self.report_path = ROOT / "build" / args.version / "report.json"
        self.deadline = time.monotonic() + args.minutes * 60
        self.counter = 0
        self.env = dict(os.environ, PYTHONPATH=str(ROOT / "build/python"))
        self.ninja = str(ROOT / "build/python/bin/ninja.exe") if os.name == "nt" else shutil.which("ninja")
        self.objdiff = str(ROOT / "build/tools" / ("objdiff-cli.exe" if os.name == "nt" else "objdiff-cli"))
        self.dtk = str(ROOT / "build/tools" / ("dtk.exe" if os.name == "nt" else "dtk"))

    def command(self, argv, label, timeout=None):
        started = time.perf_counter()
        started_at = time.time()
        self.counter += 1
        log = self.directory / f"{self.counter:03d}-{label}.log"
        # Timestamp makes logs survive repeated invocations.
        log = log.with_name(f"{time.time_ns()}-{log.name}")
        with log.open("w", encoding="utf-8") as stream:
            process = subprocess.Popen(argv, cwd=ROOT, env=self.env, stdout=stream,
                                       stderr=subprocess.STDOUT, start_new_session=os.name != "nt")
            try:
                code = process.wait(timeout=timeout or self.args.timeout)
            except (subprocess.TimeoutExpired, KeyboardInterrupt):
                if os.name == "nt":
                    subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                else:
                    os.killpg(process.pid, signal.SIGKILL)
                process.wait()
                raise
        # Diagnostic telemetry lets future sessions distinguish compiler/report/
        # comparison cost from model latency without reading full native logs.
        try:
            with (self.directory / 'command-timings.jsonl').open('a', encoding='utf-8') as stream:
                stream.write(json.dumps({'label': label, 'started_at': started_at,
                                         'seconds': round(time.perf_counter() - started, 6),
                                         'exit_code': code, 'log': str(log)}) + '\n')
        except OSError:
            pass  # Optional telemetry must never interrupt verification/recovery.
        return code, log

    def build(self, label):
        code, log = self.command([sys.executable, "configure.py", "--version", self.args.version], label + "-configure")
        if code:
            return False, log
        code, log = self.command([self.ninja, "-j", str(self.args.jobs)], label)
        return code == 0 and self.output.exists() and sha1(self.output) == self.expected, log

    def refresh(self):
        code, log = self.command([self.objdiff, "report", "generate", "-o", str(self.report_path)], "report")
        if code:
            raise RuntimeError(f"Report failed: {log}")
        return json.loads(self.report_path.read_text())

    def stop_reason(self):
        stop_file = self.args.stop_file or self.directory / "STOP"
        if Path(stop_file).exists():
            return "stop_file"
        if self.args.quota_file and remaining_usage(json.loads(Path(self.args.quota_file).read_text())) <= 1:
            return "usage_at_or_below_one_percent"
        if self.deadline - time.monotonic() < self.args.timeout * 2 + 15:
            return "deadline_reserve"
        return None

    def compare(self, unit):
        project = json.loads((ROOT / "objdiff.json").read_text())
        matches = [item for item in project["units"] if item["name"] == unit]
        if len(matches) != 1:
            raise ValueError("Expected one configured comparison unit")
        version_root = (ROOT / "build" / self.args.version).resolve()
        if any(not matches[0].get(key) or
               not (ROOT / matches[0][key]).resolve().is_relative_to(version_root)
               for key in ("target_path", "base_path")):
            raise RuntimeError(f"objdiff.json is configured for another version; configure {self.args.version} first")
        path = self.directory / (re.sub(r"[^\w-]", "_", unit) + "-diff.json")
        mappings = comparison_mappings(ROOT / matches[0]['target_path'],
                                 ROOT / matches[0]['base_path'])
        write_json(path.with_name(path.stem + '-symbol-mappings.json'), mappings)
        mapping_args = [arg for pair in sorted(mappings.items())
                        for arg in ('--mapping', '='.join(pair))]
        code, log = self.command([self.objdiff, "diff", "-p", str(ROOT), "-u", unit,
                                  "-o", str(path), "--format", "json-pretty"]
                                 + mapping_args, "objdiff")
        if code:
            raise RuntimeError(f"objdiff failed: {log}")
        result = summarize_diff(json.loads(path.read_text()))
        write_json(path.with_name(path.stem + "-summary.json"), result)
        return result

    def recover(self):
        if not self.journal.exists():
            return
        pending = json.loads(self.journal.read_text())
        if digest(self.config) not in (pending["trial_config_sha256"], pending["original_config_sha256"]):
            raise RuntimeError("Configuration changed outside the runner; refusing to overwrite it. Inspect pending.json.")
        for source in pending.get("sources", []):
            if digest(source["path"]) not in source["allowed_sha256"]:
                raise RuntimeError("Source changed outside the runner; refusing to overwrite it. Inspect pending.json.")
        # Validate every backup before restoring anything, including the fallback DOL.
        if digest(pending["config_backup"]) != pending["original_config_sha256"]:
            raise RuntimeError("Configuration backup changed; recovery left all files untouched.")
        if sha1(pending["dol_backup"]) != self.expected:
            raise RuntimeError("Executable backup does not match the pinned checksum; recovery left all files untouched.")
        for source in pending.get("sources", []):
            backup_hash = digest(source["backup"])
            expected_hash = source.get("original_sha256")
            if (expected_hash is not None and backup_hash != expected_hash) or backup_hash not in source["allowed_sha256"]:
                raise RuntimeError("Source backup changed; recovery left all files untouched.")
        for source in pending.get("sources", []):
            write_bytes(source["path"], Path(source["backup"]).read_bytes())
        write_bytes(self.config, Path(pending["config_backup"]).read_bytes())
        self.restore_build(Path(pending["dol_backup"]), "recover")
        self.refresh()
        self.journal.unlink()

    def restore_build(self, backup, label):
        if sha1(backup) != self.expected:
            raise RuntimeError("Refusing to restore an executable backup with an invalid pinned checksum.")
        try:
            ok, log = self.build(label)
            if not ok:
                raise RuntimeError(f"Restore build failed: {log}")
        except BaseException:
            shutil.copy2(backup, self.output)
            print("Verified DOL restored; journal retained. Run recover before further work.", file=sys.stderr)
            raise

    def run(self):
        self.recover()
        if self.stop_reason():
            return {"stop_reason": self.stop_reason(), "attempts": []}
        if sha1(self.original) != self.expected or not self.output.exists() or sha1(self.output) != self.expected:
            raise RuntimeError("Original and current DOL must match config.yml before trials.")
        ok, log = self.build("baseline")
        if not ok:
            raise RuntimeError(f"Baseline build failed: {log}")
        report = self.refresh()
        entries = {p: v for lib in json.loads(self.config.read_text()).values() for p, v in lib["objects"].items()}
        history = json.loads(self.history_file.read_text()) if self.history_file.exists() else []
        units = {u["name"]: u for u in json.loads((ROOT / "objdiff.json").read_text())["units"]}
        results = []
        reason = "candidates_exhausted"
        for candidate in candidates(report, entries, self.args.unit):
            reason = self.stop_reason()
            if reason or len(results) >= self.args.limit:
                reason = reason or "limit"
                break
            name, unit = candidate["source"], candidate["unit"]
            if not (ROOT / "src" / name).is_file():
                continue
            metadata = units[unit]
            fingerprint = hashlib.sha256(self.config.read_bytes() +
                (ROOT / metadata["base_path"]).read_bytes() +
                (ROOT / metadata["target_path"]).read_bytes()).hexdigest()
            requested_variants = set(self.args.variants.split(","))
            previous_variants = {a["variant"] for h in history if h["unit"] == unit and h["fingerprint"] == fingerprint
                                 for a in h.get("attempts", [])}
            if not self.args.retry and requested_variants <= previous_variants:
                continue
            original = self.config.read_bytes()
            backup = self.directory / "objects-before-trial.json"
            backup.write_bytes(original)
            dol_backup = self.directory / "main-before-trial.dol"
            shutil.copy2(self.output, dol_backup)
            accepted = False
            record = dict(candidate, fingerprint=fingerprint, attempts=[], started_at=time.time())
            try:
                for variant in self.args.variants.split(","):
                    reason = self.stop_reason()
                    if reason:
                        break
                    trial = replace_entry(original, name, matching_entry(entries[name], VARIANTS[variant]))
                    # Write the journal first so an interrupted config write is recoverable.
                    write_json(self.journal, {"unit": unit, "config_backup": str(backup),
                        "dol_backup": str(dol_backup), "original_config_sha256": hashlib.sha256(original).hexdigest(),
                        "trial_config_sha256": hashlib.sha256(trial).hexdigest()})
                    write_bytes(self.config, trial)
                    elf = ROOT / "build" / self.args.version / "main.elf"
                    old_mtime = elf.stat().st_mtime_ns if elf.exists() else None
                    accepted, log = self.build(re.sub(r"[^\w-]", "_", name) + "-" + variant)
                    attempt = {"variant": variant, "accepted": accepted, "log": str(log)}
                    if not accepted:
                        attempt.update(failure_summary(log.read_text(errors="replace")))
                        if self.output.exists() and sha1(self.output) != self.expected:
                            physical = log.with_name(log.stem + "-dol-bytes.json")
                            write_json(physical, compare_dols(self.original, self.output,
                                split_ranges(self.config.parent / "splits.txt", name)))
                            attempt["physical_diff"] = str(physical)
                        if elf.exists() and elf.stat().st_mtime_ns != old_mtime:
                            _, diagnostic = self.command([self.dtk, "dol", "diff", str(self.config.parent / "config.yml"), str(elf)], "layout-diff")
                            attempt["layout_log"] = str(diagnostic)
                    record["attempts"].append(attempt)
                    print(f"{'ACCEPT' if accepted else 'REJECT'} {name} ({variant})", flush=True)
                    if accepted:
                        entries[name] = matching_entry(entries[name], VARIANTS[variant])
                        break
            finally:
                if self.journal.exists():
                    if not accepted:
                        write_bytes(self.config, original)
                        self.restore_build(dol_backup, "restore")
                    self.refresh()
                    self.journal.unlink()
            if not record["attempts"]:
                break
            record["accepted"] = accepted
            record["comparison"] = self.compare(unit)
            history.append(record)
            results.append(record)
            write_json(self.history_file, history)
        return {"stop_reason": reason or "candidates_exhausted", "attempts": results,
                "verified_dol_sha1": sha1(self.output)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=["rank", "compare", "run", "recover"])
    parser.add_argument("--version", default="SZBE69")
    parser.add_argument("--unit", help="objdiff unit name or source path for run; objdiff unit name for compare")
    parser.add_argument("--minutes", type=float, default=30)
    parser.add_argument("--timeout", type=float, default=120, help="per-command timeout; reserves two timeouts for cleanup")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--limit", type=int, default=10)
    parser.add_argument("--variants", default="default,ipa-off")
    parser.add_argument("--retry", action="store_true", help="retry unchanged candidates from saved history")
    parser.add_argument("--refresh", action="store_true", help="build and regenerate report before rank/compare")
    parser.add_argument("--quota-file", type=Path, help="externally updated usage JSON; no account access is built in")
    parser.add_argument("--stop-file", type=Path)
    args = parser.parse_args()
    if args.minutes <= 0 or args.timeout <= 0 or args.limit < 1 or args.jobs < 1:
        parser.error("minutes, timeout, limit and jobs must be positive")
    if any(v not in VARIANTS for v in args.variants.split(",")):
        parser.error("variants must be selected from " + ",".join(VARIANTS))
    if args.mode == "compare" and not args.unit:
        parser.error("compare requires --unit")
    runner = Runner(args)
    with exclusive_lock(runner.lock_path):
        if args.mode == "run":
            result = runner.run()
        elif args.mode == "recover":
            runner.recover()
            result = {"recovered": True}
        else:
            if runner.journal.exists():
                raise RuntimeError("Pending interrupted trial; run recover first.")
            if args.refresh:
                ok, log = runner.build("refresh")
                if not ok:
                    raise RuntimeError(f"Build failed: {log}")
                runner.refresh()
            if args.mode == "compare":
                result = runner.compare(args.unit)
            else:
                report = json.loads(runner.report_path.read_text())
                entries = {p: v for lib in json.loads(runner.config.read_text()).values() for p, v in lib["objects"].items()}
                result = {"report_sha256": digest(runner.report_path),
                          "source_link_candidates": candidates(report, entries, args.unit),
                          "near_matching_functions": rank_functions(report)[:args.limit]}
        destination = runner.directory / (args.mode + "-result.json")
        write_json(destination, result)
        print(f"Saved {destination}")
        if isinstance(result, dict):
            print(json.dumps({k: v for k, v in result.items() if k not in ("attempts", "near_matching_functions", "source_link_candidates")}, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, RuntimeError, OSError, subprocess.TimeoutExpired) as error:
        print(f"Stopped: {error}", file=sys.stderr)
        sys.exit(1)

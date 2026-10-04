"""Read-only library provenance and local-reference audit; no downloads required."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[1]
LIBRARIES = [
    ("zlib", "src/system/zlib", "doc/src-old/libs/zlib", "zlib.h", r'ZLIB_VERSION\s+"([^"]+)"'),
    ("Speex", "src/system/speex", "doc/src-old/libs/speex", "libspeex/arch.h", r'SPEEX_VERSION\s+"([^"]+)"'),
    ("LibTomCrypt", "src/system/synth/tomcrypt", "doc/src-old/libs/libtomcrypt", "mycrypt.h", r'SCRYPT\s+"([^"]+)"'),
    ("STLport", "src/system/stlport", "doc/src-old/libs/stlport/stlport", "stl/_stlport_version.h", None),
    ("SoundTouch", "src/system/synthwii/soundtouch", None, "include/SoundTouch.h", r'SOUNDTOUCH_VERSION\s+"([^"]+)"'),
    ("Ogg/Vorbis", "src/system/oggvorbis", "doc/src-old/libs/vorbis", "info.c", r'(Xiph\.Org libVorbis I \d+)'),
]
TEXT_EXTENSIONS = {".c", ".cpp", ".h", ".hpp", ".inl", ".inc", ".txt", ".md", ""}


def inventory(root):
    if not root or not root.is_dir():
        return {}
    return {p.relative_to(root).as_posix(): p for p in root.rglob("*")
            if p.is_file() and p.suffix.lower() in TEXT_EXTENSIONS and ".git" not in p.parts}


def compare_trees(left, right):
    bundled, reference = inventory(left), inventory(right)
    result = {"identical": [], "line_endings_only": [], "modified": [],
              "only_bundled": sorted(bundled.keys() - reference.keys()),
              "only_reference": sorted(reference.keys() - bundled.keys())}
    for name in sorted(bundled.keys() & reference.keys()):
        a, b = bundled[name].read_bytes(), reference[name].read_bytes()
        kind = "identical" if a == b else "line_endings_only" if a.replace(b"\r\n", b"\n") == b.replace(b"\r\n", b"\n") else "modified"
        result[kind].append(name)
    result["counts"] = {k: len(v) for k, v in result.items()}
    return result


def version_evidence(root, filename, pattern):
    path = root / filename
    if not path.is_file():
        return {"value": None, "confidence": "no version evidence found"}
    text = path.read_text(errors="replace")
    if pattern:
        match = re.search(pattern, text)
        value = match[1] if match else None
        line = text.count("\n", 0, match.start()) + 1 if match else None
    else:
        values = [re.search(r"_STLPORT_" + part + r"\s+(\d+)", text) for part in ("MAJOR", "MINOR", "PATCHLEVEL")]
        value = ".".join(m[1] for m in values) if all(values) else None
        line = text.count("\n", 0, values[0].start()) + 1 if values[0] else None
    return {"value": value, "file": str(path), "line": line,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "confidence": "bundled-source declaration; binary version still requires matching" if value else "not identified"}


def audit(report, overrides=None):
    rows = []
    for name, relative, reference_relative, filename, pattern in LIBRARIES:
        root = ROOT / relative
        reference = (overrides or {}).get(name) or (ROOT / reference_relative if reference_relative else None)
        files = inventory(root)
        integrations = []
        for relative_file, path in files.items():
            if re.search(r"\b(?:MILO_FAIL|MILO_ASSERT|_MemAllocTemp|MemAlloc|OggMalloc)\b", path.read_text(errors="replace")):
                integrations.append(relative_file)
        units = [u for u in report.get("units", []) if u.get("metadata", {}).get("source_path", "").replace("\\", "/").startswith(relative + "/")]
        totals = {key: sum(int(u.get("measures", {}).get(key, 0)) for u in units)
                  for key in ("total_code", "matched_code", "complete_code", "total_functions", "matched_functions")}
        rows.append({"library": name, "root": str(root), "version": version_evidence(root, filename, pattern),
                     "source_files": len(files), "game_integration_markers": integrations,
                     "configured_units": len(units), "configured_measures": totals,
                     "reference": str(reference) if reference else None,
                     "reference_kind": "supplied reference; verify its download provenance separately" if name in (overrides or {}) else "local copy, not verified pristine upstream source",
                     "comparison": compare_trees(root, reference) if reference and reference.is_dir() else None})
    return {"generated_at": datetime.now(timezone.utc).isoformat(), "libraries": rows,
            "limitations": ["Vendor date strings do not establish an exact release number.",
                "Local reference copies can contain the same game modifications.",
                "Relative filenames must align; missing files can reflect directory layout differences.",
                "Configured metrics cover named library units, not unidentified code in the executable.",
                "Nintendo SDK, Quazal and other proprietary components require separate provenance work.",
                "Downloads and wholesale source replacements are not performed by this audit."]}


def markdown(result):
    lines = ["# Wii " + result.get('version', 'SZBE69') + " library audit", "", "Evidence comes from bundled source, not identification of the binary's exact upstream releases.", "",
             "| Library | Declared version / vendor | Files | Configured code bytes | Source-linked bytes | Modified reference files |",
             "|---|---|---:|---:|---:|---:|"]
    for row in result["libraries"]:
        counts = row["comparison"]["counts"] if row["comparison"] else {}
        m = row["configured_measures"]
        lines.append(f'| {row["library"]} | {row["version"]["value"] or "Unknown"} | {row["source_files"]} | {m["total_code"]} | {m["complete_code"]} | {counts.get("modified", "No reference")} |')
    lines += ["", "## Evidence and reuse", ""]
    for row in result["libraries"]:
        v = row["version"]
        lines.append(f'- {row["library"]}: `{v.get("file", row["root"])}` line {v.get("line")}. Integration markers: {len(row["game_integration_markers"])} files.')
    lines += ["", "Public source can accelerate reconstruction when the historical version, build flags and game changes agree. Compare an exact release in a separate directory before copying any code. Every adopted unit must pass objdiff and the selected build's complete DOL checksum.", "", "## Limits", ""]
    lines += ["- " + item for item in result["limitations"]]
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", default="SZBE69")
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--reference", action="append", default=[], metavar="LIBRARY=PATH")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_]+", args.version):
        parser.error("Invalid version")
    overrides = {}
    for item in args.reference:
        name, separator, path = item.partition("=")
        if not separator or name not in {r[0] for r in LIBRARIES} or not Path(path).is_dir():
            parser.error("Reference must name a known library and an existing directory")
        overrides[name] = Path(path)
    report_path = ROOT / "build" / args.version / "report.json"
    result = audit(json.loads(report_path.read_text()) if report_path.exists() else {}, overrides)
    result['version'] = args.version
    output = args.output_dir or ROOT / "build" / "decomp" / args.version
    output.mkdir(parents=True, exist_ok=True)
    (output / "library-audit.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    (output / "library-audit.md").write_text(markdown(result), encoding="utf-8")
    print(f"Saved audit to {output}")
    for row in result["libraries"]:
        print(f'{row["library"]}: {row["version"]["value"] or "unknown"}; {row["configured_units"]} configured units')


if __name__ == "__main__":
    main()

# Local unattended comparison tools

Run these commands from the repository root. Python's standard library is sufficient; the repository's existing compiler, Ninja, dtk and objdiff binaries must already be installed. All results stay local under `build/decomp/SZBE69`. Use the retail version explicitly; the project's default build is DEBUG.

```powershell
python tools/decomp_runner.py rank --version SZBE69 --limit 20
python tools/decomp_runner.py compare --version SZBE69 --unit main/system/math/Rot
python tools/decomp_runner.py run --version SZBE69 --minutes 30 --limit 10
python tools/library_audit.py --version SZBE69
```

`rank` reads the existing report. Add `--refresh` to rebuild and regenerate it. Rankings include small near-matching functions and units whose reported code and data match completely. Reports can hide relocation aliases and source-generated helper functions; they are candidates, not proof that a unit can be linked safely.

`compare` saves complete objdiff JSON and a smaller instruction/relocation summary. The summary identifies unpaired original and compiled functions as well as differing call targets.

`run` tests candidates by changing their source-link status and rebuilding the complete executable. Acceptance requires both a successful build and the configured retail DOL SHA-1. Default trials test existing flags and `-ipa off`; `--variants default,ipa-off,align4,align16` expands this list. `--unit system/os/OnlineID.cpp` selects a particular source file even if it is not a perfect report match. Rejected changes restore the original configuration bytes and rebuild. A rejected checksum also saves dtk layout diagnostics when a new ELF was produced. Compiler/linker failures retain their logs.

The runner fingerprints configuration and both compared objects, skips previously tried unchanged candidates, and saves trials in `trials.json`. Use `--retry` to repeat them. It takes an OS lock to exclude another instance, writes a journal before atomic configuration replacement, and kills a timed-out command's process tree. It reserves two command timeouts plus 15 seconds before starting another trial. Cleanup can extend the nominal run duration; the time limit is not a hard wall-clock guarantee.

Do not edit source or build configuration while trials are running. After an interruption:

```powershell
python tools/decomp_runner.py recover --version SZBE69
```

Recovery refuses to overwrite a configuration changed outside the runner. Inspect `pending.json` in that case. If a restore build fails or times out, the saved verified DOL is copied back and the journal remains. Other build artifacts can still be inconsistent; recovery must succeed before further work. Recovery rolls back an interrupted trial even if its executable had already matched.

Create `build/decomp/SZBE69/STOP` to stop before the next trial. Remove it when ready to resume. `--stop-file PATH` selects another stop marker.

## Account usage

The standalone script cannot query your Codex account. An external supervisor must update a JSON quota file and pass `--quota-file PATH`. Accepted schemas are `{"remaining_percent": 2}` or a parsed account usage response with `rateLimits`/`rateLimitsByLimitId`. The runner stops starting trials at 1% remaining or below; it must still finish recovery. A stale quota file cannot enforce a live account limit. Running comparisons as ordinary local scripts does not itself require model inference.

## Source variant trials

`tools/decomp_variants.py` tests explicit, independently proposed source edits. A JSON manifest names an objdiff unit, a mangled target symbol and variants. Each variant has a name and a list of `old`/`new` replacements; each replacement must match exactly once. Line endings are preserved. Keep replacements semantically equivalent: compiler comparisons cannot prove behavior of unfinished functions.

```json
{
  "unit": "main/system/math/Rot",
  "symbol": "Set__Q23Hmx4QuatFRC7Vector3",
  "variants": [
    {"name": "example-noop", "replacements": [
      {"old": "float qz = z;", "new": "float qz = z;"}
    ]}
  ]
}
```

```powershell
python tools/decomp_variants.py build/my-variants.json --minutes 30
```

The script compiles just the affected object for each trial, compares it with objdiff, and retains a candidate only when its target improves without regressing other compared functions or adding unpaired helpers. The final complete build must match the retail checksum and preserve global matched-code/function and source-linked-code metrics. Source edits share the runner lock and recovery journal. A crash rolls back to the original source; use the existing `recover` command. Logs, a rolling checkpoint and timestamped result JSON remain under `build/decomp/SZBE69`.

An integration smoke test rejected both a no-op and deliberately invalid syntax, restored original source, and preserved the retail executable. Source variants do not change object status to Matching automatically.

## Library audit

The audit extracts declared versions with file/line evidence and SHA-256, counts configured library code, finds game integration markers, and compares bundled source with local historical reference copies. It distinguishes byte-identical files, line-ending changes, modified files and files present on only one side. Full inventories are saved as JSON, with a readable Markdown report.

To compare a separately obtained historical release without replacing repository source:

```powershell
python tools/library_audit.py --reference "zlib=C:\reference\zlib-1.2.1"
```

Reference paths are read only. Local copies are not assumed to be pristine upstream releases. Ogg/Vorbis's vendor date establishes a source revision hint, not an exact release number. Nintendo SDK and proprietary middleware remain outside this six-library version audit. The audit does not download or replace source.

Historical sources can now be fetched into isolated `build/references` folders:

```powershell
python tools/library_fetch.py zlib-1.2.1
python tools/library_fetch.py speex-1.2rc1
python tools/library_fetch.py libvorbis-1.0.1
```

The fetcher records URL, redirect, time and SHA-256, rejects unsafe archive paths and links, and never runs downloaded build scripts. Every redirect must remain HTTPS. Cached archives must retain their recorded hash; all extracted source files must still match the archive, with no missing or extra files. Windows curl may be used if Python's certificate store cannot validate a mirror; HTTPS certificate checks remain enabled.

## Debug and retail builds

The confirmed debug dump is now available under ignored `orig/SZBE69_B8`. The primary target changed to B8 at the user's request during the October 2 session. Use:

```powershell
./build-wii.ps1 -Version SZBE69_B8
python tools/decomp_runner.py rank --version SZBE69_B8
python tools/decomp_runner.py run --version SZBE69_B8 --minutes 15 --limit 5
```

`build-wii.ps1` defaults to B8; pass `-Version SZBE69` for retail. Reports and recovery journals are stored per version. Both runners use one shared `build/decomp/runner.lock` because source files, `build.ninja` and `objdiff.json` are shared. Do not run either build wrapper alongside a runner. The debug checksum is read from its pinned `build.sha1` when `config.yml` has no hash. Use the same version explicitly for compare, run and recover. The retail STOP marker remains in place after the switch; remove it deliberately before starting future retail trials.

Comparisons refuse to use objects from another configured version. Automatic source-link candidates require 100% code and data matching when original data is owned by the unit; an omitted zero data score is not treated as 100%. An explicit `--unit` still permits diagnostic trials on an imperfect unit. Whole-executable checksum verification remains mandatory because unowned data and folded helpers can escape an object report. The build wrapper also checks the executable checksum independently of Ninja's cached check step.

`--allow-new-helper EXACT_SYMBOL` permits an explicitly reviewed additional compiler helper in source-variant trials. Full executable and global regression checks still apply. Literal-order helpers are dead-stripped from the final executable; do not use this option to excuse missing behavior. Rejections now show exact helper names, including their actual compiler line numbers.

Compact function inspection:

```powershell
python tools/decomp_inspect.py build/decomp/SZBE69_B8/UNIT-diff.json MANGLED_SYMBOL --output build/inspection.json
```

The inspector uses objdiff's explicit paired-symbol indexes, including mapped names, and distinguishes changed instruction text from identical instruction text with differing relocations. This is diagnostic information, not proof of equivalence: validate address-sensitive changes and whole-unit adoption with the executable checksum. Physical DOL comparison in `tools/dol_compare.py` compares actual bytes by virtual address independently of symbol aliases.

## Validation and remaining work

For current B8 downtime preparation, use `tools/decomp_prepare.ps1 -Action Run`.
It performs no source trials or automatic adoption and needs no Codex/model quota.
The measured workflow, cache behavior, optional m2c/advisory tools and checkpoint
launch flag are documented in
[DECOMP_OPTIMIZATION_2026-10-03.md](DECOMP_OPTIMIZATION_2026-10-03.md).

```powershell
python -m unittest discover -s tools/tests -v
```

On this Windows sandbox, temporary directories created by Python's restrictive permissions could not be accessed even under the workspace. Tests passed when run through the app's approved native execution path. Native compiler execution also requires that path in this environment.

The initial real trial rejected `OnlineID.cpp`, captured the checksum/layout failure and restored the matching retail DOL. Ranking and Rot comparison also completed. A later source-variant trial restored the official zlib version check and matched `deflateInit2_` exactly. Next improvements: automatic proposal generation for individual functions, a live quota supervisor, and alignment-aware reference-tree mappings. The tools automate explicit source variants, comparisons and source-link trials; they do not autonomously reconstruct missing C++ implementations.

## Coordinated source and header trials

`tools/decomp_source_trial.py` accepts a manifest with `unit`, `symbol`, and a `files` list of existing source paths and unique old/new replacements. It validates every edit before writing, backs up all sources/config/executable, shares the runner lock, rebuilds dependencies, and rolls back rejected or interrupted trials. Paths must remain under src. The target function and other functions must not regress, and full-executable verification remains mandatory. Rejected attempts preserve object summaries, target inspection, and available physical/layout diagnostics.

```powershell
python tools/decomp_source_trial.py build/decomp/SZBE69_B8/TRIAL.json --version SZBE69_B8 --minutes 20
```

`--adopt-unit` changes that unit to Matching in the same recovery transaction. It permits unchanged function scores only when verified source-linked code increases and there are no function/global regressions. This supports layout repairs after individual instructions already match. Variant results now include target inspection so a rejected trial cannot silently overwrite the explanation of its failure. The inspector handles omitted protobuf indexes as zero.

## Original literal-order proposals

`python tools/decomp_literals.py --version SZBE69_B8 --unit main/system/bandobj/PatchDir --symbol __rs__FR9BinStreamR10PatchLayer --output build/decomp/SZBE69_B8/patchdir-literals.json` prepares a coordinated source-trial manifest from the original named DTK disassembly. It checks configured version and source containment, preserves C literal escapes, rejects unsupported pool directives and refuses to overwrite existing literal forcing. It never edits source or marks a unit Matching.

Review the manifest and its evidence first. Pass the exact `review_helper` to `decomp_source_trial.py --allow-new-helper` when running it. The generated metadata uses the project's existing macro and is restricted to the selected game version. Object/regression checks and the full executable hash still decide acceptance. This recovers data ordering only; missing game behavior remains incomplete. Do not infer source-link readiness from literal recovery or fuzzy percentages alone.

## Anonymous comparison data

Strict unit comparison uses tools/decomp_rtti.py to pair named RTTI relocation owners with their anonymous class-name/base-chain data. It also pairs unique generated string symbols using exact complete, printable, relocation-free payloads. Ambiguous or changed data is not mapped. Evidence is saved in the unit's -diff-symbol-mappings.json. These mappings do not edit ELF objects, marking status or global progress. A generated symbol whose bytes are merely a fragment of a pooled source symbol remains unresolved; do not equate arbitrary offsets or bypass regression checks. Forty-one automation tests cover the combined runner safeguards.

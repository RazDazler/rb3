# Automation session checkpoint

Started 2026-10-02 08:40:17 UTC with a maximum of 30 minutes and a stop threshold of 1% account usage remaining. Changes are local and uncommitted.

Added reusable standard-library Python tools:

- `tools/decomp_runner.py`: candidate ranking, explicit objdiff comparisons, bounded source-link/flag trials, exact executable acceptance, diagnostic logs, persistent fingerprints, atomic configuration edits, exclusive lock, timeout handling, crash journal and recovery.
- `tools/library_audit.py`: six-library version evidence, configured build metrics, local-reference comparisons and integration markers. References are read only; no external downloads performed.
- `tools/tests/test_decomp_automation.py`: regression coverage for preserved configuration, quota parsing, relocation mismatches, recovery, locks and library comparisons.

Real integration validation: `OnlineID.cpp` default source-link trial failed the retail checksum as expected. The runner saved the Ninja log and dtk layout diagnostic, then restored the original configuration byte for byte and rebuilt successfully. SHA-1 of original and final DOL: `189b001188a2f8ab769e72364440022407fe09d2`. No pending journal remained. A simulated quota file with 1% remaining stopped the runner before another build. Ranking and Rot objdiff comparison completed successfully.

Results and complete diagnostics are in `build/decomp/SZBE69`. Historical trial results are in `trials.json`; `run-result.json` was subsequently overwritten by the quota-stop smoke check. The readable library checkpoint is `doc/LIBRARY_AUDIT_2026-10-02.md`; full file inventories and hashes remain in the generated audit JSON.

Known limits and next work: no live account connection in standalone scripts (quota file requires a supervisor); no C++ source reconstruction generator yet; no exact upstream-release verification for the Vorbis vendor date; proprietary SDK/middleware not audited; failed recovery can leave ELF and related build artifacts inconsistent even though the verified DOL is copied back. See `doc/DECOMP_AUTOMATION.md` for commands and recovery instructions.

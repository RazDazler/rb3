# Local Wii decompilation baseline

Verified on 2026-10-02.

## Target and inputs

- Fork remote: https://github.com/RazDazler/rb3
- Target: retail Wii `SZBE69`, explicitly selected with `--version SZBE69`.
- Extraction renamed from `orig/SZBE69_B8` to `orig/SZBE69` after verifying both hashes.
- `sys/main.dol` SHA-1: `189b001188a2f8ab769e72364440022407fe09d2`.
- `files/band_s_wii.sel` SHA-1: `ca902f87c70f57b140b411d5946341c13d7616cf`.
- Both match `config/SZBE69/config.yml`. Original game files stay in ignored `orig/`.

## Build

Python 3.13 is available. Ninja 1.13.2 and pcpp 1.30 were installed locally:

```powershell
python -m pip install --target build/python ninja pcpp
./build-retail.ps1
```

The build downloads the repository's pinned tools into ignored `build/`.
The restricted shell stalled when launching build tools; the successful run used
approved execution outside the sandbox. This may require approval on future runs.

Two retail compilation errors were repaired by guarding debug-only calls with
`MILO_DEBUG`, matching their declarations:

- `SaveLoadManager.cpp`: explicit message timer destructor.
- `CommerceMgr_Wii.cpp`: vector range assertion.

The complete build passed its SHA-1 check. `build/SZBE69/main.dol` has the same
SHA-1 as the original. These source fixes compiled successfully, but their objects
are not established as fully matching; the successful executable still links
original binary sections for unfinished objects.

## Baseline measurements

- Code matched: 2,085,256 / 8,253,320 bytes (25.27%).
- Code linked from reconstructed source: 90,700 / 8,253,320 bytes (1.10%).
- Fully linked units: 61 / 1,994.
- Matched functions: 19,003 / 59,455.
- Data matched: 12,344 / 2,451,724 bytes (0.50%).

Generated comparison configuration: `objdiff.json`.
Detailed report: `build/SZBE69/report.json`.
Summary: `build/SZBE69/progress.json`.
Final build log: `build/retail-build.log` (earlier compiler runs are not in this log).

## Next work

Use the retail report to select a small unfinished function, compare reconstructed
code with its original object using objdiff, and validate each change before
marking an object Matching. Keep matched-code and linked-code progress distinct.
This baseline is a Wii executable, not a native Windows port. Porting requires
separate platform work after enough behavior is recovered.

## Two-hour continuation

See DECOMP_SESSION_2026-10-02.md for the saved session, final measurements,
verified source-linked modules and remaining issues. Rebuild retail with
./build-retail.ps1. The final executable still matches the original SHA-1.

## Three-hour session and debug target switch

The user supplied the supported SZBE69_B8 debug executable and its linker map, then requested B8 as the primary target. Both original inputs and matching built executables are now available locally. Use ./build-wii.ps1 (defaults to B8), or explicitly pass -Version SZBE69 for the secondary retail reference.

See DECOMP_THREE_HOUR_SESSION_2026-10-02.md and DECOMP_THREE_HOUR_VERIFICATION_2026-10-02.json for separate baselines, gains, full executable hashes and remaining issues. Debug's larger existing progress is not a gain made during this session. Dump findings are recorded in DEBUGVERSION2_AUDIT_2026-10-02.md. All changes remain local and uncommitted.


## B8 medium continuation

See DECOMP_B8_MEDIUM_SESSION_2026-10-02.md and DECOMP_B8_MEDIUM_VERIFICATION_2026-10-02.json for the 20:34:17 UTC continuation, measured against its own fresh B8 baseline. Newly source-linked units: NetLoader (2,172 bytes), NetLoader_Wii (1,528 bytes), Server (2,900 bytes). HDCache, BlockMgr and NetCacheMgr have substantial additional reconstructed behavior but still link original objects. Accepted trials preserve the B8 executable hash. All work stays local and uncommitted.

## B8 High continuation

See DECOMP_B8_HIGH_SESSION_2026-10-02.md and DECOMP_B8_HIGH_VERIFICATION_2026-10-02.json for the independent High benchmark. Verified gains: 50,024 matched code bytes, 158 exact functions, 6,824 source-linked bytes and three complete units (EnvelopeWii, TDStretch, FIFOSampleBuffer). Global B8 code is 55.232883% matched and 10.795093% source-linked. Both retail and B8 executable hashes match; 41 automation tests pass. New exact implementations, metadata-only gains, partial progress, elapsed time and rounded account usage are separately recorded. Unaccepted renderer proposals are preserved in DECOMP_B8_HIGH_REMAINING_TRIALS_2026-10-02.json. All changes remain local and uncommitted.

## GPT-6.1 Sol Light — October 3, 2026

Verified B8 gains: 10,660 matched code bytes / 64 functions; 7,780 source-linked code bytes / four units and 804 source-linked data bytes. Twenty-eight new strict-exact functions (5,136 bytes), two instruction-exact functions with metadata relocations, and ten partial records are distinguished in DECOMP_B8_LIGHT_VERIFICATION_2026-10-03.json. Full build and all 43 automation tests pass; exact B8 DOL is preserved. Detailed work and comparison caveats: DECOMP_B8_LIGHT_SESSION_2026-10-03.md. Session status and final usage/time are in the verification ledger.

Light session completed at 10:00:44 UTC after 2h 36m 50s because primary usage reached 1% remaining. Rounded account consumption: 98 primary / 16 weekly points. Benchmark comparison updated in DECOMP_MODEL_COMPARISON_2026-10-02.json; remaining technical context saved in DECOMP_B8_LIGHT_REMAINING_2026-10-03.json.


## Local helper setup - October 3, 2026

Portable Ollama 0.35.1 and Qwen2.5-Coder 7B Q4_K_M are installed under ignored build/local-agent. The model is fully GPU-resident on the RTX 3070 Ti at 6,144 context tokens; the initial real-function pilot generated about 86-91 tokens/second. A finite localhost-only pipeline now runs cached model-free source searches, library auditing, source-link trials and model proposals with compiler/objdiff/exact-DOL verification and transactional restoration for semantic review.

One local-model proposal was semantically reviewed and retained: RndBitmap::SetPixelIndex now matches exactly, adding 132 matched bytes and one function, with no linked-code gain. B8 matched code is 6,324,836 bytes; linked code remains 1,241,840 bytes. All 53 automation tests and the complete B8 build pass. The preserved DOL hash is e26b3daf41886f0d09670135910f2510cd093ae8. The policy-3 variation-strategy batch runs independently with a two-hour maximum and finite attempt queue; its live status is in build/local-agent. See LOCAL_DECOMP_AGENT.md for commands, limits, realistic deadline assessment, and next tasks, and LOCAL_AGENT_SETUP_2026-10-03.json for setup/pilot verification evidence. Model proposals remain separate from retained progress until semantic review.

The completed policy-3 local batch yielded a second reviewed exact match, ByteGrinder op8 (136 bytes). Total retained local-model gains are now 268 matched bytes / two functions, zero linked bytes. Global matched code is 6,324,972 bytes. The finite worker queue has completed; expand its tasks or deliberate attempt budget before rerunning. The full executable remains exact; final evidence is updated in LOCAL_AGENT_SETUP_2026-10-03.json.


## October 3 local-assisted B8 session
Stopped at1% primary remaining after2h39m. Gains:+37,544 matched bytes/91 functions; +1,504 linked bytes/4 units; seven partial targets. Usage:+76 primary/+12 weekly percentage points from77%/50% remaining. Exact B8 SHA1 preserved. Local model:143 attempts,0 retained proposals; deterministic scripts supplied gains. Details:DECOMP_B8_LOCAL_ASSISTED_SESSION_2026-10-03.md and JSON; comparison updated in DECOMP_MODEL_COMPARISON_2026-10-02.json.


## 2026-10-03 B8 efficiency optimization

Added artifact-only downtime preparation, cached bounded evidence packets, optional m2c orientation, optional unreviewed localhost notes and native timing telemetry. The source-proposal worker remains stopped. Broad preparation covered 403 units / 2,523 functions in 342.18 seconds with prior cache reuse; its unchanged repeat took 7.77 seconds with 403 cache hits and no fresh comparisons or m2c runs. All 3,307 source/header/config files stayed unchanged during preparation. Manually reviewed RTT variance reconstruction improved strict13.33 to20 percent and remains partial: zero new exact matched/linked bytes. The B8 executable stays exact. Full measurements, failures, commands and limitations: doc/DECOMP_OPTIMIZATION_2026-10-03.md and .json.

Recovered previous session: +6864 matched bytes / 123 functions, +1964 linked bytes / 2 units, 7 retained partial records, 8523.16 seconds; usage consumed 79 primary / 13 weekly percentage points. B8 executable SHA1 e26b3daf41886f0d09670135910f2510cd093ae8 preserved. Downtime preparation completed from 401 cache hits in 8.12 seconds; launcher timeout did not lose source work.


2026-10-04 continued Ghidra milestone by ChatGPT: 1,904 matched code bytes / nine exact functions, 928 source-linked bytes / two units, 792 matched data bytes, seven retained vocal partials. Includes 492 bytes of tracker literal/assertion metadata repair; synced upstream matched gains remain excluded. Ghidra now annotates reviewed vocal signatures and narrowly justified GQR0=0, with ABI register verification. Exact B8 SHA-1 preserved; 72 automation tests pass. Session remains active.

2026-10-04 ChatGPT checkpoint: session gains 4424 matched bytes / 16 functions; 2704 linked code bytes / 3 units; 792 matched data bytes and 344 linked data bytes. StoreSongSortNode handler reconstructed and entire unit linked after original definition-order restoration. DNS thread pointer and store-offer constructor pointer corrected; USB device-range and cache error formatting matched; TrackWidget RemoveAt and Band Restart matched. Seven vocal partials retained. Exact B8 DOL SHA-1 preserved. Game inputs and generated artifacts excluded.

2026-10-04 ChatGPT Ghidra milestone: 4668 matched code bytes / 17 functions, 2704 linked code bytes / 3 units; 792 matched data bytes, 344 linked data bytes. Eleven vocal partials retained, including new ScoreSinger 98.11321%, GetBestHit 93.15789%, Poll 88.82682%. Local declaration-order search matched AddPhrasePoints after 15 candidates, without model calls; reusable bounded manifest generator added. Ten Ghidra signatures reviewed, including two stack-passed GetBestHit pointers. SetDifficultyVariables became strict-exact but was already counted in the broader report. Exact B8 DOL preserved; 72 automation tests pass. Session active.

2026-10-04 ChatGPT closing checkpoint: 6328 matched code bytes / 26 functions; 2704 source-linked code bytes / 3 units; 792 matched data bytes and 344 linked data bytes. GetBestHit became exact (760 bytes) after typed Ghidra reconstruction and reviewed deterministic lifetime/evaluation trials. Ten vocal partials remain: ScoreSinger 98.11321%, Poll 97.2067%, and other listed scoring/rollback methods. Additional exact fixes cover HTTP byte flag, bitmap nibble order, trainer lead-in subtraction, content signed-byte flag, coda output aliasing, Wii post-process initialization, OS partition-name address and hair FPS branches. Exact B8 DOL preserved. Upstream 620 matched bytes / 5 functions excluded. Private originals/tools/analysis excluded from publication.

Session stopped at observed 99% primary / 94% weekly usage (1% / 6% remaining). Started at 1% / 78% used: consumed 98 / 16 rounded account-wide percentage points in unchanged reset windows. Final exact B8 verification and 72 automation tests pass; final finite preparation cache completed without protected source/configuration changes. Closing statistics and remaining issues are saved in DECOMP_B8_GHIDRA_SESSION_2026-10-04.json/.md and the model-comparison ledger.

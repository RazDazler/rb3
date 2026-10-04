# GPT-6.1 Sol High B8 session

Start: 2026-10-03 02:20:55 UTC (October 2, 9:20:55 PM CDT). Deadline: 05:20:55 UTC, or earlier when usage becomes critically low. The companion verification JSON is the authoritative final ledger. Changes remain local and uncommitted.

## Results

At the verified final-code checkpoint, matched code increased from 6,264,020 to 6,314,044 bytes: **+50,024 bytes and +158 exact functions**. Overall code matching rose from 54.795288% to 55.232883%. Source-linked code rose from 1,227,236 to 1,234,060 bytes: **+6,824 bytes and +3 complete units**, reaching 10.795093% linked. Source-linked data increased by 736 bytes; global matched data is unchanged.

| Source-linked unit | Code bytes |
| --- | ---: |
| EnvelopeWii | 948 |
| SoundTouch TDStretch | 4,672 |
| SoundTouch FIFOSampleBuffer | 1,204 |

The full executable checksum verifies every adopted unit, including data ownership and placement. Object-level matching in incomplete units does not mean that their source is linked into the executable yet.

## What changed

Twenty-six previously missing routines now match exactly (2,944 original code bytes): Envelope ADSR setup; SoundTouch overlap-length and nominal-tempo processing; CharClipDisplay coordinates, beat labels and cursor drawing; Wii renderer callbacks, accessors, TV-format selection, draw setup, world/preclear handling, hang-detection state, home-menu opening and GX initialization. The original ClearBuffer is a four-byte no-op; its implementation reflects that original behavior.

Existing behavior corrections also made functions exact in GemPlayer, Gem visibility, MusicLibrary, CharBones, NetSync, SDK IPC/thread handling, SessionMgr, GemTrainerPanel, BandSongMgr, Quazal DuplicatedObject, AppLabel and SoundTouch RateTransposer. RateTransposer's nonconst isEmpty signature and processSamples inlining account for the final 388 matched bytes. Reviewed instruction/relocation differences and the exact executable gates govern acceptance.

B8 WiiRnd layout recovery restores its callback bases, complete render-mode structure, projection/position matrices, font header and saved viewport/projection arrays. This made 17 existing functions exact (1,708 bytes). Its packed-color inline routine also restores the original DrawLine instruction sequence. B8-specific additions are version guarded; a final retail build caught and fixed the missing GXReInit guard.

FIFOSampleBuffer activates the project's existing licensed upstream SoundTouch source. Matching required the original 20 KiB preallocation, both buffer deletions, unsigned interfaces, fast heap, failure logging, allocator ownership and inlining. All fourteen original functions are exact and the complete unit is now source-linked. Original LGPL notices remain.

Eleven reviewed literal-pool metadata corrections account for **35,056 matched bytes and 66 functions, with zero linked-code gain**. These are included in total matching but separately labeled because they recover data ordering rather than missing behavior.

## Partial progress and next candidates

Two missing CharClipDisplay routines are retained as partial reconstructions: SetStartEnd (412 bytes, 49.51456%) and DrawBlend (396 bytes, 83.83839%). Existing NetCacheMgr CheatNextServer improves from 86.440674% to 98.305084%, leaving one comparison operand-order difference. SoundTouch mono interpolation improves from 81.31868% to 82.41759%, leaving sixteen instruction differences. All remain NonMatching.

Rejected WiiRnd BeginDrawing/HomeMenuClose proposals are preserved in DECOMP_B8_HIGH_REMAINING_TRIALS_2026-10-02.json, with no rejected code retained. BeginDrawing has exact instruction text but two unresolved local callees; generated class-name string ownership also fails the strict data comparison gate. Resolve FrameQueueSync/SyncDestroy and legitimate Light string identity before retrying. TrainingPanel source adoption still moves its BSS static. Nintendo management/RefCountedObject ownership and Singer RTTI pairing also remain blockers.

## Automation and validation

The comparison runner now pairs anonymous RTTI through named relocation owners and unique compiler-generated immutable strings through their complete, exact payload. It refuses ambiguous, changed, executable or relocation-bearing data. Pairing only affects strict unit comparisons: object bytes, config status and global progress mappings are unchanged. Forty-one automation tests pass. There are no pending recovery journals, git diff --check passes, and final builds of retail and B8 reproduce the original SHA-1 values:

- B8: e26b3daf41886f0d09670135910f2510cd093ae8
- Retail: 189b001188a2f8ab769e72364440022407fe09d2

Rebuild with ./build-wii.ps1 -Version SZBE69_B8; retail uses -Version SZBE69. B8 remains the active configuration. Rejected trial diagnostics and native build logs remain under ignored build/.

## Model comparison

Medium's previous B8 session lasted 10,250 seconds and gained 60,288 matched bytes, 163 exact functions, 6,600 linked bytes and three linked units. High's final elapsed time and usage observations are recorded in the companion JSON. High gained slightly more linked code; Medium gained more matched bytes in its longer run. Different candidates, prior preparation and metadata gains prevent these sessions from isolating the effect of reasoning effort. Light's earlier work mixed retail and B8 targets, so its combined totals should not be compared directly.

Usage is measured in rounded account-wide percentage points, not tokens. High starts at 2% used in the five-hour window and 16% weekly. The API does not expose actual consumed tokens or isolate concurrent account activity. Future Light/Medium reruns should retain the same B8 target, exact-executable gate, per-session baseline, reconstruction categories and account usage snapshots.

Completed at 2026-10-03T04:51:34Z after 9039 seconds, stopping at 1% remaining in the five-hour usage window. Usage increased from 2% to 99% used (+97 percentage points) and from 16% to 31% weekly (+15). Timer::Reset stays 98.36066% after five rejected trials. Machine-readable comparison: DECOMP_MODEL_COMPARISON_2026-10-02.json.

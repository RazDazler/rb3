# B8 Ghidra-assisted session

Status: active. Model/effort: Current Codex model/effort not independently retrieved.
Elapsed: 116.78 minutes, including repository recovery, syncing and local tool setup.
Retail remains reference-only; local-model source editing remains stopped.

| Measure | Start after upstream sync | Latest | Session gain |
| --- | ---: | ---: | ---: |
| Matched code bytes | 6370000 | 6374668 | 4668 |
| Matched functions | 28779 | 28796 | 17 |
| Source-linked code bytes | 1245308 | 1248012 | 2704 |
| Source-linked units | 545 | 548 | 3 |
| Matched data bytes | 614776 | 615568 | 792 |
| Source-linked data bytes | 498140 | 498484 | 344 |

Matched code: 55.763200%; source-linked code: 10.917139%.
Upstream gains excluded: {"matched_code": 620, "matched_functions": 5, "complete_code": 0, "complete_units": 0, "matched_data": 0, "complete_data": 0}.
Original/rebuilt B8 DOL SHA-1: `e26b3daf41886f0d09670135910f2510cd093ae8`.
Strict target-unit function/data nonregression and global match/link measures guard retained trials; exact executable verification does not imply unfinished source-linked units are complete.

Usage began at 1% current-window / 78% weekly used;
latest is 81% / 91% used.
Consumed percentage points: {"primary": 80, "weekly": 13}.
Refill observed: False. These are rounded account-wide readings, not billing-token counts.
Across refills endpoint consumption is unavailable; the ledger keeps snapshots instead of inventing totals.

Retained partial reconstructions (11):

- `GetNoteSliceWeight__9VocalPartCFffi` (main/band3/game/VocalPart): 46.296295% - 648-byte note-frame integration; register allocation and constant ownership remain.
- `Rollback__9VocalPartFff` (main/band3/game/VocalPart): 96.0% - 200-byte rollback reconstruction; original cursor reload remains. UpdateMinMaxPitch call now resolves.
- `UpdateMinMaxPitch__9VocalPartFRCPC11VocalPhrase` (main/band3/game/VocalPart): 91.25% - 320-byte phrase pitch-range scan; same instructions, remaining pointer-register swaps.
- `CalculateScore__9VocalPartCFfifR15VocalScoreCache` (main/band3/game/VocalPart): 86.23853% - 436-byte scoring/debug-spew reconstruction; float local scheduling remains. Restores original zero constant and 792-byte data section.
- `CouldScoreAgainstPart__9VocalPartFfP12TalkyMatcherffRf` (main/band3/game/VocalPart): 67.93893% - 524-byte talky/pitch eligibility reconstruction; register scheduling remains.
- `GetSloppyPitch__9VocalPartCFfifRf` (main/band3/game/VocalPart): 59.227467% - 912-byte slop-window target selection and note interpolation; original equal-distance clamp order preserved; register and float scheduling remain.
- `ScoreNote__9VocalPartCFfiRfRiRfRf` (main/band3/game/VocalPart): 49.57265% - 468-byte pitch confidence reconstruction; floating fabs, octave correction, Gaussian cutoff and zero-weight gate; register and conversion ordering remain.
- `CalcPhraseScoreMax__9VocalPartCFRCPC11VocalPhrase` (main/band3/game/VocalPart): 71.15385% - 208-byte existing phrase maximum reconstruction improved from 61.538464%; pointer register allocation remains.
- `ScoreSinger__9VocalPartFffffiP12TalkyMatcherR15VocalScoreCacheRiRf` (main/band3/game/VocalPart): 98.11321% - 636-byte new singer cache/scoring reconstruction; original argument and output-local stack order verified, three assertion-register differences remain.
- `GetBestHit__9VocalPartFfiiP12TalkyMatcherRffRiRiRfRfRb` (main/band3/game/VocalPart): 93.15789% - 760-byte new pitched/talky best-hit selection; exact output-stack ABI, tie handling and talky snapshots reviewed; floating operation scheduling remains.
- `Poll__9VocalPartFfRC7SongPos` (main/band3/game/VocalPart): 88.82682% - 716-byte new vocal update: freestyle gates, phrase-cap growth, note mode and frame spew; virtual calls verified from original vtable, pointer reload/scheduling remains.

Remaining issues:

- Eleven retained vocal partials remain assembly-linked; three newly reconstructed methods plus existing scoring/rollback methods need remaining matching work.
- Ghidra CodeWarrior demangler compatibility error remains; selected vocal signatures now reviewed, unlisted types/signatures remain inferred.
- Tracker metadata-only gains (492 bytes / two functions) are separate from newly reconstructed vocal behavior and its canonical library helper.
- CircleBuffer source linking uses the synced upstream implementation, not newly authored behavior; customer-support source pre-existed.
- StoreSongSortNode now fully source-linked after matching handler and restoring original definition order; near-match register/scheduling trials remain for other units.

Git checkpoints and Ghidra audit/provenance are recorded in the JSON ledger. Source commits explicitly credit ChatGPT; private inputs, native tools and generated pseudocode remain ignored. Ghidra drafts require semantic and assembly review before trials. Different targets and setup overhead prevent treating this as a controlled model benchmark.

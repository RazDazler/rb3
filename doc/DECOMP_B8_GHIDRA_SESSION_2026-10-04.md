# B8 Ghidra-assisted session

Status: active. Model/effort: Current Codex model/effort not independently retrieved.
Elapsed: 46.89 minutes, including repository recovery, syncing and local tool setup.
Retail remains reference-only; local-model source editing remains stopped.

| Measure | Start after upstream sync | Latest | Session gain |
| --- | ---: | ---: | ---: |
| Matched code bytes | 6370000 | 6371412 | 1412 |
| Matched functions | 28779 | 28786 | 7 |
| Source-linked code bytes | 1245308 | 1245308 | 0 |
| Source-linked units | 545 | 545 | 0 |
| Matched data bytes | 614776 | 614776 | 0 |
| Source-linked data bytes | 498140 | 498140 | 0 |

Matched code: 55.734710%; source-linked code: 10.893486%.
Upstream gains excluded: {"matched_code": 620, "matched_functions": 5, "complete_code": 0, "complete_units": 0, "matched_data": 0, "complete_data": 0}.
Original/rebuilt B8 DOL SHA-1: `e26b3daf41886f0d09670135910f2510cd093ae8`.
Strict target-unit function/data nonregression and global match/link measures guard retained trials; exact executable verification does not imply unfinished source-linked units are complete.

Usage began at 1% current-window / 78% weekly used;
latest is 31% / 83% used.
Consumed percentage points: {"primary": 30, "weekly": 5}.
Refill observed: False. These are rounded account-wide readings, not billing-token counts.
Across refills endpoint consumption is unavailable; the ledger keeps snapshots instead of inventing totals.

Retained partial reconstructions (2):

- `GetNoteSliceWeight__9VocalPartCFffi` (main/band3/game/VocalPart): 46.296295% - New 648-byte constant-pitch/gliding integration; register scheduling and constant ownership remain.
- `Rollback__9VocalPartFff` (main/band3/game/VocalPart): 94.0% - New 200-byte rollback cursor reconstruction; original redundant cursor reload and undefined UpdateMinMaxPitch relocation remain.

Remaining issues:

- Vocal note slice weight retained at 46.296295%; fixed-bound changes can improve register scheduling but currently regress other constants, so restored.
- CalcNoteWeights strict-exact 460 bytes; newly emitted existing STLport weak copies reviewed separately (Answer100%, insert82.65306%, length-error91.66667%, reserve71.42857%). These helpers are not claimed as matched and the unit remains assembly-linked.

Git checkpoints and Ghidra audit/provenance are recorded in the JSON ledger. Source commits explicitly credit ChatGPT; private inputs, native tools and generated pseudocode remain ignored. Ghidra drafts require semantic and assembly review before trials. Different targets and setup overhead prevent treating this as a controlled model benchmark.

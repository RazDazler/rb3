# B8 prepared-context decompilation session, 2026-10-03

Status: complete. Stop condition: applicable account usage window at 1% or less remaining; no time limit. Model label: Current Codex model/effort not independently retrieved; earlier user-reported GPT-6.1 Sol Light context.

Elapsed: 142.05 minutes (2.368 hours). Local-only work; retail reference-only. Original/rebuilt B8 DOL SHA-1: `e26b3daf41886f0d09670135910f2510cd093ae8`. Final source trials and checkpoint preserve the exact executable. Trials protect every original function/data row in each target unit and global matched/link measures; dependent headers are rebuilt. Partial units remain assembled into the executable until source linking passes exact layout validation.

| Measure | Start | End | Gain |
| --- | ---: | ---: | ---: |
| Matched code bytes | 6,362,516 | 6,369,380 | 6,864 |
| Matched functions | 28,651 | 28,774 | 123 |
| Source-linked code bytes | 1,243,344 | 1,245,308 | 1,964 |
| Source-linked units | 543 | 545 | 2 |
| Matched data bytes | 614,640 | 614,776 | 136 |
| Source-linked data bytes | 498,004 | 498,140 | 136 |

Code matched: 55.716938%; source-linked: 10.893486%. The fuzzy score is 68.993546%, a separate metric.

Usage began at 20% current-window/65% weekly used and ended at 99%/78% used. Consumed 79 current-window and 13 weekly percentage points. Remaining 1%/22%. No refill observed. These are rounded account-wide observations, not billing-token counts. The comparison JSON contains normalized observed throughput; target selection and metadata mix prevent causal model conclusions.

SoundTouch AAFilter and FIRFilter account for 1,964 matched/source-linked bytes and 12 functions, recovered from licensed source already present locally. Game/network behavior contributes 4,900 matched bytes/111 functions. No metadata-only gains. The global report includes the 192-byte CallContext constructor as matched even though strict comparison still has two anonymous constant relocations; its missing initialization stores were restored.

Seven retained partial records: three new semantic reconstructions totaling 516 bytes, three improved existing semantic functions and one improved constructor with metadata residuals. They are not counted as source-linked gains:

- GetExcessAudioLag (main/band3/meta_band/ProfileMgr): 42.857143% - Allocate readonly audio lag temporaries without changing the addition order
- CalcPhraseScoreMax (main/band3/game/VocalPart): 61.538464% - Align vocal weighted-overlap loop with cached original pointer and float lifetimes
- AddPhrasePoints (main/band3/game/VocalPart): 85.2459% - Test readonly vocal score-field lifetimes against the original register schedule
- IsADuplica (main/network/ObjDup/DuplicatedObject): 50.0% - DuplicatedObject empty handle comparison and short-circuit forms
-  (main/network/Core/CallContext): 95.83333% - Existing constructor missing three zero stores restored, all instructions match but two anonymous constant relocations remain. Global report counts 192 bytes as matched; strict objdiff remains95.83333.
- transposeStereo (main/system/synthwii/soundtouch/source/SoundTouch/RateTransposer): 83.57143% - SoundTouch stereo explicit condition slope matches original reuse without field reload
- transposeMono (main/system/synthwii/soundtouch/source/SoundTouch/RateTransposer): 86.666664% - SoundTouch mono condition slope reuse follows original stereo observation

Remaining issues:

- VocalPart CalcPhraseScoreMax61.538464 and AddPhrasePoints85.2459 retain original overlap/cap/multiplier behavior; register scheduling remains.
- DuplicatedObject IsADuplica50 remains an empty-handle short-circuit scheduling partial.
- ProfileMgr GetExcessAudioLag42.857143 improved existing addition-order-preserving code.
- RateTransposer stereo83.57143 and mono86.666664 improve condition slope reuse; floating register assignment remains. Derived destructor inherited delete and RTTI layout still block source linking.
- CallContext constructor instruction-exact, strict95.83333 due two constant relocations; global report counts192 bytes as matched. Missing0x2c field corrected without class-size/Time-offset change.
- VocalPart difficulty log precision experiment makes the function exact but regresses anonymous data; restored. CalcNoteWeights remains a stub and may resolve original constant ordering.
- MicWii draft override/vtable ownership remains incomplete. IsPlaying was not retained because it emits wrong draft overrides.
- ProtocolRequestBroker and EndPoint layouts are missing; do not invent subobjects or field types to implement GetCurrentOperation or incoming connections.
- Performer inline defaults did not emit into original owner in the tested form, restored.
- Local-model source worker remains stopped. Finite downtime preparation writes comparison/context artifacts only, without cloud/model calls or source/config edits.

Two shared helpers were checked instruction-for-instruction against their original B8 definitions: MakeString<PCc,i,PCc> (112 bytes) and const VocalNote upper_bound (160 bytes). One unreferenced 64-byte weak abstract CallbackRoot destructor was reviewed separately; it is not claimed as matching original code. Specific helper exceptions do not disable general guards. Detailed trial manifests, outcomes and audit hashes are referenced in the JSON ledger and local build artifacts. Source/config changes and reports remain local; no commits or pushes were made.

Downtime preparation: {"status": "queue_complete", "elapsed_seconds": 8.1227, "units_compared": 0, "cache_hits": 401, "launcher_timeout": true, "note": "Launcher timed out after checkpoint save, but child completed successfully; recovered from saved latest.json."}. The local-model editing STOP and retail STOP remain set.

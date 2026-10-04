# Retail Wii decompilation session

Goal started at 2026-10-02 06:23:23 UTC (01:23:23 America/Chicago).
Work deadline: 08:23:23 UTC (03:23:23 America/Chicago).
Work is local; no commits or pushes have been requested.

## Verified progress so far

The original and rebuilt executable SHA-1 remain
`189b001188a2f8ab769e72364440022407fe09d2`.

- Corrected retail compiler flag ordering: `-O4` implies `func_align 16`, so
  `-func_align 4` must follow optimization. Confirmed in compiler help and by
  replacing original ScoreUtl code with C++ without changing the executable.
- Enabled matching C++ linking for ScoreUtl, RGState, SynchronizationEvent,
  VorbisMem, MidiVarLen, ProfilePicture_Wii, and Compress.
- Recovered Compress's 32-byte string pool at 0x80856DC0 and assigned it to its
  object. This fixes source linking without duplicating the original data.
- Matched AccomplishmentCategory::HasAward using Symbol's inequality operator.
- Matched Quest's custom-intro/outro predicates as non-inline retail functions;
  retained the debug-specific inline workaround.
- Matched retail PlatformDebugBreak's constant-false behavior.
- Matched retail StreamChecksumValidator::HandleError without debug logging, and
  emitted its original Matrix3 indexing helper.
- Identified Mic::GetDroppedSamples at 0x8067A024 from its vtable slot.
- Identified UserMgr::GetLocalUser and GetRemoteUser at 0x8031210C / 0x80312114
  from existing declarations, definitions, function order, and their vtable slots.
- Recovered Mic's 20-byte string pool, with unused debug strings excluded in
  retail builds. The object's instructions and string bytes now match.
- Assigned AccomplishmentCategory's vtable, RTTI and string data, then verified
  its complete source-linked executable build. System_Wii data now also links.
- Linked InstantiationContext, Decibels, Meta, UIResource, tomcrypt/ctr and
  OSReboot. Decibels requires direct `powf`; Meta uses `-ipa off` to retain the
  original helper order. OSReboot uses function alignment 16 and preserves the
  trailing 12-byte padding in the remaining binary split.
- Corrected retail Debug's class layout (no `mAlwaysFlush`), constructor and
  Print's debug-only body. SetReflect now matches its original offsets.
- Matched UIListArrow::Load, GameGem::Flip, CharFaceServo::ScaleAdd,
  WiiFriend::GetProfileByIdx and Latin1ToUtf8. The last uses an integer character
  temporary, reproducing the signed comparison and five-bit extraction.
- Matched UIButton::NewObject using scoped inlining with `-ipa off`.
- Recovered the BandSongPref destructor and MCContainer::BuildPath names.
- Corrected DeJitter's retail 24-byte layout (the extra float exists only in
  debug); Reset and numerous callers now use the original retail offsets.
- Recovered the DeJitterPanelTimer class name, four retail 132-byte adjustment
  thunks and three shared UIPanel thunks. Recovered the DeJitter destructor.
  DeJitterPanel now links its 1,272 code bytes and 188 data bytes from source.
- Matched both Latin1ToUtf8 and Utf8ToLatin1 and linked StringConversion.
- Reconstructed compact-quaternion setters, TransformNoScale conversion, copy,
  rotation access and Reset; emitted the original vector comparison/stream helpers.
- Reconstructed MakeVertical, MakeScale, matrix MakeEuler and the axis-angle
  quaternion setter with exact objdiff instruction comparisons. MakeEulerScale
  has the same instructions with a remaining merged empty-constructor call.
- Recovered eight StandIn method symbols and HxGuid's assignment helper.
  Matched null-symbol predicates, excluded debug SaveSize logging, recovered
  its 56-byte vtable/RTTI region and verified the complete source-linked DOL.

## Important remaining work

- Mic stays NonMatching: source linking currently merges its weak zero-return
  function and introduces four-byte data alignment padding. Full executable
  validation rejected marking it Matching. Its code/data comparisons are 100%.
- Quest, StreamChecksum, and UserMgr have matching function code but require
  data ownership / inline merge work before being marked fully Matching.
  UserMgr's vtable, nine-byte string pool and global pointer were recovered,
  but source linking introduces additional vector helpers and changes layout.
- Fully matching code comparisons alone do not prove a source-linked build:
  reports can ignore different relocation targets and function order. Every
  Matching status added here has separately passed the original DOL SHA-1 check.
- GameGem::PackRealGuitarData has identical operations but swaps registers r4/r5.
  Declaration/initialization permutations did not improve the baseline; reverted.
- UIButton's factory needed `-ipa off` for function-local inlining settings to
  take effect. Its vtable/data ownership has not yet been recovered.
- Retail report regeneration must follow configuration changes: Ninja's report
  rule does not depend on the regenerated objdiff configuration. The local
  build-retail.ps1 explicitly refreshes report and progress after a successful
  build so measurements are current.
- Debug executable input is unavailable; debug binary matching is not tested.
  Existing debug-only branches are retained where possible.

## Evidence

Full builds and comparison files are under ignored `build/goal-*.log` and
`build/*-diff.json`. The current report is `build/SZBE69/report.json`.
Measurements and subsequent work will be appended as the session continues.

## Checkpoint at 07:56 UTC

| Measurement | Initial | Current |
| --- | ---: | ---: |
| Matched code bytes | 2,085,256 | 2,093,244 |
| Matched functions | 19,003 | 19,092 |
| Code linked from source | 90,700 | 97,844 |
| Complete units | 61 | 79 |

The measured denominator changed from 8,253,320 to 8,253,332 code bytes and
59,455 to 59,456 functions because OSReboot's 12-byte padding was separated
into an original-binary split. Unit splits also increased from 1,994 to 2,002.
These bookkeeping changes do not alter the executable bytes.

Rot remains NonMatching: most of its math routines still need reconstruction.
New standalone math functions are validated with objdiff, but the full Rot
object is not linked from source. The executable retains original code for
incomplete objects. StandIn and DeJitterPanel are independently source-linked
and have passed the complete DOL hash check.

Trials that were rejected by complete executable validation remain recorded
in `build/goal-adoption-results.json`; some candidates (e.g. OSReboot) were
subsequently solved manually, so those trial records are historical.

## Final checkpoint

| Measurement | Initial | Final | Increase |
| --- | ---: | ---: | ---: |
| Matched code bytes | 2,085,256 | 2,094,468 | 9,212 |
| Matched functions | 19,003 | 19,099 | 96 |
| Code linked from source | 90,700 | 98,120 | 7,420 |
| Complete units | 61 | 81 | 20 |

Overall matched code is 25.3772%; code linked from source is 1.1889%.
The denominator is 8,253,332 code bytes / 59,456 functions / 2,006 units;
additional section ownership splits account for unit-count changes.

The beatmatch Output initializer now owns its original string pool and BSS,
so its global LogFile can link from C++. Cache now owns its vtables/RTTI, and
CacheID's retail vtable proves an empty destructor rather than a pure virtual
slot. Both changes passed the complete executable SHA-1 check.

Further Rot work reconstructs FastInvert, MakeRotQuat, MakeRotQuatUnitX,
IdentityInterp, FastInterp, quaternion dot product, and the axis-angle
constructor. Exact instruction matches are distinguished from remaining
helper relocations in the ignored objdiff JSON files. The three interpolation /
vector-to-quaternion routines have matching arithmetic with remaining merged
helper or undefined-in-this-object normalization call labels. Quaternion Set
from Euler angles remains about 81.5% in strict objdiff; multiplication still
has substantial register-order differences. These partial routines remain in
the NonMatching Rot object and do not replace original executable code.
Quaternion normalization and the larger matrix/rotation routines remain work.

OnlineID's seven-byte pool was recovered, but its linking trial emits extra
BinStream integer read/write helpers. It stays NonMatching. Its original
binary object is retained in the verified executable.

`doc/DECOMP_VERIFICATION_2026-10-02.json` records the 20 newly source-linked
units, their report measures/functions, and the final DOL SHA-1.
`doc/DECOMP_METRICS_2026-10-02.json` contains the baseline/final totals.
`doc/DECOMP_ADOPTION_TRIALS_2026-10-02.json` preserves the earlier automated
trials (including failures that were subsequently solved manually).

An extra verified executable copy is saved locally at
`build/SZBE69/verified-main.dol`; hashes and source-link configuration are
recorded in `build/SZBE69/verified-build.json`. Binary inputs, executables,
compiler downloads and full assembly/comparison logs stay under ignored
`orig/` and `build/`. All changes remain local and uncommitted.

Validation: native CodeWarrior retail build, objdiff 2.7.1 report and individual
instruction comparisons, original DOL SHA-1, and `git diff --check`.
Debug compilation/matching was not verified because the debug executable input
is unavailable. Existing debug behavior is guarded separately where changed.
The Wii decompilation is still incomplete; this is a verified continuation
checkpoint, not a native PC or PS5 port.

Session stopped at the two-hour deadline, 2026-10-02 08:23:23 UTC.


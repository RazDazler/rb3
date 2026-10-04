# GPT-6.1 Sol Light B8 benchmark — completed

The five-hour session started October 3, 2026 at 02:23:54 CDT (07:23:54 UTC). Its deadline is 07:23:54 CDT (12:23:54 UTC), with an earlier stop at critically low applicable usage. Model setting is user-reported. B8 is primary; retail is reference only. All changes remain local.

## Measured progress

| Metric | Baseline | Verified result | Gain |
| --- | ---: | ---: | ---: |
| Matched code bytes | 6,314,044 | 6,324,704 | 10,660 |
| Matched functions | 28,494 | 28,558 | 64 |
| Source-linked code bytes | 1,234,060 | 1,241,840 | 7,780 |
| Source-linked units | 535 | 539 | 4 |
| Source-linked data bytes | 497,004 | 497,808 | 804 |
| Matched data bytes | 613,848 | 613,848 | 0 |

Code is now 55.326126% matched and 10.86315% source-linked. These are different measures: matching functions in an incomplete unit can still use the original object in the executable. Source-linked units added here are OSAudioSystem (1,216 code / 128 data bytes), ChannelInfo (164 / 0), OverdriveTimeTracker (2,356 / 184), and Wii environment (4,044 / 492).

## What changed

Twenty-eight previously missing source functions now match in relocation-aware objdiff, totaling 5,136 code bytes. Recovered behavior includes texture selection/copying, draw targets, movie swapping, bitmap unlock, resize, LOD/filter setup, screenshot color conversions, texture wrapping, light setup/selection, approximate axis lighting, delayed free/destroy, frame queue sizing, frame timing stop, BeginDrawing, flare depth/area tests, shared texture setup/access, and statistics reset.

Two more newly reconstructed functions, HomeMenuClose and OnVIPostRetrace, have identical instruction text and count as matched in the global report, but retain strict metadata/callee relocation differences. These are recorded separately. Compiler-generated helpers and virtual thunks contribute to the 64 global matched-function gain; they are not counted as newly reconstructed game behaviors.

The Wii camera constructor proves a 48-byte derived view transform at offset 0x278, with its virtual base starting at 0x2a8. Restoring that real field matches the constructor and associated generated functions (612 bytes / 14 functions). It also enables exact approximate-lighting reconstruction. The environment constructor now matches after direct RGB/dirty state updates, a cached material pointer, the original Environ class name, and original LightId assertion spelling. All 17 environment functions match; restoring definition order and restricting Light helper emission to its owning renderer unit permits exact source linking.

Two Tour assertion-line fixes account for 408 matched bytes / two functions of metadata-only progress. The 860-byte environment constructor gain mixes source corrections with a final literal repair and is listed separately in the ledger. Its entire credit appears when the last mismatch disappears; it would be misleading to treat all of that gain as newly reconstructed behavior or as a pure metadata change.

## Partial work and next candidates

| Function | Strict match | Remaining issue |
| --- | ---: | --- |
| WiiTex::DeleteSurface | 73.86% | Packed ownership flags and scheduling |
| WiiTex::LockBitmap | 68.66% | Flag normalization and registers |
| WiiTex::DisableFiltering | 33.33% | Packed boolean update |
| YUV444ToRGB | 89.87% | Eight temporary/register differences |
| WiiTex::Compress | 88.18% | Twenty-four temporary-register differences |
| FrameQueueSync | 31.11% | Original shared BSS grouping/offsets |
| WiiRnd::YRatio | 71.43% | Shared versus duplicate return blocks |
| StartRenderingFrame | 85.71% | Three shared BSS offsets |
| SetNextGXBreakpoint | 54.29% | Shared BSS base and pointer scheduling |
| WiiCam::ProjectZ | 33.33% | Ten floating-register differences |

The first five partials recover previously missing texture behavior. Frame synchronization and breakpoint recycling also have behavior reconstructed. YRatio improves an existing implementation; ProjectZ corrects an existing incomplete formula. Generic statistics records retain unknown counter indices rather than invented names. Remaining renderer dependencies include camera selection and GPU hang recovery. Near-match candidates in OggMap, GuitarController, ByteGrinder, Bitmap, Timer and OutfitProvider were reviewed without accepted gains.

## Verification and automation

The full B8 build passes, and the built executable and original both have SHA1 e26b3daf41886f0d09670135910f2510cd093ae8. The preserved retail reference has SHA1 189b001188a2f8ab769e72364440022407fe09d2. Each accepted source trial checks exact executable output plus target-unit/global nonregression. The environment has a weak RTTI comparison residual at object level, but its source-linked executable is exact.

The replacement runner now handles inherited mixed LF/CRLF source while rejecting ambiguous matches and preserving untouched bytes. All 43 automation tests pass, git diff --check passes, and no pending source-trial journal remains. Logs are build/light-20261003-final-verification.log and build/light-20261003-final-tests.log. clang-format is unavailable on PATH; no upstream submission was made. Apply the repository formatter before submitting.

## Model comparison

| User-reported setting | Previous elapsed time | Matched bytes / functions | Linked bytes / units | Account usage consumed |
| --- | --- | --- | --- | --- |
| Medium | 2h 50m 50s | 60,288 / 163 | 6,600 / 3 | Comparable start unavailable |
| High | 2h 30m 39s | 50,024 / 158 | 6,824 / 3 | 97 primary / 15 weekly points |
| Light | 2h 36m 50s | 10,660 / 64 | 7,780 / 4 | 98 primary / 16 weekly points |

High included 35,056 bytes / 66 functions of metadata-only repairs and 26 new exact functions / 2,944 bytes. Light has fewer raw matched gains but more source-linked bytes and 28 strict new functions / 5,136 bytes, with much less pure metadata work. This is not a controlled model comparison: candidates, starting state, prior preparation and opportunities differ. Light also benefits from a saved High-session drawing proposal and earlier tooling. These results do not establish that one reasoning setting is intrinsically more efficient.

Usage observations are rounded account-wide percentage points, not token counts. The verification JSON stores starting/final usage, elapsed time, accepted trial evidence, categories and remaining issues. The session stopped at the critical-usage condition.

## Final stop checkpoint

Stopped October 3 at 05:00:44 CDT (10:00:44 UTC), after 2h 36m 50s, with 1% remaining primary usage and 53% weekly. Rounded account consumption was 98 primary / 16 weekly percentage points. Primary refill is October 3 at 07:23:55 CDT (12:23:55 UTC); weekly refill is October 09 at 16:11:16 CDT (21:11:16 UTC). No actual token-consumption figure is exposed. The five-hour deadline was not reached. All source edits, configuration changes and reports are local; nothing was committed or pushed.

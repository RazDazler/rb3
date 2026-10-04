# Wii decompilation session, October 2, 2026

Started at 10:31:25 AM America/Chicago (15:31:25 UTC). Deadline: 1:31:25 PM (18:31:25 UTC). Stop earlier if an applicable account usage window reaches 1% remaining. All work stays local and uncommitted.

## Checkpoint

Session finished at the three-hour limit. At the user's request at approximately 12:49 PM, the primary target changed to the supported SZBE69_B8 debug build. The original three-hour deadline remains in effect. Both versions now have matching executables; metrics below keep their baselines separate.

- retail matched_code: 2,103,112 (baseline 2,094,468)
- retail matched_functions: 19,132 (baseline 19,099)
- retail complete_code: 112,348 (baseline 98,120)
- retail complete_units: 93 (baseline 81)
- total_code: 8,253,332 (baseline 8,253,332)

## Verified work

1. Added explicit source-variant testing with transactional recovery, an exclusive runner lock, objdiff regression checks, and final full-executable verification. Added focused physical DOL comparison to diagnose failures hidden by symbol aliases, and compact paired-function inspection. Recovery validates backup checksums before writing. Cached official source trees are revalidated against their archives. Runners support the debug checksum manifest and share a lock across versions. Automation regression suite: 27 tests passed.
2. Downloaded isolated official zlib 1.2.1, Speex 1.2rc1, and Vorbis 1.0.1 reference sources. Recorded transport/provenance and hashes; the Vorbis archive hash also matches the publisher's SHA256SUMS. References were compared with bundled source, not installed over it. See LIBRARY_UPSTREAM_REFERENCES_2026-10-02.md.
3. Matched zlib deflateInit2_ (708 bytes), three trigonometry functions (444 bytes), interpolation functions (420 bytes), ChannelInfo destructor (104 bytes), and TrackTypeToSym (68 bytes), and CacheWav (236 bytes). Full builds preserve the retail executable.
4. Source-linked complete units: Speex bits.c (1,304 bytes), zlib deflate.c (7,444), Interp.cpp (2,760), ChannelInfo.cpp (296), TrackType.cpp (176), TomCrypt crypt.c (272), MBT.cpp (328), ProductSpecifics.cpp (80), OnlineID.cpp (272), and synth/Utl.cpp (264). Some were already instruction-matched at the baseline; these gains must not be counted as newly reconstructed bytes.
5. Recovered data ownership for TrackType and MBT. Confirmed shared empty Interpolator destructor and the identical int/unsigned-char formatting-template implementation by instructions, callees, and full DOL verification. These are linker folding/metadata corrections, not additional gameplay code.

6. Source-linked QuestJournal and DOFProc (the latter with IPA disabled). Matched Color's TextStream output (136 bytes); MakeColor has a tested partial implementation but is not matched or linked from source.
7. Corrected BlockMgr's LRU timestamp condition, restored the original chained AnimFilter stream read, and recovered retail UILabel/VocalTrackDir string pools. Identified that retail Rnd lacks the two debug ScreenDump virtual slots; the guarded header correction added 2,844 matching bytes and 12 functions while preserving the full retail executable. These changes account for additional matching gains; string-pool fixes can improve multiple callers simultaneously.
8. Audited both added dumps. The first was another retail version without additional conventional symbol files. The second exactly matches supported B8 and includes a 12.3 MB CodeWarrior linker map. Its names are already almost completely represented by the project's debug metadata, but the executable enables local named debug disassembly. See DEBUGVERSION2_AUDIT_2026-10-02.md.

## Debug target switch

Prepared only the confirmed executable, selector and map under ignored orig/SZBE69_B8. Generated debug original objects with DTK --no-update. The first full debug build caught earlier retail-only changes to TrigTableInit and GameGem::Flip. Both now retain separate debug and retail implementations. The complete debug build then passed SHA-1 `e26b3daf41886f0d09670135910f2510cd093ae8`.

Verified debug baseline before new B8 trials:

- matched_code: 6,203,180 / 11,431,676 (54.263084%)
- matched_functions: 28,170 / 41,333
- complete_code: 1,217,928 (10.6539755%)
- complete_units: 524 / 1,876

These debug totals are existing project progress, not gains produced by this session. Debug and retail percentages cannot be subtracted: they have different executable sizes, optimization, function folding and metadata.

Use ./build-wii.ps1 -Version SZBE69_B8 for the primary debug build. ./build-retail.ps1 remains available for the secondary retail reference. The retail trial batch stopped cleanly via its STOP marker before switching targets.

Verified B8 gains after the switch:

- Source-linked five complete units: MetroTRK target_options (16 bytes), OSSync (132), StringConversion (320), OSStateFlags (524), and Speex bits (1,716). Total additional linked code: 2,708 bytes, from 524 to 529 complete units.
- Restored NetLoader's direct string initialization and original cache assertion: its 148-byte constructor now matches exactly. Its restored string pool also completes AttachBuffer (148 bytes) and PollLoading (256 bytes). Total additional report-matched code: 552 bytes and three functions.
- Reconstructed the missing 384-byte NetLoaderStub constructor, including local-path formatting, FileLoader creation, original assertion, and simulated download timing. Strict comparison is 98.95833%, with one remaining call to FilePath's destructor instead of String's destructor. This is partial progress and contributes no matched-code gain above.
- Confirmed GetServerRoot returns a string pointer; corrected its previously guessed int declaration from the named debug call and `%s` formatting use.
- Local inline-control trials and a transactional implicit FilePath destructor trial did not fix the remaining constructor difference. The global header trial rebuilt exactly but did not improve the function; its source was restored. Timer::Reset still has one floating-point operand-order mismatch; attempted expression/cast variants were rejected.

Latest B8 report: 6,203,732 / 11,431,676 code bytes matched (54.267914%); 28,173 / 41,333 functions matched; 1,220,636 code bytes linked from source (10.677665%); 529 / 1,876 complete units. These gains are measured against the debug-switch baseline, not the retail baseline.

Both full builds and independent executable checksum checks passed after the shared-source changes. B8 remains the configured primary target. Automation regression suite: 27 tests passed, including refusal to compare objects from a different configured version. No recovery journal remains pending.

## Remaining issues and useful next candidates

- ProductSpecifics RTTI and shared Quazal RootObject RTTI are now identified, verified, and source-linked.
- Trig.cpp has all current source functions matched, but four original header helpers and their symbol/data ownership remain unresolved.
- Vec.cpp appears to own adjacent matrix static initialization. Implementing Mtx.cpp requires proper types and split/BSS ownership; do not fake missing functions merely to raise a metric.
- CacheWav now matches all 236 bytes and its unit links from source. Direct formatting arguments preserve the retail call order and eliminate an unnecessary Symbol copy. Its disc lookup helper is correctly declared with a path argument; the helper implementation itself remains undecompiled.
- The standalone runners need an externally updated quota file for account limits. They do not themselves query Codex or generate arbitrary missing C++.
- Rejected trial evidence, source-variant manifests, objdiff summaries, and recovery logs remain under build/decomp/SZBE69. Run recovery before new work if pending.json exists.

Final checkpoint: DECOMP_THREE_HOUR_VERIFICATION_2026-10-02.json. Both original/built executable SHA-1 pairs were rechecked at shutdown, and no recovery journal remains pending. All changes remain local and uncommitted.

Next B8 work: resolve the NetLoaderStub destructor call, Timer::Reset operand ordering, and RefCountedObject data ordering before source adoption. Inline-size 24/32/48 trials did not improve NetLoaderStub and were rolled back.

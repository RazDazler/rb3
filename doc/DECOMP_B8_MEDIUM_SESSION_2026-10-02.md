# B8 decompilation session, October 2, 2026

Started at 3:34:17 PM America/Chicago (20:34:17 UTC). Deadline: 6:34:17 PM (23:34:17 UTC), or earlier at 1% remaining in an applicable account usage window. Stopped at 23:25:07 UTC when five-hour usage reached 1% remaining, before the three-hour deadline. B8 is primary; retail remains a secondary reference. All edits stay local and uncommitted.

## Verified work

- Used the original B8 executable, named functions, generated disassembly and vtables to prioritize changes. Saved compact triage for 53 near-matching functions across 30 units under build/decomp/SZBE69_B8/medium-triage.json.
- Exact matches: GemPlayer trill equality; StreakFocusTracker positive-streak test; Automator new-screen selection; SoundTouch unsigned channel comparison (both conversion paths); AccomplishmentManager original available-song-count assertion; SongParser explicit zero test for non-part tracks.
- Completed every function and data item in NetLoader.cpp, including its factory, NetLoaderStub constructor/destructor, and DataNetLoader construction, cleanup, failure check and DTZ/stream decoding. The timing expression used division by the simulated transfer rate. Recovered three pure virtual methods from the B8 vtables and corrected callers.
- Source-linked the complete NetLoader unit: 2,172 bytes. Inline getter declarations initially matched callers but moved function bodies; local auto-inline controls and the original function order produced a full-executable match.
- Recovered NetLoaderWii layout and reconstructed the complete HTTP download state machine, request/response handling, cleanup and URL construction. Corrected port typing, response-size reuse, template placement and literal ordering. Source-linked the complete unit: 1,528 bytes; full B8 executable matches.
- Recovered cache-manager server records, service ID, FileCache pointer and loader-reference lists from named B8 calls and offsets. Matched server accessors, readiness tests, constructor/destructor, reference counters and lifecycle methods. Server lookup and queue routines retain register-allocation differences. Compiler-only metadata/destructor helpers were individually reviewed; the cache-manager unit still links original code.
- Added coordinated multi-file source trials, optional source adoption, backup/recovery, global regression gates and physical/layout failure diagnostics. Saved per-trial inspection evidence; fixed protobuf omitted-zero paired indexes. Added a version-pinned literal-pool proposal script with assembly refusal checks. Automation regression suite passed 32 tests.

- Recovered the complete cache-manager message handler, state transitions and Wii manager allocation. Initialization and queue scheduling are reconstructed but still partial.
- Corrected post-processing overlay output redirection; its function matches. Recovered Quazal LogEntry composition; logging matches strictly, but its source-adoption trial failed data ordering and was restored.
- Original literal ordering recovered additional exact function matches in BandPatchMesh, PatchDir, VocalTrack, GamePanel, BandStarDisplay, SongUpgradeMgr, UTF8, AccomplishmentProgress, BlockMgr, PlatformMgr_Wii and TourPerformer. These are metadata-order improvements; missing implementations remain incomplete.
- Corrected GroupSeqInst pool deletion to use its derived size (0x40 instead of inherited 0x34); destructor matches strictly.
- Cache-manager startup matches after restoring original definition order. OnInit is 99.04306%, with only the placement of a host-pointer initialization store differing.
- GemManager, Singer and MetaPanel literal trials were rejected by data regression guards. At least one is objdiff pairing an anonymous RTTI record with a string pool at the same offset; do not bypass the guard without checking actual data identities.

- Matched AccomplishmentManager award-description and reward-vignette empty Symbol fallbacks by restoring assignment to the default-constructed local.
- Recovered Game::PopulatePlayerLists null-user handling: the original null branch comes from multiple-inheritance pointer conversion, not an additional user guard. Instructions now agree; four relocation differences remain to other incomplete callees (96.52%).
- Reconstructed BlockMgr GetAssociatedBlocks (88.46%), KillBlockRequests (72.73%), AddTask (93.91%) and Poll (94.74%). Poll recovers read retry/completion, seek diagnostics, request/task dispatch, disc-write scheduling and idle request selection. Restored seek-stat globals and reviewed standard compiler helpers. Remaining differences are saved as instruction evidence; this unit still links original code.
- Reconstructed HDCache ReadAsync (88.89%), WriteAsync (91.80%), header sizing (100%), encrypted header writing (96.58%), cache initialization (96.34%) and archive file creation/opening (97.44%). Recovered the header version/GUID/count, per-archive bitsets, SHA-1 text digest and page padding. Fixed ReadDone's missing return, WriteDone's signature/behavior, and UnlockCache's predecrement; all three now match strictly. OpenHeader matches strictly after restoring the loop exit structure. Poll is 97.01% with two branch-layout differences.
- Corrected cache handle vectors to File pointers and FileMkDir's path argument. Reviewed emitted File base methods and standard vector allocation/destructor helpers individually; helper allowances are recorded in trial evidence. Initialization uses unsigned saved record lengths and byte-buffer allocation, as shown by B8 instructions.
- Initialization reproduces an observed B8 expression advancing an int pointer by savedBytes before clearing bytes. That is evidence for matching, not a proposed portable implementation or proof of runtime memory safety. The HD cache paths still need matching and runtime scrutiny before any native-port use.

- Corrected MainHubPanel exit to reset the waiting override, PassiveMessenger to compare its queue count as signed, RockCentral patch-byte encoding to extract unsigned high/low nibbles, and Performer to consult Game::Properties::mCanLose. Each changed routine now matches strictly.
- Corrected Server::GetCustomAuthData and WiiServer's declaration to return the AnyObjectHolder address, as shown by B8 initialization/return instructions. All Server functions and data match; source-linked the complete 2,900-byte unit with an exact B8 DOL. The session now has three newly source-linked units (6,600 bytes total).
- Restored B8 renderer and vocal-guide literal ordering using reviewed version guards. Exact metadata gains: Rnd 792 bytes/3 functions; VocalGuidePitch 232 bytes/1 function. These are not new behavior implementations.
- FileCacheFile::Eof matches after restoring operand evaluation order. File-cache failure-predicate trials are gated individually; rejected candidates remain in build/decomp evidence. Seek retains shared ClampEq temporary/load-order differences; do not alter the shared math template without global checks.
- Final automation suite passed all 32 tests. Retail executable was recompiled and matched after shared interface changes. Both versions have fresh matching executable checks and reports; B8 is active. No pending trial remains; the advisory lock is released. Its persistent marker is expected.

## Checkpoint metrics

{
  "matched_code": 60288,
  "matched_functions": 163,
  "complete_code": 6600,
  "complete_units": 3,
  "matched_data": 2488
}

Gains are measured against medium-session-baseline.json, a fresh B8 report before these trials. Matching functions and source-linked units are different milestones; these measures must not be added together or compared directly with retail percentages.

## Remaining work

- Resolve NetCacheMgr OnInit host-pointer store scheduling, server/SSL comparison operand order and loader queue registers. State transitions and reference lifecycle behavior are now reconstructed.
- Resolve BlockMgr scheduling registers and KillBlockRequests argument load order. HDCache Init, OpenFiles and WriteHdr have register swaps; Poll has one branch inversion/extra branch; ReadAsync/WriteAsync retain reload scheduling. All methods now have implementations, but these two units still link original objects. Preserve matches and the full executable hash.
- Timer::Reset still has one floating-point operand-order mismatch. RefCountedObject still has unresolved data layout.
- Scripts automate explicit trials and validation, not autonomous proposal generation. Quota files require an external usage update.

Detailed evidence and rejected trial manifests are under build/decomp/SZBE69_B8. Use B8 explicitly for runner commands. Never run builds concurrently with trials. Run recovery first if pending.json exists.

## Final verification

Both original inputs and rebuilt executables match the pinned SHA-1 checksums. B8 is the active build configuration. Automation tests: 32 passed; git diff --check passed. No pending trial remains, and the shared advisory lock can be acquired and released. All work is local and uncommitted. Final measures, hashes and stop reason are in DECOMP_B8_MEDIUM_VERIFICATION_2026-10-02.json.

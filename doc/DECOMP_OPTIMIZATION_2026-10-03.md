# B8 decompilation efficiency checkpoint

This session replaces the default recommendation to run the local source-proposal
worker with deterministic, artifact-only preparation. Keep the old model worker
stopped. A small local-model pilot previously contributed two exact functions
(268 bytes), but the following 143-attempt session retained no model proposals.
That does not justify continuing its unrestricted attempt queue.

## Measured results

| Workload | Result | Elapsed |
|---|---|---:|
| Initial three-unit preparation pilot | 17 packets, no source/config edits | 6.55 s |
| Initial three-unit repeat | Three comparison cache hits; no new m2c runs | 4.26 s |
| 64-unit preparation batch | 145 packets; 142 m2c outputs, three failures | 41.77 s |
| Final 210-unit fresh preparation | 374 packets | 110.67 s |
| Same 210-unit workload, catalog cached | 210 cache hits, zero comparisons/m2c runs | 5.29 s |
| Broad small-function preparation | 403 units, 2,523 packets; reused 210 comparisons | 342.18 s |
| Same broad workload, catalog cached | 403 cache hits, zero comparisons/m2c runs | 7.77 s |
| Local-model advisory pilot | Three functions, 18 observations, at least three clear errors | 13.32 s |

The final cached preparation is approximately 21 times faster than its fresh
run. This measures preparation, **not** a 21-fold increase in matching progress.
The 210-unit queue contains seven string-metadata candidates, 70 register-allocation
hints and 297 instruction/behavior cases. Classification is triage, not semantic
proof. The broad scan has 1,703 missing compiled functions, 720 instruction/behavior
cases, 93 register hints and seven string-metadata cases. m2c returned 2,362
unreviewed outputs, 93 failed tool exits and 68 unavailable extracts. None are
claimed matches. Its 342.18-second run reused prior comparison/pseudocode work;
it is not a fully cold benchmark. All 403 units were cache hits on the repeat.

Game source, headers and configuration are hashed before and after preparation:
3,307 protected files were unchanged in the measured runs. The verified B8 DOL
SHA-1 remains `e26b3daf41886f0d09670135910f2510cd093ae8`.

One deliberately separate, manually reviewed source trial used m2c's RTT output
as orientation. It restored the missing variance accumulator update, preserved
unsigned modular arithmetic and arithmetic sign extraction, and improved
`Quazal::RTT::Adjust` from 13.33% to 20% in strict objdiff. It remains partial.
Twenty-four declaration orders were tested; variant 4 was retained after whole-unit
nonregression and full native verification. This is a maintainer-approved edit,
not an unattended preparation action. Exact matched and source-linked gains are
both zero for this experiment; do not count a partial as a completed function.
The reviewed unsigned expression also agreed with a separate simulation of the
original PPC operation chain on 10,512 boundary/random input triples. This
supplements the modular-arithmetic review without executing game code on the host.

## Run while Codex is unavailable

From the `rb3` repository directory in PowerShell:

```powershell
# Default: finite, hidden, read-only preparation of near-matching small functions.
powershell -NoProfile -File tools/decomp_prepare.ps1 -Action Run -Minutes 60 -Limit 256

# Broader queue, including low-scoring small functions.
powershell -NoProfile -File tools/decomp_prepare.ps1 -Action Run -Minutes 60 -Limit 1024 -MinScore 0

powershell -NoProfile -File tools/decomp_prepare.ps1 -Action Status
powershell -NoProfile -File tools/decomp_prepare.ps1 -Action Stop
```

The wrapper reuses the audited m2c checkout if present, without downloading or
starting a model. Foreground preparation is also available:

```powershell
python tools/decomp_prepare.py --minutes 60 --limit 256
```

Add `--m2c build/references/m2c-708d2d2cb2698f091a92492b328f73b24209f72d/m2c.py`
to request pseudocode. Output is under `build/decomp/preparation/SZBE69_B8/`:
read **HANDOFF.md** for a bounded shortlist, **latest.json** for all candidates,
and the selected per-function packet for original/compiled rows, relocations,
original map ownership, compiler flags and historical trials. Advice and pseudocode
are unreviewed evidence. The normal native runner still decides whether edits match.

This runner makes no cloud/model requests, ignores the externally recorded Codex
quota, and can therefore run when Codex has no allowance. It has a time budget,
fresh-comparison budget, command timeouts, STOP marker and the shared native lock.
Cached units do not consume the fresh-comparison budget. It requests process-scoped
sleep prevention and releases it on exit. The current bounded command may finish
after STOP/deadline; it does not start the next unit. Start it after other native
trials finish. A busy lock or pending journal makes it fail without editing sources;
recover pending trials separately with `decomp_runner.py recover`.

No source replacements, configuration adoption, automatic proposal execution,
journal recovery, game execution or model editing occurs. Only build outputs and
review artifacts are written. Source/config changes detected during a run make
its final state fail; no external edits are reverted. Native synchronization
assumes the project is already configured for B8 and rejects another version.

When saving a session ledger with `tools/decomp_session_checkpoint.py`, the new
`--prepare-downtime` flag launches a finite broad preparation queue **after** exact
verification and after releasing the native lock. Supply the account usage/reset
measurements as before. The script cannot obtain account usage itself, wake Codex,
refill an allowance or resume this chat. Reserve cleanup time before reaching 1%.

## Why the local model is optional

The advisory test supplied only original assembly, required existing row citations,
used deterministic sampling, and prohibited replacement code. The model still:

- Reversed `fsubs f3,f5,f6` in ProjectZ: the operation is `f5 - f6`.
- Reversed `subf r8,r0,r4` in RTT: the operation is `r4 - r0`.
- Described a USB relative branch displacement as a packet row number.

Citation checks passed because the rows existed; they did not prove the claims
true. Deterministic memory/call extraction already supplies most correct facts
in these responses. Increasing output speed or attempts does not address these
errors. A different/larger model could improve accuracy, but this small benchmark
does not justify a download or prediction of gains. The 8 GB GPU already ran the
7B model entirely on GPU; the observed bottleneck is task quality, not inference speed.

`tools/decomp_advice.py --packet <absolute packet path>` remains an explicitly
optional localhost-only experiment, with bounded inputs/output and one cached
response per packet/model digest. It writes notes only and never runs generated
commands or code. Notes require semantic review and a freshness check against the
current packet. The default preparation/checkpoint launch does not call it.

## Resources and next-session process

The original B8 map supplies mangled names, addresses, sizes and object ownership.
It does not supply complete local-variable types or original source. Preparation
combines this map with DTK's original assembly, current compiled rows, existing
headers/compiler settings and strict objdiff evidence. The map is indexed once
per run. Actual source/reference/object/tool hashes invalidate cached comparisons;
packet catalogs additionally account for selected functions, map/history changes,
original assembly and the m2c Python tree. Artifact hashes detect changed cached
packets/pseudocode. Native commands now record compact timing telemetry in
`build/decomp/SZBE69_B8/command-timings.jsonl`.

At the start of a decompilation session:

1. Check the B8 executable and pending journal; read the small preparation handoff.
2. Pick a packet, then confirm original semantics and relevant headers. Read full
   unit JSON only when the packet lacks necessary evidence.
3. Use the literal tool for string-only differences; investigate symbol ownership
   for relocation-only differences; use reviewed local enumerators for equivalent
   register/lifetime variants. Use m2c orientation for missing behavior.
4. Validate proposed edits with existing transactional source/variant trials,
   all original function/data checks and the exact full executable. Link units only
   after section/layout evidence agrees. Record rejected strategies to avoid repeats.
5. Save partial reconstructions separately from exact matched/linked gains and
   start preparation at the checkpoint if the queue needs refreshing.

The library audit was refreshed under the preparation folder. Bundled declarations
identify zlib 1.2.1, Speex 1.2rc1, LibTomCrypt 0.70, STLport 5.0.1, SoundTouch 1.3.0
and the Vorbis vendor revision `20030909`. Historical zlib/Speex/Vorbis releases
were already downloaded and compared in earlier sessions. Many files already
agree, so downloading those files again offers no reconstruction benefit.
Prioritize game changes, exact build flags, declarations and ownership differences.
Vendor declarations alone do not establish the exact binary's upstream version.

## Tool provenance and limits

[m2c](https://github.com/matt-kempster/m2c) documents big-endian PowerPC,
CodeWarrior and partial C++ support. Audited local commit:
`708d2d2cb2698f091a92492b328f73b24209f72d`; archive SHA-256:
`847a5f6f2dcece29bb2424d993ff70433c7e81bc9fd235bd9be5a1b1946b8bb3`.
Its public GPL-3.0 license/source and download record remain in the isolated
checkout. No global Python dependencies were installed. Missing data context,
complex C++ and unsupported symbols can yield incorrect pseudocode or failures.
Successful tool exit is not semantic approval or an exact match.

[LLM4Decompile](https://github.com/albertan017/LLM4Decompile) documents Linux
x86-64/GCC support; it does not establish suitability for Wii matching.
[decomp-permuter](https://github.com/simonlindholm/decomp-permuter) supports
PowerPC and can complement reviewed finite searches. Its C parsing, isolated
compilation adapter and scoring need integration work for this C++ repository;
random score improvements require semantic review. It was researched, not installed.

Keeping context bounded and loading detailed evidence only when needed is also
consistent with [OpenAI's guidance on avoiding bloated agent context](https://developers.openai.com/blog/rethinking-skills-and-prompts-for-gpt-6-astra).
This session implements that principle with packets and a shortlist rather than
adding a large general instruction file. There is no measured guarantee of
completing the remaining decompilation this month.

Regression suite: 70 tests passed, including cached repeat/object invalidation,
version/path boundaries, source preservation, pending-journal refusal, STOP behavior,
localhost restrictions and note-citation validation. Machine measurements and the
broader scan checkpoint are saved separately in the session JSON.

## Closing checkpoint

Optimization goal completed after 43.5 minutes. Observed allowance at the final check: 81% primary and 35% weekly remaining. Approximate account-wide use changed by 19 primary and three weekly percentage points; these are rounded percentages, not a billable token count or a model-specific measurement. All background preparation jobs finished. The legacy source-proposal STOP marker remains in place. The final B8 SHA-1 is exact, with no pending journal.

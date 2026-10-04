# Local B8 decompilation helper

**Current recommendation (October 3 optimization session): keep the source-proposal
worker stopped and use `tools/decomp_prepare.ps1 -Action Run` for downtime work.**
The subsequent 143-attempt model session retained no proposals; a narrower
assembly-note pilot also made operand-order errors. The deterministic preparation
runner writes review artifacts without game-source edits, caches comparisons and
can use the audited PowerPC m2c tool. See
[DECOMP_OPTIMIZATION_2026-10-03.md](DECOMP_OPTIMIZATION_2026-10-03.md)
for measurements and commands. The setup/pilot history below is preserved;
its initial successes do not establish reliable unattended reconstruction.

Setup is verified. The installed model digest is
`dae161e27b0e90dd1856c8bb3209201fd6736d8eb66298e75ed87571486f4364`.
Ollama reports **100% GPU** at the configured 6,144-token context, about 4.85 GB
model/runtime allocation; total GPU memory use during initial testing was about
5 GB. Actual four-task pilot generation was 85.95–91.20 tokens/second after
loading, with 8,952 local prompt tokens and 984 local output tokens. The pilot
took 58 seconds including native verification. These are small-task measurements,
not throughput forecasts for arbitrary functions or larger contexts.

One pilot proposal made `RndBitmap::SetPixelIndex` match exactly (96.97% to 100%,
all instruction and relocation differences removed). After semantic review and
another full B8 verification, its branch-local pixel-read temporaries were
retained: **+132 matched code bytes / +1 function; +0 linked bytes**. Current
matched code after that pilot was 6,324,836 bytes. A regressing ByteGrinder proposal was rejected
and restored; ProjectZ and YUV initially returned unchanged code. A subsequent
cached batch found no additional improvement. The policy-3 pipeline now supplies
distinct temporary-lifetime strategies and modestly increasing sampling
temperature. Its finite background batch has a two-hour maximum and can finish
earlier when its queue is exhausted.

The policy-3 batch has now finished. Its first ByteGrinder op8 proposal used two
u8 temporaries and improved 97.06% to a strict 100% match. After reviewing call
order, truncations and XOR behavior, it was retained through another full build.
**Total reviewed local-model gains: 268 matched bytes / two functions; zero
linked bytes.** Matched code is now 6,324,972 bytes. No local worker is currently
running; re-run the pipeline after expanding/revising the queue or increasing
its deliberate attempt budget. The Ollama service remains available and unloads
idle models automatically.

All 53 automation tests pass. The full B8 build remains exact and the final
verification checkpoint has no pending journal. Evidence and pilot details are
saved in [LOCAL_AGENT_SETUP_2026-10-03.json](LOCAL_AGENT_SETUP_2026-10-03.json).
Remaining work is to review any further verified proposals, expand the trusted
queue to other small functions, and compare reviewed gains per hour before
investing in a larger model or task auto-generation. The small pilot establishes
that offloading works, not that it can complete the game within this month.

This helper offloads candidate generation and verification to this PC. It does
not consume Codex inference allowance, increase that allowance, or keep the
Codex conversation active after its quota expires. Preparing tasks and reviewing
results in Codex still consumes Codex usage. The worker continues independently
while the computer is awake, until its finite queue, STOP file, deadline, or a
verification failure stops it. There is no cloud fallback or paid API key.

## Hardware and model

Detected October 3, 2026: i5-14600K (14 cores / 20 threads), RTX 3070 Ti (8 GB,
driver 596.36), 31.7 GiB usable system RAM, about 1 TB free on H:. The initial
model is `qwen2.5-coder:7b-instruct-q4_K_M` (4.7 GB weights). Configuration:
6,144-token context, q8_0 KV cache, flash attention, one inference at a time,
900 output tokens, four compiler jobs. The portable Ollama 0.35.1 archive is
pinned to SHA256 `dc50b9ca7f9023c86525012632cd1615b093d0407987444a7f62ecab617e8e93`.
The installed model digest is recorded in each run. Actual GPU residency and
generation speed must be measured; model size alone is not a fit guarantee.

The 7B coder is a practical starting point, not a model proven best for this
game. LLM4Decompile is specialized, but its documented support targets Linux
x86_64/GCC; there is no demonstrated Wii PowerPC/CodeWarrior matching support
in that documentation. A general coder supplied with the original assembly and
compiler context is the better initial experiment. Larger models can spill into
system RAM but can reduce throughput on this GPU; only compare them after this
pilot produces a measurable baseline.

Official references: [Ollama Windows standalone runtime](https://docs.ollama.com/windows),
[model tags and sizes](https://registry.ollama.com/library/qwen2.5-coder/tags),
[GPU, context, cache, and local-only settings](https://docs.ollama.com/faq),
[LLM4Decompile's documented architecture](https://github.com/albertan017/LLM4Decompile),
[Codex usage rules](https://learn.chatgpt.com/docs/pricing).

## Start, monitor, stop

From the repository directory in PowerShell:

```powershell
# Download/verify portable runtime and model (already done after setup succeeds).
powershell -NoProfile -ExecutionPolicy Bypass -File tools/local_agent.ps1 -Action Setup

# Start a hidden finite pipeline. No GUI or Codex connection is needed.
powershell -NoProfile -ExecutionPolicy Bypass -File tools/local_agent.ps1 -Action Run -Minutes 120 -AttemptsPerTask 8 -Limit 32

powershell -NoProfile -ExecutionPolicy Bypass -File tools/local_agent.ps1 -Action Status
powershell -NoProfile -ExecutionPolicy Bypass -File tools/local_agent.ps1 -Action Stop

# After the worker exits, stop the owned model server to release resources.
powershell -NoProfile -ExecutionPolicy Bypass -File tools/local_agent.ps1 -Action StopServer
```

The pipeline requests that Windows stay awake while it runs (the display may
turn off), then releases that request on exit; global power settings are unchanged.
Manual sleep or a failed sleep-prevention request still interrupts progress.
Stop is graceful: allow the current
inference/build and recovery to finish. Avoid killing the worker or changing
active trial sources. All native tools share `build/decomp/runner.lock`; the model
worker releases it during inference and rejects stale source/config snapshots.
Run only one pipeline. Manual builds should use the shared runner too; a bare
Ninja command does not acquire this lock.

Runtime, model weights, logs, downloaded archives and experiment cache live in
ignored `build/local-agent/`. Ollama listens on `127.0.0.1:11435`, with cloud
features disabled; no global PATH or automatic Windows startup is configured.
The runtime can also create its normal Ollama key/config files in the user profile.
This setup does not train a model or send source/binaries to a model provider.

The pipeline runs the existing library audit, the reviewed deterministic search,
bounded source-link trials, then the local model queue. The source-link runner
keeps changes only when the source-built complete B8 executable is exact.
Previously exhausted unchanged variants/source-link trials are skipped. The
library audit inventories bundled source versions and integration differences;
it does not assume downloaded modern libraries are drop-in replacements.

## Model tasks and verification

`config/local-agent/b8-small-functions.json` is a trusted task queue authored by
the decompilation maintainer. It selects an existing source file, exact original
debug-symbol name, unchanged function signature and relevant type/behavior
context. The first four tasks are ProjectZ, ByteGrinder op8, Bitmap SetPixelIndex
and YUV444ToRGB. These are known difficult residual register-order cases, so this
queue tests the workflow rather than predicting the average success rate.

Each request includes both full small-function disassemblies, strict objdiff
differences, current C++, the actual per-unit compiler flags and the last few
failed proposals. The model can return only a JSON body and explanation. It
receives a different reviewed variation strategy on each attempt (block-start
declarations, temporary reuse, shorter lifetimes, integer operand forms, etc.).
Temperature rises modestly across attempts; compiled duplicate detection remains
active. The model's explanations are hypotheses, not compiler measurements.
The initial pilot incorrectly claimed differing assembly was identical on two
tasks; only the verifier determines matching progress.
The model
cannot choose paths, execute commands, edit headers/configuration, change build
flags, emit directives, introduce new helpers or mark code Matching. Host code
never executes a generated game function. Input/output limits and policy checks
reject unsupported proposals before compilation. These are structural filters,
not a proof that the generated C++ preserves semantics.

The source trial checks the full B8 build and its pinned SHA1
`e26b3daf41886f0d09670135910f2510cd093ae8`, target improvement, all original
function/data comparisons, unpaired helper additions and global nonregression.
Every model trial uses review-only recovery: sources, configuration and executable
are restored even when the proposal improves the comparison. Such an improvement
is a **verified proposal**, not retained decompilation progress. Code in an
unlinked unit can change without affecting the executable; therefore DOL equality
alone cannot validate its semantics. A maintainer/Codex must inspect the source
and original behavior before adopting it through the same source trial.

Results and full evidence: `build/local-agent/latest.json`, `pipeline-latest.json`,
`history.json`, `worker.stdout.log`, `worker.stderr.log`, and timestamped `runs/`.
Each attempt records local prompt/output tokens, measured generation speed,
target comparison, potential matched/linked gains, source hashes and DOL hash.
Do not count potential gains as retained gains. Verified manifest paths are in
`verified_proposals`; after semantic review, apply one with:

```powershell
python tools/decomp_source_trial.py "build/local-agent/runs/<run>/<attempt>/manifest.json" --version SZBE69_B8
```

If interrupted native work leaves `build/decomp/SZBE69_B8/pending.json`, stop
other native work and run `python tools/decomp_runner.py recover --version SZBE69_B8`.
Recovery refuses to overwrite sources modified outside its journal; inspect
that condition before proceeding. Failure does not authorize discarding changes.

History is keyed by task, source/config/reference snapshots, prompt policy and
installed model digest. Re-running an exhausted queue does no new inference.
To explore further deliberately, increase `-AttemptsPerTask` and `-Limit`, revise
a task's focused instructions/context, or install/test another local model via
the Python worker's `--model` option. Preserve history instead of deleting it.
After an improved proposal is found, that unchanged task pauses for semantic
review. This avoids spending hours polishing a proposal that is awaiting review.

## Model-free work and deadline

`python tools/local_decomp_search.py run` enumerates 39 valid independent-load
orderings for ProjectZ, with the original arithmetic/dependencies unchanged.
It compiles and compares each locally, caches exhausted searches, and retains
only verified improvements. The initial search found no improvement and preserved
the exact executable. More useful offloading will come from additional reviewed
enumerators for temporary lifetimes, integer commutative forms, declaration
placement, library build-flag grids and stable context caches, rather than asking
a small model to reconstruct large unknown systems unsupervised.

Baseline: 6,324,704 / 11,431,676 code bytes matched (55.3261%); 1,241,840
source-linked (10.8632%); 539 / 1,876 units linked. The last Light session linked
7,780 bytes in 9,410 seconds, about 2,976 bytes/hour. Medium/High observed roughly
2,318/2,718 linked bytes/hour. Candidate mix differs, so these are not controlled
model speed measurements. At the Light rate the remaining 10,189,836 linked-code
bytes represent about 3,423 active hours, versus fewer than 700 wall-clock hours
remaining in October. This is only a linear extrapolation, not a completion
forecast: bulk library recovery could accelerate it, while difficult unknown
code could slow it. A full matching source reconstruction by October 31 is not
a credible current commitment. A native PC port would additionally require
platform replacement work after reconstruction. The local helper should be
judged by reviewed gains per hour and reduced Codex tokens per accepted change.

# Local B8 Ghidra workflow

ChatGPT previously used DTK-generated original PowerPC assembly, B8 symbol/map
context, compiler trials, strict objdiff comparisons, and optional m2c drafts.
Ghidra now adds cached pseudocode and caller/callee references for selected
functions. All analysis and decompilation run locally on the CPU, with no model
or cloud calls. Drafts never edit source or configurations automatically.

Portable tools are installed under ignored `build/references/ghidra`:

- Ghidra 12.1.4, official release package dated 2026-09-21, verified against its
  GitHub asset SHA-256 `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db`.
- GameCube Loader release 1.3.1, its Ghidra 12.1 package, verified SHA-256
  `9892f28fc1e7f19bb3fcec7cfb4ad2ab7504503b571d93de8b142a6f7bec8ed0`.
- Existing JDK 25; no global Java installation or system settings were changed.

The loader supports DOL binaries and the Gekko/Broadway language. See the
[loader project](https://github.com/Cuyler36/Ghidra-GameCube-Loader) and
[official Ghidra release](https://github.com/NationalSecurityAgency/ghidra/releases/tag/Ghidra_12.1.4_build).
For another checkout, extract these public packages into the same ignored paths
and install the extension in `Ghidra/Extensions`; retain their licenses. Original
game inputs must be supplied privately and are never downloaded by this workflow.

Run from the repository root:

```powershell
python tools/ghidra_context.py init
python tools/ghidra_context.py export --symbol CalcNoteWeights__9VocalPartFv --symbol GetNoteSliceWeight__9VocalPartCFffi
```

`init` verifies the exact supported B8 DOL, imports repository function names and
boundaries, disassembles their known bodies, and analyzes once. Automatic map
loading is explicitly disabled because the extension otherwise opens a file
dialog during headless import. The imported B8 executable is private in the
Ghidra project, which stays under ignored `build/decomp/ghidra/SZBE69_B8/project`.
The first successful import created 82,161 labels and 41,253 function boundaries
with zero script warnings; import/analysis took 140.49 seconds. The broader
repository function count includes entries not imported as mapped, sized bodies.

`export` processes the cached project read-only without repeating analysis.
Select exact original names from `config/SZBE69_B8/symbols.txt`. It writes requests,
pseudocode, call references, logs and an index under the ignored `packets/`
directory. Four initial exports completed in 3.44 seconds. Default future launches
use at most four analysis CPUs and a 4 GB Java heap unless the user's Ghidra
environment overrides the heap. The 8 GB GPU is not needed.

The current extension's CodeWarrior demangler raised a compatibility exception
during analysis, although the remaining analysis, save and all four decompilation
exports succeeded. Original mangled names remain available. Ghidra also initially
misinterpreted `_savegpr_26` as returning `this`, inferred some float values as
double, and emitted GQR quantization branches for paired-single save/restore.
The exporter now annotates standard compiler save/restore helpers as register-preserving bookkeeping in a temporary read-only analysis view; a native export test confirms the false helper-return input is removed. Return types and other signatures remain inferred, and GQR noise remains. These are analysis artifacts, not discovered game logic. Assembly and existing
headers must decide calling convention, types, NaN behavior, field layout and
floating-point operation order. Do not copy these artifacts into reconstructed
source or claim a generated draft matches merely because decompilation completed.

Review a draft together with the corresponding original assembly and prepared
objdiff packet, reconstruct meaningful C++, and submit it to
`tools/decomp_session_trial.py` with an active B8 ledger. Strict original-unit
function/data nonregression and exact full executable verification remain required.
New emitted shared helpers require separate ownership/behavior review; incomplete
units remain assembly-linked until exact source layout passes. Trial failures are
restored automatically and recorded instead of silently weakening guards.

This establishes a usable Ghidra-assisted workflow. Its net efficiency advantage
has not yet been measured over enough comparable targets to claim a speedup.

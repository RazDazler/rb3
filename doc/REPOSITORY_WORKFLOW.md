# Local B8 session workflow

User-authorized policy: retain verified progress locally throughout the session,
then commit and push reviewed source, configuration, tools and reports to
`origin/master` (RazDazler/rb3) at the end. Commit messages must explicitly state
that ChatGPT wrote the reconstructed code and automation; retain existing library
licenses and upstream authorship. Never force-push or publish original game inputs.

Before committing, run script tests for changed automation and `build-wii.ps1`
for the B8 target. Require the original executable SHA-1, regenerate objdiff, and
record matched/link gains separately from upstream updates and metadata-only work.
Format non-library C/C++ changes using `.clang-format`; protect multiline inline
assembly with clang-format directives. Review the staged file list and scan for
game binaries, compiler distributions, model files and credentials before pushing.

Private inputs are canonical under ignored `orig/SZBE69_B8`: `sys/main.dol`,
`files/band_r_wii.map` and `files/band_r_wii.sel`. Their hashes were verified against
`debugversion2` before deleting both obsolete dump folders on 2026-10-04 UTC.
Retail originals under `orig/SZBE69` remain reference-only. Historical audit docs
retain the paths used when the dumps were first examined.

Keep Ghidra projects, decompiler packets, original objects/assembly, downloaded
reference sources, models, caches and native tools under ignored `build/`.
Keep the local model source-edit worker stopped unless a later explicit experiment
justifies it. Artifact preparation is finite and does not modify game source.

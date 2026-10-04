# Local decompilation sessions

- Focus on Wii `SZBE69_B8`; retail `SZBE69` is reference-only.
- Use original assembly, B8 symbols/map, existing headers, cached Ghidra context
  and prepared comparison packets. Generated drafts are unreviewed evidence;
  reconstruct meaningful C++ and validate with objdiff before retaining changes.
- Keep the exact B8 executable build. Use transactional source/variant trials,
  protect original-unit function/data scores and global matched/link measures,
  and verify SHA-1 `e26b3daf41886f0d09670135910f2510cd093ae8`. Review new shared
  helpers individually and do not adopt incomplete units just to increase counts.
- The local-model source-edit worker remains stopped unless the user authorizes
  a new experiment. Finite artifact preparation may run without source changes.
- Record each session's baseline, matched/link gains, retained partials, elapsed
  time and observed account usage. Separate upstream gains and setup overhead.
  Never label an unknown model/effort or usage across refills as measured exactly.
- The user authorizes committing and pushing reviewed completed-session work to
  `origin/master` at `https://github.com/RazDazler/rb3`. Useful intermediate
  verified checkpoints may also be pushed to protect long sessions. Preserve
  upstream authorship and state in each new commit message that ChatGPT wrote
  or integrated the changes. Do not force-push.
- Exclude original game inputs, extracted dumps, executables, archives, compiler
  distributions, models and generated analysis artifacts. Keep these in ignored
  `orig/` or `build/`; audit the staged paths before each publication. Canonical
  B8 inputs are in `orig/SZBE69_B8`, not the deleted debug dump folders.
- Format changed non-library C/C++ using `.clang-format`, preserving multiline
  assembly with format directives. Run checks appropriate to changed automation
  and the exact B8 build before publishing. Preserve library licenses.
- See `doc/REPOSITORY_WORKFLOW.md`, `doc/GHIDRA_WORKFLOW.md` and the latest session
  ledger for procedures and remaining issues. `doc/LOCAL_DECOMP_PROGRESS.md` is
  Windows-1252; append ASCII safely rather than rewriting its encoding.

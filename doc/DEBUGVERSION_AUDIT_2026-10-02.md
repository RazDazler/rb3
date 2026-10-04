# Extracted older Wii dump audit

The folder `debugversion` was examined without changing its contents or selecting it as the retail build input.

- DATA/sys/main.dol: 9,335,648 bytes; SHA-1 `af1954096b55a15add2e69d4772683de48faf5c7`.
- This differs from both the current retail executable (`189b001188a2f8ab769e72364440022407fe09d2`) and the supported SZBE69_B8 debug executable (`e26b3daf41886f0d09670135910f2510cd093ae8`). Do not configure this dump as SZBE69_B8.
- DATA/files/band_s_wii.sel has 375 RSO exports. Its exported names are identical to the current retail selector; addresses differ. A module name ending in `.elf` inside the selector is not a full ELF file.
- No loose ELF, map, PDB, SYM, or DBG files were found. The encrypted ARK version 6 index was decoded and structurally parsed through all 9,161 file entries with no trailing bytes. None of its indexed filenames has those symbol-file extensions.
- Debug messages and "Internal Build" strings in a DOL do not establish that a build retains debug symbols. DOL files themselves do not contain a conventional ELF/DWARF symbol table.

Conclusion: no additional full debug-symbol file was found in this extraction. This appears to be another retail build. Its different executable could eventually help cross-version analysis, but it supplies no new selector names and is not immediately interchangeable with our verified build. Files with unusual names or embedded symbol data are not ruled out by a filename audit.

Reproduce the read-only inspection:

```powershell
python tools/game_dump_audit.py debugversion --reference-selector orig/SZBE69/files/band_s_wii.sel --output build/decomp/SZBE69/debugversion-audit.json
```

The dump folder is ignored by Git to keep game data out of source changes. The verified retail input and build configuration remain unchanged.

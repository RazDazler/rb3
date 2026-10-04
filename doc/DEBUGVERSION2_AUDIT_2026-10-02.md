# Supported debug Wii dump confirmed

`debugversion2/DATA/sys/main.dol` is 13,068,128 bytes and has SHA-1 `e26b3daf41886f0d09670135910f2510cd093ae8`, exactly the supported SZBE69_B8 executable recorded in `config/SZBE69_B8/build.sha1`.

The extraction includes `DATA/files/band_r_wii.map` (12,347,692 bytes). This is a CodeWarrior linker map with named function/data addresses, sizes, alignment, library and translation-unit ownership. It is not original source or a full DWARF type/line database. The selector has 396 exported names, with 26 names absent from the retail selector and five retail-only names.

The map audit parsed 87,407 section-layout entries and 76,777 unique names. Only four names were absent from the existing debug symbols configuration: `__sinit_\\vec_cpp`, `_ctors$99`, `_dtors$99`, and `extab` (mostly spelling or section/linker-generated entries). Thus the project already incorporated essentially all of these map names; this does not suddenly add tens of thousands of newly identified functions. The valuable new local input is the matching debug executable, permitting disassembly of named functions and comparison with reconstructed code or the retail binary. Debug addresses must never be applied directly to the retail executable.

The game's archive index was structurally parsed: 9,166 entries, no additional conventional symbol-file extensions. No loose full ELF file was found.

Copied only the executable, selector and map to ignored `orig/SZBE69_B8/sys` and `orig/SZBE69_B8/files`. Generated original debug objects without editing source or debug metadata:

```powershell
python tools/game_dump_audit.py debugversion2 --reference-selector orig/SZBE69/files/band_s_wii.sel --output build/decomp/SZBE69/debugversion2-audit.json
build/tools/dtk.exe dol split --no-update -j 4 config/SZBE69_B8/config.yml build/SZBE69_B8
```

At the user's request, the active compiler configuration is now SZBE69_B8, whose built DOL retains SHA-1 `e26b3daf41886f0d09670135910f2510cd093ae8`. The secondary retail build also retains SHA-1 `189b001188a2f8ab769e72364440022407fe09d2`. Private dump folders and original inputs are ignored by Git. Audit JSON and the map coverage details are under `build/decomp/SZBE69/`.

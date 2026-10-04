# Retail Wii library audit

Evidence comes from bundled source, not identification of the binary's exact upstream releases.

| Library | Declared version / vendor | Files | Configured code bytes | Source-linked bytes | Modified reference files |
|---|---|---:|---:|---:|---:|
| zlib | 1.2.1 | 28 | 24596 | 8308 | 3 |
| Speex | 1.2rc1 | 119 | 38416 | 19668 | 5 |
| LibTomCrypt | 0.70 | 66 | 11076 | 988 | 7 |
| STLport | 5.0.1 | 317 | 0 | 0 | 220 |
| SoundTouch | 1.3.0 | 32 | 0 | 0 | No reference |
| Ogg/Vorbis | Xiph.Org libVorbis I 20030909 | 80 | 63808 | 25672 | 25 |

## Evidence and reuse

- zlib: `H:\Xbox360\Rock Band 3 Decomp\rb3\src\system\zlib\zlib.h` line 40. Integration markers: 0 files.
- Speex: `H:\Xbox360\Rock Band 3 Decomp\rb3\src\system\speex\libspeex\arch.h` line 43. Integration markers: 0 files.
- LibTomCrypt: `H:\Xbox360\Rock Band 3 Decomp\rb3\src\system\synth\tomcrypt\mycrypt.h` line 17. Integration markers: 0 files.
- STLport: `H:\Xbox360\Rock Band 3 Decomp\rb3\src\system\stlport\stl\_stlport_version.h` line 24. Integration markers: 0 files.
- SoundTouch: `H:\Xbox360\Rock Band 3 Decomp\rb3\src\system\synthwii\soundtouch\include\SoundTouch.h` line 82. Integration markers: 1 files.
- Ogg/Vorbis: `H:\Xbox360\Rock Band 3 Decomp\rb3\src\system\oggvorbis\info.c` line 419. Integration markers: 3 files.

Public source can accelerate reconstruction when the historical version, build flags and game changes agree. Compare an exact release in a separate directory before copying any code. Every adopted unit must pass objdiff and the complete retail DOL checksum.

## Limits

- Vendor date strings do not establish an exact release number.
- Local reference copies can contain the same game modifications.
- Relative filenames must align; missing files can reflect directory layout differences.
- Configured metrics cover named library units, not unidentified code in the executable.
- Nintendo SDK, Quazal and other proprietary components require separate provenance work.
- Downloads and wholesale source replacements are not performed by this audit.

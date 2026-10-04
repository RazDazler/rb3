# Historical public library comparison

Official source archives were downloaded into isolated `build/references` folders. Downloaded code was not installed or built with its own scripts. Existing game source remains the baseline for comparisons.

| Source | Official archive | Archive SHA-256 | Bundled files agreeing after CRLF normalization | Modified shared files |
|---|---|---|---:|---:|
| zlib 1.2.1 | [zlib fossils](https://zlib.net/fossils/zlib-1.2.1.tar.gz) | `94ded52040ee9dd1c70cc3b9f01da283803c28c1194a5a40659e8cf7545990e3` | 22 | 6 |
| Speex 1.2rc1 | [Xiph release](https://downloads.xiph.org/releases/speex/speex-1.2rc1.tar.gz) | `342f30dc57bd4a6dad41398365baaa690429660b10d866b7d508e8f1179cb7a6` | 107 | 7 |
| libVorbis 1.0.1 | [Xiph release](https://downloads.xiph.org/releases/vorbis/libvorbis-1.0.1.tar.gz) | `20b3cbdb4b05322d470404a7d2e8cdae1e0ce5372113218ae3cada3b29da70f7` | 40 | 24 |

Vorbis was compared against the archive's `lib` directory because the game combines library and include files differently. Its archive SHA-256 agrees with [Xiph's publisher checksum list](https://downloads.xiph.org/releases/vorbis/SHA256SUMS). The zlib and Speex hashes above were computed locally from official HTTPS downloads. Evidence files are under `build/decomp/SZBE69/upstream-audit` and each reference's `download.json`.

The many identical files show why wholesale downloads have limited additional benefit: this project already carries substantial historical public source. Speex `vbr.c` is identical to the 1.2rc1 release after normalizing line endings. Its remaining mismatch therefore requires compiler settings or reconstruction of game-specific build choices, not a fresh copy of the same file. Vorbis `info.c` matches the 1.0.1 release except for include paths; this strengthens source-release identification without proving every binary component uses that exact release.

One targeted upstream change paid off immediately: zlib `deflateInit2_` used a literal version byte instead of the upstream `my_version[0]` expression. Restoring the upstream expression changed its strict objdiff result from 96.61017% to 100%, added 708 matched code bytes and one matched function, and preserved the full retail DOL SHA-1. The runner rejected the additional copyright-data variant because it did not improve the target further.

Speex `bits.c` initially reported 100% code matching but failed all four source-link flag trials because `speex_bits_reset` was not exported. A retail-only external inline declaration fixed that error. The subsequent checksum failure identified `speex_bits_insert_terminator` in the wrong location. Restoring a retail-only external declaration and its upstream definition order resolved the layout mismatch. The complete 1,304-byte unit is now source-linked and the full retail DOL matches exactly. Debug declarations/order remain unchanged. Logs remain in the persistent trial history.

Public library copyright/license notices remain in their downloaded source archives and bundled files. SoundTouch, STLport and LibTomCrypt already have source in the repository; their version declarations and local-reference comparisons are in the library audit. No proprietary SDK or middleware download was attempted.

Further retail verification source-linked the complete zlib deflate unit (7,444 bytes) by recovering its upstream function order, and LibTomCrypt crypt.c (272 bytes) by restoring its build-settings text and correct retail globals. Speex nb_encode's loop condition and historical assertion-line offset were corrected, improving its strict comparison from 97.96049% to 98.21542%; it remains unfinished. Two stack-spilled rodata address relocations were supplied to DTK explicitly from their original high/low instruction pair. All accepted retail changes preserved the full executable checksum.

Cached reference trees were subsequently revalidated against every regular file in their respective archives. No modified, missing or extra files were found. Comparison counts in the table describe the initial audit; the current bundled source may have additional deliberate matching changes.

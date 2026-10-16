# Cold-start credits: 33-call source and interval census

This address-only audit checks the complete U17 printer-call run as one static interval. The [first caller](NATIVE_COLD_START_CREDITS_FIRST_PRINTER_SOURCE_GATE.md), [thirty-second caller](NATIVE_COLD_START_CREDITS_THIRTYSECOND_PRINTER_SOURCE_GATE.md), [credits source contract](COLD_START_CREDITS_SOURCE_CONTRACT.md), and [terminal wait gate](NATIVE_COLD_START_CREDITS_TERMINAL_WAIT_SOURCE_GATE.md) supplied bounds; the counts and hashes below were independently recomputed from ignored DOS v1.3 originals and retained U17-entry RAM. A bounded `deja` lookup timed out without an indexed result. No emulator was started under the host's heavy-native hold. This census is separate from any individual caller translation or live display trace.

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17-entry RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

U17 is loaded at segment `3909`. Its complete printer-call interval is `+0258..+0615`, original GAME.OVR `[0x0032C,0x006E9)`, exactly 957 bytes (`33 × 29`). The original span SHA-256 is `11033337def20b92005c394f474a839512f651350e12df9fcfbec1fb450c77d6`; loaded RAM SHA-256 is `f1e1c9d1b43dbe78f79a8f388084d92baa35e4e5d21a07b26317da9dbb46a518`. Partitioning at `+0258 + 29n` for `n = 0..32` produces 33 full records with no gap or overlap. The first ends at `+0275`; the last begins at `+05F8` and ends at `+0615`, where the separately audited terminal wait starts. There is no thirty-fourth printer caller.

Every original 29-byte record decodes to the same instruction layout: three `MOV AL`/`PUSH AX` operand pairs, `LEA DI` from a BP-local destination followed by its segment and offset pushes, `MOV DI` for a CS Pascal-source pointer followed by its segment and offset pushes, then far calls to SYSTEM `0AF8:0B0B` and resident printer `0709:0626`. The SYSTEM call starts 19 bytes into each record; the printer call starts 24 bytes in. Each loaded record differs from its original at only relative offsets `0x16,0x17,0x1B,0x1C`, the two far-call segment words: 132 changed bytes across all 33 records, zero elsewhere. This identifies 33 static SYSTEM calls and 33 static printer calls; it does not assert that a native run reached them.

The 33 caller operands reference 29 distinct Pascal source addresses. All 29 original records are contiguous in U17 `CS:+004B..+0206`, original GAME.OVR `[0x0011F,0x002DA)`, 443 bytes, SHA-256 `b4905f882b908d6a8a22b883b953df6959ca84cf7a887f07904927c915f8b66c`; that entire bank is identical in retained RAM. The 29 length prefixes plus 414 unique data bytes account for all 443 bytes. Four source addresses are each used twice:

| U17 source offset | Pascal data bytes | Caller uses |
| --- | ---: | ---: |
| `+00CA` | 12 | 2 |
| `+00ED` | 10 | 2 |
| `+0104` | 10 | 2 |
| `+0167` | 13 | 2 |

Those four repeated sources add 45 characters to the 414 unique characters, yielding 459 characters across the 33 calls. For every caller, the BP-local destination size equals its Pascal length plus one, and the source record bytes match between original and RAM. The numeric cell footprints from the caller row, column, and Pascal length are pairwise disjoint: 459 distinct cells, columns starting from 1 through 26, rows 1 through 23, colors 10, 11, and 14. Every footprint remains within 40 columns and 25 rows. This is static source placement, not a captured per-cell native write.

The [source-to-frame contract](COLD_START_CREDITS_SOURCE_CONTRACT.md) separately compares predicted credits pixels with a retained native frame. This census verifies only original/loaded code and source-bank structure; it does not prove a first live writer, EGA page, per-call glyph branch, C execution, full-frame route parity, or a game-function count.

# Unit 0x9E public entry 0: far-return leaf at +0x1932

`src/recovered/overlay_u9e_entry0_1932.c` models the complete five-byte
DOS v1.3 `GAME.OVR` U9E public entry 0, `+0x1932..+0x1936`. The body is
`PUSH BP; MOV BP,SP; POP BP; RETF`; it has no outgoing calls. The ledger
records no incoming call sites, so the game-routine identity and strict
full-C count remain **provisional**. No saved-game caller frame was reached.

The pre-implementation `deja "poolrad U9E entry0 +1932 original bytes parity
decomp"` query timed out (exit 124), so it supplied no reusable claim. The
full-state original-byte audit and synthetic native capture structure reuse
the reviewed U3B0 entry1 approach in
[U3B0_ENTRY1_0000.md](U3B0_ENTRY1_0000.md), but all results below are
measured for these five original bytes.

| Item | Evidence |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Entry | U9E slot 0; runtime stub segment `0xB0`; file `[0x2C81C,0x2C821)`; unit `[+0x1932,+0x1937)` |
| Body | 5 bytes / 4 instructions; `55 89 E5 5D CB`; SHA-256 `fc1e97b8fe4158fdf9a46de4d3a1b4a04602fb787f6793778bc6d2b7a758d780` |
| Exit | `RETF`, with no arguments and no calls |

The model receives full 1 MiB RAM and the 14 relevant 16-bit CPU words. It
preserves the transient `PUSH BP` write, restores BP, pops the far return,
and advances SP by four. A 16-bit SP wrap between the two return words
uses the wrapped offset for the second word, as measured by DOSBox-X's
normal 386 core. Each word's high byte still uses contiguous physical
memory, including at `SS:FFFF`.

| Field | Verdict |
| --- | --- |
| `target_function` | Original U9E public entry 0 `+0x1932..+0x1936` |
| `source_candidates` | `pr_u9e_entry0_1932` in `src/recovered/overlay_u9e_entry0_1932.c` |
| `signals` | Exact file/body hashes, four-instruction bound, no calls, `PUSH/MOV/POP/RETF` state and stack writes |
| `score` | 60/60 original-byte Unicorn cases match full 1 MiB RAM and all 14 CPU words away from return-SP wrap; 2/2 DOSBox-X synthetic wrap frames match all 14 CPU words, 16 stack bytes, and eight adjacent physical bytes |
| `verdict` | `match` for this bounded body on the tested native semantics; `unclear` for game-routine identity |
| `review_notes` | Unicorn 2.1.4 reads the second `RETF` word linearly at `SP=FFFE/FFFF`, while DOSBox-X normal 386 reads it using wrapped SP. The C model follows the native measurement. No natural caller or strict count is claimed. |

`tests/recovered/check_overlay_u9e_entry0_1932.py` runs the original bytes
against the C model over 60 synthetic non-wrap frames, comparing all RAM
and CPU words. `tools/overlay/u9e_entry0_1932_native.py` copies those
same five bytes into a separate DOS segment at offset `0x1932`, enters
two boundary frames, and compares native output with C. The ignored report
is `resources/u9e-1932-parity/native/report.json`; all generated COM and
capture files stay under ignored `resources/`. The DOSBox-X executable
used for the native frames has SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`.
The focused C11
`-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest covers ordinary and
wrapped returns.

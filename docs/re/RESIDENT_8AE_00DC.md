# Resident 0x8AE:0x00DC dispatch

This note describes only addresses, counts, hashes, and decoded control flow
from the owner's ignored DOS v1.3 inputs. It does not establish live saved
one-Up execution or a game-specific identity for this resident target.

## Prior work and reproducibility

Before decoding, `deja "Poolrad resident 8AE:00DC START.EXE coordinate writer
gameplay loop"` found no matching prior session; `deja "8AE:00DC"` returned
the earlier gameplay callsite trace. Reused pointers:
[U68 coordinate writer](U68_COORDINATE_WRITER_073B.md),
[U68 entry 1](U68_ENTRY1.md), [gameplay path](GAMEPLAY_PATH.md), and the
[resident inventory](RESIDENT_FUNCTION_INVENTORY.md). The caller ABI and
procedure boundary below were rechecked independently against the hash-locked
DOS image.

Input SHA-256: `START.EXE
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The existing `tools/overlay/resident_loader_map.py` expands the START loader
and records copied-byte provenance; `resident_inventory.py` checks overlay
far-call fixups. These scripts reject other input hashes.

| Procedure | Loaded image | START file | Extent | Instructions | SHA-256 |
| --- | --- | --- | ---: | ---: | --- |
| `RES_08AE:00DC` | `0x08BBC..0x08C5A` | `0x08BD3..0x08C71` | 159 bytes | 77 | `aae1673000ebd85e67c66df978d2b986de8e4d52cdd7994ade46f1ac5d50bb2a` |
| local `+181A` | `0x0A2FA..0x0A30F` | `0x09FF7..0x0A00C` | 22 bytes | 12 | `4b13f9458c5b74c2a0dde8cd81bfd304d197c7712bf282efcf6cf4e38b88b79a` |

Both spans decode contiguously in x86 16-bit mode, have consecutive source
provenance, and use copied rather than loader-filled bytes. The target has 13
near calls and 13 conditional or unconditional jumps. Its final instruction
is `RETF 2` at `+0178`, ending at `+017A`. A separate Pascal prologue
begins at `+017B`; the older inventory's `+00DC..+01A5` interval therefore
contains two procedures. The loader map reports 19 fixup-confirmed overlay
calls, four relocated resident far-call sites, and an exact prologue. The
19 overlay sites distribute across units `0x01C` (3), `0x05E` (6),
`0x068` (4), `0x086` (4), and `0x3C6` (2).

## Caller and body contract

The far procedure consumes one full WORD at `[BP+6]` and returns with
`RETF 2`. Unit `0x068:+0742` passes DS word `[0x2628]`; its entry 1
alternate site `+1037` passes `[0x2622]`. Unit `0x01C:+34D3` is another
gameplay route. The caller-side observations do not by themselves prove which
branch ran in the restored one-Up execution. The body itself has no direct
nonstack store. It reads byte `[0x5372]` at `+00DF` and again at `+0137`,
and word `[0x5374]` at `+00E9` and `+0156`. Callee side effects can change
the later reads.

| Gate | Effect |
| --- | --- |
| `+00DF`: byte `[0x5372] == 2` | Return immediately; no calls. |
| `+00E9`: word `[0x5374] > 0` | Call `+137C` at `+00F1` before argument dispatch. |
| Argument `0` | Call `+17AE(0)` at `+0100`, `+1742(0)` at `+0107`, then `+181A(0)` at `+010E`. |
| Argument `1` | Call `+181A(1)` at `+011D`. |
| Argument `0x00FF` | Call `+17AE(0)` at `+012B`, then `+1742(0)` at `+0132`. |
| Other argument; reread mode `1` | Call `+1742(argument - 1)` at `+0144`, WORD arithmetic. |
| Other argument; reread mode `0` | Call `+17AE(argument - 1)` at `+0153`. Other modes make no call here. |
| `+0156`: reread word `[0x5374] > 0` | Call `+006C(10)` at `+0162`, `+17AE(0)` at `+0169`, `+1742(0)` at `+0170`, and `+13B1()` at `+0174`. |

The `+181A` leaf sets DS to `0x8AE`, stores the low argument byte at
`DS:+020D` at `+1827`, restores DS, and returns with `RETF 2`.
The C model merges this exact leaf effect. The other five target offsets
remain fallible host calls. `+137C`/`+13B1` reach DOS vector and PIT/speaker
I/O; a timer or sound role is plausible but remains an inference. Their
complete external effects and the behavior of `+006C`, `+1742`, and
`+17AE` are outside this model. Missing hooks stop only if reached, after
prior effects. No return-register result is claimed.

## Verification

`test_resident_8ae_dispatch.c` covers mode-2 bypass; argument 0, 1,
`0x00FF`, and `0x0100`; mode 0/1/other; active-count call order; state
changes by earlier calls; and missing/failing reached hooks. The model is a
**bounded resident dispatch model**, not a full translation or strict C
credit. The inventory's unresolved game/library classification remains.

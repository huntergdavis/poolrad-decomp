# OVL_03CE_00F1: tile placement through 08D4 (raw CPU, composed)

`pr_ovl03ce_00f1_cpu` is a raw-CPU, full-state model of GAME.OVR unit `3CEh` entry 2, `+00F1`. It places one tile in the
off-screen object `DS:4A20` through `0464:08D4`, the tile compositor, and composes `RES_0464_08D4`'s merged model.

**Claim: evidence and readiness only.** The row is `game_or_other_library_unresolved`. Its own bytes read only the two
tile-set pointers and the object pointer and forward to `08D4`, which is not a game-state use. No identity is argued here
(PM i22129). `RES_0464_08D4` itself is **uncredited**: it is `unresolved` in the resident ledger, and its evidence is
PR #1413 ([RES_0464_COMPOSE_08D4_CPU.md](RES_0464_COMPOSE_08D4_CPU.md)). The credit target that uses this model is
[OVL_03B6_0000](OVL_03B6_0000_CPU.md).

## Exact original

| Item | Value |
| --- | --- |
| Span | unit `3CEh` `+[00F1,014E)`, 93 bytes, 38 instructions, `RETF 8` |
| SHA-256 | `666d36b53e2f606c9aefc039a0afff1b0af2e518e2c5313bd738f59194c5bcdd` |
| Unit | 1,605 code bytes, entries `640, 22, F1, 14E, 1C8, 4FC`; stub `03E9`, entry 2 at `03E9:002A` |
| Far calls | `LCALL 0464:08D4` at `011D` and `0145`; segment words `CS:0120` and `CS:0148` are loader fixups |

Inputs are the pinned GAME.OVR (SHA-256 `872964a3…`) and START.EXE (`d825df2b…`). `emit_ovl03ce_00f1.py` generates the
C from the bytes with `emit_raw_cpu_c.py`, and `--check` regenerates it byte for byte.

## Behaviour

The arguments are `[BP+6]` (a word that is never read), `[BP+8]` the tile byte, `[BP+0Ah]` and `[BP+0Ch]` two
coordinate bytes.

- **Tile above `7Fh`** (unsigned byte compare, `JBE` at `00F8`): the tile set is the far pointer `DS:5E56/5E58`, and the
  tile is `[BP+8] AND 7Fh`.
- **Tile `00h..7Fh`:** the tile set is `DS:5E52/5E54`, and the tile is unchanged.

Either way the `08D4` frame is: object `DS:4A20/4A22`, the tile set, flag `0` in AL, the tile, then sign-extended
`[BP+0Ah]` and `[BP+0Ch]`. AH of each byte word is whatever the preceding instruction left, so the high bytes are pushed
as the CPU has them.

**Statuses.** `OK`, `ARGUMENT` (null pointers, RAM not 1 MiB, IP not `00F1h`; nothing changed), `CALLEE` (`08D4`
stopped; the diagnostic gives the site and `08D4`'s status) and `TRANSFER`. There is no backward jump, so there is no
`LIMIT` of its own. No instruction leaves a flag undefined.

## Validation

The audit is shared with `OVL_03B6_0000`: `tools/overlay/ovl03b6_0000_cpu_audit.py`, with
`CHECKER_ROWS = ('OVL_03CE_00F1', 'OVL_03B6_0000')`. See [OVL_03B6_0000_CPU.md](OVL_03B6_0000_CPU.md) for the fixture,
the case list, the native run and the mutation check. The figures for this body:

- **Unicorn:** 16 direct-entry cases, plus 1,875 composed calls from the 75 returning 03B6 cases (25 each). Starts 38/38; branch outcomes 2/2.
  CPU14 and the whole 1 MiB are compared, with no mask.
- **Focused C test:** `test_overlay_u3b6_window_0000_cpu.c` checks both tile-set paths through both call sites. It
  reads the `08D4` frame left below SP and the return, plus `ARGUMENT` for an IP other than `00F1h`.

```sh
python3 tools/overlay/emit_ovl03ce_00f1.py --check
```

# Resident palette setter `0x464:0x2A0D` and its static call gate

Builds on [RESIDENT_VIDEO_INIT_4426.md](RESIDENT_VIDEO_INIT_4426.md). `deja "palette setter 0x464 2A0D first view call gate"` found only this session's task message. Text and numeric evidence only; no screenshot.

## The routine (full return)

START.EXE (65,231 bytes, SHA-256 `d825df2b...a07bd`) unpacked image `0x0704D..0x07089`, **61 bytes through `retf 4` (`CA 04 00`)**, SHA-256 `9a5d907528d68e38983cc795b891dfbc5bb20aa0f8053494f47e553edbfff0a8`.
Two words pushed, low bytes read: `[bp+8]` (pushed first) = palette register, `[bp+6]` = colour value.

| Step | Effect |
| --- | --- |
| `+0x2A10`, `+0x2A15` | `[0x4A03] := 0x10` (AH), `[0x4A02] := 0` (AL) |
| `+0x2A1A..+0x2A30` | value < 8: `[0x4A05] := value`; else `[0x4A05] := (value + 8) & 0xFF` (BH) |
| `+0x2A33` | `[0x4A04] := [bp+8]` (BL) |
| `+0x2A39..+0x2A41` | `push 0x10; push ds:0x4A02; lcall 0xA32:0x000B` (`Intr(0x10, Registers)`) |
| `+0x2A46` | `pop bp; retf 4` |

That is the whole routine: it builds BIOS `INT 10h` AX = 0x1000 (set one palette register; BL = register, BH = value, a BIOS contract rather than recovered code). The one call is the same explicit boundary as `pr_vi_hooks` in the merged video-init model.

## C helper

`pr_res_set_palette` (`resident_palette_2a0d.{h,c}`): full translation with that one hook, focused test (`tests/recovered/test_resident_palette_2a0d.c`: every value 0..255 over 52 register values, the 8-bit wrap, untouched record bytes and DS words, hook-visible record, stops). Library-like support: no game-function credit.

## Callers (ten fixup-backed overlay sites)

| Unit (stub para) | Sites | (register, value) -> (BL, BH) in order |
| --- | --- | --- |
| `0x17` entry 1 (`+0x0627..+0x0808`, 482 bytes, SHA-256 `02adf3ddfc09a9cf558fe9735f53e78858bb4ec57471672d33be6082677f137d`) | `+0x076B, +0x077F, +0x0793, +0x07A7, +0x07BB, +0x07CF` | (1,9)->(1,17); (1,11)->(1,19); (1,15)->(1,23); (1,11)->(1,19); (1,9)->(1,17); **(1,1)->(1,1)** |
| `0x3C6` entry 2 (`+0x00E3`, gated by `[0x4A1E] = 1`) | `+0x00F3, +0x00FE` | (0,8)->(0,16); (8,0)->(8,0) |
| `0x3C6` entry 3 (`+0x0105`, same gate) | `+0x0115, +0x0120` | (0,0)->(0,0); (8,8)->(8,16) |

Unit `0x17` entry 1 is a picture screen: it loads and shows a name through `0x464:0x111`/`0x464:0x1951`, then loops until a key is pressed (`0xA40:0x2FD`) or a tick count elapses (`0x709:0x0C3F`), flickering palette register 1 through
the five values above with `0xA40:0x029C` delays; the loop body is `+0x070E..+0x07DB` (206 bytes, SHA-256 `30389c623a05d5af02d93f7358601c48e77b43c5446bed875e05c6d3ce8d282b`), entered only when `[0x4A1E] = 1` (`+0x070E`)
and a random-delay accumulator passes `0xC350`. **Every iteration ends with register 1 := 1** (the last site, the straight-line sequence has no exit between the five calls), so the flicker leaves the palette register at its default; the exit tests are after the sequence (`+0x07D7..+0x07F9`).
Callers of the unit `0x3C6` entries are in units `0x2F` (`+0x64`), `0x6C` (`+0x385A`, `+0x3F28`) and `0x77` (`+0x0D10`, `+0x0AC1`); their reachability from the slot-A route is not established.

## Static call gate on a plain launch

The resident main body (image `0x0F6..`) calls unit `0x17` entry 1 (`lcall 0x19:0x25`, image `0x11D`) as follows (image `0x0FD..0x126`, 42 bytes, SHA-256 `962f5862d51183a237def6639d88fc51f68d3a71f5310554ddd9553ce54c83fe`): after one overlay call, `0xAF8:0x16BC(dest, 1)`
(the runtime's `ParamStr`-shaped routine: index argument, DOS-version/environment path for index 0) is compared by `0xAF8:0x0C10` with the Pascal string at `cs:0` (length 5, hash prefix `8f64161fb6a04660`; text kept out of the repository);
**equal skips** the two calls at image `0x11D` and `0x122` (unit `0x17` entry 1, then `lcall 0x1C:0x25`). The oracle launcher starts `START` with no argument (`tools/oracle/launch.sh`), so, if `0x16BC` is the command-line argument as its shape suggests,
the argument is empty, differs from the constant, and the picture screen with the flicker is reached before any menu. This is a static gate only.

## What is and is not known

* **Execution in the first-view window is unknown**: no native execute watch was run for this note (the private DOSBox-X setup needs an interactive debugger session, not done here). The merged native RAM images show the register record at AX `0x0500` at `+196F`, `+3382`, `+3387` (consistent with, not proof of, no palette call in that window; see the earlier note).
* The intro flicker, if it runs, restores register 1 each iteration and runs before the loader and first view; the unit `0x3C6` swaps are gated by `[4A1E] = 1` (consistent natively) with unknown reachability.
* BIOS effects of `INT 10h` are not modelled; no pixel or palette-value claim for the visible frame.

No strict full-C, count, identity, DOS-native or one-Up parity claim.

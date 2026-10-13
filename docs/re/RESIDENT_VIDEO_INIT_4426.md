# Resident startup video-mode selection and the display state it leaves

One bounded display-state transition on the saved slot-A first-view route: the resident program selects the BIOS video mode and the two video page segments before any
overlay runs. For the owner's EGA run this is **mode 0x0D (320x200, 16 colours), page 0 shown and drawn at segment 0xA000**. It fixes the geometry and memory plane of the visible
initial screen; it does not draw any pixel. `deja "first view palette display state slot-A 320x200 initial screen"` found only this session's own task message.

## Pinned inputs

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` (unpacked image by `resident_loader_map.unpack`) | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| image `0x04426..0x044B5` (selection slice) | 144 | `2440fd4b5d83a6f17e4fa0b430ade9577c13dee48c4419aa0cf63c998e960dc5` |
| `0x0464:0x00CF..0x00FC` SetVideoMode (image `0x0470F`) | 46 | `00b4eeb04b3d56df415fd30afc06459e274781988b95d51d458bcbacf4df6bc1` |
| `0x0709:0x0071..0x009D` SetActivePage (image `0x07101`) | 45 | `2b82498089cf568b357c15015858e6560186bfabf18e16471aada53a1dee04b2` |
| `0x0709:0x00A0..0x00B4` SetDrawPage (image `0x07130`) | 21 | `73b8762504f0c61eb4fe3426a7281330e44312937a36515344321b28801dfc61` |
| native RAM at `2A8E:196F` (DS = 0x14A0) | 655,360 | `3e46c01d37b393bd88b558584f72f8f93b8ab545a6a607fdddbb738eb0a45844` |
| native RAM at `3582:3382` | 655,360 | `31424ea33ec4abce1967fff7032be17b95d4b70b4cfab0d9624c29a54e3072c3` |
| native RAM at `3582:3387` | 655,360 | `d5cc4a1f61561bbb19766616f3addfa100fb63284bc248854ce13079d678f7a0` |

The three RAM images are the ones hash-pinned in [SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md](SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md). This note decodes code and reads bytes of those images; it adds no native run.

## Decoded sequence

Static fact (decoded code) and native fact (endpoint values) are kept apart below: the code description is exact; the native comparison only shows that the model's endpoint equals the three snapshots.

The Turbo Pascal `Registers` record sits at `DS:0x4A02` (AX BX CX DX BP SI DI DS ES Flags); `0xA32:0x000B` is `Intr(intno, regs)` (loads the registers, calls the vector at `[intno*4]`, stores them back).

| Step | Effect |
| --- | --- |
| `+0x4426..+0x4441` | AH := 0x0F, `Intr(0x10)`; `[4A1C] := AL`; `[4A1D] := [4A1C]` (the DOS mode at startup becomes both previous and current) |
| `+0x4444..+0x4449` | compare adapter byte `[0x484C]` with `0x45` |
| `[484C] = 0x45` | `[4A1E] := 1`; `SetVideoMode(0x0D)`; `[4A1B] := [4A1A] := 0`; `SetActivePage([4A1B])` (AH := 5, AL := page, `Intr(0x10)`, `[4A16] := 0xA000 + (page << 9)`); `SetDrawPage([4A1A])` (`[4A18] := 0xA000 + (page << 9)`) |
| `[484C] = 0x54` | `[4A1E] := 2`; `[440A] -= 0x400` through the scratch words `4848/484A`; `SetVideoMode(9)` |
| any other | `[4A1E] := 0`; `SetVideoMode(4)` |

`SetVideoMode` is `[4A1C] := [4A1D]; AH := 0; AL := mode; Intr(0x10); [4A1D] := mode`.

## Native check (numbers only)

At all three stops `DS:4A1E = 1`, `DS:4A1D = 0x0D`, `DS:4A1C = 3`, `DS:4A1A = DS:4A1B = 0`, `DS:4A16 = DS:4A18 = 0xA000`, and the register record's AX is `0x0500` (AH = 5, AL = 0: consistent with the page-select call
being the most recent writer of this register record; the snapshots do not prove it) with BX..ES zero. `pr_vi_select_mode` run on adapter letter `0x45` with a fabricated BIOS whose mode query answers 3 reproduces **all 23 compared values with zero mismatches at each of the three stops** (an endpoint match: the snapshots do not show the order or number of intervening calls)
(`tests/replay/check_first_view_display_state.sh DIR` verifies the image hashes and runs `tools/replay/first_view_display_state.c`). The register record and `4A1C` are the same in the `+3382` and `+3387` images; that endpoint equality does **not** show that no `Intr` call happened inside the first view refresh (a same-value call, or a palette call followed by a page select, would leave the same endpoint). The previous mode 3 and every BIOS side effect of the calls (the hardware mode change, the default EGA palette) are host/BIOS inputs, not recovered code.

## Palette negative result

The palette setter `0x0464:0x2A0D` (AH := 0x10, AL := 0, BL := register, BH := value with values >= 8 shifted by 8; `retf 4`) has ten fixup-backed overlay callers: unit `0x17` file `0x083F..0x08A3` (six calls) and unit `0x3C6` entries 2 and 3 (`+0x00E3`
and `+0x0105`, file `0x3082D..0x3085A`, four calls). The two unit-`0x3C6` routines are gated by `[0x4A1E] = 1` (true here) and set palette register 0 to value 0x10 and 8 to 0 (entry 2) or 0 to 0 and 8 to 0x10 (entry 3); their callers are in units
`0x2F`, `0x6C` and `0x77`. The register record holds AH = 5 (not 0x10) at every stop from `+196F` through the first view return, so **no palette call is evidenced in that window**. This is weaker than proof: only the three snapshots are known, and a palette call followed by another page select (AH = 5, AL = 0) before the next stop would leave the same record. The window's visible screen is therefore only
*consistent with* the palette the BIOS installs for mode 0x0D (and with the standard 16-colour EGA palette the existing private frame comparison found). Whether a palette call runs earlier or later on the route is not established.

## Open items

* `[0x484C]` is `0x46` in all three images but the slice compares it with `0x45`/`0x54` when it runs; the value is read from a startup text-file read at image `0x043D4..0x0441E` and rewritten at image `0x0456C`/`0x04617`. `[4A1E] = 1` in the images is *consistent with* the `0x45` branch having run; it is not proof of it. A byte scan of the unpacked resident image and of `GAME.OVR` finds exactly three direct-address stores to `DS:4A1E`, all in the decoded slice (values 1, 2, 0 at `+0x444B`, `+0x447A`, `+0x44A9`); every other `DS:4A1E` operand is a load or a compare. Indirect writers (pointer or block-move stores) are not excluded by such a scan, and the later `0x46` in `[0x484C]` is not explained here.
* The code after `+0x44B6` and the hardware effect of `INT 10h` are not modelled. The visible pixels, tile selection and fonts belong to other lanes.

## Claims

Full translations of the three callees with one explicit BIOS hook, a bounded model of the 144-byte selection slice, synthetic tests (`tests/recovered/test_resident_video_init_4426.c`) and a hash-pinned private native comparison. No strict full-C, count, identity or one-Up parity claim; not a full-routine proof.

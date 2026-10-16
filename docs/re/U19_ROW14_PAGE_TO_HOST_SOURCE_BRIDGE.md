# U19 row-14 page-to-host source bridge

This source-only follow-on connects the merged
[guest page-return boundary](COLD_START_CODEWORD_ROW14_PAGE_RETURN_RGB_HANDOFF.md)
to the host display-start and EGA line source. It extends the prior
[host RGB source chain](U19_ROW14_HOST_RGB_SOURCE_CHAIN.md) at its page
input. Heavy native remained held. No page-helper execution in the saved
U19 interval, page flip, EGA plane change, host presentation time, or
full-frame parity is established.

Before this work, bounded `deja "Poolrad U19 row14 selected store same-run
capture host RGB page return source join"` found no indexed matching
session. The merged guest page-selector audit supplied RES_0709 `+0071`
and its relocated runtime Intr(10h) call at `+0087`; the merged host chain
supplied the EGA decoder and renderer-cache windows. This audit pins the
intervening ignored DOSBox-X source at revision
`5a9c7c53481c2e933948039c33256776653f4faf`, using the heavy executable
SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

## Conditional page-to-line path

The source windows below are exact in the pinned private checkout. The
audit also locks the complete four source files by SHA-256.

| Source lines | Bytes | SHA-256 | Source consequence |
| --- | ---: | --- | --- |
| `int10.cpp:143–170` | 780 | `090e5fd80c31a0ccd0112e45faff0bfc620a6ffaa7b62d8ccd65a737069f1b19` | INT 10h AH=05 dispatches to active-page service outside its Tandy-specific branch. |
| `int10_char.cpp:1184–1213` | 1097 | `17d105217bf5a6d88f4416a3d38b8f22ac61c8d0bcf564fb4874040c0ad1906e` | Page × BIOS page size sets BIOS current start/page; CRTC 0C/0D receive its start address. The EGA/VGA shift depends on mode. |
| `vga_crtc.cpp:247–259` | 687 | `0f6f4cd6fb0781a3f7fd86776596dfcd8b809e0c03b22e8b7a918f3250b33274` | CRTC writes set `display_start` and mark a redraw for the next frame. |
| `vga_draw.cpp:6123–6156` | 1547 | `0e74faacbb721cf65ff7a4534f6d7a32a837aadbe7fa50a971785440ad231fb8` | EGA schedules display-start latching at vertical retrace end; other machine types differ. |
| `vga_draw.cpp:4119–4148` | 1103 | `bffc6f2bee77aff833ea09a015384a2700ae3f5b266a3673de1f761e773c4000` | The latch copies masked `display_start` into `real_start`, subject to display overrides. |
| `vga_draw.cpp:6259–6290` | 1151 | `888d758087aa016fa34a920fd9913b6393723e920c22478c128ad5f3e3e9b9a9` | Frame start derives draw address from `real_start`; the EGA branch applies byte skip, four-plane scaling, address shift and mask. |
| `vga_draw.cpp:3956–4018` | 2309 | `8f26cd690da5ad9853d53d9ed06dc81382efd428c7e4b7242e7ad75b44974438` | EGA line drawing passes the selected address to its decoder and then to `RENDER_DrawLine`. |
| `vga_draw.cpp:6814–6830` | 403 | `0a2692162240ae0cd1e7d7fca508fcf7f76102a4ac0a51567b5a34675b4a9e4c` | An active debugger redraw can itself force a display-start latch and timer refresh. |

If the saved helper's runtime call really enters INT 10h AH=05 during U19,
and the host uses this EGA branch, the request can change the CRTC
`display_start` before `real_start` changes at retrace. The EGA line then
uses a mode-dependent address derived from that latched start. The prior
host-chain audit pins planar decode, palette translation, render-cache
suppression and eventual GFX update. A CRTC write also marks redraw even
when the numerical page start is unchanged; that is a conditional source
effect, not proof that a same-value page call occurred in the saved run.

The retained U19 `+02DD/+02E2/+02FD/+0302` RAM records show equal
DS:4A16/4A18=`A000/A000` and page indexes `0/0`. Their row-14 host
mismatch counts are `477/477/477/0`; successive RGB deltas are
`0/0/6612`. They lack a synchronized BIOS frame, CRTC/display-start
state, retrace latch, selected draw address, and four-plane sample. These
endpoints therefore cannot choose among a delayed latch, render-cache
behavior, a prior plane change, or a page request that rewrote the same
value. The debugger-redraw source adds a separate capture perturbation to
check before attributing screenshot timing to game execution.

## Distinct next same-run discriminator

After a separately authorized native slot, tie any reached guest
`+0074/+0087/+008C/+0099/+009C` page-helper frame to its far caller and
U19 stop order. Record actual INT 10h AH/AL and BIOS return, CRTC 0C/0D,
`display_start`, the subsequent EGA retrace latch and `real_start`, the
mode and address-shift bits that select the draw path, the four-plane
`A000:118F` witness, and canonical RGB at U19 `+02D8/+02DD/+02E2/+02FD/+0302`.
Also record whether a debugger redraw occurred. If those host fields
cannot be captured safely in one invocation, report that typed blocker;
equal guest page words or an RGB interval cannot replace them. This B gate
is separate from the selected guest store and its immediate four-plane
before/after pair.

Reproduce the read-only source lock and existing guest audit without an
emulator launch:

```sh
python3 tools/replay/cold_start_codeword_row14_page_rgb_bridge_audit.py \
  --source resources/dosbox-x-16a1-private-src \
  --heavy resources/dosbox-x-heavy-src/src/dosbox-x
python3 tools/replay/cold_start_codeword_row14_page_selector_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

Only addresses, hashes, counts, and conditional source relationships are
public. Original media, host source/binary, RAM, plane and RGB artifacts
remain under ignored `resources/`. This adds no native writer, page/RGB,
C, strict, or full-frame credit.

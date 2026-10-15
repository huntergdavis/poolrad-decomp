# Saved slot-A first-view EGA screen and display-state join

The stable native first-view screenshot is an exact rendering of the same
run's four 8,000-byte A000 EGA planes: **64,000 of 64,000 indexed pixels
match**. Every screenshot pixel is an exact 2×2 block, and all 11 colors
present use the standard 16-color EGA RGB values. A RAM capture at the
same paused screen has mode `0x0D`, active and draw segments `0xA000`,
page zero, and the 14-byte BIOS register record previously modeled by
`RESIDENT_VIDEO_INIT_4426` (23 compared display-state bytes, zero
mismatches). This joins the first-view page to its displayed pixels and
software display metadata; it does not read the hardware attribute-controller
registers or establish every intermediate draw or BIOS palette call.

## Provenance and private inputs

`deja "Poolrad saved slot A first-view EGA palette metadata attribute
controller DAC RGB parity"` was run before this audit and timed out after
25 seconds with no returned session. This work reuses the EGA mode/page
mapping and its BIOS boundary from
[RESIDENT_VIDEO_INIT_4426.md](RESIDENT_VIDEO_INIT_4426.md), the exact 2×
screen crop and RGB table from
[SLOT_A_STATIC_VIEW_FIRST_REGION.md](SLOT_A_STATIC_VIEW_FIRST_REGION.md),
and the four-plane read-map procedure in
[SLOT_A_POSTFLUSH_PAGE_CHANGE.md](SLOT_A_POSTFLUSH_PAGE_CHANGE.md)
(PR #447). The neighboring printer path is separately owned.

The new isolated DOSBox-X run is
`resources/oracle-runs/slot-a-screen-palette-p5-001/`. `prepare_run.sh`
copied owner-supplied DOS media into ignored `resources/`. Its launcher
ran `DEBUGBOX START` in a sandbox without network, then selected the saved
slot A. A screenshot was captured at the stable first-view prompt before
pausing; a second screenshot after `Alt+Pause` has the **same SHA-256**.
The paused debugger produced a 640 KiB conventional RAM image and four
successive A000 `0x1F40`-byte dumps with graphics-controller read-map
selector 4 set to planes 0 through 3. Each dump was renamed before the
next selector change. The source run and all snapshots stay ignored.

| Private input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | owner media | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `SAVGAMA.DAT` | owner media | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| DOSBox-X debugger | host binary | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |
| screenshot before pause; screenshot after pause | 1280×1024 each | `2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134` each |
| paused conventional RAM | 655,360 | `e257bcc50c50e252313ce1c6e31273729bfe3c9839811426679c016a9cedd6c6` |
| A000 plane 0 | 8,000 | `68638729c3ac5a483f4fb023cf839dd4fea8432b51ec8e96d96d1fb3a8b35da1` |
| A000 plane 1 | 8,000 | `975cb7c40471ebb947734deae142c526bf37e0a756f3cb49c409492d432add35` |
| A000 plane 2 | 8,000 | `8c29a55dbd09cc924a3295263172b6336dd2d1c69caf3524f800af4929920404` |
| A000 plane 3 | 8,000 | `d1f05b349fd8db17bbf4739a7869448c4e8252f093db9c9592b597dfbb83c351` |

The four plane hashes also equal PR #447's separately captured
post-drawer-return page hashes. The screenshot hash equals the older
`slot-a-walk-001` first-view screenshot hash. Those are independent
cross-run consistency checks; the zero-mismatch result above uses the
new **same-run** screenshot and plane snapshots.

## Addressed relationship and measured result

For native `(x,y)` in `320×200`, A000 byte offset is `40*y + x/8` and its
bit is `7 - (x % 8)`. Plane 0 contributes index bit 0; plane 3 contributes
index bit 3. The DOSBox screenshot's centered `(320,312)..(960,712)`
crop is the corresponding `640×400` exact 2× image. The audit checks
all four pixels in each 2×2 block and reverses their observed RGB using
the 16-color EGA table. The comparison has **0/64,000 index mismatches**,
**0/64,000 scaling failures**, and **0 out-of-palette RGB values**.

At the paused stop, the DOS data segment is `0x14A0`. Its 14-byte
`DS:4A02` BIOS register record starts with AX=`0x0500`; the remaining
register words are zero. `DS:4A16` and `DS:4A18` are both `0xA000`;
`DS:4A1A..4A1E` are page 0, page 0, previous mode 3, current mode
`0x0D`, EGA selector 1. All 23 checked bytes match the existing
video-init model's endpoint. The later `DS:484C=0x46` has the same
documented startup ambiguity and is not used to infer the earlier mode
selection branch.

The screenshot proves the *observed* RGB mapping of this paused frame.
The hardware attribute-controller registers and DAC state were not read,
so the result does not attribute those RGB values to a particular BIOS
palette program or rule out intervening same-value palette calls.
The native screenshot is stable at the capture and pause stops; this
audit does not claim the pause was at a named first-view routine PC.

## Reproduction

With the private run retained under ignored `resources/`:

```sh
python3 tools/replay/slot_a_screen_palette_join_audit.py \
  resources/oracle-runs/slot-a-screen-palette-p5-001
python3 -m unittest tests/replay/test_slot_a_screen_palette_join.py
bash tests/replay/check_first_view_display_state.sh
resources/build-first-view-display/first_view_display_state \
  resources/oracle-runs/slot-a-screen-palette-p5-001/ram-stable-first-view.bin
```

The audit rejects paths outside this clone's ignored `resources/`, hashes
each private input before reading it, and prints only counts. The synthetic
test covers plane bit order and significance, 2× pixel blocks, RGB rejection,
and page-state rejection. No original screenshot, palette bytes, media, or
pixel buffer is committed.

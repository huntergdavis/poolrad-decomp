# U19 row-14 page and host-RGB same-run discriminator

This source-only gate extends the merged [page-to-host bridge](U19_ROW14_PAGE_TO_HOST_SOURCE_BRIDGE.md) from PR #713. It identifies debugger observations that can separate a reached guest page request, a queued CRTC start, a retrace-latched start, and a host RGB update. It supplies no new native helper hit, page flip, retrace event, four-plane sample, or RGB timing. Heavy native remains held.

Before this work, bounded `deja "Poolrad U19 row14 selected A000 118F four-plane GC Seq selector restoration page CRTC retrace RGB same-run capture"` found no matching indexed session. PR #713 supplied the conditional INT 10h AH=05 → CRTC `display_start` → EGA latch `real_start` → line renderer source chain. The merged [page-return contract](COLD_START_CODEWORD_ROW14_PAGE_RETURN_RGB_HANDOFF.md) supplied the guest +0074/+0087/+008C/+0099/+009C and +00A0/+00B0/+00B3 boundaries. The new audit pins read-only debugger commands and retained RGB file identity against ignored DOSBox-X source revision `5a9c7c53481c2e933948039c33256776653f4faf` and heavy executable SHA-256 `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

## What a paused host can expose

The audit locks two complete private source files and seven exact windows:

| Private source | Lines | SHA-256 | Observation boundary |
| --- | ---: | --- | --- |
| `debug/debug.cpp` | 3215–3240 | `49371ae314c4b5293c2e8d4d26d74ec9e28649c619e6bda5d8f303c45cfa8727` | `VRD` calls `VGA_DebugRedraw`; `VGA DS START/X` changes display overrides and redraws. Neither belongs in a passive capture. |
| `debug/debug.cpp` | 3307–3326 | `da5ad1cfb9718557b1689eba5b428b6615d0be45584e24b976eb74e99ce449e2` | `VGA CRTC` reports start-address registers 0C/0D directly. |
| `debug/debug.cpp` | 3405–3433 | `ad1df338666e5c9f25005391b01308b0f3ac5572f7131e9527718bfa5d41d328` | `VGA DRAW` reports the current draw address, byte skip, and line progress; that address may be mid-frame. |
| `debug/debug.cpp` | 3434–3468 | `8e641ca88669a77bed8b4ce3ed124ba6cbe6c87356bdaff5281c4c0d33fc9efa` | `VGA MODE` reports `display_start`, `real_start`, retrace, mode, chaining, address shift, read/write mode, and read map. The command reads these fields without invoking `VGA_DebugRedraw`. |
| `hardware/vga_draw.cpp` | 4119–4148, 6123–6156, 6814–6830 | `bffc6f2bee77aff833ea09a015384a2700ae3f5b266a3673de1f761e773c4000`, `0e74faacbb721cf65ff7a4534f6d7a32a837aadbe7fa50a971785440ad231fb8`, `0a2692162240ae0cd1e7d7fca508fcf7f76102a4ac0a51567b5a34675b4a9e4c` | For the EGA branch, vertical retrace schedules a latch that copies masked `display_start` to `real_start`; debugger redraw can force that latch and a vertical-timer cycle. |

`VGA CRTC`, `VGA MODE`, and `VGA DRAW` expose three different boundaries. The CRTC register pair is a programmed start, `display_start` is the host's pending start, and `real_start` is the latched start subject to overrides. `VGA DRAW` is a live scan address, so equality with a guest page word is not a valid page proof. Reading these debugger fields does not by itself locate the retrace callback between CPU stops; a later authorized host event trace must do that if exact timing matters. Avoid `VRD` and display-override commands during the discriminator because their source changes the latch/timer path being measured.

## One-run decision order after native authorization

First establish a **reached** `RES_0709` active-page helper with its far caller, `SS:[BP+6]`, `+0087` request word `0x0500 | low(argument)`, and `+008C` return; then pair `+0099/+009C` around DS:4A16. Keep draw-helper `+00A0/+00B0/+00B3` separate: it stores DS:4A18 without an INT 10h call. A startup helper call does not establish U19 reachability. At every reached helper boundary and U19 `+02D8/+02DD/+02E2/+02FD/+0302`, keep the same run token, chronological CPU/return frames, RAM, `VGA CRTC`, `VGA MODE`, `VGA DRAW`, and a canonical host RGB digest. Use a separately verified, nonperturbing four-plane witness: debugger A000 reads load the EGA latch, so restoring only the GC selector is insufficient before guest continuation. Verify that the RGB capture mechanism itself did not request a redraw.

The observations answer different questions:

1. A helper hit and BIOS return, with matching caller and request, prove a guest page request; a DS:4A16 same-value write alone does not.
2. A changed CRTC start and `display_start` with old `real_start` is a queued host start, not a latched page. Equal values after a same-value request still require the reached helper frame to prove the call.
3. A later `real_start` change is consistent with a latch between observations; attribution and exact retrace timing require the host latch callback trace. Check mode, address shift, and display override before mapping it to a plane/page.
4. A later decoded RGB change is an observation interval. Pair it with the selected four-plane byte, latched start, palette/decoder branch and renderer update before attributing a pixel to a page. The merged host-chain note explains why an unchanged render cache can delay a visible update.

The retained U19 `+02DD/+02E2/+02FD/+0302` PNG files are available and hash-locked; the first three are byte-identical and the fourth differs. The retained RAM words remain `A000/A000` with page indexes `0/0`, while the earlier decoded RGB counts are `0/0/6612`. Those old files lack synchronized `VGA MODE`/CRTC/DRAW fields and a reached helper frame. They cannot select one of the four decisions above. A new native slot and any source instrumentation stay on hold.

Read-only reproduction:

```sh
python3 tools/replay/cold_start_codeword_row14_host_state_discriminator_audit.py \
  --source resources/dosbox-x-16a1-private-src \
  --heavy resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/codeword-row14-p5-001
python3 tools/replay/cold_start_codeword_row14_page_selector_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

Only addresses, hashes, counts, and conditional source relationships are public. Original media, RAM, controller logs, EGA planes, and RGB remain under ignored `resources/`. No live writer, page/RGB, C, strict, or full-frame credit follows.

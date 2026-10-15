# Saved slot-A palette transition and one-Up RGB frame

The saved slot-A native route has a measured AC and DAC state change across
the resident `SetVideoMode(0x0D)` BIOS call. Its first-view and one-Up palette
snapshots equal the state immediately after that call. With the native one-Up
planes, that measured palette reproduces every pixel in the native screenshot.
This establishes the **call boundary** for the observed palette transition and
a complete **native** post-input RGB frame. The individual BIOS or emulator
port-write instructions inside the call were not stopped, and a complete C
post-Up page is still blocked at key dispatch.

Before debugging, `deja "Poolrad saved slot A EGA 0x0D VGA attribute
controller DAC palette writes BIOS 10h post input RGB frame"` timed out after
30 seconds without a result. This audit reuses the merged resident
[video-mode code and ABI](RESIDENT_VIDEO_INIT_4426.md), the
[first-view RGB mapping](SLOT_A_RGB_FRAME_PALETTE.md) (PR #464), and the
[indexed screen normalization](SLOT_A_SCREEN_PALETTE_JOIN.md). The
[key-to-script join](SLOT_A_KEY_SCRIPT_JOIN.md) identifies the surviving C
dispatch blocker.

## Hash-locked call boundary

The owner `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The unpacked image `0x0470F..0x0473E` is the 48-byte
`RES_0464:00CF..00FE` routine; its SHA-256 is
`1291e63d47f0d18e3c3708159546250c155647778751c8e8a2123f8bcc44afb3`.
At canonical `+00EE`, its far call to the resident Turbo Pascal `Intr` helper
at `0A32:000B` invokes the INT 10h vector with the register record at
`DS:4A02`. Its next instruction is `+00F3`.

The same-run debugger stopped at native `0C88:00EE` and `0C88:00F3`.
Immediately before the call, `DS:4A02` contained AX=`0x000D`, the BIOS
video-mode function and mode. The two-byte private snapshot SHA-256 is
`f566cc6fccc657365c0197accf3a7d6f80f85209ff666ff774f4dcbc524aa842`.
The debugger was resumed from the first stop to the second, so the hardware
change is bracketed by that one call. There is no direct `INT` instruction at
`+00EE`; an `INT` opcode breakpoint would miss this Pascal helper path.

| Hardware query | Before `0C88:00EE` | After `0C88:00F3` |
| --- | --- | --- |
| VGA mode | `M_TEXT` | `M_EGA` |
| AC control / horizontal pan | `0x0C` / `0x08` | `0x09` / `0x00` |
| AC map | 9 of 16 entries differ | 16 entries measured |
| DAC | 49 of 256 entries differ; 68 of 768 six-bit channels | all 256 entries measured |
| DAC pixel mask | `0xFF` | `0xFF` |

The following same-run stable first-view and stable one-Up AC/DAC/mode
queries equal the post-call state exactly. This proves snapshot equality at
those stops; intervening same-value writes or temporary changes are not
excluded. The call boundary does not identify the BIOS implementation's
individual AC or DAC port writes. The observed AC/DAC change is a hardware
side effect of the BIOS call, not a new translated game routine.

## Post-input full-frame check

The ignored run is
`resources/oracle-runs/slot-a-palette-mode-p5-002`. Its finalized debugger
transcript SHA-256 is
`adb097bd6a16c49acacd6776c09af3383c7a1b15379742d15779acb08f700500`.
The owner save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`;
the heavy DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The stable first-view screenshot has the same hash as PR #464,
`2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134`.
After one Up input the stable and paused screenshots are byte-identical;
their SHA-256 is
`ecb89bbc83b4cafa8a50500b3a16f9cd286f0b41191d05c75c4e77db9b17e3c7`.
The same-run private post-Up conventional RAM dump SHA-256 is
`2d08287fa4185124c182c0ec74215d0dad2ac8d0304db1e02a888ea24cb6ea47`.

At the post-Up stop, the four queried `A000` read-map planes have these
SHA-256 values, in plane order:

| Plane | SHA-256 |
| --- | --- |
| 0 | `334d8dce215b43db059b13746aeb005551a0eefe0e3f56034641fecf390e8a58` |
| 1 | `bf420fb4c558119a97bbf6580f15480d0db058760de933d2aa32ae6de8373091` |
| 2 | `b59db1ee961ca5fbcb9cbf8c6429e64175aee41f43ded143ed579e98443aeaed` |
| 3 | `54a0c23217d11cc1d0e64de83994872aa8599eb898b257f80341848356acee02` |

Each plane has 8,000 bytes; the concatenated 32,000-byte page SHA-256 is
`f963d2bcdb1d0f0cf1a0fdc32e285e5299993dd7f9a7b474cfbbd6978b4cbbe1`.
The same-run AC/DAC map converts those planes to RGB SHA-256
`8912d397e356eee080b90be2e836c9bcc4d3dfb4d769f5774a384d7c0d8897b3`.
The centered native screenshot crop SHA-256 is
`27001077963c89a498e8e872c710411954578095c02024bbfc6e3c17ff3cff56`.
All 64,000 logical pixels, expanded to all 256,000 2× display pixels, equal
the screenshot; 11 EGA indices occur.

The previously verified **first-view** C page differs from the **post-Up**
native page in 725 of 32,000 plane bytes (174, 106, 308, and 137 by plane)
and 1,979 of 64,000 RGB pixels. Thus the first-view C page cannot stand in
for post-Up C parity. `pr_slot_a_native_one_up` still returns
`PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH`; the joined C route cannot produce a
complete post-Up page from the owner save. This is the specific remaining
blocker for a C/native post-input RGB claim.

## Reproduction

With the ignored inputs and run above present locally:

```sh
python3 -m unittest -v tests/replay/test_slot_a_palette_write_route.py
python3 tools/replay/slot_a_palette_write_route_audit.py \
  --run resources/oracle-runs/slot-a-palette-mode-p5-002 \
  --exe resources/DOS/POOLRAD/START.EXE \
  --save resources/DOS/POOLRAD/SAVGAMA.DAT \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --c-first-page resources/oracle-runs/slot-a-postflush-plane-change-p5-001/postflush-prompt-probe-output-private.bin
```

The audit locks the source and private capture hashes, checks the ordered
hardware reports, compares all native post-Up RGB pixels to the screenshot,
and measures the first-view C page delta. Synthetic tests exercise report
completeness, channel and entry counting, and logical-pixel comparison.
No original game bytes, pixels, screenshots, or derived media are committed.

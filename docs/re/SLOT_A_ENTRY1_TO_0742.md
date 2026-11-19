# Saved slot-A Up: unit 0x68 entry 1, `+0x0D6D` to the `+0x0742` resident call

Slice 1 of the t4492 entry-1 composition. `pr_slot_a_entry1_to_0742`
([header](../../include/poolrad/replay/slot_a_entry1_to_0742.h)) takes a
conventional RAM image at the moment `+0x0D6D` is about to run, reads the entry-1
inputs from it, runs the merged supplied-state projection
[`pr_u68_e1_prefix`](U68_ENTRY1_FAIL_CLOSED_PREFIX.md), and on the selector-1 /
entry-3-result-1 path writes the six stack stores that the original bytes make up
to the far call `0x8AE:0x00DC` at `+0x0742`. It then **stops**. It does not
execute that call, the rest of the coordinate writer `+0x073B`, the commit tail,
`0xBA:0x290B`, or any callee. This is a bounded step, not a one-Up frame, not a
same-invocation C route, and no function count or credit is claimed.

Reused with citation: [U68_ENTRY1.md](U68_ENTRY1.md) (branch map, frame),
[U68_COORDINATE_WRITER_073B.md](U68_COORDINATE_WRITER_073B.md),
[U3BD_FIELD_ENTRY3.md](U3BD_FIELD_ENTRY3.md),
[SLOT_A_POST_EXIT_NATIVE_RETURN.md](SLOT_A_POST_EXIT_NATIVE_RETURN.md) and
[SLOT_A_ONE_UP_NATIVE_RESIDENT_DISPATCH.md](SLOT_A_ONE_UP_NATIVE_RESIDENT_DISPATCH.md).

## Original bytes the frame arithmetic rests on

Ignored `GAME.OVR` (206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`), unit
`0x68` at file `0x166CB`:

| Unit offset | Span (bytes, SHA-256) | Meaning |
| --- | --- | --- |
| `+0x0D6D` | 7, `ac7b855675a4ac988fe3f9399cd6a294dd72ab5e4b673dad25449f1d7844efdf` | `push bp; mov bp,sp; sub sp,0x12C` |
| `+0x0DA7` | 4, `14470acec7c07571af0efb073e074fab9fb7da33c4f9f14d7fdf22181ddc8ee2` | `mov byte [bp-1],1` (entry 3 returned 1) |
| `+0x1027` | 10, `95d26270a6325429435a650b58bb6205bb9cd67983b0f455044c726123917676` | `cmp [bp-1],0; je; push cs; call near +0x073B`; the call is at `+0x102E`, so the return IP is `0x1031` |
| `+0x073B` | 10, `6e8c87ccf30d55ea8536ecba2d8557f7d84b75865717d271d504e2e4488158e1` | `push bp; mov bp,sp; push word [0x2628]`, then the far call `0x8AE:0x00DC` at `+0x0742` |

The audit also checks that the **loaded** bytes at `2E60:0D6D`, `0DA7`, `1027` and
`073B` have the same digests in both retained RAM images.

With the caller's far call already pushed (SP `3EF2`, BP `3EF8`, CS `2E60`):
saved BP at SS:`3EF0`, `[bp-1]=1` at SS:`3EEF`, SP after `sub sp,0x12C` is `3DC4`,
`push cs` at `3DC2`, near-call return IP `0x1031` at `3DC0`, `+0x073B`'s saved BP
(`3EF0`) at `3DBE`, and the pushed word `[2628]` (10) at `3DBC`. Final SP `3DBC`,
BP `3DBE`, IP `0742`: equal to the native `CPU-0742` transcription.

## Evidence (hashes only; media and RAM stay in ignored `resources/`)

Retained same-run stops from the earlier isolated run (`t4492-entry1-002`,
ten 64 KiB segments reassembled; CPU files are manual transcriptions of
debugger output):

| Item | SHA-256 |
| --- | --- |
| `RAM-0D6D-SEGMENTED.BIN` (655,360 B) | `ded9ac1d1db89fead04ed13a6515be9152662478e10d2edc721e6bb8cc5a868e` |
| `RAM-0742-SEGMENTED.BIN` (655,360 B) | `2ef55fd5fd893f3e5397130a35bfc77777c4e126eda939bebda5da3cace7de4e` |
| `CPU-0D6D.TXT` | `54564c50156141b2e550a2d59ad6c34df44be42425cc1de68ee18d9178d25cd3` |
| `CPU-0742.TXT` | `0fcc08571fcf5771e1d9c177554ce9b192e75f5090789c8e93d2f38b94d89efd` |

The unsegmented `RAM-348D.BIN`/`RAM-0D6D.BIN` of that run are invalid (byte-identical,
garbage party fields) and are not used.

Result of `check_slot_a_entry1_to_0742.sh`, comparing **all 655,360 bytes** of the
model's RAM against the native `+0742` RAM:

* the model writes exactly 11 bytes and every one equals the native byte;
* native differs from the model in exactly **28** other bytes, all in SS `1D16` at
  `3D9D..3DA6` and `3DAA..3DBB`, all below the stop SP `3DBC`; zero differences
  anywhere else.

The 28 bytes are the dead-stack residue the unit `0x3BD` entry-3 call chain (and any
interrupt frame) left below the final SP. They are pinned by exact ranges and are
**not modeled**; whether any of them was an interrupt frame is not distinguishable
from one capture.

## Verification

* `poolrad_slot_a_entry1_to_0742_tests` (CTest, synthetic, no game data): the exact
  stored set (11 bytes, over a non-zero stack filler so a stored zero is a real
  change), every non-`+0742` projection stop leaving RAM byte-identical, window word
  `FE` versus `FF`, facing and map inputs read from the right fields, missing-input
  atomicity, SP wrap, argument guards. The tests use explicit checks, not `assert`,
  and pass at `-O0 -UNDEBUG` and `-O2 -DNDEBUG`. 15 source mutants (flag, saved BP,
  locals size, pushed word, CS slot, return IP, window offset, facing, map id, BP
  slot, stop IP, store count, party x, selector, buffer pointer) were each killed;
  two mutants (facing, map id) first survived and drove two added cases.
* `tests/replay/check_slot_a_entry1_to_0742.sh GAME.OVR RAM_0D6D RAM_0742 CPU_0D6D
  CPU_0742` (private inputs, hash-locked by `audit_slot_a_entry1_to_0742.py`):
  passes. Swapped inputs, the invalid dump and a wrong CPU file each fail.

## Fresh same-invocation capture (run `t4492-slot1-capture-025`)

A new isolated native run reached the caller stops in one invocation
(slot A, one Up, breakpoints at `3582:345B`, `348D`, `3492`):

| Stop | RAM SHA-256 (655,360 B) | CPU (hex) |
| --- | --- | --- |
| `345B` | `82f6f9c602dd9d40b3ff7b85041e6cbfd2a7add6347153fde8412c6350d2e981` | `AX=0000 BX=3ED4 SI=3E8A DI=84CA BP=3EF8 SP=3EF6 DS=14A0 ES=14A0 SS=1D16 FLAGS=7202` |
| `348D` | `8c9aa3eee434e0bcc9ee3b260a588f2df645ee12a1fb6cbab0ea90b89eed0ea0` | `AX=0004 BX=3ED4 SI=3E8A DI=0000 BP=3EF8 SP=3EF6 DS=14A0 ES=3996 SS=1D16 FLAGS=7246` |
| `3492` | `f66a6d9a9234121c4d96e3b81adfddc818deb36c4973d87c4622a7628511c04c` | `AX=0000 BX=3DA8 DX=03C5 SI=3D9A DI=6B06 BP=3EF8 SP=3EF6 DS=14A0 ES=14A0 SS=1D16 FLAGS=7246` |

Measured: `CPU-348D.TXT` (`eacba5ce…`) is identical to the earlier runs'; the
loaded caller bytes hash to `785a11da…`; `345B→348D` changes exactly one byte
(physical `0x39B40`, `1 → 15`); `348D→3492` changes **6,201** bytes (the earlier
receipt recorded 6,198; the runs differ), party `(15,4,6) → (14,4,6)`, window-0
`+0x18E` `0 → 1`, `+0x190/+0x192` unchanged. The merged
`pr_slot_a_caller_to_entry1` full-RAM test (`test_slot_a_post_exit_native`) passes
on the fresh `345B`/`348D` pair. Limits: CPU files are manual transcriptions; the
debugger's own breakpoint display is the only stop receipt; the controller's final
wait-record gate and the observer verdict were partial at cleanup (observer exit
125, no post-audit), and the supervisor had to kill the debugger-held process. No
device, EGA or high-memory state beyond the 640 KiB conventional dump was
captured, and none is zero-filled.

## Explicit stops (not modeled, not skipped)

* Everything after `+0x0742`: `0x8AE:0x00DC` with `[0x2628]=10` (its callees
  `+137C`, `+006C`, `+13B1`, and the INT 21h vector and PIT port effects between
  them), then the coordinate writer's x/y step, unit `0x3BD` entries 2 and 4, the
  `FillChar` over `DS:6DDE..6DE0` (inferred; the pre-state already holds `1,1,1`, so
  no retained dump can confirm it), and unit `0x7D` entry 1 with the limit table at
  `DS:363A` and list head `[5D96]`.
* The commit tail: `0xBA:0x290B` (about 33 callees including `RES_03F1:0137`,
  `RES_0709:0626` and display output), unit `0x3B9` entry 3, and two
  `0x464:0x2595` releases.
* The 28 residue bytes above, and the transient overlay/display bytes of the full
  `348D→3492` delta.
* The struct-versus-RAM gap for `pr_u68_entry1` itself: this slice uses the pure
  projection plus explicit frame stores and never runs `pr_u68_entry1`'s hooks.

## Next integration step

Compose `+0x0742`: the resident dispatch `0x8AE:0x00DC` through
`resident_08ae_dispatch_00dc_cpu`, ending at its first service stop (`+137C`,
INT 21h AH=35h). Its inputs exist in the retained `0742`/`1386` stops and the
`service-003` run; the remaining blocker is that no retained or fresh capture
contains the INT 21h/PIT effects as a same-run `3492` partner.

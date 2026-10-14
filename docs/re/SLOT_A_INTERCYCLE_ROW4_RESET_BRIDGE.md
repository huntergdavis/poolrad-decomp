# Saved slot-A UI: row-four reset and persistent C page bridge

This bounded result carries one four-plane C EGA page from printer call 2's
native entry through call 42's return. The C probe applies 41 reached
printer requests and six reached zero-row effects, including the inter-cycle
row-four reset. All **82 entry/return checkpoints match the native pages,
32,000/32,000 bytes each**. Native pages after call 2 are comparison targets,
never C page inputs. This is page-effect parity for the measured sequence;
it does not produce the printer requests, the initial page, or a complete
static C frame.

Before this work, `deja "Poolrad slot A first-view 410-byte inter-cycle EGA
page reset between call24 return and call25 entry writer arguments source"`
found no matching indexed session. The preserved first-cycle composition
branch, the [second-cycle immediate-return audit](SLOT_A_SECOND_CYCLE_IMMEDIATE_RETURNS.md),
and the [persistent second-cycle page audit](SLOT_A_SECOND_CYCLE_PERSISTENT_PAGE.md)
were used as leads. Their original DOS bytes and all captures remain ignored.

## Live row-four route

The fresh private run `slot-a-ui-intercycle-reset-p9-026` uses the same
DOS v1.3 images and saved slot A as the prior audits:

| Pinned input | SHA-256 |
| --- | --- |
| START.EXE | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| SAVGAMA.DAT | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| DOSBox-X heavy debugger | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

At the seventh `08DE:07EA` hit after loading slot A, the caller's
`SS:SP=1D16:3EB6`, `BP=3ECC`, and local low bytes were row 4 and column
17. Its four pushed low-byte arguments were `(4,38,4,17)`. The reached
`RES_00BA:07BF..07EF` path increments the row by two after the second
title printer return, loads the far pointer at `DS:5D96`, tests it for
nonzero, and pushes `(left,row,38,row)` for the `+07EA` far call.
The live pointer was `449A:0008`, both in `DS:5D96` and the caller local.
The branch is therefore observed as taken on this saved route. The aligned
instructions from the call-24 return at `+07BF` through the `+07EA` call
have no EGA page store, so call-24 return is the pre-clear page. The
`+07EF` stop is immediately after that far call.

The call reaches `0C15:01F1` within `RES_03F1:0137`; its `BP=3EB0`
frame returns to `08DE:07EF` and carries `(4,38,4,17)`. It then reaches
`0F2D:00B7` in `RES_0709:00B7`; its `SS:SP=1D16:3E94` frame returns to
`0C15:01F6` and carries
`(color=0,page=0,bottom=4,right=38,top=4,left=17)`.
The outer frame has EGA page mode 1 and both page segments `A000`.

| Original or loaded span | Bytes | SHA-256 |
| --- | ---: | --- |
| Original `RES_00BA:07BF..07EF` caller path, expanded `[0x135F,0x138F)` | 48, 18 aligned instructions | `31ca672eb365796871962333732456524892e788e30cda9de7f6079f63f6ca57` |
| Loaded `08DE:07BF..07EF` caller path | 48 | `440e336aeb4cfcf75d40ac752abdfa4e7ef138b5d7ec5b512595f04deffff15a` |
| Original `RES_03F1:0137..02E8`, expanded `[0x4047,0x41F8)` | 433 | `1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc` |
| Loaded `0C15:0137..02E8` | 433 | `f50a7fde3dcc5886f55beb81c5fcdc22efe4348b52d8df35c70a646d5a309d5a` |
| Original `RES_0709:00B7..0171`, expanded `[0x7147,0x7201)` | 186 | `f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5` |
| Loaded `0F2D:00B7..0171` | 186 | `02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f` |
| Original outer far call `[0x138A,0x138F)` | 5 | `395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f` |
| Loaded outer far call `08DE:07EA` | 5 | `2f89ade8cd970b19f81e674324278b7e2f7845bd119dbf92374051d645fc1850` |
| Original nested far call `[0x4101,0x4106)` | 5 | `2286767d9eccf2e3fe83669ef5367f5ef97da8c2145e12d54b0291510e19093c` |
| Loaded nested far call `0C15:01F1` | 5 | `8d1b1dec8fbef9dea92ee3e851c55572850da6b639cd9fb3890c998e918cc8e2` |

The three private 640 KiB RAM captures at caller, outer call, and rectangle
entry have SHA-256 `848bbadb5b82e407b54463fc811894962c9bc7e05d0777ce1fe457bda352c39c`,
`86e2205dd0a5353cc40a783c33c5e6d0be167722a7d8aae56f9f819ff1ecceb1`,
and `3ad791e7b0fa7bb18e62fc24acb74e6a9b19d8bfc177aa607c94a44486fa4d1d`.
The private audit checks these whole-image hashes and each bounded code span.

## Complete page effect and C result

The independently captured call-24 return page has SHA-256
`3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738`.
The four-plane page dumped at the immediate `08DE:07EF` return has SHA-256
`2aa65739f6bc280936e6203d6bb6cbaad26663475ee86e51468d48a4b7b4bfdf`.
It equals the independently captured call-25 entry page in all 32,000 bytes.
Exactly **410** changed plane bytes, all nonzero to zero, lie in row four,
scanlines 32..39, columns 17..38. Applying the complete four-plane,
eight-scanline by 22-byte zero rectangle to the pre-page reproduces the
immediate return page exactly. No further EGA page difference appears before
call 25. Same-value stores and transient hardware state are outside this
page comparison.

`pr_slot_a_second_zero_row_apply` now admits the separately reached row
four with the previously proved rows five through nine. It still rejects
other rows, colors, columns, and short page spans before writing. The
synthetic test checks row-four targeting and unchanged neighboring bytes.

The new C probe starts with call 2's native entry page, SHA-256
`606d838123a21edb436339b6fbf4fb2583ae79b38964d89f24e763cdb34c7c77`.
It applies the 23 first-cycle printers, then the row-four reset, then the
18 second-cycle printers with five inter-call clears. Source slices, font,
and call arguments come from hash-locked native RAM entry frames; later
native pages are compared only. Its 82-page stream is **2,624,000 bytes**,
SHA-256 `a139cb8c478593ae14346f02f60a59b209535ca193fb54a2d86c13c421bbf1ac`,
and each checkpoint matches its independently captured native page.
The C report is 170 printer cells, 5,440 printer plane-byte stores, 3,158
summed per-call value changes, six clears, 4,224 clear targets, and 1,404
clear value changes. These are bounded page counts, not game-function counts.
The focused synthetic test passed **1/1**, and the full strict CTest suite
passed **178/178** in the local build below.

## Reproduction

```sh
cmake -S . -B resources/build-slot-a-reset-local \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-reset-local --target \
  poolrad_slot_a_second_cycle_zero_row_tests \
  poolrad_slot_a_two_cycle_reset_compose_probe
ctest --test-dir resources/build-slot-a-reset-local \
  -R '^poolrad_slot_a_second_cycle_zero_row_tests$' --output-on-failure
python3 tools/replay/slot_a_two_cycle_reset_compose_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --first-run resources/oracle-runs/slot-a-ui-whole-first-view-p9-021 \
  --second-run resources/oracle-runs/slot-a-ui-second-cycle-p9-023 \
  --reset resources/oracle-runs/slot-a-ui-intercycle-reset-p9-026 \
  --caller-run resources/oracle-runs/slot-a-ui-second-cycle-callers-p9-025 \
  --probe resources/build-slot-a-reset-local/poolrad_slot_a_two_cycle_reset_compose_probe
```

The fresh replay was prepared with `tools/oracle/prepare_run.sh`, then run
through the private heavy debugger using breakpoints `08DE:07EA`,
`0C15:01F1`, `0F2D:00B7`, and `08DE:07EF`. At the seventh `07EA` hit,
the frame was dumped; the nested stops supplied their argument frames;
the immediate return supplied the four EGA planes via graphics-controller
read-map selections. At each RAM stop, `MEMDUMPBIN 0000 0000 A0000`
captured 640 KiB. At `08DE:07EF`, each plane used `OUTP 3CE 04`,
`OUTP 3CF 00` through `03`, then `MEMDUMPBIN A000 0000 1F40`;
the read map was restored to `00`. Original media, save, RAM, text, font, EGA page
captures, and C probe input/output remain under ignored `resources/`.

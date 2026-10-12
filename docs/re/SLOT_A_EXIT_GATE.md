# Saved slot-A EXIT gate at ECL PC `0x9B67`

This audit composes the existing bounded unit `0x1C:+0x0001` EXIT model with
the ECL runner at the native run010 EXIT entry. It verifies the reached
branch, resulting PC, and runner stop-byte lifecycle against four hash-locked
RAM snapshots. It does **not** replay the route into `0x9B67`, prove full
one-Up parity, or promote any strict/full-C or game-identity count.

Before debugging, `deja "Poolrad saved slot A ECL EXIT PC 0x9B67 run010
0x84E2 0x4430 0x6F70 0x31AA 0x31B0"` returned no exact prior session.
Reused and checked [the native AL10 branch](SLOT_A_ONE_UP_NATIVE_AL10_BRANCH.md),
[the saved COMPARE continuation](SLOT_A_COMPARE_CONTINUATION.md),
[the EXIT contract](U1C_EXIT_0001.md), [the ECL runner](U1C_ECL_RUN_3171.md),
and [the save transfer map](SLOT_A_NATIVE_SAVE_ADAPTER.md). The ignored
snapshots were read only from p9's run010 capture; no RAM or game bytes are
published here.

## Hash and width gates

| Ignored input | Size | SHA-256 |
| --- | ---: | --- |
| `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| saved script transfer, file `0x1401..0x3200` | 7,680 | `bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| unit `0x1C:+0x0001..+0x00F3`, file `0x0D47..0x0E39` | 243, 91 instructions | `9f2483f23b51d3dbc54a83592241cbd6f5bc27e1173e62f17c54c3b6672ca493` |

The 243-byte source span ends at `retf`; the next routine begins at
`+0x00F4`. The native relocated copy at `3582:0001..00F3` in the EXIT-entry
snapshot hashes `c09b0f2dd45cb0ccad3b25afa54ea0e885dff87873679d630319ee47310d70ff`.
That relocated hash is intentionally distinct from the file hash.

| run010 RAM snapshot | Size | SHA-256 | DS:49ED | DS:442E |
| --- | ---: | --- | ---: | ---: |
| dispatcher `06`, `bp-script-06-3582-2f2d-ram.bin` | 655,360 | `bae33746a473a4b2c28aa40097095f67615cf1552641b0a6002bf02bbe4500a8` | `0x9B67` | 0 |
| EXIT entry `07`, `bp-script-07-3582-0001-ram.bin` | 655,360 | `7d9303e803eaf54ecf8d30db5aa339a2107546543c1678debf1eee5189196091` | `0x9B67` | 0 |
| runner `08`, `bp-script-08-3582-31aa-ram.bin` | 655,360 | `12a715b7ee9ca93844ed2edd0127a10353e4df38f0f389d245d985f32264557a` | `0x9B68` | 1 |
| runner `09`, `bp-script-09-3582-31b0-ram.bin` | 655,360 | `d43b75633114880a24480a73a64bc623bbadb736729d39d78d2612e94f2c2d12` | `0x9B68` | 0 |

The audit reads DS at `0x14A0`, with each byte at physical
`(0x14A0 << 4) + offset`. It reads the window-0 far pointer as two
**16-bit words** at DS:`49D2`/`49D4` (`3996:0000`) and then reads window
words little-endian at byte offsets `+0x18E`, `+0x190`, `+0x192`, and
`+0x1CC`. The gate at DS:`84E2` and character flag at DS:`4430` are
**bytes**. The call-list head at DS:`6F70`/`6F72` is two **words**. The
runner PC at DS:`49ED` is a **word**, while latch DS:`6F7E`/`6F7F` and stop
DS:`442E` are **bytes**.

## Reached EXIT path

At snapshots `06` and `07`, PC is `0x9B67`, previous/current latch is
`0x1A/0x00`, `[84E2]=0`, `[4430]=0`, and the list head `[6F70:6F72]=0:0`.
The saved window-0 word at byte `+0x1CC` is 1, but the first gate skips its
test. The EXIT byte decode gives these reached decisions and effects:

| Unit offsets | Instruction decision or effect |
| --- | --- |
| `+0007..+000C` | `cmp byte [84E2],0; je +0066` taken: neither resident box call is reached |
| `+0066..+006B` | `cmp byte [4430],0; je +0080` taken: no character pointer restore |
| `+0080..+008C` | FillChar of **two bytes** `[84C8:84C9]` with zero |
| `+0091..+00A0` | clear `[84E0]`, `[84E2]`; set `[442E]=1`; increment **word** `[49ED]` to `0x9B68` |
| `+00A4..+00AB` | OR the two call-list-head words; `je +00E1` taken: no FreeMem call |
| `+00E1..+00EB` | set `[5E25]=0x11`, `[5E24]=1`; clear `[84DE]` |
| runner `+31AA..+31B0` | clear `[442E]` again and return |

The native snapshots confirm PC `0x9B68`, stop 1 at `+31AA`, and stop 0
at `+31B0`, with the other measured bytes zero. `[5E24]=1` and
`[5E25]=0x11` are already those values in the entry snapshot, so their
*write* is proved by the instructions, not by a RAM delta. Likewise, zero
values in the entry snapshot do not alone prove a clear; the instructions
identify the reached stores.

## Model comparison and source boundary

`tests/replay/test_slot_a_exit_gate.c` runs a different synthetic one-byte
EXIT fixture and checks the reached-hook guard: with `[84E2]=0`, no box
hook is required; with `[84E2]=1`, missing the now-reached hook stops before
the PC increment and other EXIT effects. Its private test maps the save's
three word windows and script, then supplies only the measured runtime
values required at this **already-reached** entry. The saved minute word
`+0x18E=0` is explicitly replaced with native run010 `1`; window-2
`+0x21E=10` is the native post-RANDOM value; the previous latch `0x1A`,
`[84E2]=0`, `[4430]=0`, nil list, and `[49FF]=0` are from the entry RAM.
The active-character far pointers are neither sourced nor compared because
`[4430]=0` skips them. The box callback and heap effect are not reached.
The test checks one dispatch, PC `0x9B68`, handler-local stop 1, runner-final
stop 0, unchanged word windows, and no reached callback or list release.

`bash tests/replay/check_slot_a_exit_gate.sh` compiles under C11 with
`-Wall -Wextra -Wpedantic -Werror` and runs the synthetic test. Passing
the ignored save path adds the hash-locked private C comparison. Passing
the ignored run010 directory as a second argument runs
`audit_slot_a_exit_snapshots.py`, which independently verifies each RAM
hash and the numeric gate/PC/stop fields. The model composition remains a
bounded gate over the EXIT route; the preceding key dispatch and elapsed
one-Up effects are outside this test.

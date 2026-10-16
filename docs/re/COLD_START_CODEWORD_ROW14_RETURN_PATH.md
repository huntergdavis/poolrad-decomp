# U19 row-14 call and return boundary

The U19 call at `+02DD` returns at `+02E2` from resident `0899:0104`.
It is **not** the line-input call: U19 reaches line input at `+02FD` and
returns at `+0302` from resident `0709:09E3`. The nearest preceding U19
call whose static code can write fixed row 14 is its `+02D8` printer call.
The paired screenshots change row 14 only within `+02FD→+0302`, but they
do not identify the first value-changing guest EGA store or explain host
presentation timing.

Before this audit, bounded `deja "Poolrad U19 +02DD +02E2 0899:0104 +02FD
0709:09E3 row14 first changing path"` timed out. The merged
[native key-read branch](NATIVE_COLD_START_U19_KEY_READ_BRANCH.md) supplied
the observed Return-specific branches and call frames; the
[paired-frame boundary](COLD_START_CODEWORD_ROW14_BOUNDARY.md) supplied the
four same-run screenshots; the
[printer-route note](COLD_START_CODEWORD_ROW14_PRINTER_ROUTE.md) supplied
the preceding `+02D8` row-14-capable path. Their relevant original offsets,
targets, and capture hashes were checked again. This note does not touch
the separate U17 first-credits printer route.

## Hash-locked original control path

The ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
ignored `GAME.OVR` hashes
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Original spans below are before loaded-segment relocation.

| Span | Bytes | SHA-256 | Reached role |
| --- | ---: | --- | --- |
| U19 `+02DD..+02E2` | 5 | `ca3aa4c3b05c30afded7501c51739379fbd4c291e8886a81c097f97313530356` | far call `0899:0104` |
| `RES_0899:0104..0122`, expanded image `[0x8A94,0x8AB2)` | 30 | `1a9b687c96c69b4b791be00bd72730f6df05513ce522a18d53041f8846ce4d93` | `+010A` unconditional jump to `+011E` epilogue |
| U19 `+02E2..+02FD` | 27 | `6aa8dc6978771bd029e8fd923a232ef8cb405176b9571aa503c019b48e49966e` | caller setup and `+02F2` runtime Pascal copy `0AF8:0B0B` |
| U19 `+02FD..+0302` | 5 | `32ca391ccf9a6a85b6399ab782aa4a24a56c5030ffca617a8a31265fee3c8541` | far call `0709:09E3` |
| `RES_0709:09E3..0B66`, expanded image `[0x7A73,0x7BF6)` | 387 | `822b4aeac02e3396c177fbdb18187c6040862ec3595d7ffde16b8eb221565baf` | bounded line-input body |
| `RES_0709:05EB..0626`, expanded image `[0x767B,0x76B6)` | 59 | `a41af7187ac3e0bf5a09311bad9fae220c498f0f9aad9492438661381d4b1171` | one-cell clear helper, calls drawer at `+061F` |

At `0899:0104`, the reached prologue allocates one local word, then the
`+010A` short jump skips the intervening call and local-write loop. The
epilogue restores the stack and far-returns to U19 `+02E2`. That reached
path has no display store. U19 `+02E2..+02FD` prepares local Pascal data:
`+02F2` calls `0AF8:0B0B`, then it pushes low-byte values `13` and `6`
before the `+02FD` line-input call. This is caller instruction evidence;
the audit does not substitute a full runtime-copy or keyboard model.

The input body's direct screen-call inventory is:

| `RES_0709` call site | Target | Direct screen argument |
| --- | --- | --- |
| `+0A0A` | local clear `+05EB` | row 24, columns 0..39 |
| `+0A1D` | string printer `+0626` | row 24 |
| `+0A9C` | string printer `+0626` | row 24, printable-key arm |
| `+0AE6` | local clear `+05EB` | row 24, backspace arm |
| `+0B0A` | local clear `+05EB` | row 24, Return/escape exit arm |

The clear helper bounds its row at 24 and calls drawer `+01DF` from
`+061F`. The prior native trace observed the one-Return path through input
branches `+0A38→+0AA1→+0AEC→+0AFD`; this route reaches the first clear,
first printer, keyboard read, and exit clear. The printable and backspace
arms are static alternatives, not observed for that Return. These direct
calls address row 24. An untraced nested callback or earlier EGA state
cannot be excluded from the screenshot evidence alone.

The closest statically established fixed-row-14-capable route in the U19
suffix is **before** `+02DD`: U19 `+02D8` calls printer `0709:0626` with
row 14, as the [separate source audit](COLD_START_CODEWORD_ROW14_PRINTER_ROUTE.md)
checks. The +02DD screenshot still has 477 source-pixel mismatches across
19 row-14 cells. Whether the earlier printer's guest stores changed EGA
planes and when the host presented them require synchronized plane or
write-watch evidence. No first-writer, full-frame, full-C, identity, or
fresh-native credit is claimed here.

## Paired capture gate

The four ignored 655,360-byte RAM/screenshot pairs at U19 `+02DD`,
`+02E2`, `+02FD`, and `+0302` are SHA-256 checked by the audit. Canonical
host RGB changes are **0**, **0**, then **6,612** pixels across the three
successive intervals. The fixed row-14 mismatch is 477 source pixels at
the first three stops and zero at `+0302`. The captured display words
remain `[A000,A000,0000,0001]` at all four stops.

Reproduce from ignored inputs with no emulator launch:

```sh
python3 tools/replay/cold_start_codeword_row14_return_path_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

Only addresses, hashes, and counts are public. Original code, strings,
RAM, and screenshots remain under ignored `resources/`. The separately
queued EGA plane-store watch recipe is outside this task-A note.

# Cold-start U19 row-14 display boundary

Four paired native stops bound the remaining fixed row-14 screenshot gap in
the codeword screen. The row still differs from its U19 source at the
`+02DD`, `+02E2`, and `+02FD` stops, then matches at `+0302`. This identifies
the **display-change interval** as the `+02FD` line-input call. It does not
identify a guest instruction that wrote EGA memory: the captures have RAM and
host RGB screenshots, but no synchronized EGA plane images or write watch.
No full-screen, full-C, or game-identity count follows from this note.

Before this audit, bounded `deja "Poolrad U19 codeword row14 477 pixel mismatch
0899:0104 +02DD pre-input screenshot source"` timed out. The merged
[selected-record source audit](COLD_START_CODEWORD_RECORD_EFFECTS.md) provided
the 477-pixel mismatch and font contract; the
[native key-read branch](NATIVE_COLD_START_U19_KEY_READ_BRANCH.md) provided
the same-run call order and reached `0899:0104` branch. Those claims were
rechecked against the original code and four private capture pairs. The
separate [box source](COLD_START_CODEWORD_BOX_SOURCE.md) supplied the border
boundary; this audit does not revisit the earlier credits border path.

## Original code identity

The ignored DOS `START.EXE` and `GAME.OVR` SHA-256 values are respectively
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Hashes below cover original bytes, before loaded-segment relocation.

| Span | Size | SHA-256 | Bounded observation |
| --- | ---: | --- | --- |
| U19 `+02DD..+02E2` | 5 | `ca3aa4c3b05c30afded7501c51739379fbd4c291e8886a81c097f97313530356` | far call `0899:0104` |
| `RES_0899:0104..0122`, image `[0x8A94,0x8AB2)` | 30 | `1a9b687c96c69b4b791be00bd72730f6df05513ce522a18d53041f8846ce4d93` | reached `+010A` short jump to `+011E` epilogue |
| U19 `+02FD..+0302` | 5 | `32ca391ccf9a6a85b6399ab782aa4a24a56c5030ffca617a8a31265fee3c8541` | far call `0709:09E3` |
| `RES_0709:09E3..0B66`, image `[0x7A73,0x7BF6)` | 387 | `822b4aeac02e3396c177fbdb18187c6040862ec3595d7ffde16b8eb221565baf` | line-input body; near calls at `+0A0A` and `+0B0A` target local `+05EB` |

The reached `0899:0104` jump bypasses its intervening instructions. The
previous key-read audit measured the Return path through `0709:0A38`,
`+0AA1`, and `+0AEC..+0AFD`. The two direct `+05EB` clear calls in this
line-input body push row `0x18` (24), not row 14. That direct call evidence
does not rule out other display effects inside nested calls, emulator
presentation, or a prior EGA write shown later on the host screen.

## Same-run display comparison

Each ignored RAM capture is 655,360 bytes; each screenshot is 640×400 and
hashed after canonical RGB conversion. All four RAM stops retain display
words `DS:[4A16,4A18,4A1A,4A1E] = [A000,A000,0000,0001]`.
The screen source is 320×200, doubled to host RGB pixels. The row-14 source
comparison uses all 36 cells of the U19 fixed Pascal row and the captured
font; the mismatch is 477 source pixels across 19 cells until `+0302`.

| Stop | RAM SHA-256 | Canonical RGB SHA-256 | Row-14 source mismatch |
| --- | --- | --- | ---: |
| before U19 `+02DD` | `84191adc6aab0f6272fe0dc128b1cbff06668a01c9c40fd5fc1202d490b06699` | `5044229ac07b046aaf144113a90e8fec64edcfef47f978d843319991603dabcb` | 477 pixels, 19 cells |
| return `+02E2` | `081901c405f2d67a3e5fa68a34bfa67a536f25b39c9641acfdcc7b5863d66f18` | same as `+02DD` | 477 pixels, 19 cells |
| before U19 `+02FD` | `b4dc553331795b63fb6e8fb6678df59b0a72799f4c386ccdc21ce81b1ec892a3` | same as `+02DD` | 477 pixels, 19 cells |
| return `+0302` | `c35e6bf7920805fe864d5f31aa59b65fa1a321c9e2a6efadc29d1d931f40eeb5` | `b01aed35573d243847675d969712a3606336ca28dd9e2d34885e965c8040a683` | 0 |

The `+02DD→+02E2` and `+02E2→+02FD` screenshots have **zero** changed host
RGB pixels. Across `+02FD→+0302`, **6,612** host RGB pixels change. At row 14,
477 source blocks change in all four host pixels. At row 24, 1,174 blocks
change in all four host pixels and four blocks change in two; the 6,612 total
must not be reduced to a whole number of uniform doubled source pixels.
No other source row changes in this paired comparison.

The measured interval includes the complete `0709:09E3` input call and its
nested callbacks. The first **value-changing guest EGA store** for row 14 is
therefore unresolved. A synchronized native EGA plane snapshot or bounded
write watch at the pre/post stops is required to distinguish a store in this
interval from delayed host presentation of an earlier store. This remains a
source-only attribution gate; no heavy native run was made for this note.

Reproduce the read-only audit with ignored inputs and the copied, ignored
same-run capture pairs:

```sh
python3 tools/replay/cold_start_codeword_row14_boundary_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

The script checks every original span and capture hash, far/near targets,
display words, source-font mismatch, and paired RGB deltas. It prints only
addresses and counts; code bytes, strings, RAM, and pixels remain under
ignored `resources/`.

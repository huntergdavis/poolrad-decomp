# U19 +02DD return frame and page-source boundary

This read-only gate narrows the U19 `+02DD→+02E2` interval to the reached
`RES_0899:0104` skip path and its retained conventional-RAM effect. The
entire 640-KiB RAM difference is six stack bytes containing the saved BP
and far return pointer. The display-page words are unchanged. The RAM
capture excludes EGA planes, so it cannot establish a first value-changing
guest store or explain host presentation timing. No emulator was started.

The initial bounded `deja` attempt for this U19/RES0464 work could not
obtain the local index lock; PM declined a retry under host load. No
indexed finding was reused. The merged [U19 return path](COLD_START_CODEWORD_ROW14_RETURN_PATH.md),
[native key route](NATIVE_COLD_START_U19_KEY_READ_BRANCH.md), and
[page-selector source](COLD_START_CODEWORD_ROW14_PAGE_SELECTOR.md) supply
the caller and page context. The new [focused audit](../../tools/replay/u19_02dd_return_frame_audit.py)
checks the original bytes and RAM pair independently.

## Original call and reached helper path

Ignored DOS v1.3 `START.EXE` and `GAME.OVR` have SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U19 `+02DD..+02E1` is the five-byte far call to original
`RES_0899:0104`, SHA-256
`ca3aa4c3b05c30afded7501c51739379fbd4c291e8886a81c097f97313530356`.
The complete 30-byte helper is expanded START image `[0x8A94,0x8AB2)`;
its contiguous original file range is `[0x8AB3,0x8AD1)` and SHA-256 is
`1a9b687c96c69b4b791be00bd72730f6df05513ce522a18d53041f8846ce4d93`.

At the reached `38D3:02DD` call, the earlier native trace records
`SS:SP=1D16:3DB6`, `BP=3EF8`. The original helper executes `PUSH BP`,
`MOV BP,SP`, `SUB SP,2`, then its `+010A` unconditional jump to `+011E`.
That jump skips the intervening far call and local-write loop. The epilogue
uses `MOV SP,BP`, `POP BP`, `RETF` and returns to `38D3:02E2`. This source
path predicts final `SP=3DB6` and restored `BP=3EF8`; the retained
`+02E2` artifact is RAM/screenshot, not a complete post-return CPU tuple.
The `SUB SP,2` can change arithmetic FLAGS, which this gate does not compare.

## Same-run RAM and page evidence

The retained `+02DD` and `+02E2` 655,360-byte RAM images hash to
`84191adc6aab0f6272fe0dc128b1cbff06668a01c9c40fd5fc1202d490b06699`
and `081901c405f2d67a3e5fa68a34bfa67a536f25b39c9641acfdcc7b5863d66f18`.
Exactly six bytes differ, all at physical `[0x20F10,0x20F16)` or
`SS:3DB0..3DB5`. At the return stop those stale stack words are:

| Offset | Retained word | Original source role |
| --- | ---: | --- |
| `SS:3DB0` | `3EF8` | helper's pushed caller BP |
| `SS:3DB2` | `02E2` | far return IP pushed by U19 call |
| `SS:3DB4` | `38D3` | far return CS pushed by U19 call |

No byte changes elsewhere in the conventional RAM image. In particular,
`DS:[4A16,4A18,4A1A,4A1E]` remains
`[A000,A000,0000,0001]` at both stops. The reached helper path contains
no direct RES_0709 active-page or draw-page call, no EGA store, and no
Graphics Controller or Sequencer write. The original `RES_0709:+0071`
active-page helper and `+00A0` draw-page helper therefore do not run
**inside this bounded call**. This does not exclude earlier, later,
nested elsewhere, or asynchronous host display behavior.

The retained +02DD and +02E2 canonical host RGB images are byte-identical
and each still has 477 row-14 source-pixel mismatches across 19 cells,
as the merged paired-capture audit checks. This new audit does not reopen
those images. Its 640-KiB RAM pair cannot reveal EGA `A000` plane changes,
hidden latches, the host CRTC/page selection, or the live RGB update rule.

| Parity-review field | Finding |
| --- | --- |
| `target_function` | Reached U19 `+02DD` far-call return to `+02E2` |
| `source_candidates` | Original `RES_0899:0104` skip path and the retained same-run RAM pair |
| `signals` | Hash-locked call/helper, skip to epilogue, exact six-byte frame, unchanged page words |
| `score` | 4/4 bounded source and RAM-frame checks passed; complete CPU/EGA state unavailable |
| `verdict` | `match` for the bounded stack-memory explanation; `unclear` for post-return CPU flags or EGA/host timing |
| `review_notes` | No C translation, first writer, live page-helper frame, or full-function parity follows |

Run the focused audit with ignored originals and the retained same-run
captures:

```sh
python3 tools/replay/u19_02dd_return_frame_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001
```

All original media, RAM, EGA planes, and screenshots remain under ignored
`resources/`. This source gate adds no native, full-C, game-identity,
page-flip, or first-writer credit.

# U19 row-14 page-return and host-RGB handoff contract

This source-only follow-on makes the next same-run page-selector observation
reviewable. It extends the merged [page-call gate](COLD_START_CODEWORD_ROW14_PAGE_CALL_GATE.md)
with the far-call return frame, exact before/after stops, and host-RGB
correlation points. There is no new native capture, observed U19 page-helper
call, measured page flip, or host presentation timing here.

The bounded deja query `Poolrad U19 row14 RES0709 0074 0087 0099 00B0 same
run page selector return host RGB handoff` returned no indexed match. The
merged [page-selector audit](COLD_START_CODEWORD_ROW14_PAGE_SELECTOR.md) and
[row-14 boundary](COLD_START_CODEWORD_ROW14_BOUNDARY.md) supplied the
original/loaded mapping and retained endpoint counts. Their source spans and
helper instruction boundaries were independently rechecked against ignored
START.EXE; no game bytes or captured pixels are copied here.

## Pinned helper identity and frame

The startup selection image `[0x4426,0x44B5)` is 143 bytes, SHA-256
`eb428dc50cedede319fd7f0fc936ba592548a3125d07a100e7d08176ec21457b`.
Its direct calls at image 0x4466 and 0x446F target RES_0709:0071 and :00A0.
Those startup callsites do not prove another helper invocation inside U19.

| Helper | Original image | Bytes | Original SHA-256 | Loaded/source effect |
| --- | --- | ---: | --- | --- |
| Active +0071..+009F | `[0x7101,0x7130)` | 47 | `26854c57a4d438a0e37a6c00ab8a7a6b9bff9567da09f681ee4247aeccd42b77` | At +0087 far-call runtime `1256:000B` after relocation; +0099 stores DS:4A16 |
| Draw +00A0..+00B6 | `[0x7130,0x7147)` | 23 | `6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d` | +00B0 stores DS:4A18 without a BIOS call |

The loaded active span SHA-256 is
`62df52ae567e90dc75b8b235ec70a39cae47256482d2af99bd5d4c4394417310`;
the only original/loaded difference is the runtime call segment at +0087,
original `0A32:000B` versus loaded `1256:000B`. The loaded draw span is
byte-equal to its original. Both helpers end with `RETF 2`.

After each helper's `PUSH BP; MOV BP,SP` prologue, `SS:[BP+2]` and
`SS:[BP+4]` hold the far caller return IP and CS, and `SS:[BP+6]` holds the
WORD argument. The helpers use **only its low byte** as page index. Record
this frame at active +0074 and draw +00B0, then verify it is stable through
their respective stores. A helper hit with no resolved caller chain to the
U19 interval remains a helper observation, not U19 reachability.

## One-run capture order and interpretation

Use one private run token and monotonically numbered stops. Capture full
CPU, SS:SP/BP and the far caller frame, 640-KiB RAM, DS:4A02 register
record, DS:4A16..4A1E display words, EGA controller/map state, the four
`A000:[1180,12C0)` plane windows, and canonical 640x400 host RGB where
listed. Keep a complete bounded hit log for these PCs, including page
stores that repeat an existing value.

| Stop group | Required same-invocation stops | What can be checked |
| --- | --- | --- |
| Active-page request | `0F2D:0074`, `:0087`, `:008C` | At +0087 DS:4A02 request AX should be `0x0500 | low([BP+6])`; +008C is the actual return from the relocated runtime far call. Capture its CPU and device/BIOS page state; the RAM request word alone does not prove hardware selection. |
| Active-page segment store | `0F2D:0099`, `:009C` | Before/after DS:4A16; after should be the 16-bit result `0xA000 + (low([BP+6]) << 9)`. A same-value write is still a reached store. |
| Draw-page segment store | `0F2D:00A0`, `:00B0`, `:00B3` | +00A0 is helper entry; at +00B0 the BP frame is established. Before/after DS:4A18; after uses the same 16-bit formula. No BIOS call occurs in this helper. |
| U19 handoff | before `38D3:02D8`, then `38D3:02DD`, `:02E2`, `:02FD`, `:0302` | Tie each helper's return frame and chronological hit to the U19 caller interval. Retain RGB and all four plane windows at each endpoint. |

At +0087/+008C, +0099/+009C, +00B0/+00B3, and the U19 endpoints,
save the private RGB image and a SHA-256 of its decoded 640x400 RGB bytes.
Compare full-frame changed-pixel counts and the four doubled host pixels
covering source `(122,112)` (`x=244..245`, `y=224..225`). Pair those
observations with page words, actual device page state if exposed, and the
four-plane witness `A000:118F` bit `0x20`. Restore the Graphics Controller
selector/read map after every plane read. If the device page state or a
safe plane snapshot is unavailable, report that specific blocker. An RGB
change between stops gives an interval; it does not alone identify the
guest instruction or host presentation time.

The retained four U19 endpoints all have DS:4A16/4A18=`A000/A000`, page
indexes 0/0, adapter 1, video modes 03/0D, and DS:4A02=`0500`. Their
DS:4A16..4A1E SHA-256 is
`52c20cdb7fceeb8f44f77d9d25201b13b4e056958430161515115c310de4d9d3`.
Host row-14 mismatch counts are `477/477/477/0`; successive full-host RGB
change counts are `0/0/6612`. These endpoints cannot exclude an
intervening same-value page call and contain no synchronized EGA planes.

The existing read-only source/capture audit is the focused local check:

    python3 tools/replay/cold_start_codeword_row14_page_selector_audit.py \
      --exe resources/DOS/POOLRAD/START.EXE \
      --ovr resources/DOS/POOLRAD/GAME.OVR \
      --run resources/oracle-runs/codeword-row14-p5-001

All original media, stack/RAM captures, EGA planes, and RGB images stay under
ignored `resources/`. This contract gives no native first-writer, observed
U19 page invocation, page/host timing, full-frame, C, or identity credit.

# U19 row-14 page selector and presentation source boundary

This source-only audit separates the drawer's page-segment reads from the
resident BIOS active-page selection. Four retained U19 stops have equal display
words, but none captures synchronized EGA planes or a page-helper instruction
stop. They do not prove a live page flip or explain when the host presented
the fixed row-14 pixels. No new native run was made.

Before analysis, the bounded deja query for the Poolrad U19 row-14 page
selector and DS:4A16/4A18 presentation source timed out after 20 seconds.
The merged
[video initialization](RESIDENT_VIDEO_INIT_4426.md),
[row-14 boundary](COLD_START_CODEWORD_ROW14_BOUNDARY.md), and corrected
[plane-watch note](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md) supplied the
startup mode, same-run endpoints, and drawer mapping. Their relevant original
and loaded addresses were independently checked here.

## Source identity and reads

The ignored START.EXE unpacked image and GAME.OVR hashes are checked by the
focused audit. Image [0x4426,0x44B5) is 143 bytes, SHA-256
eb428dc50cedede319fd7f0fc936ba592548a3125d07a100e7d08176ec21457b.
Its far calls at image 0x4466 and 0x446F target RES_0709:0071 and :00A0.
The earlier video note's stated 144-byte length for this half-open range is
one byte too long; this audit uses the exact 143-byte slice.

| Resident span | Original image | Bytes | SHA-256 | Source effect |
| --- | --- | ---: | --- | --- |
| +0071..+009F active page | [0x7101,0x7130) | 47 | 26854c57a4d438a0e37a6c00ab8a7a6b9bff9567da09f681ee4247aeccd42b77 | +0074 sets register AH=5; +007C sets AL to page; +0087 calls runtime Intr(10h); +0099 stores A000+(page<<9) at DS:4A16 |
| +00A0..+00B6 draw page | [0x7130,0x7147) | 23 | 6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d | +00B0 stores A000+(page<<9) at DS:4A18; no BIOS call in this helper |

The loaded active helper at 0F2D:+0071..+009F has SHA-256
62df52ae567e90dc75b8b235ec70a39cae47256482d2af99bd5d4c4394417310.
It differs from the original only at relative offsets 0x19/0x1A: the
+0087 far-call runtime segment relocates from 0A32 to 1256, with offset 000B
unchanged. The loaded draw helper is byte-equal to the original.

The hash-locked drawer reads DS:4A1E at +0238 to choose video mode. Before
glyph stores, +040A loads ES from DS:4A16 and +0430 from DS:4A18. Before
zero stores, +043D and +0449 make the same respective reads. All four
eight-byte read/setup spans are byte-equal in the captured loaded image.
Those are source-level page choices immediately upstream of the four store
PCs +0412/+0438/+0445/+0451; a page word of A000 alone cannot identify which
physical plane or host page changed.

## Retained endpoint and bounded negative

At U19 +02DD, +02E2, +02FD, and +0302, the nine-byte DS:4A16..4A1E state
has SHA-256
52c20cdb7fceeb8f44f77d9d25201b13b4e056958430161515115c310de4d9d3:
active/draw segments A000/A000, draw/active indexes 0/0, previous/current
mode 03/0D, adapter mode 01. The register record AX at DS:4A02 is 0500 at
all four stops. The host row-14 source mismatch is 477/477/477/0 pixels
and the three successive host RGB deltas are 0/0/6612.

The original input body RES_0709:09E3..0B65, image [0x7A73,0x7BF6),
387 bytes, SHA-256
822b4aeac02e3396c177fbdb18187c6040862ec3595d7ffde16b8eb221565baf,
and U19 +02D8..+0302, 42 bytes, SHA-256
518c6f63fa4c0ed6ff7dcf6b986fb4748ecf3e51a366f9005685d7f7e630a00c,
contain no exact direct far-call encoding to either page helper. This
bounded byte scan excludes neither nested or indirect calls nor a same-value
page call. Equal endpoint words and AX also do not prove no intervening call.

The first **identified source-level BIOS active-page decision** is therefore
RES_0709:+0074..+008C, reached by the startup selection's direct +4466
call. Its execution inside U19 +02DD..+0302 is unproved. The drawer's
DS:4A16/4A18 reads are the first identified source page-segment selections
at the row-14 store boundary. Neither source fact proves which page or plane
the host displayed at the paired stops.

## Next native discrimination gate

At one same-run stop before U19 +02D8, capture CPU, DS, the full register
record, DS:4A16..4A1E, Sequencer map mask, four A000 plane windows covering
the row-14 witness at A000:118F, and host RGB. Watch live RES_0709:+0074,
+0087, +0099, +00A0, and +00B0 for page-helper entry/return and same-value
selection. Watch drawer +0238, +040A, +0430, +043D, +0449 and the four store
PCs to bind each effective ES:DI, plane mask, and pre/post plane byte.
Repeat at U19 +02DD, +02E2, +02FD, and +0302. Restore Graphics Controller
read-map state after each plane capture as the
[plane-watch recipe](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md) requires.
Only synchronized plane and page observations can separate a guest store from
delayed host presentation. Heavy native remains held.

Reproduce the source/capture audit using only ignored inputs:

    python3 tools/replay/cold_start_codeword_row14_page_selector_audit.py \
      --exe resources/DOS/POOLRAD/START.EXE \
      --ovr resources/DOS/POOLRAD/GAME.OVR \
      --run resources/oracle-runs/codeword-row14-p5-001

Only addresses, hashes, and counts are public. Original bytes, captured RAM,
EGA planes, and pixels remain under ignored resources/. No full-frame, C, or
identity credit follows from this note.

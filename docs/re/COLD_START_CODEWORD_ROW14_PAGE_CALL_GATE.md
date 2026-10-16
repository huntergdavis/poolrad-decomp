# U19 row-14 page call and host presentation gate

This source-only gate refines the active/draw page helpers identified in the
[page-selector audit](COLD_START_CODEWORD_ROW14_PAGE_SELECTOR.md). It does not
establish that either helper ran between retained U19 stops, that the displayed
page changed, or when the host presented the fixed row-14 pixels. Heavy native
capture remained held.

The bounded deja query `Poolrad U19 row14 RES0709 0074 0087 0099 00B0 page
call return host presentation` returned no matching indexed session. The
page-selector audit and [row-14 boundary](COLD_START_CODEWORD_ROW14_BOUNDARY.md)
were reused for the exact source spans and four same-run endpoint states;
the instruction boundaries below were independently decoded from the ignored,
hash-locked START image.

## Exact source boundary

The START image [0x4426,0x44B5) is 143 bytes with SHA-256
`eb428dc50cedede319fd7f0fc936ba592548a3125d07a100e7d08176ec21457b`.
Image far calls at 0x4466 and 0x446F target RES_0709:0071 and :00A0.

| Helper | Original image | Length | SHA-256 | Bounded behavior |
| --- | --- | ---: | --- | --- |
| Active page +0071..+009F | [0x7101,0x7130) | 47 | `26854c57a4d438a0e37a6c00ab8a7a6b9bff9567da09f681ee4247aeccd42b77` | BIOS page request, then DS:4A16 store |
| Draw page +00A0..+00B6 | [0x7130,0x7147) | 23 | `6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d` | DS:4A18 store, without a BIOS call |

The loaded active helper SHA-256 is
`62df52ae567e90dc75b8b235ec70a39cae47256482d2af99bd5d4c4394417310`.
Only the runtime far-call segment at +0087 differs from the original: its
target is relocated from 0A32:000B to 1256:000B. The loaded draw helper is
byte-identical to the original. The checked-in page-selector audit verifies
these identities against an ignored retained RAM image.

The active helper reads the low byte of its `[BP+6]` argument at +0079. At
+0074 it sets the high byte of the DS:4A02 request word to 5, and +007C
sets its low byte to the page index. At +0087 it far-calls the runtime BIOS
bridge; +008C is the first instruction after that call. The helper reloads
the low argument byte at +008C, shifts it left nine at +0094, adds A000 at
+0096, stores the resulting segment to DS:4A16 at +0099, and reaches the
next instruction at +009C. It returns at +009D. The draw helper makes the
same low-byte shift/add and stores DS:4A18 at +00B0, with +00B3 as its next
instruction and +00B4 as its return. The argument high byte is not used by
these computations. These are instruction-level source effects; runtime BIOS
and host presentation effects remain unmeasured.

## Retained endpoints and next capture

The same-run U19 +02DD, +02E2, +02FD, and +0302 RAM endpoints all have
active/draw segments A000/A000, indexes 0/0, video modes 03/0D, adapter 01,
and DS:4A02=`0500`. Their nine-byte DS:4A16..4A1E region has SHA-256
`52c20cdb7fceeb8f44f77d9d25201b13b4e056958430161515115c310de4d9d3`.
The host row-14 mismatch counts are 477/477/477/0 and successive host RGB
change counts are 0/0/6,612. Equal RAM endpoints cannot exclude a same-value
page-helper call; RGB counts cannot identify a guest page flip or presentation
time.

In one future bounded run, retain a unique run identifier and chronological
CPU/DS, `[BP+6]`, DS:4A02 and DS:4A16..4A1E, four EGA plane windows around
A000:118F, Graphics Controller/Sequencer state, and host RGB with each stop.
For an active-page invocation, pair +0074 before the request, +0087 before
the runtime call, +008C after its return, and +0099/+009C before/after the
segment store. For a draw-page invocation, pair +00A0 entry and +00B0/+00B3
before/after its store. Record the caller return frame to connect a helper
invocation to the U19 interval; the startup direct calls alone do not do so.
Preserve a complete hit log for these watched PCs, including same-value
stores. Take the U19 +02DD/+02E2/+02FD/+0302 endpoints in the same run and
restore the GC read-map selector after every plane snapshot. Only this
same-run evidence can correlate a helper call, selected page, guest plane
change, and host RGB presentation; the previous endpoints alone cannot.

The source/capture audit is read-only and uses ignored inputs:

    python3 tools/replay/cold_start_codeword_row14_page_selector_audit.py \
      --exe resources/DOS/POOLRAD/START.EXE \
      --ovr resources/DOS/POOLRAD/GAME.OVR \
      --run resources/oracle-runs/codeword-row14-p5-001

Only addresses, hashes, and counts are public. Original bytes, RAM, EGA
planes, and pixels remain under ignored `resources/`. This note claims no
full-frame or strict-C credit.

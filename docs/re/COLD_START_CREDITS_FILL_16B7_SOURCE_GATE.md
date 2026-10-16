# Cold-start credits FillChar `+16B7`: source operands and capture gate

This source-only audit narrows the possible first credits box clear to the
SYSTEM `RES_0AF8:+16B7` `REP STOSB` instruction. It pins the original
operands and the rectangle caller's local EGA mask writes. No native credits
stop at `+16B7`, first live writer, guest timing, additional frame parity, or
full-C game-function credit follows from this evidence.

Before analysis, bounded `deja "Poolrad RES_0AF8 +16B7 credits plane store
mask first-write capture"` failed because local `index.db.lock` was
read-only. This audit reuses the checked-in [first credits box caller
chain](NATIVE_COLD_START_CREDITS_WRITER_OWNERSHIP_GATE.md), the
[rectangle-fill contract](RES_0709_RECT_FILL_00B7.md), and the
[native EGA capture conventions](RES464_16A1_NATIVE_CAPTURE_RECIPE.md).
PM cleared the `+16B7` operand/mask scope against p9's U17 source notes and
p5's U19 row-14 watch recipe before edits. No emulator was started.

## Original-image contract

The ignored DOS v1.3 `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The [resident loader map](../../tools/overlay/resident_loader_map.py) expands
it and tracks each byte's original file position. The
[read-only auditor](../../tools/replay/cold_start_credits_fill_16b7_source_audit.py)
checks that each span below is contiguous in the original file, equals its
expanded image bytes, and has the stated hash. The different file shifts
are intentional loader provenance; one global image-to-file subtraction
would be wrong.

| Original span | Expanded image | START file | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0709:+0000..+0071` EGA reset | `[0x07090,0x07101)` | `[0x070C9,0x0713A)` | 113 | `5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5` |
| `RES_0709:+0103..+010F` mode setup | `[0x07193,0x0719F)` | `[0x071CC,0x071D8)` | 12 | `1bc31bc16742523ae70e987f6d344bda3144ee1717f2230d5ea6f0362dd06337` |
| `RES_0709:+0124..+0145` fill-call setup | `[0x071B4,0x071D5)` | `[0x071ED,0x0720E)` | 33 | `58ac0462a1c5d546fa5771ef003f227fa20acb99e6815a2858d21ae15a122df3` |
| `RES_0AF8:+16A8..+16BC` FillChar body | `[0x0C628,0x0C63C)` | `[0x0C31E,0x0C332)` | 20 | `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7` |

The two-byte `+16B7..+16B9` opcode is `F3 AA`, SHA-256
`50b2d3be54fb620b49268d3b4bd0c759531794c4c735fc2916071807cf9ee7a6`.
It is START file `[0x0C32D,0x0C32F)` and loaded image offset
`(resident segment << 4) + 0x16B7`; prior retained RAM used loaded segment
`131C`, but a future run must read its relocated call target rather than
assume that segment.

| Source site | Decoded effect |
| --- | --- |
| `RES_0709:+0057..+0062` | Select GC register 8 and write bit mask `FF`. |
| `RES_0709:+0063..+006E` | Select Sequencer register 2 and write map mask `0F`. |
| `RES_0709:+0103..+010E` | Select GC register 5 and write mode value `02`. |
| `RES_0709:+0124..+012D` | Load drawing segment from `DS:4A18`, pass it as `ES`, and push `ES:DI` as far destination. |
| `RES_0709:+012E..+013B` | Compute inclusive row width `right-left+1` from low-byte arguments and push it as `CX`. |
| `RES_0709:+013C..+0140` | Push fill color from `SS:[BP+06]`; far-call original `0AF8:16A8`, returning at `RES_0709:+0145`. |
| `RES_0AF8:+16A8..+16B9` | `BX=SP`; `LES DI,SS:[BX+8]`; `CX=SS:[BX+6]`; `AL=SS:[BX+4]`; `CLD`; `REP STOSB ES:[DI],AL`; `RETF 8`. |

The first U17 box uses inclusive cell rectangle `(1,1)..(38,3)`, color
zero, and the `RES_03F1:+0137` mode-1 page-0 arm before its page-1 arm,
as pinned by the existing caller note. **If** that arm reaches this fill
body with its documented page state, the first row call expects
`ES:DI=A000:0141`, `CX=38`, `AL=0`; the rectangle covers screen scanlines
8–31 and byte columns 1–38, or 912 CPU byte-store iterations per page.
These are source-derived expectations, not observed credits registers or
measured hardware stores. The local reset does not capture current GC6,
Sequencer 4, memory handler, bank, or display-page state. Seq2=`0F`,
GC8=`FF`, and GC5=`02` are source writes, not a native register receipt.
Under a confirmed standard unchained write-mode-2 mapping, zero color
would clear each enabled plane byte; that mapping remains to be measured.

## Future first-write capture gate

This is a recipe for a separately authorized native slot **after** the
heavy-native hold. It is not a capture receipt.

1. Use a new ignored `resources/` run with the pinned originals and retain
   the full debugger transcript. Break at the loaded U17 first-box far call
   corresponding to original `+022D`. Verify its opcode and rectangle
   arguments, then the nested `RES_03F1:+0311`, `+01F1`, and
   `RES_0709:+0140` calls. At `+0140`, record the actual relocated SYSTEM
   target and require the expected return frames before attributing any
   later `+16B7` stop to the first credits box. A `+16B7` hit alone could
   belong to another FillChar caller.
2. Break at that *measured* loaded `+16B7` target before execution. Before
   changing any VGA selector, record `EV CS IP DS ES SS SP BP AX BX CX DX SI
   DI FLAGS`, the relevant stack words, `DS:4A18/4A1A/4A1E`, `VGA GC`,
   `VGA SEQ`, and `VGA MODE`. Require `CX>0` and verify actual `ES:DI`,
   `AL`, direction flag, GC5, GC8, Seq2, GC6, Seq4, and handler/bank state.
   Stop as unmeasured if the debugger cannot expose the latter state.
3. Preserve four pre-write plane bytes at the actual selected destination
   and four immediate post-write plane bytes using a non-mutating host-side
   plane tap. If only debugger read-map dumps are available, preserve the
   original GC selector/GC4, select read maps 0–3, save each overwritten
   `MEMDUMP.BIN` under ignored `resources/`, and restore both values before
   resuming. Treat those dumps as diagnostics until the read-map procedure's
   effects on latches and mapping are excluded.
4. A data `BPM` may report an initial baseline event, and a changed-byte
   watch can miss same-value writes. Record the event type and guest PC,
   then distinguish CPU write attempts from four-plane **value changes**.
   To claim the first executed graphics write after U17 entry, instrument
   all relevant VGA write attempts in order, including same-value stores,
   and link the earliest event to the guarded call stack. A single BPM or
   pre/post page delta cannot establish that order.

Any future sample, stack, EGA state, plane dump, or screenshot stays under
ignored `resources/`. The current source gate leaves the live first writer,
visible-page selection, and natural title-to-credits interval unresolved.

Run the bounded source check with:

```sh
python3 -B tools/replay/cold_start_credits_fill_16b7_source_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
```

# Unit 0x25 entry 19: one party-position step

This is a bounded DOS v1.3 movement contract and C translation for
`GAME.OVR` unit `0x25:+0x19DB`, public entry 19 (runtime stub
`0x2B:0x007F`). The local input is the party's facing byte; the routine
writes one party coordinate, then refreshes two map lookup results. It is
one piece of the slot-A Up route, **not** a runnable native movement loop.

The prior-session query `deja "poolrad DOS slot A Up input movement unit 0x68
entry 1 2 unit 0x1c 0x32f8 party position time"` returned no reusable
session result. I reused and rechecked the address map in
[GAMEPLAY_PATH.md](GAMEPLAY_PATH.md), the slot-A frame layout in
[DOS_SAVGAM.md](../formats/DOS_SAVGAM.md), and the translated unit `0x3BD`
[cell](U3BD_CELL_ENTRY4.md) and [nibble](U3BD_NIBBLE_ENTRY2.md) contracts.
The bytes and call edges below were independently checked for this branch.

## Pinned input and extent

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x25` starts at file `0x74D3`. Entry 19 starts at file `0x8EAE` and
returns at file `0x8F37` (`+0x1A64`): **138 decoded code bytes** through
`retf`, SHA-256
`2b25f7430c81bea5d725954276eb2c8fba6762abc315f7c29d9322a7d2ef4622`.
The ledger's 148-byte candidate bound includes ten trailing non-code bytes;
the C model covers the 138-byte code span. `objdump -D -b binary -m i8086`
was started at the known entry boundary, and calls were reconciled with
`tools/overlay/callgraph.py` using the pinned inputs. No original bytes or
game text are committed.

## Entry 19 contract

`[0x6AAD]` is the x byte, `[0x6AAE]` the y byte, `[0x6AAF]` facing.
The compares are signed byte compares; the coordinate stores are bytes.

| Facing | Original instruction span | Coordinate write |
| ---: | --- | --- |
| 0 | `+0x19DE..+0x19F8` | `y > 0 ? y - 1 : 15` |
| 2 | `+0x19F9..+0x1A10` | `x < 15 ? x + 1 : 0` |
| 4 | `+0x1A11..+0x1A28` | `y < 15 ? y + 1 : 0` |
| 6 | `+0x1A29..+0x1A3E` | `x > 0 ? x - 1 : 15` |
| other | `+0x1A2B` branch to `+0x1A3F` | neither coordinate changes |

At `+0x1A3F..+0x1A4F`, it pushes the **new** x/y bytes, calls unit
`0x3BD` entry 4 (stub `0x3D5:0x34`) and stores AL at `[0x6AB1]`.
At `+0x1A4F..+0x1A64`, it pushes x/y/facing, calls unit `0x3BD` entry 2
(stub `0x3D5:0x2A`) and stores AL at `[0x6AB0]`, then returns. The C
translation calls the already merged lookup functions in that order.
Entry 2's unmatched-facing result is an unwritten stack byte, modelled as
an explicit `nibble_frame_residue`; real facing writers are bounded to
0, 2, 4 and 6 in [U3BD_NIBBLE_ENTRY2.md](U3BD_NIBBLE_ENTRY2.md).

The owner-held slot-A frame has selector `[0x49FA] = 1` and party bytes
`(15,4,6)`. For entry 19 with those bytes, the direct position write is
`(14,4,6)`, matching the original DOS oracle's displayed one-Up step.
This verifies the writer's **conditional effect**; the screen observation
alone does not prove that entry 19 was executed for that particular keypress.

## Connected call path and remaining gate

The pinned, decoded path is:

1. Unit `0x68` entry 2 at `+0x0989` checks `[0x49FA]`. With selector 1,
   its extended-key path compares scan code `0x48` at `+0x0AAD` and calls
   internal `+0x068D` at `+0x0AB1`. That helper calls unit `0x3BD` entry 3
   at `+0x06C0`, derives candidate coordinates in locals, and handles
   out-of-bounds cases. It does not perform the ordinary in-bounds x/y
   write. Entry 2 returns the key byte at `+0x0D45`.
2. The gameplay loop, unit `0x1C:+0x32F8`, receives that byte at
   `+0x3395`. The non-`E` route can run the ECL entry `[0x49E3]` at
   `+0x3453`, then copies party x/y into window-0 words `0xF0`/`0xF1`
   at `+0x3473` and calls unit `0x68` entry 1 at `+0x348D`. This is an
   address and branch contract; the ECL side effects remain necessary.
3. The fixup-aware overlay call graph finds one direct incoming overlay call to entry 19:
   unit `0x1C:+0x2DA8`, inside the ECL opcode `0x2D` (`CALL`) handler
   `+0x2CCF`. That handler invokes entry 19 only when the operand word
   assembled by unit `0x25` entry 9 is `0xC01E`: subtracting `0x7FFF`
   then gives the compared value `0x401F` (`+0x2DA3..+0x2DA8`). Other
   operand values take different branches.

A read-only check of the ignored slot-A world frame's saved ECL buffer
decodes its five entry addresses as `0x9914`, `0x9975`, `0x9A0E`, `0x9A49`,
`0x9A5E`. At logical script address `0x9945` (buffer offset 69) sits an
opcode `0x2D` with one tag-`0x01` operand whose stored low/high bytes
assemble to `0xC01E`. The loader copies those bytes to operand slot 1;
this **particular handler** calls unit `0x25` entry 9 to assemble them
directly. It does not call the tag-aware slot-value helper, so no
dereference changes this handler's operand. The conditional dispatch to
entry 19 is therefore byte-backed *if execution reaches `0x9945`*.
These are addresses, tags and derived values only; the saved script is
not published.

**Next integration gate:** show that the restored slot-A Up-key ECL run
reaches `0x9945`, then account for the game's time increment and the
entry-1/search effects. The available DOS screenshot confirms position and
displayed minute before/after Up; it is not a memory-level trace of that
ECL PC. Native save/load/move/save-reload also needs the complete
restore and writeback paths. None of those gaps earns strict full-C credit
here.

## Verification

`tests/recovered/test_overlay_u25_move.c` uses synthetic map bytes. It
checks the slot-A coordinate tuple, all four directions at ordinary and
boundary cells, signed out-of-range inputs, ordered map lookup outputs,
map-0/10 early gates, and the unmatched-facing frame residue. A direct
`cc -std=c11 -Wall -Wextra -Wpedantic -Werror` build of this source and
the two lookup sources passes; CMake's focused test is the PR gate.
No original save or executable bytes enter the test or repository.

## Parity review

| Field | Finding |
| --- | --- |
| `target_function` | `GAME.OVR` unit `0x25:+0x19DB`, code file `0x8EAE..0x8F37` |
| `source_candidates` | `src/recovered/overlay_u25_move.c:pr_u25_entry19` |
| `signals` | Exact entry and `retf` extent; four facing constants and signed boundary branches; byte stores to x/y; ordered fixup-confirmed calls to unit `0x3BD` entries 4 and 2; AL stores to `[0x6AB1]` and `[0x6AB0]`; synthetic boundary and lookup checks |
| `score` | 9/10 for the isolated routine on defined map-buffer inputs; no CPU-level differential oracle yet |
| `verdict` | `match` for the isolated function's defined path; `unclear` for the full Up-to-time route |
| `review_notes` | The C API reports missing host map memory instead of attempting the original far-pointer access; unmatched facing passes entry 2's unwritten frame byte as an explicit input. The slot-A script contains the conditionally matching `CALL`, but Up-to-`0x9945` reachability has not been proved. No time writer or complete native ECL effects are claimed. |

## Reproduce (original-byte checker)

`OVL_0025_19DB` is checked against the original bytes by the registered checker `tools/overlay/u25_move_original_audit.py`.
It replaces the cited shared tool, `callgraph.py`, as this body's re-runnable evidence (VP i21878, PM i21879).

**Original side.** The original `+19DB` runs under Unicorn. Its two far calls go through overlay stub `03D5` (unit
`3BDh`) in its loaded state: entries 4 (`+057A`, cell) and 2 (`+047B`, nibble) hold `JMP FAR` to the unit's segment, as
the overlay manager writes them. Units `25h` and `3BDh` load at their own segments with every fixup applied, and the
relocated START.EXE supplies the stub and data segments.

**C side.** `pr_u25_entry19` runs, composing the translated 3BD entries, through `u25_move_original_shim.c`.

**Compared, per call:**

- the five state bytes: `[6AAD]` x, `[6AAE]` y, `[6AAF]` facing, `[6AB1]` cell, `[6AB0]` nibble;
- the whole 64 KiB data segment, which must change in exactly those bytes;
- the model's status: OK for every matched facing; UNSPECIFIED only for an unmatched facing that reaches entry 2's
  selector dispatch.

**The one supplied value.** On the unmatched path, entry 2 returns its unwritten `[BP-1]` stack byte. The model declares
that byte as a host input (`nibble_frame_residue`). The checker reads it from the original at the instruction that loads
it (`3BD:+0571`) and passes the same value in. Nothing else is supplied, and nothing is masked.

**Result.** All 3,000 cases pass: 2,030 with status OK and 970 UNSPECIFIED, the unmatched-facing residue path.

**Fixtures.** Random x and y, with the edges 0, 15, 16, 7Fh, 80h and FFh. Facing is one of 0/2/4/6 or an unmatched
value. The map id and the 1 KiB map buffer at far `[6A5C]` are random.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/u25_move_original_audit.py --output resources/u25-19db-audit/report.json
python3 tools/overlay/u25_move_original_audit.py --compile-only   # CHECKER_BUILD_OK, no media
```

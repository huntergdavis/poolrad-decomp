# Saved slot-A output handoff to the first gameplay view refresh

One native DOS v1.3 saved-slot replay passed the unit `0x6F:+196F` caller,
copied its returned string into the caller's basename buffer, and later
reached the unit `0x1C:+3382` view-refresh call. Direct debugger stops prove
the loaded far-call stub, unit `0x3B4` entry-1 target, return frame, and
return to `+3387`. The emulator window changed by 628 RGB pixels across
that call. This is a bounded native route observation, not an exhaustive
trace of the intervening loader, a C display parity result, or a new game
routine count.

Before tracing, `deja "Poolrad slot A returned +196F output caller first
view refresh"` found no exact session, while `deja "196F"` surfaced the
earlier `+196F` caller/table leads. I used the merged
[`+196A` resident return](SLOT_A_NATIVE_U6F_196A_RESIDENT_RETURN.md),
[`+0425` output](SLOT_A_NATIVE_0425_RETURN_OUTPUT.md),
[`+1983` filename](SLOT_A_NATIVE_1983_FILENAME.md),
[`+1C08` piece gate](SLOT_A_NATIVE_1C08_PIECE_GATE.md), and
[`+334D` restore gate](SLOT_A_NATIVE_RESTORE_FLAG_GATE.md) as route leads.
The original and loaded bytes, live branch inputs, far target, frame, and
screen change below were checked independently in this replay.

## Pinned inputs and code

All original media, save, full-RAM images, debugger output, and screenshots
are ignored under `resources/oracle-runs/slot-a-native-first-view-049/`.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| DOS v1.3 `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The unit `0x6F` code begins at `GAME.OVR[0x1DFE6]`. The unit `0x1C`
code begins at file `0x0D46`, and unit `0x3B4` begins at `0x2DB87`.
The original overlay descriptor has stub segment `0x03CA` and entry 1
offset `+016D` for unit `0x3B4`. The 29-byte `+336A..+3386` caller span
is seven complete i8086 instructions: two byte comparisons and conditional
branches, a resident far call, a byte store, and the view far call.

| Original / loaded code span | Bytes | SHA-256 |
| --- | ---: | --- |
| Unit `0x6F:+196F..+1973`, file `0x1F955..0x1F959` | 5 | `fe81385603a3fc55df47de14941cf1bdbf72d0bd7c936edaecb4ceda0c23d52e` |
| Loaded `2A8E:196F..1973` | 5 | `78cec5fcdde872ab5e4c4add6d5debd1f4cf3d3430f3e12b1a051c4ce061d147` |
| Unit `0x6F:+1974..+1982`, file `0x1F95A..0x1F968` | 15 | `40340a7f4f937729d3786c40ce28c8cc26d796f9abd977727eaafe0a254b1253` |
| Loaded `2A8E:1974..1982` | 15 | `85ccb6667d3a08f5fe27f10b74ee586c9487648ced403928853c8d8c06d4aa6c` |
| Unit `0x1C:+336A..+3386`, file `0x40B0..0x40CC` | 29 | `6f90c7c01bc467148af435b6348d20ae6e97d7bb2ad8e08d3a4b0f26a3c1cf48` |
| Loaded `3582:336A..3386` | 29 | `cfc00b5e4f51e5754d3bfa8aed995187e14e1ad59f76afcddf8fe43743fdda86` |
| Loaded far stub `0BEE:0025..0029` | 5 | `edeabeacb40dbdec68484c4dea4f72e0c4a64f80d020f2d1e068e71f7e03d025` |
| Unit `0x3B4:+016D..+017C`, file `0x2DCF4..0x2DD03`; loaded `2E35:016D..017C` | 16 | `1cf21bcb074db68d9bf2e7461b2e3db63fe9e9f5e9223de262b12a7762e02144` |

The table call at `+196F` changes only its far segment word from
`0802:0425` to `1026:0425`. The copy setup at `+1974..+1982` changes
only its far segment word from `0AF8:0B25` to `131C:0B25`. In the
`+336A` span, exactly four bytes differ after relocation: the two segment
words at `+337B/+337C` and `+3385/+3386`. Its original far targets are
`00BA:27D9` and `03CA:0025`; the loaded targets are `08DE:27D9` and
`0BEE:0025`. The loaded stub is one far jump to `2E35:016D`. The
16-byte original and loaded callee-entry spans match byte for byte.

## Early RAM snapshots and CPU-pinned view stops

At the first observed `2A8E:196F` stop, `SS:SP=1D16:37B0` retains the
output far pointer `1D16:3AB6`. The debugger displayed `+1974` after the
`+0425` table call and `+1983` after the capped copy; a full 655,360-byte
RAM image was saved at each displayed address. **This run did not preserve
full `EV` CPU transcript rows for `+1974` or `+1983`.** The checker treats
those two points as hash-locked RAM snapshots and makes no complete CPU-tuple
claim for them. The Pascal value at `SS:3AB6` in the `+1974` snapshot has
length 8, and its nine-byte length-plus-payload
SHA-256 is
`6c110ad7a5f343c8961fdb3e3762d2d8c7ebfba40902e1fd85015fcf640c3cdd`.
At `+1983` the caller's `SS:3BD4` basename buffer has exactly the same
nine-byte value. All nine destination bytes changed within its 40-byte
capacity. A later hit at `+196F` in the same loader loop was observed
before the replay advanced to the gameplay stop; this note's three early
RAM snapshots cover the **first** hit only. The second hit and intervening
loader calls are not instruction-traced here.

| CPU transcript stop | `CS:IP` | `SS:SP` | Measured boundary |
| --- | --- | --- | --- |
| Mode branch | `3582:336A` | `1D16:3EF6` | `DS:49F3=4`, `DS:4432=0`; party `(15,4,6)`, window-0 pointer `3996:0000`, time words `(0,5,10)` |
| View call | `3582:3382` | `1D16:3EF6` | Same full-RAM SHA as `+336A`; original `+336F` mode branch falls through and `+3376` jumps over the resident call |
| Far stub | `0BEE:0025` | `1D16:3EF2` | Stack holds return IP `3387`, CS `3582` |
| View entry | `2E35:016D` | `1D16:3EF2` | Loaded unit `0x3B4` entry 1, same frame |
| View return | `3582:3387` | `1D16:3EF6` | Party `(15,4,6)`, window-0 time words `(0,5,10)` remain |

At `+3382`, the low byte at `DS:4432` is **zero**. The debugger's
two-byte display of that address included an adjacent byte and must not
be read as a byte value of one. The original `+3376` `JNE` therefore
skips resident `+3378`; direct `+336A` and `+3382` full-RAM images are
identical. The far call at `+3382` changes exactly four bytes at
physical `0x21052..0x21055`, the saved return frame at
`SS:3EF2..3EF5`. The stub jump preserves it. The view call returns to
`+3387` with `SP=3EF6`; the two 655,360-byte RAM images at call and
return differ at 147 addresses. This is the first gameplay-loop view
refresh on the static `+32F8` initialization path; the run proves this
instance reached it, without assigning every callee side effect.

| Ignored full-RAM stop | SHA-256 |
| --- | --- |
| First `2A8E:196F` | `3e46c01d37b393bd88b558584f72f8f93b8ab545a6a607fdddbb738eb0a45844` |
| `2A8E:1974` | `d0ebc24e57fec9d352628a1d3198d2690a34ffdb58d322e83d7faf5dd1eb6b4e` |
| `2A8E:1983` | `61d3de1e4897f362e66cd92f21457c5a4bcdbbda08b923caf0c7590e112d92b5` |
| `3582:336A` and `3582:3382` | `31424ea33ec4abce1967fff7032be17b95d4b70b4cfab0d9624c29a54e3072c3` |
| `0BEE:0025` | `e5e8d23f49f5c2348c79e3b4009067b056197a5236dff1d61c84686a6530259f` |
| `3582:3387` | `d5cc4a1f61561bbb19766616f3addfa100fb63284bc248854ce13079d678f7a0` |

Private 640×400 DOSBox-X window captures at `+3382` and `+3387`
have PNG SHA-256 values
`f4eadb2cdcc25b159d63f917793eb344b9e7e3d449a01477a4ada9f3900a3b9a`
and `6f6b65475848ab63e35bc4fb55549c04a5957e8007cd6633fe1d4be377e61b7e`.
Their decoded RGB SHA-256 values are
`1907d3488d948f0df53ae2b1e0ef686527d61b6176a3e07220e4159ef02f9399`
and `9f5f210a0483d6d7aa86ab80cd198007e6e60dc21e7322c84be760ee827c22ff`.
Exactly 628 RGB pixels changed, within bounding box `(418,112,622,126)`.
This observation concerns the native emulator window only. It does not
establish a C display implementation or C/native pixel parity.

## Reproduce and check locally

Prepare a fresh ignored run with the pinned files, then launch the private
heavy DOSBox-X executable with a PTY attached:

```sh
tools/oracle/prepare_run.sh slot-a-native-first-view-NEW
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s '-screen 0 1280x1024x24' \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-native-first-view-NEW \
  -time-limit 1200
```

At the visible saved-game chooser, pause with Alt+Pause and arm
`BP 2A8E:196F`, `BP 2A8E:1974`, `BP 2A8E:1983`,
`BP 3582:336A`, `BP 3582:3382`, and `BP 3582:3387`.
Resume with the debugger `RUN` command, then choose A. For a new replay,
record `EV CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000:0000 A0000` at all three early stops, moving each ignored
`MEMDUMP.BIN` before resuming. Run 049 preserved the `+196F` CPU row and
all three RAM snapshots, but no `+1974`/`+1983` CPU rows. A later `+196F`
hit is the next loader-loop record;
use `BPLIST` and `BPDEL` to remove the three early breakpoints, then
resume to `+336A` and `+3382`. At `+3382`, arm the observed loaded stub
`0BEE:0025` and target `2E35:016D`; capture their CPU/frame and the
`+3387` return. Capture private DOSBox-X window images at `+3382` and
`+3387` for the screen check. All dumps and captures stay in ignored
`resources/`.

The optional checker reads those ignored files plus the pinned media,
and uses Pillow to decode the private PNG captures. It verifies original
and loaded caller/stub hashes and operands, overlay entry mapping, branch
bytes, RAM snapshot hashes and count, CPU stops and return frame, screen
hashes, and the 628-pixel difference:

```sh
python3 tests/recovered/check_slot_a_first_view_native.py \
  resources/oracle-runs/slot-a-native-first-view-049
```

The six-row ignored numeric CPU transcript contains `+196F` and the five
later view-route stops, with no `+1974` or `+1983` rows. Its SHA-256 is
`4c7f2193e77befaf44464fd3f427bc96bf60d49d67f41b195af26a5112a899a8`.
The checker prints only a pass/fail summary; it does not print or store
game payload bytes or screenshot pixels in the repository.

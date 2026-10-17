# Unit 0x2F +0131 party pick

`pr_u2f_party_pick_0131` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x2F:+0131..+01F5` and composes its one external call,
unit `0x9E` public entry 8 (`+0DE3`, the strict
[dice roll](U9E_ENTRY8_ROLL.md)), reached through the overlay stub
`00B0:0048`. It walks the party list, rolling once per node, and reports
through a far pointer the node whose linked record byte is highest. The
two sibling callers of the same entries, `0x38:+2D38` and `0x33:+0000`,
share this note's audit; see [U38_RECORD_ROLL_2D38.md](U38_RECORD_ROLL_2D38.md)
and [U33_GRID_ROLL_0000.md](U33_GRID_ROLL_0000.md).

The required recall was attempted once this session with
`timeout 90 deja "poolrad next unowned original function cluster strict
full-C translation queue"`; it found no indexed session. Current `main`
and every remote branch were searched for translations of these three
originals: none exist. Reused merged work: the entry-8 full-state chain
and its SYSTEM random and seed models, the
[entry-9 roll latch](U9E_ENTRY9_ROLL_LATCH_0E2E.md), the
[U9B composition pattern](U9B_ENTRY_ZERO_COMPOSITION.md), and the
[U33 cell lookup](U33_CELL_LOOKUP_012B.md) method of reading relocated
far-call operands and loaded stubs from RAM.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x9E entries 8 and 9 | `+0DE3..+0E51`, 111 bytes, SHA-256 `9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b`; stub paragraph `00B0`, offsets `48` and `4D` |
| SYSTEM generator | START.EXE `0xBE36..0xBEBC` (135 bytes): the `+11C0` wrapper (`e4337c9e…`), the `+120F` seed step (`f5b67b3e…`) and the multiplier word `8405` at `+1245`, as the merged [entry-8 roll evidence](U9E_ENTRY8_ROLL.md) pins them |
| Unit 0x2F | GAME file base `0x9807`, stub segment `0x0035`; `+0131` is an internal prologue |
| Function | GAME file `0x9938..0x99FC`; **197 bytes, 68 instructions, five conditional branches, one far call** |
| Function SHA-256 | `00c3b715a709a62689da381a7e80a04ae6b5e3acf6c8edd48a2d7b1fa0c9a489` |
| Far call | `+015D` `LCALL 00B0:0048` (entry 8), segment word fixup-backed |
| Exit | `RETF 4` at `+01F3`; exact end `+01F6` exclusive |

Every branch target lies inside the 197-byte body. The audit decodes each
unit-0x2F routine from its own prologue and finds exactly two aligned
`PUSH CS; CALL +0131` sites, both in the game-evidenced `+007A` at
`+00FE` and `+0119`, which pass the address of a local far pointer and
then call `+01F6` on the node it returns. The body reads the party head
`DS:5D96/5D98` and node links `+104/+106` that the reviewed
[party index](U25_PARTY_INDEX_0756.md) walks, and the far pointer `+108`
of each node. No gameplay meaning is inferred for the record byte `+3`,
the roll or the choice; "party pick" names the operation.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS, `[SP+4]` the result offset and `[SP+6]` its segment; `RETF 4`
leaves SP eight bytes above entry. The routine pushes BP and reserves eight
local bytes: best byte `[BP-1]`, best roll `[BP-2]`, this roll `[BP-3]`
and the current node far pointer `[BP-7/-5]`.

With current := the head, it loops while either pointer word is nonzero:
it pushes count 1 and range `64h` as live AX words (AH retained), calls
entry 8 and stores AL as this roll. It then forms DX := the best byte,
LES the node, LES its `+108` pointer, sign-extends that record's byte
`+3` and compares; a byte above the best makes this roll the best roll.
The same comparison is formed again; a byte not below the best whose roll
is not below the best roll (unsigned) makes this roll the best roll, the
byte the best byte, and stores the node's offset and segment through the
result pointer. Current := the node's `+104/+106` link. After the walk, a
best byte of zero clears both result words with `XOR AX,AX`; otherwise
the final `CMP` of that byte with zero supplies the returned flags. AX and
DX return as the null pointer words left them; BX, CX, SI, ES and DI are
whatever the last roll's generator and LES left. A cyclic list never
returns, in DOS and in C.

Each far call is modelled as the loaded-state transfer: the relocated
segment word is read from the call site in RAM, the five-byte far jump at
that paragraph's stub offset must name the original entry offset, CS:IP
move to the jump target, and the merged chain runs on the same RAM and CPU
words: `pr_u9e_entry8_fullstate_compose_0de3` (entry 8, its loop, its
SYSTEM call at `0AF8:11C0`, the seed step at `0AF8:120F` reading and
writing the DS dword `4428`, and its RETF 4) with a host cap of 256 call
slices that a byte count can never reach. A site or stub that is not the
loaded-state original reports `CODE` before any effect; a chain stop
reports `CALLEE` with the chain's own status in `*chain_status`; a frame
that no longer returns reports `TRANSFER`. The merged chain requires entry
8 at offset `0DE3` of its segment and SYSTEM at paragraph `0AF8`, which is
where the game loads them. Offsets wrap at 16 bits; words, LES operands
and return frames are physically contiguous; physical addresses wrap at
1 MiB. Null RAM/CPU or RAM that is not exactly 1 MiB is rejected before
mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`) runs the merged
chain on synthetic RAM: a four-node list whose kept node depends on two
equal bytes and their rolls, a list of zero bytes that clears the result,
an empty list with no roll and preserved registers, a stub that is not a
far jump (`CODE`) and the host guards; the expected rolls come from the
same generator formula the merged evidence pins.

The original-byte oracle passes **297 returning cases**, each
comparing the entire 1 MiB RAM and all 14 CPU words with no flag mask:
fourteen byte patterns with eight seeds, an empty list, a head cleared
while nodes exist, 160 random lists, seeds, flags and ES values, the
result pointer inside the frame, on the argument, on the return IP, on a
record and on the seed, nodes under the stack, records across A20, node
offset wrap, eleven stack offsets at both segment ends, A20 wrap of the
stack and DS, a random background and preserved DF. All 68 instruction
starts and all 10 conditional outcomes are reached. One directed
self-linked fixture fails the original's return budget and is excluded;
C is not invoked on it.

The DOSBox-X normal-core/386 synthetic oracle passes **80 cases** for this
body (80 for `+0131`, 80 for `+2D38` and 60 for `+0000` in one run),
comparing all 14 CPU words, 128 frame bytes, the seed dword, the three
latch bytes `6818`, `6819` and `681C` and the 1,257-byte grid with no
mask. The COM keeps the original unit 0x9E span at `CS:0DE3`, copies the
original SYSTEM bytes to the absolute paragraph `0AF8` at start-up (COM
offset `4000` with the observed segment `0814`), holds the three original
caller spans elsewhere and relocates every far call's segment word to one
stub paragraph by the loader's arithmetic. The harness checks that its
code, fixture table and absolute SYSTEM copy stay clear of every DS global
the bodies touch (an earlier layout let the seed dword overwrite one
fixture row). Maskable interrupts are disabled from frame priming through
capture. The result SHA-256 is `2713b2d07c7a47be2771afa2e8d437b1e7a787d4991f7d3bd4148a49826151ba` with COM segment `0814`; another DOS
allocation changes the hash while retaining parity. Original bytes,
generated assembly/COM, results and logs stay under ignored `resources/`;
public fixtures are synthetic, and the public C tests run the merged chain
without any original code bytes.

```sh
cmake -S . -B resources/build-dice-callers -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-dice-callers --target poolrad_overlay_u2f_party_pick_0131_tests \
  poolrad_overlay_u38_record_roll_2d38_tests poolrad_overlay_u33_grid_roll_0000_tests
ctest --test-dir resources/build-dice-callers \
  -R 'poolrad_overlay_(u2f_party_pick_0131|u38_record_roll_2d38|u33_grid_roll_0000)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 2400s \
  python3 tools/overlay/u33_grid_roll_0000_audit.py --unicorn --native \
  --output resources/u33-0000-dice-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x2F` `+0131..+01F5` with entry 8 and SYSTEM composed |
| `source_candidates` | `src/recovered/overlay_u2f_party_pick_0131.c` |
| `signals` | Exact hash/CFG, two aligned game-evidenced callers, the documented party list globals, whole-RAM/CPU differential with full instruction and branch coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` for terminating source paths under the documented RAM/CPU contract |
| `review_notes` | Cyclic lists excluded; loaded-state stubs only; natural saved-game reach unmeasured; the chain's 256-slice host cap is unreachable with byte counts |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.

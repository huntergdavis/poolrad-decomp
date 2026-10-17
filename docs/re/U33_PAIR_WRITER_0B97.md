# Unit 0x33 +0x0B97 pair writer

`pr_u33_pair_writer_0b97` translates the complete call-free routine
`+0x0B97..+0x0BDD`. For disjoint state it writes bytes `0x34` and
`0x35` at consecutive grid cells, using the live far pointer at DS
`0x6716/0x6718`, signed low-byte coordinates, a 50-byte row stride and
a seven-byte displacement. It then increments a byte in the caller's
SS frame. Ordered aliases, arithmetic flags and the return frame are
part of the model.

## Prior work and game ownership

The bounded recall query
`timeout 30s deja "Poolrad U33 0B97 0221 1403 U7F 0020 003A full C claimed translation grid 6716"`
ended 124 without results. Current-main recovered-source inspection
found only the adjacent [U33 +08A4 translation](U33_BYTE_FLAGS_08A4.md),
which ends at `+0x0B96`, and no independent `+0x0B97` implementation.
The new source/header/test/oracle are separate files.

The [U9E game-state scan](U9E_0160.md) independently passes the same
DS `0x6716/0x6718` far pointer to its mode-5 scan. In U33, the privately
decoded `+0x0221` helper checks columns `0..49` and rows `0..24` before
storing through that pointer. The adjacent translated U33 routine
reads the documented window-0 pointer and game classification state.
These observations and the public-entry chain below establish a
game-owned grid operation without assigning gameplay names to the
pair bytes or the caller-local counter.

All three aligned near calls belong to U33 `+0x0BDE`. At each, PUSH BP
passes the caller frame offset, then PUSH CS supplies the far return
frame before the near CALL:

| Call site | GAME file offset |
| --- | --- |
| `+0x0CB6` | `0x0D287` |
| `+0x0CFB` | `0x0D2CC` |
| `+0x0D0C` | `0x0D2DD` |

Descriptor-seeded public entry 1 `+0x1E31` has the static call path
`+0x1EDE -> +0x1244`, `+0x12DA -> +0x11B4`,
`+0x1219 -> +0x0BDE`, `+0x0CFB -> +0x0B97`.
The audit pins this path and caller alignment. It establishes callable
ownership, not a naturally reached saved-game frame.

## Exact original and complete ABI

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x33 | GAME file base `0x0C5D1`, public stub segment `0x003B` |
| Function | GAME file `0x0D168..0x0D1AE`; **71 bytes, 29 instructions, zero calls/conditional branches** |
| Function SHA-256 | `75c464f3df7839fc6a44ab167138b0addb4a329758a775b3255c50a35108b52a` |
| Exit | `RETF 6` at `+0x0BDB`; end `+0x0BDE` exclusive |

The API enters after CALL, with IP/CS at SS:entry-SP. Its three word
arguments are the caller frame offset at entry-SP+4, column at +6 and
row at +8. The coordinate words' high bytes are ignored; their low
bytes are sign-extended. The routine saves BP and reserves no locals.
Final SP is entry SP + 10 modulo 65536.

For the first store it reads column, then row, performs unsigned word
MUL by 50 after sign extension, reloads the grid pointer and adds the
low product and column to DI. Byte `0x34` is stored at `ES:[DI+7]`.
For the second it rereads both arguments, increments the sign-extended
column, repeats the MUL and reloads the pointer before storing `0x35`.
Coordinates outside the visible grid retain their 16-bit wrapping
behavior. The first write can change the second write's arguments or
grid pointer; the model caches neither across that write.

After both stores, DI reloads the caller-frame word and byte
`SS:[DI-2]` is incremented modulo 256. This can alias either grid write,
an argument, saved BP or the return frame. The counter byte is read
after both stores. Saved BP and return IP/CS reload from shared RAM.

AX/DX retain the low/high halves of the second unsigned MUL; CX retains
the second sign-extended column plus one, ES the second grid segment,
and DI the reread caller-frame argument. Negative row bytes therefore
can produce a nonzero unsigned high product. BX, SI, DS and SS are
preserved.

The final byte INC preserves CF from the last word ADD DI,CX. PF, ZF,
SF, AF and OF follow that byte increment. Earlier arithmetic flags are
overwritten before observation. All six returned arithmetic flags are
defined; all other flag bits are preserved. No flag mask or undefined
flag normalization is used.

Effective offsets wrap at 16 bits; word/LES/RETF reads cross offset
boundaries contiguously, with physical A20-off wrapping at 1 MiB.
The code must remain immutable, as in the adjacent U33 contract. The
original-byte audit checks the original body remains unchanged before
calling C. Null RAM/CPU or a size other than exactly 1 MiB is rejected
before mutation. There are no external callbacks, unresolved callees,
runtime caps or source loops.

## Validation and parity review

The independent C11 test passes **65,536 coordinate pairs**, all
possible low-byte column/row inputs, with dirty high argument bytes.
It checks both stores, every counter-byte value, both carry states,
the unsigned high product, defined INC flags, preserved registers,
far return and invalid host arguments. It initializes RAM once and
resets the independent inputs/observed cells per case. Both the new
test and adjacent U33 byte-flags test pass with
`-Wall -Wextra -Wpedantic -Werror`.

The original-byte oracle passes **785 cases**, each comparing the
entire 1 MiB RAM and all 14 CPU words. All 29 instruction starts and
all 28 internal fallthrough edges are covered; there are no conditional
branches. Cases cover signed coordinate/counter flag boundaries,
random grid offsets, stack and A20 boundaries, changed arguments and
grid pointer between stores, counter/grid aliases, saved-BP/return
aliases and incoming flags. Every original case returns and leaves
the instruction bytes immutable before the candidate is called.

The DOSBox-X normal-core/386 oracle passes **256 synthetic cases**,
all counter bytes across signed-coordinate boundary pairs. It compares
all 14 CPU words, 32 frame/sentinel bytes, four grid-pointer bytes,
the counter byte and an eight-byte grid/sentinel window containing
both stores. The native harness primes that observed window per case;
the separate original-byte audit compares the full 1 MiB RAM. No flag
or compared RAM byte is masked. The result SHA-256 is
`ce922ad061bad7e39e60f225820c51ccd4e538b578c3515268e1c79b88d1cc2b`.
The result header includes a DOS allocation segment, so another setup
may produce a different hash while retaining parity.

Native execution uses a copied hash-locked leaf in a synthetic COM.
Code, grid window, counter and frame are proven disjoint in that
harness. Maskable interrupts are disabled from priming through capture,
with entry FLAGS `0x0002`, then restored for DOS output. Original bytes,
generated assembly/COM, diagnostics, logs/results and build files stay
under ignored `resources/`; public fixtures are synthetic.

Neither the descriptor-seeded static path nor this native harness
establishes a naturally reached saved-game call/return frame, actual
loaded game-unit segment or full-game replay parity. The model and
oracle adapt the [U7F full-state primitives](U7F_STATE_FLAGS_0F7E.md),
[party reorder compilation/CPU utilities](U6A_PARTY_BEFORE_14A9.md),
[unit 0x5E helpers](U5E_PARTY_VALUE_2879.md), and the documented
[U7D native interrupt boundary](U7D_CARRY_02F2.md). The immutable-code
contract also follows the adjacent U33 translation.

```sh
cmake -S . -B resources/build-u33-0b97 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u33-0b97 --target \
  poolrad_overlay_u33_pair_writer_0b97_tests poolrad_overlay_u33_byte_flags_08a4_tests
ctest --test-dir resources/build-u33-0b97 \
  -R 'poolrad_overlay_u33_(pair_writer_0b97|byte_flags_08a4)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn timeout 180s \
  python3 tools/overlay/u33_pair_writer_0b97_audit.py --unicorn --native
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33:+0x0B97..+0x0BDD` |
| `source_candidates` | `src/recovered/overlay_u33_pair_writer_0b97.c` |
| `signals` | Exact hash/CFG, three aligned frame-passing callers/public-entry path and mapped game grid, full-RAM/CPU differential, exhaustive independent coordinates and native execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented immutable-code, A20-off RAM/CPU contract |
| `review_notes` | CF survives from the final word ADD; the other arithmetic flags follow byte INC; native IRQ isolation and synthetic caller; natural route unmeasured |

No classifier or count ledger is changed. Strict credit and identity
remain the PM's review decision. Disjoint follow-ons U33 `+0x0221`
and `+0x1403` are queued separately, pending ownership review.

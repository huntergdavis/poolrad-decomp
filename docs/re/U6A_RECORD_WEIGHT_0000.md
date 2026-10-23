# Unit 0x6A +0000 record weight

`pr_u6a_record_weight_0000` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x6A:+0000..+0158` and composes its one external call,
unit `0x86` public entry 6 (`+323D`, the merged
[record-table predicate](U86_RECORD_PREDICATE_323D.md)), reached through
the overlay stub `0096:003E`. The routine scans 21 record bytes and a
far-linked node list through a DS table, stores a tier byte into the
record and returns a weighted 16-bit sum. The companion routine
`+019C` is in [U6A_LINK_HIGH_CLEAR_019C.md](U6A_LINK_HIGH_CLEAR_019C.md);
both share one audit script.

The required recall `deja "poolrad next unowned original function
cluster strict full-C translation queue"` (timeboxed to 90 s) returned
no matching session, and `deja` reported zero indexed sessions. No
prior-session finding is reused. Current `main` and every remote branch
were searched for `+0000`/`+019C` translations of this unit: none exist.
Reused merged work: the `+323D` full-state model and its CPU/RAM audit
helpers, the [U9B composition pattern](U9B_ENTRY_ZERO_COMPOSITION.md)
for internal calls, and the [U7F](U7F_STATE_FLAGS_0F7E.md) and
[U6C](U6C_RECORD_TABLE_SUM_3F31.md) differential/native methods.

## Loaded-state precondition

Added 2026-10-22 by par-e (credit authority), VP i21329, ruling i21377. This states a condition the credit always
assumed; the credit is unchanged.

This credit holds **only while every overlay unit the body far-calls is resident**.
The C now reads each listed live `LCALL` binding and requires its stub to hold a
five-byte far `JMP` (`EA`) to the expected entry. Explicit callee-segment
arguments must agree with the jump target; U6A's predicate uses the live jump
segment. If a required unit is absent or a binding is malformed, **the C
refuses with `ARGUMENT` before changing CPU14 or RAM**. The caller no longer
has to enforce this listed precondition on the C's behalf.

Bindings and code must remain immutable throughout the synchronous call.
The overlay manager's DOS loading path is still outside this C model.
Loaded-state differentials execute real far-JMP stubs; separate host checks
exercise cold stubs and malformed bindings and verify atomic refusal. Host
checks are not original executions and add no parity-case or credit count.
Updated validation results are pending; see
[the guard work report](../LOADED_STUB_GUARDS.md).

| Credit | Stub | Target | Call sites |
| --- | --- | --- | --- |
| `OVL_006A_0000` | `0096:003E` | unit `0x86` entry 6 `+323D` | `+008A` |

Stubs were resolved from the original bytes: each `LCALL` segment is matched to its unit's `memory_segment`, and
the slot `0x20 + 5 * entry` gives the entry. SYSTEM (`0AF8`) calls are resident and need no stub.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x6A | GAME file base `0x17831`, stub segment `0x0075`; `+0000` is the first prologue, not a public entry |
| Function | GAME file `0x17831..0x17989`; **345 bytes, 121 instructions, twelve conditional branches, one far call** |
| Function SHA-256 | `b361d881a02fc656fa6e5e7ad390b54724164d9f8eab5c751a3b57545da0760a` |
| Far call | `+008A` `LCALL 0096:003E`, segment word fixup-backed; unit `0x86` entry 6 `+323D`, 77 bytes, SHA-256 `2ec1f95931eeb474593d0733ece3c8d582ab3a2a4d1c23b9de59d8f0bb5e87cd` |
| Exit | `RETF 4` at `+0156`; exact end `+0159` exclusive (the next prologue is the strict `+0159` leaf) |

Every branch target lies inside the 345-byte body. The audit decodes
each unit-0x6A routine from its own prologue and finds exactly one
aligned `PUSH CS; CALL +0000` site: `+077A` at `+07A1` (game-evidenced),
which pushes a far pointer from its locals and compares the returned
word with another local word. Callee ownership is established in the
`+323D` note through ten aligned game callers. The DS table at `31FB`
with a 16-byte stride is the table family that the reviewed
[`0x9B:+0000` routine](U9B_ENTRY_ZERO_COMPOSITION.md) scans at
`31FA + 16*index`, and the node byte `+2E` indexes the `5582` table
exactly as the predicate's record byte does. The linked-list shape
(far link at `+C8/+CA`, node links at `+2A/+2C`) is read from the
original bytes. No gameplay meaning is inferred for the record bytes,
the table values, the tier or the sum; "weight" names the arithmetic.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS, `[SP+4]` record offset and `[SP+6]` record segment; `RETF 4`
leaves SP eight bytes above entry. The routine pushes BP and reserves
fourteen local bytes: first maximum `[BP-3]`, second maximum `[BP-4]`,
first sum `[BP-5]`, second sum `[BP-6]`, node far pointer `[BP-0A/-08]`,
index `[BP-0B]`, lookup `[BP-0C]`, tier `[BP-0D]`, result word `[BP-2]`.

Pass one runs the RAM index from 0 to `14h`. It reloads the record
pointer from the stack for every access, reads record byte `+17+index`
and, when that byte is above `7F` unsigned, rereads it, masks the low
seven bits, shifts left four with `CL=4`, reads DS `31FB+that`, stores
the lookup, raises the unsigned first maximum and adds to the 8-bit
first sum. Pass two loads the record's `+C8/+CA` far pointer into the
locals and loops while either pointer word is nonzero: it pushes the
segment then the offset (rereading between pushes), performs the far
call, and when `AL` returns nonzero scans node bytes `+3B+index` for
index 1..3 with the same lookup into the second maximum and second sum,
then follows the node's `+2A/+2C` link. The tier byte becomes 4 when
either sum is nonzero and 6 when either maximum exceeds 2, each compare
rereading its live local; it is stored at record `+2C`. Three `MUL`s
compute `15*sum2` (BX), `15*sum1` (CX) and `60*tier`, the two `ADD`s
produce AX, which passes through `[BP-2]`, and the live frame is
restored from RAM.

The far call is modelled as the loaded-state transfer: the caller's CS
and the return offset are pushed, the `+323D` model executes on the same
RAM and CPU words, and its RETF frame is checked; a frame that no longer
returns to the call site reports `TRANSFER` (not reachable through the
callee's own writes, which touch only its `[BP-1]` local). The entry
`cpu->ip` is the body's load offset, 0 in the game, and the pushed return
offset is `ip + 8F`. DX returns the high word of the last product, SI is
preserved, ES:DI is the record pointer, BX/CX are the two products and
the flags are the final `ADD AX,BX` (all six arithmetic bits defined).
Intermediate `SUB`, `XOR`, `AND`, `SHL`, `INC`, `ADD8`, `CMP` and `MUL`
flags are folded because a later flag writer precedes every observation.
Effective offsets wrap at 16 bits; words, LES operands and the return
frame are physically contiguous; physical addresses wrap at 1 MiB. Null
RAM/CPU or less than 1 MiB is rejected before mutation. A cyclic node
list never returns, in DOS and in C; no host cap is added.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`) checks a mixed
record with a true and a false node (result `15*123 + 15*5 + 360`,
tier 6, nodes unmodified, exact registers and ADD flags), the empty
case, the 8-bit sum wrap, the tier-4 case and the host guards.

The original-byte oracle passes **507 returning cases** for this body,
each comparing the entire 1 MiB RAM and all 14 CPU words with no flag
mask: every item position with five table values, every table index
through items 0 and 20, sum wrap, ascending and descending maxima,
one- to three-node lists with all-true, all-false, mixed, zero-index and
`FF`-index predicates over five byte patterns, 160 random records,
lists, tables, flags and ES values, sixteen stack offsets at both
segment ends, A20 wrap of the stack, DS, record and nodes, offset wrap
of the record and nodes, the record aliasing the return IP, the locals
and the argument, nodes under the stack and inside the callee's frame,
both DS tables inside the frame, the record as its own node,
offset-only and segment-only heads, a random background and two
push-placed frames at the segment top. All 121 instruction starts and
all 24 conditional outcomes are reached. One directed self-linked node
fixture fails the original's 100,000-instruction budget and is excluded
from returning parity; C is not invoked on it.

The DOSBox-X normal-core/386 synthetic oracle passes **200 cases** for
this body (and 200 for `+019C` in the same run), comparing all 14 CPU
words, 128 frame bytes, the 288-byte record and the 256-byte node area
with no mask. The COM copies the original caller bodies, the original
callee and a loaded-state five-byte far-jump stub; the far call's
segment word is relocated by the same paragraph arithmetic the DOS
loader applies to that fixup, so the call still reaches stub offset
`003E`. Maskable interrupts are disabled from frame priming through
capture. The result SHA-256 is
`19e6e9fb4325b2a995aac25e2ea34e07994e144d3f614a9a9ee50705e859a231`
with COM segment `0814`; another DOS allocation changes the hash while
retaining parity. Original bytes, generated assembly/COM, results and
logs stay under ignored `resources/`; public fixtures are synthetic.

```sh
cmake -S . -B resources/build-u6a-0000 -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u6a-0000 --target \
  poolrad_overlay_u6a_record_weight_0000_tests poolrad_overlay_u6a_link_high_clear_019c_tests
ctest --test-dir resources/build-u6a-0000 \
  -R 'poolrad_overlay_u6a_(record_weight_0000|link_high_clear_019c)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 900s \
  python3 tools/overlay/u6a_record_weight_0000_audit.py --unicorn --native \
  --output resources/u6a-0000-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x6A` `+0000..+0158` with its `+323D` dependency composed |
| `source_candidates` | `src/recovered/overlay_u6a_record_weight_0000.c` |
| `signals` | Exact hash/CFG, one aligned game-evidenced caller and the shared `31FB`/`5582` table families, whole-RAM/CPU differential with full instruction and branch coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` for terminating source paths under the documented RAM/CPU contract |
| `review_notes` | Cyclic lists excluded; loaded-state stub only (the not-loaded `INT 3F` path is the overlay manager's); natural saved-game reach unmeasured; `TRANSFER` is a guard, not an exercised path |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.

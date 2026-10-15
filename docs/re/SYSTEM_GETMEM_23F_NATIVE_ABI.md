# SYSTEM AF8:023F GetMem native/heap ABI with explicit core gates

The new addressed adapter verifies three independent fragments: the
saved-A +19B5 far call and wrapper prefix, the requested-size scratch
store, and the actual split-writer/return tail. It never bridges snapshots
across the unknown allocator core or injects an allocator outcome into
continuous C replay. The first fragment blocks before SYSTEM +0245's
near call to +0308; the scratch-store fragment blocks before +030F's near
call to +0491. The independent tail computes returned DX:AX from the real
stack and stops at caller +19BA before any caller instruction executes.

This separate branch starts at fresh main
`30cd26ad3a0f8d6df2d85acffa5d46b8a8a01b82`, after PR #451. Bounded
`deja "Poolrad SYSTEM AF8 023F GetMem heap first writers native DX AX
19B5 19BA ABI"` expired before implementation without reusable output.
Reused repository leads are [allocation setup](SLOT_A_ALLOCATION_19B1_ABI.md),
[earlier allocation return](SLOT_A_NATIVE_19BA_RECORD_RETURN.md), and
[wrapper context](RESIDENT_SIDECAR_APPEND.md). Original bytes and all
new frames/writers/returns were independently rechecked.

Before review, the branch was restacked onto current main
`55f80b150bd23833495362b8c99e1723b6e5dddd` (after PR #463). Owned C,
header, checks and their shared state header retained identical hashes;
the complete focused audit was rerun after the restack.

## Pinned original code and natural native capture

| Ignored input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| SAVGAMA.DAT | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original/loaded SYSTEM +023F..0253 wrapper | 21 | `d4ceab56ec05612e8ba6c90dab11d9da62f1f38235503781f80d636daa63d346` |
| Original/loaded SYSTEM +0308..03D2 core | 203 | `03e08d678db922d962aafbd79b301ca90556775acef11b832fecbeb29fe0dff0` |
| Original/loaded SYSTEM +0491..04A1 normalizer | 17 | `440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d` |
| Original caller +19B1..19B9 | 9 | `27678668b204ad2d84785e89897bf20454582a70e1f7e34bd363cab31373a5b2` |
| Loaded caller +19B1..19B9 | 9 | `963c38901cd3f9bc0c0473f7e06cb17e4f7559edff39d3d1827cfd9d489092f9` |
| Clean raw 33-row transcript | — | `f5fe196a15c9cf3ba8a9af361070d7a5a363b346eee234c8b5db1207f2d48997` |

SYSTEM bytes come directly from START file base AC76 plus the listed
offset. These three spans have no loaded-byte differences. The caller's
single far-call segment word relocates AF8 to loaded 131C. Every span is
checked against every captured RAM image, along with complete input hashes.

The clean ignored root is `slot-a-getmem-23f-split-abi-20261014`. Its 33
same-run stops pin all sixteen CPU fields and all 655360 RAM bytes. It
loads ordinary saved A with no guest register/RAM edits. Old breakpoints
are cleared between stops so repeated free-list addresses remain distinct.
The exploratory run stayed private; its +19BA safety stop prevented caller
execution when the initial heap-tail lead proved wrong. The clean capture
measures the actual free-block split and closes before caller +19BA executes.

At +19B5, CS:IP=2A8E:19B5, SS:SP=1D16:37B6, BP=3DCA, AX=011D,
BX=0006, CX=0000, DX=3734, SI=379C, DI=3704, DS=14A0, ES=SS,
FS=GS=0000 and FLAGS=7202. The actual far frame at SYSTEM +023F is
`[SS:37B2] = 19BA, 2A8E, 011D`: return IP, return CS and size.
PUSH BP establishes BP=37B0. At core +0308, SP=37AE and the real near
return word is 0248. At normalizer +0491, SP=37AC and return word is 0312.
Original-byte execution of both near CALLs matches native full state;
C blocks before those near-frame pushes.

The actual free-list walk visits 41DF:0008, 42AB:0000 and 449A:0008,
then takes the split branch at +0336/+0343. It does not take heap-tail
growth or its indirect hook. Requested size 011D is stored by +030C at
physical 1D154. The selected block is 449A:0008; the remainder header is
44AC:0008, with next pointer 44B2:0008 and size words 0008/0001.

| Original instruction | Actual writer | Stored word | Net changed bytes |
| --- | --- | --- | ---: |
| +030C MOV [8754],AX | DS:8754 / physical 1D154 | 011D | 2 |
| +0363 POP ES:[DI] | 44AC:0008 / physical 44AC8 | 0008 | 1 |
| +0366 POP ES:[DI+2] | 44AC:000A / physical 44ACA | 44B2 | 2 |
| +036A MOV ES:[DI+4],CX | 44AC:000C / physical 44ACC | 0008 | 2 |
| +036E MOV ES:[DI+6],BX | 44AC:000E / physical 44ACE | 0001 | 2 |
| +0378 MOV ES:[DI],CX | 42AB:0000 / physical 42AB0 | 0008 | 0 |
| +037B MOV ES:[DI+2],BX | 42AB:0002 / physical 42AB2 | 44AC | 1 |

The predecessor offset store executes even though its value stays 0008;
the segment changes from 449A to 44AC. The split tail changes eight RAM
bytes total. The whole native +19B5-to-+19BA net change is 30 bytes,
including requested-size storage and real stack writes.

## Return frame and independent C coverage

At split-tail input +0363, SS:SP=1D16:37A2 and the eleven stack words are
`0008, 44B2, 0000, 42AB, 0008, 449A, 0248, 3DCA, 19BA, 2A8E, 011D`.
They supply the next free pointer, predecessor pointer, allocated pointer,
near return, saved BP, far return and caller argument. POP AX / POP DX
compute the allocated pointer from this frame. The wrapper POP BP and
RETF 2 restore BP=3DCA and SP=37B8. The native caller-entry state is
CS:IP=2A8E:19BA, DS=14A0, ES=42AB, SS=1D16, FS=GS=0000,
AX=0008, BX=44AC, CX=0008, DX=449A, SI=449A, DI=0000,
BP=3DCA, SP=37B8, FLAGS=7202. No caller instruction has executed.

Returned DX:AX is **449A:0008** in this run. An independent synthetic
tail frame returns 5000:000C, proving the C result is read from the live
frame rather than supplied as a constant or injected allocator outcome.

| Independent input and endpoint | C instructions | Post-input native C checkpoints |
| --- | ---: | ---: |
| Caller +19B5 through wrapper +0245, before near CALL +0308 | 4 | 3 |
| Requested-size store +030C through +030F, before near CALL +0491 | 1 | 1 |
| Split tail +0363 through caller +19BA, before caller store | 17 | 12 |
| Total across separate inputs | 22 | 16 |

The audit compares all sixteen CPU fields and all RAM bytes after every
implemented instruction against original-byte execution. Native checkpoint
comparisons are the sixteen-checkpoint subset above, not all 125 steps.
Eleven synthetic cases bring the total to **125 C instruction comparisons**:
three wrapper size/stack cases, two requested-size values, five split-tail
cases and one carry-error wrapper case. These exercise alternate live
returns, stale flags, stack/header and predecessor/header aliasing, a high
segment offset, wrapped SP, zero/FFFF arguments and the runtime-error gate.
The alias cases retain original read/write order, including idempotent stores.
Strict C11 warnings (including Wconversion), standalone checks and the full
125-comparison audit passed; AddressSanitizer and UndefinedBehaviorSanitizer
also passed both standalone and differential checks.

| Native RAM image | SHA-256 |
| --- | --- |
| Caller +19B5 | `1ca6a13b9b1e061f5b31e993f7a3717566d28393dc16e87c1f29328d59e4ce0e` |
| Independent tail +0363 | `b8e76cb40a3ad27a9397e66434499e0accac07dd82d60a8ce4f90e3c05e076d0` |
| Caller +19BA | `bbf5f620dd74e4bce0be4ac45eeacf819ee9b9aefd175ea0703771e0efda6f6f` |

## Flags discrepancy and explicit gates

The normalizer is deliberately outside the C adapter. Starting from the
actual +0498 state and executing its three SHR instructions, native +049E
FLAGS=7216 while raw Unicorn 2.1.4 gives 7206: only AF differs. AF is
undefined for SHR. The audit records and asserts this discrepancy without
masking flags or rewriting Unicorn state. Subsequent AND reconverges the
captured flags, but that does not establish full allocator parity. No C
fragment crosses this sequence or resumes an earlier blocked state from a
later native snapshot.

The adapter blocks before +0245's near call to original AF8:0308, before
+030F's near call to AF8:0491, at runtime-error entry +00E2 and at caller
+19BA. Reports preserve near-call targets and return PCs. Unknown core,
normalizer and unlisted PCs stay blocked. The error wrapper is covered by
an independent synthetic carry input; it is not a naturally reached failure.
Exact-fit, heap-tail growth, exhaustion and indirect allocator hooks remain
unproved. No whole-core, continuous GetMem C, symbol identity, playable
progress or decompilation-count claim follows from these fragments.

## Review and reproduction

- target_function: SYSTEM original AF8:023F wrapper with separately entered
  +030C requested-size store and +0363 split-writer/return tail.
- source_candidates: `system_getmem_23f_abi.c` addressed adapter only.
- signals: pinned original/loaded code, 33 same-run native rows, actual far
  and near frames, exact heap writers, computed DX:AX, full-state differential
  comparisons, alias and error cases, sanitizer checks, retained AF mismatch.
- score: 5/5 checked evidence categories for the three bounded fragments;
  ordinal evidence coverage, not a probability or full-function score.
- verdict: bounded fragment match; full core and normalizer remain unclear.
- review_notes: independent native inputs are never bridged; all prior replay
  gates, the immutable #410 map, CMake and key/printer lanes stay untouched.

Run the media-free check with
`bash tests/replay/check_system_getmem_23f_abi.sh`.
With pinned private inputs and Unicorn 2.1.4 available, run
`POOLRAD_UNICORN_PYTHON=/path/to/python bash tests/replay/check_system_getmem_23f_abi.sh resources/oracle-runs/slot-a-getmem-23f-split-abi-20261014`.
The runner accepts captures only below this clone's ignored `resources/`,
limits each fragment to 100 instructions and ten seconds, and limits the
audit process to 120 seconds with a five-second kill grace. Assertions must
remain enabled. Public files contain hashes and behavioral measurements;
original media, RAM dumps, raw captures and build products stay ignored.

Two disjoint follow-ons are scoped before handoff:

1. SYSTEM +0308/+0491 selection and normalization: resolve the AF evidence
   boundary and independently measure free-list/exact-fit/growth/exhaustion
   effects, retaining explicit unknown indirect-hook gates. Own new runtime
   files; exclude caller stores and avoid extending a continuous replay claim
   across unproved allocator outcomes.
2. Unit 6F caller +19BA pointer stores and loader-argument setup through
   +19D7's near CALL +0AEC (return +19DA), from the actual native allocation
   return. Own new caller files; stop before the loader call, without heap
   translation or an injected continuous-replay bridge.

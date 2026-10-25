# U7F entry 3: party pool distribution

The recovered body distributes seven signed pool dwords at DS:67F4 over
party-node words at +88h, subject to the class threshold and node total at
+102h. It makes two passes, preserves residual pools, and sets DS:6DC0 when
a pool remains nonzero. The names of the seven pool types are not established
by this evidence.

The second pass does not repeat the first pass's status test. Its available
capacity is the unsigned word subtraction `threshold - node_total`, including
wraparound for an already oversized total. These are original behaviors,
not additional eligibility or saturation rules imposed by the C model.
Nonpositive signed pool dwords take the zero-share path. Division quotients
and remainders are narrowed to local words, and final residual high words
are zero. Thus the ordinary small-positive-pool conservation examples are
not a claim of conservation over all signed 32-bit inputs.

## Claim and composition

| Target | Source / role | Observable contract |
| --- | --- | --- |
| OVL_007F_063A | `overlay_u7f_063a_cpu.c`, `pr_u7f_063a_run` | Entire [063A,0A01), 967 bytes, 385 instructions, plain RETF; CPU14, FLAGS, RAM, bus node accesses and stop state |
| OVL_007F_05E0 | `pr_u7f_party_count_05e0` | Merged strict 90-byte party count; flat RAM; unbudgeted; finite traversal required |
| OVL_007F_0054 | `pr_u7f_0054_cpu` | Merged strict cap test; live far record/output arguments; bus node reads; shares instruction budget |
| OVL_007F_0000 | `pr_u7f_entry0_0000_cpu` | Merged strict class threshold, including resident 13AB/11EA closure; flat RAM; shares budget |
| OVL_007F_003A | `pr_u7f_record_word_add_003a` | Merged strict node-total add; live argument reload before store; flat RAM; unbudgeted |
| SYSTEM 0AF8:0A76 | `pr_system_longdiv_0a76` | Signed 32-bit division, CPU/RAM/FLAGS and divide-error stop; unbudgeted |
| SYSTEM 0AF8:16A8 | `pr_system_fillchar_bus_16a8` | Original far-argument FillChar ABI, CLD, bus stores and live return; unbudgeted |

The entry requires 1 MiB A20-off RAM, IP=063A, SP in [0200,FFFC], immutable
code, and distinct CPU/bus/budget/diagnostic storage. Party traversals must
terminate, including aliases with stack writes in the unbudgeted +05E0
callee. No finite execution guarantee is claimed for cyclic lists.

The caller's node accesses use `pr_bus`; its code, DS globals and SS stack
access flat RAM. Callees retain their existing contracts. In particular,
flat +05E0/+003A and the +0000 resident closure do not establish hardware
parity for pointers into the EGA aperture. The C aperture test checks the
caller's bus mechanism, not full-composition EGA parity.

The excluded inputs are constructed separately by `exclusions()` in the
checker: (1) head 3000:0100, status 00, self-link 3000:0100; (2) head
A000:0100, RAM status 80, EGA plane-0 status B3, terminating links and
EGA read mode/map 0. The first probe executes original bytes until CPU14
and all 1 MiB RAM repeat at the party-loop head, establishing nontermination
without invoking the unbudgeted C callee. The second supplies actual
read-map-0 plane bytes on original aperture reads and compares the +0655
arrival: original count 1 versus flat-callee count 0. It establishes a
specific divergence, not general EGA emulation parity.

These two families are excluded from parity totals. For every claimed
Unicorn parity case, memory hooks require zero reads or writes in physical
[A0000,C0000), code fetches stay outside it, and each entered +05E0 must
return before C is invoked. Thus neither a cyclic count traversal nor an
aperture-reaching access can overlap the claimed fixture set.
For budgets that stop before +05E0, a separate ten-counted-instruction
original prefix proves its termination on the same input first.
The memory observer is registered only over the aperture window, with a
16-byte margin for crossing operands. A bounded diagnostic showed that
unrestricted Unicorn memory hooks perturb RETF: SP advances but IP stays
at 0639, causing a second return. Both no-hook and aperture-only traces
return normally to 0655. The audit changes neither the original bytes nor
CPU state to compensate; full RAM comparison uses the backing buffer.

A media-free, one-byte reproduction is
[repro_unicorn_retf_memory_hooks.py](../../tools/overlay/repro_unicorn_retf_memory_hooks.py).
Unicorn oracles must not register full-range MEM_READ hooks (RETF corruption, see the repro).
Run `python3 tools/overlay/repro_unicorn_retf_memory_hooks.py`. It places
`CB` (RETF) at 6000:0639 and a synthetic return frame `(0655,6000)` at
2000:7FD0. Its code hook stops before the second instruction; memory hooks
only log accesses and never inspect or modify CPU state. Measured with
Unicorn **2.1.4**:

| Memory observer | Next CS:IP | SP after RETF |
| --- | --- | --- |
| None | 6000:0655 | 7FD4 |
| Full-range READ | 6000:0639 (would repeat RETF) | 7FD4 |
| Full-range WRITE | 6000:0655 | 7FD4 |
| Full-range READ + WRITE | 6000:0639 (would repeat RETF) | 7FD4 |
| Aperture-only READ | 6000:0655 | 7FD4 |

Both failing variants log exactly two reads: physical 27FD0 and 27FD2,
two bytes each. This demonstrates observer-induced control-flow corruption;
it does not establish whether another audit detects or conceals that fault.
The reproducer also accepts normal returns from a future corrected Unicorn
and reports the installed version and every observed state.

Native fixtures additionally require all six loaded/captured 64 KiB regions below
A0000. The original-only recurrence assumes no interrupts or external
memory mutations, matching the standalone model's execution environment.

Each caller instruction and every +0054/+0000/resident-closure instruction
consumes one budget unit. The other four callees are indivisible under this
budget. LIMIT exposes the next counted instruction; CODE stops before a
malformed call adds its return frame; TRANSFER exposes a changed return
target; CALLEE preserves the failing callee's state. Argument rejection is
atomic. Far-call segment relocation words and return frames remain live.

## Provenance and identity evidence

START.EXE: 65,231 bytes,
SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
GAME.OVR: 206,105 bytes,
SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The body begins at GAME.OVR file offset 152259 and has SHA-256
`f23626a4d567fce56738f4d2c250c263f5b8b90a716121a070bfab52db85c736`.
The checker verifies every instruction boundary, all call offsets, four
far-segment fixups, internal branch destinations, and the sole RETF at 0A00.
It loads the original bytes for every composed callee; there are no callee
stubs in the original-byte oracle.

The preclaim at main `fae95767e200a7419caf1d4a74c326f431c793d2` priced
967 new bytes and zero new callee bytes, with six of six direct callees
composable and zero downstream sole-blocker rows. The canonical accounting
queue SHA-256 was
`972d878ef7206076e83bd9f947c5b563c66dc2aa9be72da2c87d1864c5165fbc`;
the strict queue SHA-256 was
`ca30ea99bd31f9f7f04bdb66584ad02c80bde055dac0015a6ee99ce2edec4020`.
All six source-search hits were inspected and were caller provenance,
boundary references, or unrelated offsets, rather than an existing body.

Three fixup-backed calls to entry stub 008F:002F supply caller evidence:
unit 0023:+0663 (fixup 0666, caller 053E), unit 001E:+0DF4 (fixup 0DF7,
caller 0D09), and unit 0020:+106A (fixup 106D, caller 0ECD). Independently,
the body reads the party root at 0640/0643 and 08A8/08AB, checks node status
+84h at 0732, reads/writes indexed +88h words, reads +102h at 08CD
and updates node totals through +003A,
follows +104h/+106h links, and writes residual pool dwords at 09D6/09DA.

The role interpretation reuses the caller analysis in
[U7F_RECORD_WORD_ADD_003A.md](U7F_RECORD_WORD_ADD_003A.md), the identity
arguments in [U7F_IDENTITY_ARGUMENTS.md](U7F_IDENTITY_ARGUMENTS.md), and
[U7F_05E0_NATIVE_REACH_FULLC_GATE.md](U7F_05E0_NATIVE_REACH_FULLC_GATE.md).
The priced claim was approved in PM message i22398 after claim i22389.
The independent identity argument was re-decoded by the reviewer in
[PR #1507](https://github.com/huntergdavis/poolrad-decomp/pull/1507), confirmed
in PM message i22421. Identity integration and credit remain reviewer-owned.

## Validation contract

`ovl_007f_063a_cpu_audit.py` registers `CHECKER_ROWS=('OVL_007F_063A',)`.
`--compile-only` builds without game media. `--unicorn` compares CPU14,
the remaining budget, stop class and every byte of 1 MiB RAM against original
instructions. FLAGS differences must satisfy the explicitly pinned
last-writer MUL/DIV/shift rules in `pinned_shift_flags.py`; no bits are masked.
Each caller arithmetic instruction is modeled separately, including INC/DEC
preserving CF and pinned SHL AF=1. No caller flag-folding assumption is used.

`--native` runs completed calls with the pinned DOSBox-X normal/386 binary
SHA-256 `c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457`,
source revision `5a9c7c53481c2e933948039c33256776653f4faf`. It compares
all CPU14 words and five 64 KiB RAM regions without masks; IF is cleared
for capture. Native evidence does not cover all 1 MiB, divide-error stops,
instruction-budget stops, or EGA aperture behavior.

`--mutate` compiles altered implementations and requires observable
rejection. The synthetic CTest independently checks distribution, cap
residuals, divide error, argument atomicity, budget and code guards, and
bus writes. Assertions must remain enabled when building that test.

The focused `poolrad_overlay_u7f_063a_cpu_tests` CTest passed on 2026-10-23
with `-UNDEBUG`, no build type, and assertions enabled (one test, zero
failures). Its independent distribution, residual, divide-error, guard,
argument-atomicity and bus-mechanism assertions all ran.

The pinned-native full run passed all 16 completed-call cases, including
status mixtures, cap residuals, large/negative pools, class mixtures, stack
endpoints, odd SP, record wrap and a node word crossing FFFF. All CPU14 words
and five 64 KiB RAM regions matched without masks. The separate cyclic-list
probe repeated full CPU14 and 1 MiB RAM at +05F7 after a 3,328-instruction
period (steps 32 and 3360); the unbounded C callee was not invoked.

## Measured result and parity review

The complete `--unicorn --native --mutate` run exited zero on 2026-10-23.
The [public receipt](OVL_007F_063A_CPU_REPORT.json) records the source pins,
executable-file hashes, case counts, native result hashes and mutation
failures, with the machine-specific binary path removed.

| Check | Measured result |
| --- | --- |
| Original-byte differential | 630 fixtures plus 385 caller-instruction budget boundaries: **1,015/1,015** comparisons passed |
| Instruction starts | **385/385**, no missing starts |
| Conditional outcomes | **34/34**, no missing or declared-infeasible edges |
| State compared | CPU14, remaining budget, stop class and **all 1,048,576 RAM bytes** per comparison |
| Stops | 162 RETURNED, 15 CALLEE (divide error), 3 TRANSFER, 835 LIMIT |
| FLAGS | 833 exact arrivals; 182 declared shift-AF arrivals; zero masks and zero observed MUL/DIV splits |
| Domain checks | Zero aperture accesses; 995 completed +05E0 calls in compared traces; early-budget inputs separately checked through the original +05E0 prefix |
| Constructed exclusions | Two separate passing families, excluded from parity totals: cyclic-list recurrence and aperture count divergence (original 1, flat C 0 at +0655) |
| Pinned native | **16/16**, CPU14 plus five 64 KiB regions each, no masks |
| Compiled mutations | **4/4 rejected**: cleared SHL AF, changed node store, wrong residual flag, inverted cap branch; each unchanged baseline passed first |
| Synthetic CTest | **1/1 passed**, assertions enabled |

| Review field | Assessment |
| --- | --- |
| target_function | OVL_007F_063A, original [063A,0A01), entry 3 |
| source_candidates | `overlay_u7f_063a_cpu.c:pr_u7f_063a_run` plus the six named composed callees |
| signals | Pinned bytes/instruction boundaries, live call frames and relocations, independent two-factor identity, full caller coverage, full-state comparisons, pinned native corroboration, synthetic semantic assertions and rejected mutations |
| score | 1,015/1,015 differential comparisons; 385/385 starts; 34/34 outcomes; 16/16 native cases; 4/4 mutations rejected |
| verdict | **match within the declared interface and input domain** |
| review_notes | Finite +05E0 traversal and inherited flat-callee/aperture limits remain explicit and constructed. Native capture clears IF and covers five RAM regions. These measurements do not claim arbitrary EGA-composition parity or execution of cyclic lists. Credit and merge require independent review. |


## Reviewer acceptance record (added by the review lane, not the author)

Reproduced by par-e at main `ea768440` (2026-10-23/24), through `poolrad-heavy.sh`. The checked C closure, headers and
audit are byte-identical on `6d369fe3`. Unicorn: 630 cases / 1,015 comparisons, 385/385 body starts, 34/34 conditional edges, 0 aperture accesses; 162 RETURNED / 835 LIMIT / 15 CALLEE / 3 TRANSFER; declared shift-AF split 182; all 1,048,576 RAM bytes and 14 CPU words, flag masks 0.
Mutation: 4/4 rejected (shift-af, node-write, residual-flag, cap-branch). Native: 16 cases (PINNED build 5a9c7c53, SHA-256 c05ce28a..., via POOLRAD_NATIVE_DOSBOX; completed calls, five 64 KiB regions). Code body 967 B, SHA-256 `f23626a4d567fce56738f4d2c250c263f5b8b90a716121a070bfab52db85c736`, re-derived from GAME.OVR.

Identity: par-a's two-factor argument in [U7F_IDENTITY_ARGUMENTS.md](U7F_IDENTITY_ARGUMENTS.md) (at `4a6ddec2`). I re-derived every factor-1 site from the
media: each near site decodes as `0E E8` at an instruction boundary inside the caller extent par-a names.

Native oracle: PINNED build 5a9c7c53, SHA-256 c05ce28a..., via POOLRAD_NATIVE_DOSBOX; completed calls, five 64 KiB regions. The primary evidence is the full-1 MiB Unicorn differential.

**063A-specific.** The two excluded input families were constructed and re-run by the checker, and they behave as the
document states:
- **Self-cycle:** a cyclic party list repeats the original's full state with period 3,328 steps, and the C is not run.
  The contract claims no finite execution for cyclic lists, as in DOS.
- **Aperture head:** a party head at `A000:0100` gives an original count of 1 against a flat-callee count of 0. That is
  the declared divergence of the flat `+05E0` callee contract. Party nodes are heap records the game never places in
  video memory, so this is outside the game's input domain, by the same rule as the loaded-state precondition (i21377).
I re-ran factor 1 with `far_call_sites.py 008F:002F`. It finds three fixup-backed sites: `1E:+0DF4`, `20:+106A`, `23:+0663`.
The Unicorn memory observer is limited to the aperture window (`9FFF0..BFFFF`). A full-range read hook corrupts `RETF`
in Unicorn 2.1.4, and no credited audit registers one (census on `ea768440`, i23023).

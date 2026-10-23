# U7F item-record initializer +1010

pr_u7f_caller_1010 translates the complete executable U7F entry 13 body,
composing the raw-CPU +0FD2 class roll, U9E +0DE3 dice chain, and SYSTEM
FillChar. There is no caller-supplied callback, captured-output substitution,
or emulator dependency in the C implementation. Independent verification
and acceptance remain the reviewer's decision; this change edits no ledger.

## Loaded-state precondition

Added 2026-10-22 by par-e (credit authority), VP i21329, ruling i21377. This states a condition the credit always
assumed; the credit is unchanged.

This credit holds **only while every overlay unit the body far-calls is resident**: each stub below must hold the
loaded five-byte far `JMP` (`EA`) to the named entry. The credited C never reads these stubs. It models each `LCALL`
as a direct transfer to the loaded entry, so it cannot refuse when the unit is absent, and a caller must check the
stub before it runs the C. When the unit is not resident, the original's `LCALL` lands in the overlay manager, which
loads the unit with DOS reads. This C does not model that path. The differential in this document runs only the
resident case: it either places a loaded stub or patches each `LCALL` straight to the entry.

| Credit | Stub | Target | Call sites |
| --- | --- | --- | --- |
| `OVL_007F_1010` | `00B0:0048` | unit `0x9E` entry 8 `+0DE3` | `+10C7, +1596, +1611, +1630, +1649, +1662, +167E, +1692, +16AB, +170F, +174B, +17A1, +17E6` |

Stubs were resolved from the original bytes: each `LCALL` segment is matched to its unit's `memory_segment`, and
the slot `0x20 + 5 * entry` gives the entry. SYSTEM (`0AF8`) calls are resident and need no stub.

## Identity and source extent

- target_function: OVL_007F_1010, U7F entry 13, stub 008F:0061 (entry slot 13).
- Original executable interval [1010,18E0), GAME.OVR file
  [25C99,26569): **2,256 bytes, 826 instruction starts**, ending in
  RETF 6 at +18DD.
- SHA-256: 2509b684aaa48da83c6d04bcc270cf3e4b731cd521f08b70ef9fc9f45a2bacd9.
- The schema-8 queue ranks this candidate 31 and reports 2,466 bytes / 921
  instructions. That interval includes trailing data after the RETF. Those
  bytes are not executable coverage or part of this caller's translation.
- Three FillChar far calls, thirteen U9E dice far calls, and one
  PUSH CS; CALL +0FD2 compose the caller. The U9E generator
  includes SYSTEM +11C0 and +120F.
- The six executable spans total **2,489 bytes / 921 starts**. The generator
  also reads the two-byte multiplier at SYSTEM +1245; the harness loads and
  pins it separately. Its data bytes are not instruction coverage.

Media pins: START.EXE SHA-256
 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd;
GAME.OVR SHA-256
 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The audit asserts the entry slot, original fixup sites, hashes, body lengths,
instruction counts and call targets. Original media, listings, capture images
and reports remain in ignored resources/u7f-1010/.

## Behavior and ABI

The initial far-call frame has output offset at SS:SP+4, output segment at
+6, and an item-type byte in a word at +8. The routine clears the 63-byte
output, sets defaults, computes type-dependent class/value/weight fields,
rolls item-specific values, and fills selected fields from the caller's live
DS table at +0970. The table is input RAM, not reconstructed game content.

The public API takes a distinct CPU14 object, exactly 1 MiB of A20-off RAM,
a U9E code segment, a positive caller-loop budget and a per-dice-call SYSTEM
budget. IP must be +1010; initial SP must be in [0040,FFF6]. SYSTEM is
fixed at segment 0AF8 by the existing dice-chain contract. U7F +0FD2 reads
its actual loaded far-call operand and loaded stub jump; the binding must
resolve to the supplied U9E segment at +0DE3. Direct caller dice calls bind
to that same body. This is a loaded-overlay composition, not loader recovery.

The CPU object must not alias RAM. Access is synchronous; original modeled
code, the RNG multiplier, and loader bindings must remain immutable, and
writes must avoid those regions. These are caller preconditions, not a new
runtime memory-protection layer. Words are physically adjacent with A20
wrapping; effective offsets wrap to 16 bits. LES reads its four bytes
physically contiguously. Return frames that wrap the 16-bit SS offset are
outside this caller contract, including frames changed through aliasing.

The shared caller budget counts **taken loop-back entries** +1608, +181A
and +18AA, before their INC instructions. These stops follow defined CMP
flags. Each U9E call receives a fresh SYSTEM-call budget; zero is allowed
and stops at its +0E0E boundary. Status values are 0 full return, 1 initial
argument rejection, 2 caller limit, 3 dice limit, 4 dependency stop, and
5 unexpected direct-callee return. Initial rejection is atomic; all later
stops retain the actual partial RAM/CPU. The caller never invents a return.
A changed outer return frame is used as written; a changed nested return
is exposed as a transfer or dependency stop.

SHL's intermediate undefined AF follows the accepted normal/386 helper
convention. MUL defines CF/OF; its undefined flags are superseded before
any exposed boundary. Neither oracle comparison masks FLAGS. The measured
contract is not a claim about every CPU's undefined-flag behavior.

## Evidence

The main original-code differential passes **2,337** comparisons: 2,332
full returns, three caller limits and two dice limits. The separately
reproducible alias suite passes **eight** more: five full returns, one caller
limit and two actual changed-return transfers. Each compares all 14 CPU
words and the entire 1 MiB of RAM, with zero FLAGS masks and zero emulator
substitutions. Combined: **2,345 cases**, 2,337 returns, four caller limits,
two dice limits, two transfers.

Coverage is **823/826 caller starts and 248/262 caller conditional
outcomes**; the whole chain reaches **917/921 starts and 258/278 outcomes**.
The alias suite adds the +1608 partial-stop case but does not increase these
coverage totals. Unvisited caller sites are +1600, +175D and +1761; the only
unvisited helper site is U9E +0E05. The first caller site skips an iteration
when roll(1,3) is below 1; the latter two require roll(1,3)+1 = 1. These do
not occur in the measured nonalias input domain. This is not a claim that
all RAM aliases preserve those range invariants.

Every unmeasured conditional outcome (source namespace and target offset):

- dice:0DF1->0E1F
- dice:0DFE->0E1F
- dice:0E1D->0E05
- sys:11CD->11D3
- u7f:0FEB->0FF7
- u7f:0FF9->1005
- u7f:1231->123B
- u7f:1480->1486
- u7f:15FE->1600
- u7f:165A->166F
- u7f:16A3->16B8
- u7f:1726->1732
- u7f:1734->173E
- u7f:1738->173E
- u7f:175B->175D
- u7f:17AE->17BA
- u7f:17BC->17C8
- u7f:17F3->17FF
- u7f:1801->180B
- u7f:1805->180B

The main matrix includes all 256 item-type bytes, 256 seeds for each of
eight randomized families, SP endpoints 0040/0041/FFF5/FFF6, relocated code,
A20 aliases, caller limits and zero dice budgets. The supplemental cases
cover output overlapping the RNG seed, source table and two nested return
frames, shared DS/SS, physical word seams, and the +1608 limit.

**37 DOSBox-X normal/386 captures** all return and match CPU14, all 65,536
DS bytes, and 864 SS bytes: windows [EF00,F040), [0000,0100), [FEE0,10000).
That is **66,400 RAM bytes per capture**, without masking flags. The cases
cover type families, several RNG seeds, and all four SP endpoints. Native
partial stops and whole-1-MiB native RAM comparison are not claimed.
DOSBox-X binary SHA-256:
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.

**108 semantic checks** pass with strict C11 warnings and ASan+UBSan.
They check output defaults and table fields, class/value calculation,
partial loop state, the dice cap, actual overwritten return frames, a
malformed binding stop, and atomic initial rejection. The recovered
library build and focused CTest pass (1/1). Native captures, original-byte
reports and the final commit-bound receipt are retained in
resources/u7f-1010/evidence.json and its sibling files.


## Reuse and review assessment

- source_candidates: this complete caller plus existing
  overlay_u7f_roll_class_0fd2_cpu, the U9E entry8 full-state component
  chain, and system_runtime_leaves_cpu.
- Memory/flag primitives reuse accepted U7F +0508 commit
  8fc4f4dfbddd414942d19d25362d3a70c3207019, derived from U7F +0A01.
- Strict +0FD2 dependency: Claude PR #965, commit
  be0bddf39411ecfc3f8bc6a00ceeafb131a2e64f. Its existing implementation
  is composed without edits.
- U9E composition provenance: a540fab9f24874a7a335bc02e56a12c694df9e6e;
  SYSTEM leaves: cfa24e99f63f91a7a7aa058e87d0651e28c3790d.
- Recall query U7F 1010 0FD2 returned Codex session 01a13c11-83c
  and Claude session 281c8700-5ac. Reused their ownership/queue history;
  neither session is parity evidence. Fresh byte extraction corrects the
  historical caller extent.
- signals: entry and fixup identity, complete fixed instruction mapping,
  original full-chain CPU14/RAM comparisons, independent-engine native
  captures, and synthetic semantic/negative tests.
- score: 9/10 for this bounded author profile; verdict: likely_match,
  pending independent review, not self-accepted.
- review_notes: no natural game route, hardware portability, original
  loader behavior, native partial stops, or wrapping far-return frame is
  claimed. Queue readiness alone was insufficient while +0FD2 was a
  callback model; this implementation requires its concrete raw-CPU body.

## Reproduction

With owner media available at ignored resources/DOS/POOLRAD:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u7f_caller_1010_audit.py --output resources/u7f-1010/audit.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u7f_caller_1010_audit.py --aliases-only --output resources/u7f-1010/aliases-public.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u7f_caller_1010_native.py
    cmake -S . -B resources/u7f-1010/build
    cmake --build resources/u7f-1010/build --target poolrad_overlay_u7f_caller_1010_tests
    ctest --test-dir resources/u7f-1010/build -R '^poolrad_overlay_u7f_caller_1010_tests$' --output-on-failure

The native harness reserves the fixed SYSTEM window in its synthetic COM
image and checks its expected COM segment before copying code. DOSBox-X
uses normal core / 386, disabled IRQs during execution, and synthetic input
tables. No original game text, image or data table is checked in.

## Correction, 2026-10-21: the FillChar LCALLs' segment is the live code word, not 0AF8h

The C model entered SYSTEM FillChar with `CS := 0AF8h`, a constant, at its three LCALLs (`1022`, `1044`, `1088`), where the
original's `9A` carries a segment word the loader relocates (`0AF8` only at the unrelocated base): the words at `1025`,
`1047` and `108B`. FillChar's own model never looks at its entry CS, and the audit runs at the unrelocated base, so no
differential could see the difference. Each site now enters at its own word, read from the caller's code. The catcher is a
**link-wrapped spy** (`test_overlay_u7f_caller_1010_live_cs.c`, `-Wl,--wrap=pr_system_fillchar_16a8`): it records the CS
FillChar is entered with and the pushed return IP (which names the site), the three sites get three different words in
three sets (`0AF8` ×3; `1234`/`2345`/`3456`; `0001`/`FFF0`/`10F8`), all 256 type values are run, and all three sites must be
reached (4,610 checks); it fails on the old model (`seen_cs[k]==sets[s][site]`) and passes on the new one. The dice call
sites take their segment from the caller's `dice_cs` argument and were not affected. Body bytes, hash, identity and credit
unchanged.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven in a worktree WITHOUT `resources/` (what CI has) with

```sh
python3 tools/overlay/check_checker_builds.py OVL_007F_1010 --require-current-main
```

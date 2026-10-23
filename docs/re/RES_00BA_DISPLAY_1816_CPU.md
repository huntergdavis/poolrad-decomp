# 00BA +1816: raw node display and optional suffix

pr_r00ba_display_1816_cpu implements resident [1816,1892), composing raw
0709 +0626 and SYSTEM StrLoad +0B0B. The printer composes StrStore, +01DF
and UpCase. All three direct calls execute their C implementations without
callbacks replacing their behavior. This continues the assigned
[16EC blitter plan](RES_00BA_16EC_PORT_HARNESS.md) after merged #1058.
No accounting or credit change is made here.

## Identity and reused evidence

- 00BA:[1816,1892), unpacked image [023B6,02432): **124 bytes / 49
  instructions**, sole terminal RETF 0Ah at +188F. Body SHA-256:
  645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175.
- Direct far calls: +1853/+1887 to 0709:0626; +1882 to SYSTEM 0AF8:0B0B.
  The latter is **26 bytes / 12 instructions**, body SHA-256:
  4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f.
- The length-two Pascal literal at CS:1813 occupies three bytes; SHA-256:
  4bd4cdfadef19a0803e203a5c05b37a6a3e79e76d25454fa559743b912409844.
  Its payload stays private and is read from supplied RAM, never embedded
  in public source. Focused tests use synthetic payloads.
- Owner START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

Reused evidence: [bounded +1816 analysis](RESIDENT_UNEQUAL_DISPLAY_1816.md),
[overlay caller routes](RESIDENT_1816_OVERLAY_CALLER_ROUTES.md),
[raw printer](RES_0709_PRINTER_0626_CPU.md), and
[raw SYSTEM string leaves](SYSTEM_STRING_LEAVES.md). The original bounded
model records eight overlay far-call sites and a separate resident near-call
route; those are not new credit or natural-reachability claims. Existing
queue snapshots are not a fresh readiness ranking.

The implementation's memory/flag primitives and original/native oracle
structure reuse the raw printer work and its accepted glyph/UpCase, memory
bus and EGA dependencies. The required deja query timed out; bounded health
and repair attempts produced no usable health report. No prior-session
claim was reused. The pinned original bodies and public evidence above are
the sources used here. Media, extracted bytes and captures remain ignored.

## Behavior

Five argument words follow the far return pair: flag, row, column, node
offset and node segment. Only the low scalar bytes are consumed initially.
Node byte +10Dh selects color12 when zero. Otherwise byte +10Eh selects
color14 when exactly1 and color11 for every other value.

The first printer receives the node's Pascal string, original row/column
and selected color. After it returns, a zero flag byte skips the optional
path. Otherwise the caller rereads the node pointer and length, adds that
length to the original column as a zero-extended **word** sum, and pushes
that sum. It then copies CS:1813 into SS:BP-4 using raw StrLoad and prints
the copied suffix. StrLoad's RETF4 leaves the destination pointer already
on the stack for the printer call. The epilogue preserves the final
printer's AL and returns through the live frame.

These reads cannot be collapsed into a cached length or column. An explicit
oracle case makes ordinary output overlap node.length; rendering changes
it to FF before the second column calculation. The focused C test checks
the resulting 0102 column word, retained high byte, and final0104 after
two suffix characters. The printer itself consumes/increments only the
low column byte. Its x/y guards and byte wrapping remain unchanged.

## API contract and stop states

- Exactly 1MiB A20-off RAM, attached EGA, initial IP1816, SP[0144,FFF2].
  The lower bound leaves 20 bytes for this frame/first printer call, then
  the printer's required0130 minimum. The upper bound leaves the complete
  14-byte entry frame.
- Initial CS/DS/SS and node segment windows exclude A0000..AFFFF because
  raw StrLoad/StrStore operate on ordinary RAM. CS:1813 must hold a Pascal
  string of length2. Validation occurs before mutation, even if flag0
  would skip the suffix. Payload bytes are supplied memory; the contract
  does not require embedding or exposing the original literal.
- Code, bindings and the literal are immutable; writes avoid them and
  reached return frames do not wrap. Other aliases remain live. CPU/bus/
  EGA/diagnostic objects are distinct from RAM and each other. Callbacks
  only observe supplied operands or refuse OUT; they cannot mutate state.
- Reached +1853/+1887 bindings require far CALL offset0626, +1882 requires
  offset0B0B. Live segment operands select loaded dependencies. CODE
  preserves the pre-call state. A dependency failure returns CALLEE with
  its stopped state and raw status. TRANSFER preserves an unexpected
  successful dependency return. No TRANSFER case was measured.
- Diagnostic callee0 is the first printer, 1 is StrLoad, 2 is the suffix
  printer, -1 is none. The nested printer diagnostic retains its most
  recent call and its glyph/UpCase diagnostics. Each printer call gets a
  fresh printer_loopbacks budget; each glyph gets a fresh glyph_loopbacks
  budget. There is no separate outer loop. Nested LIMIT/PORT/CODE failures
  propagate rather than being treated as successful returns.
- ARGUMENT preserves CPU/RAM/EGA and resets the optional diagnostic.
  Normal/386 flags; no arbitrary-PC resumption, interrupts or faults.
  All +0626 hardware limits apply. Modes0/2 have ordinary-memory parity,
  not CGA/Tandy hardware closure. Native evidence below is EGA mode0Dh,
  read/write mode0, full bit mask.

## Validation

**126 original-byte cases passed**, comparing CPU14, **983,040 ordinary RAM
bytes**, ordered OUTs, EGA write operands and all six diagnostic integers,
with **zero flag masks**. Coverage: **49/49 caller starts, 6/6 conditional
outcomes and 533 composed starts**. Observed **8,308 OUTs and 15,126 EGA
write operands**. Stops: **93 returns, 3 CODE and 30 CALLEE**. CALLEE cases
include printer/glyph budgets, bad nested bindings and refused OUTs, with
failures specifically reached in the second printer call as well.

Cases cover all three color branches, both flag paths, modes0..3, six flag
profiles, empty/255-byte node strings, x wrap, coordinates, source/font
wrapping, stack endpoints/odd stacks/A20, shared DS/SS, source-copy overlap,
shared/ordinary pages, late node-length mutation and deterministic random
inputs. Six dependency starts remain unvisited: glyph +021A/+021D/+022D/
+0235/+05E2 and StrStore +0B3B. Transform1/count1/cap255 explain ordinary
calls' restricted paths; this is not a proof that all alias-sensitive paths
are unreachable or a claim of complete transitive branch coverage.

**44 native DOSBox-X cases passed** under machine=ega, mode0Dh,
core=normal, cputype=386. Each compares **CPU14, SS1024 bytes, DS64KiB,
font-buffer64KiB, node-buffer64KiB and all four 64KiB planes**, with
**zero flag masks**. Separate INT1 guest runs compare **4,760 ordered OUTs**;
plain runs supply CPU/RAM/plane evidence. There are **seven pre-CALL and
four pre-OUT cases**, including second-printer failures, at six distinct
CALL sites and three distinct OUT sites. Two cases use EGA font reads,
including font/destination overlap.

The pre-instruction checkpoints use a far-JMP capture trap without changing
general registers, flags or stack; reported CS:IP names the original
boundary. Final controller/latch state is not independently dumped. Native
runs are synthetic direct executions, not saved-game reachability evidence.
Native RESULT.BIN SHA-256:
c914b38f56168a1357a01b4910c74a323e6227c8df74fddb8435b16a5a130734.

Strict C11 ASan/UBSan passed **2,008 semantic checks**, including expected
pixels for all colors/flag paths, supplied suffix payloads, late length
mutation, exact nested failures in each printer, and argument rejection
without mutation. Focused CTest passed **1/1** on integration base
0c422028361d8d496cad891b4be1ea37ed739ee5, the merge of #1058. Authoring
started on #1058's head; fast-forwarding to the merge changed none of the
**23 dependency pins**, so original/native reruns were unnecessary.
Refreshed origin/main still matched the tested base before publication.

The private receipt resources/1816-evidence.json pins **28 public files and
24 private artifacts**, owner media, the literal identity and tool binaries.
It includes eight compiled C sources, headers/imports, original/native
reports, input/output, guest harness/configuration, dependency pins and
sanitizer/CTest logs. Documentation is excluded to avoid a self-hash cycle.
Capstone5.0.9 and Unicorn2.1.4 were used. Receipt SHA-256:
54297f15366005b1f432f3220052224bb8bc8d45429ac729e20112f9189b92e8.

## Reproduction

Requires Capstone, Unicorn, DOSBox-X, GNU as/ld and a C compiler. Owner
START.EXE stays under resources/DOS/POOLRAD.

    python3 tools/overlay/res00ba_display_1816_cpu_audit.py --unicorn --native --output resources/1816-run.json
    cmake -S . -B resources/1816-build
    cmake --build resources/1816-build --target poolrad_resident_00ba_display_1816_cpu_tests
    ctest --test-dir resources/1816-build -R '^poolrad_resident_00ba_display_1816_cpu_tests$' --output-on-failure

## Author review

- target_function: RES_00BA_1816 complete [1816,1892), three direct calls.
- source_candidates: resident_00ba_display_1816_cpu.c and accepted raw
  printer, StrLoad, StrStore, glyph/UpCase, memory bus and EGA dependencies.
- signals: pinned identities, original operand/call order, live late length,
  complete caller starts/outcomes, original full ordinary-RAM/I/O parity,
  separate native original EGA plane evidence and expected-pixel checks.
- score: strong within the stated CPU/bus contract; original length-two
  literal contract and ordinary node/source storage remain explicit.
- verdict: likely_match within contract; EGA-only native hardware evidence.
- review_notes: no self-credit/accounting changes. Independent reviewer/VP
  decides verification and credit. No natural-game reachability claim.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven in a worktree WITHOUT `resources/` (what CI has) with

```sh
python3 tools/overlay/check_checker_builds.py RES_00BA_1816 --require-current-main
```

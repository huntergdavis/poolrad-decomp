# 03F1 +0137: raw mode-dependent rectangle clear

pr_r03f1_clear_0137_cpu implements resident [0137,02E8), composing raw
0709 +00B7 and bus-aware SYSTEM FillChar +16A8. The rectangle dependency
composes raw page-select +00A0 and EGA-reset +0000. No callback replaces
these effects. This is the next rectangle-path step in the assigned
[16EC dependency plan](RES_00BA_16EC_PORT_HARNESS.md), toward 00BA +17E3.
No accounting or credit change is made here.

## Identity and reused evidence

| Body | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
|03F1:[0137,02E8)|433 / 188|1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc|
|0709:[00B7,0171)|186 / 89|f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5|
|0709:[00A0,00B7)|23 / 10|6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d|
|0709:[0000,0071)|113 / 58|5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5|
|SYSTEM 0AF8:[16A8,16BC)|20 / 7|f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7|

The caller occupies unpacked image [04047,041F8), with terminal RETF8 at
+02E5. It calls +00B7 at +01F1/+020C and FillChar at
+01A5/+01C7/+027E/+029B/+02B8/+02D5. Owner START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

The [cold-start box source map](COLD_START_CODEWORD_BOX_SOURCE.md) supplied
the span and call-chain lead. The accepted
[raw rectangle](RES_0709_RECT_FILL_00B7_CPU.md), memory bus and EGA model
supply dependency semantics. Memory/flag primitives reuse the raw printer
composition. Original/native oracle mechanics reuse the accepted rectangle
and [display composition](RES_00BA_DISPLAY_1816_CPU.md) harnesses. The
loader ledger's 20 overlay call sites do not establish unique callers,
new credit, or natural-game reachability. Old queue snapshots and the
fleet's broad Poolrad task are not a fresh per-body readiness ranking.

The required deja query updated the index but yielded no usable session
hit within its time bound. A bounded deja-health --repair attempt produced
no usable health report. No prior-session claim was reused. The pinned
original bytes and public sources above are the evidence used here.
Original media, extracted bodies and native captures remain ignored.

## Behavior

Four argument words follow the far return pair: bottom, right, top, left.
The mode byte DS:4A1E is loaded once, after establishing the local frame.
The coordinate low bytes are sign-extended to words where used. Arithmetic
wraps at 16 bits, and packed-row comparisons are unsigned.

| Mode | Caller behavior | Evidence scope |
| --- | --- | --- |
|0|Rows from top*4 through bottom*4+3 inclusive; stride80, left*2, width(right-left+1)*2; zero-fill B800 and BA00 each row.|Original CPU/ordinary-RAM parity; native pre-write arithmetic checkpoint only.|
|1|Call raw +00B7 twice, with color0 and pages0 then1, rereading arguments for each call.|Original state/I/O and native EGA planes.|
|2|Rows from top*2 through bottom*2+1 inclusive; stride160, left*4, width(right-left+1)*4; zero-fill B800, BA00, BC00, BE00.|Original CPU/ordinary-RAM parity; native pre-write arithmetic checkpoint only.|
|other|Return without drawing.|Original and native control-state checks.|

Packed modes skip their loops when the unsigned start row exceeds the
unsigned end row. Wrapped widths can request large fills. Mode1 inherits
+00B7's own row/width arithmetic and stop contract. The mode is not cached
before the prologue, and arguments/counters remain live memory reads.
An alias case places SS over B800 so a fill repeatedly clears the live
row counter; another overwrites FillChar's return pair. Both retain their
actual changed state rather than being normalized into a successful clear.

## API contract and stops

- Exactly 1 MiB A20-off RAM, attached EGA, initial IP0137, SP[003E,FFF4].
  The lower bound leaves the 14-byte local frame plus 16 bytes for the
  rectangle arguments/far return, then that callee's 0020 minimum. The
  upper bound leaves the complete 12-byte caller entry frame.
- DS/SS segment windows exclude A0000..AFFFF because the reused rectangle
  helpers access ordinary stack RAM. Code and bindings are immutable,
  outside EGA; writes avoid code and reached return frames do not wrap.
  Other aliases remain live. CPU/bus/EGA/diagnostic objects are distinct
  from RAM and each other. Callbacks observe only supplied operands or
  refuse OUT; they cannot mutate execution state.
- Each reached direct far binding must have opcode9A and offset00B7 or
  16A8 as listed above. Live segment operands select loaded dependencies.
  CODE retains the pre-call state. CALLEE propagates the dependency status
  and stopped state. TRANSFER retains an unexpected successful dependency
  return, including the measured overwritten FillChar return pair.
- Diagnostic callee0 means direct FillChar, 1 the first rectangle, 2 the
  second rectangle, -1 none. rect retains the latest rectangle diagnostic,
  identifying page/reset/fill effects below it.
- max_loopbacks counts taken outer JNEs at +01D2/+02E0. LIMIT stops before
  the exhausted branch; zero permits the first iteration. rect_loopbacks
  is fresh per rectangle call. Nested CODE/PORT/LIMIT become CALLEE.
- ARGUMENT preserves CPU/RAM/EGA and resets the optional diagnostic.
  Normal/386 flags; no arbitrary-PC resumption, interrupts or faults.
  MUL/SHL flag effects are overwritten by ADD/CMP before any observable
  call, branch, OUT, budget stop or return. Their register effects remain;
  comparisons use zero flag masks.

The bus models A0000..AFFFF as planar EGA and other addresses as ordinary
RAM. Modes0/2 therefore do not establish CGA/Tandy hardware closure. Native
hardware evidence below uses EGA mode0Dh, read/write mode0 and full bit mask
at setup. The rectangle temporarily programs write mode2 as in the original.

## Validation

**177 original-byte cases passed**, comparing CPU14, **983,040 ordinary RAM
bytes**, ordered OUTs, EGA byte-write operands and all four diagnostic
integers with **zero flag masks**. Coverage: **188/188 caller starts,
14/14 caller conditional outcomes and 352/352 composed starts**. Observed
**4,216 OUTs and 100,124 EGA write operands**. Stops: **109 returns,
8 CODE, 48 CALLEE, 1 TRANSFER and 11 outer LIMIT**.

Cases cover all modes, signed-coordinate boundaries, reversed rows,
zero/wrapped widths, six flag profiles, eight active pages, complete screen,
stack endpoints/odd stacks/A20, shared DS/SS, prologue/mode aliasing, live
loop-counter mutation, changed return frames, all eight direct bad bindings,
bad nested binding, all 44 OUT event positions across both rectangles,
separate budgets and deterministic random boxes. Complete composed starts
are not a blanket transitive-branch claim; the accepted rectangle's +011A
JA-taken outcome is impossible because its comparison uses unsigned zero.

**63 native DOSBox-X cases passed**, machine=ega, mode0Dh, core=normal,
cputype=386. Each compares **CPU14, SS256 bytes, DS64KiB and all four 64KiB
planes**, with **zero flag masks**. Separate INT1 guest runs compare
**1,769 ordered OUT events**; plain runs supply CPU/RAM/plane evidence.
There are 34 completed mode1 cases, two no-draw modes, five pre-CALL cases
and 22 pre-OUT cases covering every static OUT in the closure.

Two of the five pre-CALL cases stop packed modes0/2 before their first
FillChar, checking signed-coordinate arithmetic before any packed write.
They do not claim native packed framebuffer parity. The other three CALL
checkpoints cover the two outer rectangle sites and its nested FillChar.
Far-JMP capture traps preserve general registers, flags and stack; reported
CS:IP names the original unexecuted boundary. Final controller/latch state
is not independently dumped. Native cases are synthetic direct executions,
not saved-game reachability proof.

Native RESULT.BIN SHA-256:
d1408f56bba2b644c7e2c14accd898dbfe17f930696f4dd7253f9a706b9cc289.

Strict C11 ASan/UBSan passed **4,514 semantic checks**: expected zeroed EGA
pixels on both pages, preserved neighboring pixels, expected packed banks,
active-page restoration, fresh per-rectangle budgets, live mode/counter
aliases, changed returns, exact failure states and rejection without mutation.
Focused CTest passed **1/1** on integration base
5438ae2f027244b0a258e9ecca5a1826de3c5a02. Refreshed origin/main matched
that base; all **20 dependency pins** remained unchanged. No rebase or
repeated oracle run was necessary.

The private receipt resources/0137-evidence.json pins **26 public files and
21 private artifacts**, owner media and tool binaries. It includes seven
compiled C sources, headers/imports, original/native reports, input/output,
guest harness/configuration and sanitizer/CTest logs. Documentation is
excluded to avoid a self-hash cycle. Capstone 5.0.9 and Unicorn 2.1.4 were
used. Receipt SHA-256:
9ecd0aebd8b492d6ad4195ceac9e08bce599a195e2aa7c66187c4a5013866935.

## Reproduction

Requires Capstone, Unicorn, DOSBox-X, GNU as/ld and a C compiler. Owner
START.EXE stays under resources/DOS/POOLRAD.

    python3 tools/overlay/res03f1_clear_0137_cpu_audit.py --unicorn --native --output resources/0137-run.json
    cmake -S . -B resources/0137-build
    cmake --build resources/0137-build --target poolrad_resident_03f1_clear_0137_cpu_tests
    ctest --test-dir resources/0137-build -R '^poolrad_resident_03f1_clear_0137_cpu_tests$' --output-on-failure

## Author review

- target_function: RES_03F1_0137 complete [0137,02E8), eight direct call sites.
- source_candidates: resident_03f1_clear_0137_cpu.c and accepted raw
  rectangle/page/reset, bus FillChar, memory bus and EGA implementations.
- signals: pinned identities, original operand/call order, all caller
  instructions/outcomes and composed starts, exact ordinary RAM/I/O,
  separate native original EGA planes and expected-memory assertions.
- score: strong within the stated CPU/bus contract; native packed-video
  hardware closure remains open.
- verdict: likely_match within contract; EGA-only native hardware evidence.
- review_notes: no self-credit/accounting edits. Independent reviewer/VP
  decides verification and credit. No natural-game reachability claim.

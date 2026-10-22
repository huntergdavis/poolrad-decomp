# 0709 +0DD3: raw mode-dependent rectangle clear

pr_r0709_clear_0dd3_cpu implements resident [0DD3,0F82), composing raw
rectangle +00B7, page-select +00A0, EGA-reset +0000 and bus-aware SYSTEM
FillChar +16A8. The implementation follows the complete caller instruction
stream and preserves both same-segment near calls to the rectangle. This
continues the assigned [16EC dependency plan](RES_00BA_16EC_PORT_HARNESS.md)
and removes another dependency of +0768. The +0BF5 path and its remaining
port-bound dependencies still prevent +0768 closure.

All compiled dependencies were already on main. This change does not depend
on open PRs #1060/#1061/#1062. Old schema-4 queue snapshots are not a fresh
per-body ranking. No accounting changes or self-credit are included.

## Identity and reused evidence

| Body | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
|0709:[0DD3,0F82)|431 / 190|0ca48149f5b50fcb93ed265bb72528f33ae5ec659cd134f205f6f312c50905f2|
|0709:[00B7,0171)|186 / 89|f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5|
|0709:[00A0,00B7)|23 / 10|6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d|
|0709:[0000,0071)|113 / 58|5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5|
|SYSTEM:[16A8,16BC)|20 / 7|f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7|

The caller occupies unpacked image [07E63,08012), ending at RETF8 +0F7F.
Owner START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

The [rectangle design](RES_0709_RECT_FILL_00B7_DESIGN.md) identifies +0DD3
as the next composable sibling. The [reached rectangle evidence](RES_0709_RECT_FILL_00B7.md)
supplies the earlier near-call sites, return frames, and two coordinate
examples. This suite replays those coordinates synthetically; it does not
claim a new saved-game capture. The [raw rectangle contract](RES_0709_RECT_FILL_00B7_CPU.md)
and raw printer arithmetic/stack primitives supply the dependency behavior.
Author work in [PR #1060](https://github.com/huntergdavis/poolrad-decomp/pull/1060)
supplied the +0137 generator/oracle approach. The new public scripts import
only dependencies already on main and independently pin +0DD3's bytes.

The required deja query "Poolrad 0709 0DD3 raw CPU rectangle clear" timed
out after15 seconds while updating the index; no usable session hit was
returned. A bounded deja-health --repair attempt also produced no usable
report. No prior-session claim was reused. Original media, extracted bytes,
generated guest programs and captures remain ignored under resources/.

## Behavior

Four words follow the far return pair: bottom,right,top,left. The mode byte
DS:4A1E is read once after establishing the local frame. Coordinate low
bytes are sign-extended where used, arithmetic wraps at16 bits, and packed
row comparisons are unsigned.

| Mode | Behavior |
| --- | --- |
|0|Rows top*4 through bottom*4+3 inclusive, stride80, left*2, width(right-left+1)*2; zero-fill B800 and BA00.|
|1|Call rectangle +00B7 twice with color0, pages0 then1; reread arguments for each call.|
|2|Rows top*2 through bottom*2+1 inclusive, stride160, left*4, width(right-left+1)*4; zero-fill B800, BA00, BC00 and BE00.|
|other|Return without drawing.|

Packed modes skip when the unsigned starting row exceeds the ending row.
Wrapped widths can request large fills. Mode, arguments, row counters and
return frames stay live, including permitted aliases. Measured aliases
change the mode through PUSH BP, repeatedly zero the live packed-row
counter, or overwrite FillChar's return pair.

## API and stops

- Exactly1MiB A20-off RAM, attached EGA, initial IP0DD3, SP[003E,FFF4].
  The lower bound leaves14 bytes of caller frame plus16 bytes for the
  rectangle arguments/far-return frame, before its minimum0020 stack.
- DS/SS whole segment windows exclude A0000..AFFFF. Code/bindings remain
  immutable outside EGA; writes avoid code; reached frames do not wrap.
  Other aliases remain live. CPU/bus/EGA/diagnostic objects are distinct
  from RAM and each other; callbacks observe/refuse OUT without mutation.
- Far FillChar calls +0E41/+0E63/+0F18/+0F35/+0F52/+0F6F require opcode9A
  and offset16A8. Their live segment operands select the loaded body.
- Near rectangle calls +0E8E/+0EA8 require E8 and relative wordsF226/F20C.
  Each follows PUSH CS at +0E8D/+0EA7. The C model retains that pushed word
  on a rejected binding, then pushes only the return IP when the call is
  accepted. The two successful return offsets are0E91 and0EAB.
- CODE stops before the call; CALLEE retains the exact nested status/state;
  TRANSFER retains an unexpected successful child return. The original
  alias case reaches TRANSFER after FillChar pops a zeroed CS:IP.
- Diagnostic callee0=direct FillChar,1=first rectangle,2=second rectangle,
  -1=none. rect retains the most recent rectangle diagnostic. Outer budget
  counts taken JNE0E6E/0F7A and stops before an exhausted branch; zero
  permits the first iteration. The rectangle budget resets per invocation.
- ARGUMENT preserves CPU/RAM/EGA and resets diagnostics. Normal/386 flags;
  no arbitrary-PC resume, interrupts or faults. MUL/SHL flags are overwritten
  by ADD/CMP before an observable call, branch, OUT, budget stop or return;
  their register effects remain exact. Comparisons use zero flag masks.

The bus models planar EGA at A0000..AFFFF and ordinary RAM elsewhere.
Modes0/2 therefore establish ordinary-memory behavior, not CGA/Tandy
hardware closure. Native hardware validation below is EGA-only.

## Validation

**187 original-byte cases passed**, comparing CPU14, **983,040 ordinary RAM
bytes**, ordered OUTs, EGA byte-write operands and all four diagnostic
integers with **zero flag masks**. Coverage: **190/190 caller starts,
14/14 caller conditional outcomes,354/354 composed starts**. Observed
**4,326 OUTs and104,172 EGA write operands**. Stops: **111 returns,16 CODE,
48 CALLEE,1 TRANSFER,11 outer LIMIT**.

Cases cover every mode, signed coordinates, reversed rows, zero/wrapped
widths, six flag profiles, eight active pages, full screen, odd/end/A20
stacks, shared DS/SS, live mode/counter/return aliases, both opcode and
operand failures at all eight direct calls, nested binding failure, all44
OUT refusal positions, fresh/exhausted budgets and deterministic boxes.
Two additional boxes use the earlier captured coordinates (22,38,17,1)
and (5,38,5,17). Complete composed starts are not blanket transitive branch
closure; the inherited rectangle JA011A-taken outcome is impossible with
its unsigned-zero comparison.

**65 native DOSBox-X cases passed**, machine=ega, mode0Dh, core=normal,
cputype=386. Each compares **CPU14, SS256 bytes, DS64KiB and all four64KiB
planes**, with **zero flag masks**. Separate INT1 guest runs compare
**1,857 ordered OUTs**; plain runs provide state/plane evidence. There are
36 completed mode1 cases,2 no-draw cases,5 pre-CALL cases and22 pre-OUT
cases. Every static OUT site in the closure has a pre-OUT checkpoint.

The pre-CALL cases cover both near rectangle calls, nested FillChar and
packed modes0/2 before their first write. The latter establish arithmetic
state only, not native packed framebuffer parity. Far-JMP capture traps
preserve registers/flags/stack and report the original unexecuted CS:IP.
Final controller/latch state is not independently dumped. Native cases
are synthetic direct executions, not new natural-game reachability proof.
Native RESULT.BIN SHA-256:
7111237d005d1a9c25f1129f9cecf05b2c5bc77d820a94c94dd22bee9010243f.

Strict C11 ASan/UBSan passed **4,526 semantic assertions**: expected zeroed
pixels and preserved neighbors on both EGA pages, packed banks, restored
active page, fresh rectangle budgets, live aliases, changed returns, exact
nested stops and rejected-input immutability. New assertions also inspect
both rejected near-call stack frames, including the retained CS and live
AX high byte in the pushed color word.
Focused CTest passed **1/1** on integration base
5438ae2f027244b0a258e9ecca5a1826de3c5a02. Refreshed origin/main matched
that tested base; all **20 dependency pins** remained unchanged.

Private resources/0dd3-evidence.json pins **26 public files and21 private
artifacts**, including seven compiled C sources, headers/imports,
original/native reports, guest inputs/output/bodies/harness/configuration,
sanitizer binary/log and CTest logs. Owner media and tool binaries are
also pinned. Documentation is excluded to avoid a self-hash cycle.
Receipt SHA-256:
87ce506f87d425470eeb6f632df15cc01c873f18fbcfd9788fd66c28bca82d61.

## Reproduction

Requires Capstone5.0.9, Unicorn2.1.4, DOSBox-X, GNU as/ld and a C compiler.
Owner START.EXE remains under resources/DOS/POOLRAD.

    python3 tools/overlay/res0709_clear_0dd3_cpu_audit.py --unicorn --native --output resources/0dd3-run.json
    cmake -S . -B resources/0dd3-build
    cmake --build resources/0dd3-build --target poolrad_resident_0709_clear_0dd3_cpu_tests
    ctest --test-dir resources/0dd3-build -R '^poolrad_resident_0709_clear_0dd3_cpu_tests$' --output-on-failure

## Author review

- target_function: RES_0709_0DD3 complete [0DD3,0F82), eight direct calls.
- source_candidates: resident_0709_clear_0dd3_cpu.c and pinned raw
  rectangle/page/reset, bus FillChar, memory bus and EGA implementations.
- signals: pinned body identities, original operand order, exact near-call
  frames, all caller starts/outcomes and composed starts, exact ordinary
  RAM/I/O, native EGA planes and expected-memory assertions.
- score: strong within the stated CPU/bus contract; native packed-video
  hardware closure and transitive branch closure remain open.
- verdict: likely_match within contract; EGA-only native hardware evidence.
- review_notes: independent reviewer/VP decides verification and credit;
  no self-credit/accounting edits or new natural-game reachability claim.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: `--compile-only` builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS` is declared. Proven in a worktree WITHOUT `resources/` (what CI has):

```sh
python3 tools/overlay/check_checker_builds.py RES_0709_0DD3 --require-current-main
```

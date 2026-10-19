# 0709 +06A7: raw parent-frame text loop

pr_r0709_text_06a7_cpu implements resident [06A7,0714), composing raw glyph
+01DF (and SYSTEM UpCase +182A) plus CRT Delay 0A40 +029C. It removes one
remaining dependency of +0768 in the assigned [16EC plan](RES_00BA_16EC_PORT_HARNESS.md).
Both child implementations were already on main. This target does not depend
on the still-open clear PRs #1060/#1061. Old schema-4 snapshots are not a
fresh readiness ranking; +0768 still needs its other port-bound callees.
No ledger/accounting changes or self-credit are included.

## Identity and reused evidence

| Body | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
|0709:[06A7,0714)|109 / 41|c9674dcc914fe3e3bb0ed3d54e58776e351f7d3888ccfd15d5b4320d97241bc5|
|0709:[01DF,05EB)|1036 / 403|a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840|
|0A40:[029C,02C9)|45 / 19|d493452e23ca3a0e81d53c6c6e31171d56f543409f3881247d729c5e394e29cb|
|SYSTEM:[182A,183D)|19 / 8|738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c|

The caller occupies unpacked image [07737,077A4), with RETF2 at +0711.
Owner START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

The required deja query "Poolrad 16EC CRT timer sound next 0A40 port CPU"
returned no matches and reported zero indexed sessions. No prior-session
claim was reused. The public [glyph](RES_0709_GLYPH_01DF_CPU.md),
[printer](RES_0709_PRINTER_0626_CPU.md) and
[raw delay](RES_0709_DELAY_0DBE_CPU.md) evidence supplied contracts,
arithmetic/stack primitives and oracle mechanics. Private media, extracted
bytes and native captures stay ignored under resources/.

## Behavior

The single argument is an offset into the current SS, identifying a parent
frame. After PUSH BP, each iteration rereads that offset and its fields:

| Location | Use |
| --- | --- |
|SS:[parent-0103]|Current byte index; unsigned comparison against the end.|
|SS:[parent-0104]|Last byte index, inclusive.|
|SS:[parent-0100+index]|Character passed to glyph +01DF.|
|SS:[parent+000C]|Glyph color byte.|
|DS:5E24 / DS:5E25|Current x / y bytes.|
|DS:4A00|Optional delay gate, reread after the glyph returns.|
|DS:49E2|Delay amount byte, multiplied by10 after zero-extending.|

The glyph receives transform1 and count1. Its near CALL +06E8 follows a
PUSH CS, so the child receives a far-return frame. The delay uses far CALL
+06FD and its live segment operand. On successful child returns, the caller
increments the live index and x as bytes, then jumps back unconditionally.
An end byte255 can therefore make the index wrap indefinitely. Out-of-range
x/y or a no-draw video mode still permit the index/x updates and optional
delay. Other aliases remain live; fields are not snapshotted into host locals.

## API contract and stop states

- Exactly1MiB A20-off RAM, attached EGA, initial IP06A7, SP[002C,FFFA].
  The lower bound reserves18 bytes before the glyph's minimum001A stack;
  the upper bound contains the far return pair and one argument.
- DS/SS whole segment windows exclude A0000..AFFFF. Code/bindings remain
  immutable outside that aperture, writes avoid code, reached frames do
  not wrap. CPU/bus/EGA/diagnostic objects are distinct from RAM and each
  other. Callbacks observe/refuse OUT without mutating execution state.
- Reached +06E8 must contain E8/FAF4, targeting +01DF; +06FD requires
  9A/029C, whose live segment selects CRT Delay. CODE stops before the CALL.
  CALLEE preserves a child status and stopped CPU/RAM/I/O. TRANSFER retains
  an unexpected successful child return; this suite does not independently
  exercise that wrapper path.
- Diagnostic callee0=glyph,1=delay,-1=none; glyph_callee/glyph_status retain
  the latest glyph's UpCase diagnostic even if a later delay stops.
- max_loopbacks counts each executed backward JMP070E, including the last
  iteration before a later guard exit. LIMIT stops before that JMP.
  Glyph and delay budgets reset per invocation. Delay LIMIT retains its
  inherited target-IP convention after the exhausted taken branch; glyph
  LIMIT retains its pre-transfer convention. ARGUMENT preserves CPU/RAM/EGA
  and resets diagnostics.
- Normal/386 flags. MUL's architecturally undefined SZP bits follow its
  product and AF is cleared, matching the accepted delay-wrapper convention.
  Its pre-CALL state is checked against both Unicorn and DOSBox-X, with no
  flag mask. No arbitrary-PC resume, interrupts or faults are modeled.

## Validation

**112 original-byte cases passed**, comparing CPU14, **983,040 ordinary RAM
bytes**, ordered OUTs and EGA byte-write operands, and all four diagnostic
integers with **zero flag masks**. Coverage: **41/41 caller starts,4/4 caller
conditional outcomes,466/471 composed starts**. Observed **6,106 OUTs and
11,490 EGA write operands**. Stops: **62 returns,2 CODE,46 CALLEE,2 outer LIMIT**.

Cases include all modes, all16 colors, six flag profiles, enabled/disabled
and zero delays, fresh and exhausted budgets, zero calibration, maximum
speed, index255 wrap, x wrap, y rejection, shared DS/SS, odd/end/A20 stacks,
parent-offset wrap, parent-index and calibration aliases, missing font,
shared pages, pattern and character boundaries, all three checked call
sites, and36 OUT refusal positions spanning the first and second glyphs.

The five unvisited child starts are +021A/+021D (untransformed character),
+022D (zero-count exit), and +0235/+05E2 (another glyph-count iteration).
This caller normally supplies transform1/count1. No claim is made that all
possible aliases cannot reach those paths; this is not transitive branch
closure. Planar EGA-font reads are covered natively below, not by the flat
Unicorn RAM oracle. Its original cases use ordinary-memory fonts.

**51 native DOSBox-X cases passed**, machine=ega, mode0Dh, core=normal,
cputype=386. Each compares **CPU14, SS1024 bytes including the parent frame,
DS64KiB, font64KiB, and four64KiB planes**, with **zero flag masks**.
Separate INT1 guest runs compare **3,842 ordered OUTs**; plain runs provide
state and plane evidence. Included are **three pre-CALL** checkpoints
(+06E8,+06FD,+0207), **three pre-OUT** checkpoints (+03AD,+03D5,+0470), and
one EGA-aperture font case. Far-JMP capture traps preserve registers/flags/
stack and report the original unexecuted CS:IP boundary.

Native delay runs use a synthetic BIOS byte0040:0000=42h and calibration
counts1/2, with interrupts disabled during the body. This establishes the
measured control/state behavior, not real elapsed milliseconds or hardware
timing. Native hardware evidence is EGA-only: modes0/2 ordinary-memory
parity does not establish CGA/Tandy closure. Final EGA controller/latch state
is not independently dumped. Synthetic execution is not saved-game reachability.
Native RESULT.BIN SHA-256:
851f0b8e620008092275cd4c207e8291d4ca41163961e894d9d726e5723959f4.

Strict C11 ASan/UBSan passed **4,226 semantic assertions**: expected colored
pixels on both pages, preserved neighbors, live index/x updates and wrap,
rejected glyphs, fresh per-call delay budgets, exact nested failure state,
AH/AX delay argument state and rejected-input immutability.
Focused CTest passed **1/1** on integration base
5438ae2f027244b0a258e9ecca5a1826de3c5a02. Refreshed origin/main matched
that tested base; all **17 dependency pins** remained unchanged.

Private resources/06a7-evidence.json pins **23 public files and20 private
artifacts**: six compiled C sources, headers/imports, original/native
reports, guest inputs/output/bodies/harness/configuration, sanitizer
binary/log and CTest logs. Owner media and tool binaries are also pinned.
Documentation is excluded to avoid a self-hash cycle. Receipt SHA-256:
9ad49cb0b0cb850f1ee57f6ef9c4e47d25669717e40d3034eca84c4e8327a557.

## Reproduction

Requires Capstone5.0.9, Unicorn2.1.4, DOSBox-X, GNU as/ld and a C compiler.
Owner START.EXE remains under resources/DOS/POOLRAD.

    python3 tools/overlay/res0709_text_06a7_cpu_audit.py --unicorn --native --output resources/06a7-run.json
    cmake -S . -B resources/06a7-build
    cmake --build resources/06a7-build --target poolrad_resident_0709_text_06a7_cpu_tests
    ctest --test-dir resources/06a7-build -R '^poolrad_resident_0709_text_06a7_cpu_tests$' --output-on-failure

## Author review

- target_function: RES_0709_06A7 complete [06A7,0714), two direct call sites.
- source_candidates: resident_0709_text_06a7_cpu.c and pinned raw glyph,
  UpCase, CRT Delay, memory bus and EGA implementations.
- signals: pinned identities, live operand/call order, every caller start
  and branch outcome, exact CPU/ordinary RAM/I/O, native EGA planes and
  independent expected-pixel/loop-state assertions.
- score: strong within the stated CPU/bus contract; timing, other video
  hardware, transitive branch closure and natural-game reachability remain open.
- verdict: likely_match within contract.
- review_notes: independent reviewer/VP decides verification and credit;
  no self-credit, accounting edits or inherited dependency credit.

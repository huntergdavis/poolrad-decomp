# 0709 +0626: raw Pascal-string printer composition

pr_r0709_printer_0626_cpu implements resident [0626,06A7), composing the
accepted raw SYSTEM StrStore and resident +01DF glyph renderer (which calls
raw UpCase). It replaces neither dependency with a callback. Native EGA
plane evidence covers the composition; modes 0/2 retain ordinary-memory
parity only. No credit or accounting change is made here.

## Identity and reused evidence

| Body | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
|0709:[0626,06A7)|129 / 51|b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed|
|0709:[01DF,05EB)|1036 / 403|a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840|
|SYSTEM 0AF8:[0B25,0B49)|36 / 16|009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118|
|SYSTEM 0AF8:[182A,183D)|19 / 8|738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c|

The caller occupies unpacked load-image [076B6,07737), ends with RETF 0Ah
at +06A4, and calls StrStore at +063C and +01DF at +0691. Owner START.EXE
SHA-256: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

This continues the assigned [16EC dependency plan](RES_00BA_16EC_PORT_HARNESS.md)
after merged +01DF, toward 00BA +1816. The loader ledger records 142 overlay
call sites for +0626; that is not a count of unique callers or new credit.
Existing queue snapshots are not a fresh readiness ranking.

Reused sources are the [first-copy gate](RESIDENT_0709_0626_FIRST_COPY_GATE.md),
[post-copy control](RESIDENT_0709_0626_POSTCOPY_CONTROL.md),
[raw StrStore](SYSTEM_STRSTORE_0B25.md),
[raw glyph renderer](RES_0709_GLYPH_01DF_CPU.md), and its accepted memory bus,
EGA and UpCase dependencies. The earlier +0626 models stop at +064F; this
implementation covers the remaining loop within the contract below.
Memory/flag primitives and oracle infrastructure reuse the glyph work.
The required deja query produced no usable prior-session hit; the pinned
original bytes and public evidence above are the sources used here.
Original executable bodies, inputs and captures remain in ignored resources/.

## Behavior and API

The five words after the far return pair are string offset, string segment,
color, y and x. StrStore first copies the length-prefixed source, with cap255,
into SS:BP-100h. That copy occurs even when the later x/y guards reject the
operation. It retains forward-copy overlap behavior and clears DF.

The outer guard accepts x and y bytes at most39. Empty strings return.
Each character is passed to +01DF with transform1 and count1; that callee
independently rejects y>24 or x>39. The outer loop still increments x after
each successful glyph return, including rejected glyphs. X wraps as a byte,
so a long string starting at39 can resume rendering after wrapping through0.
The copied bytes, local counters and arguments remain live memory reads.

- Exactly 1 MiB A20-off RAM, attached EGA, initial IP0626, SP[0130,FFF2].
  The lower bound leaves the outer frame, glyph arguments and nested glyph
  frame; the upper bound leaves the complete 14-byte entry frame.
- Initial DS/SS and the source string's segment window must exclude
  A0000..AFFFF. StrStore uses ordinary RAM; this API does not add EGA source
  string semantics. The source-segment check precedes execution/mutation.
- Code and bindings are immutable outside EGA; writes avoid code and reached
  return frames do not wrap. Other data aliases remain live. CPU/bus/EGA/
  diagnostic storage is distinct from RAM and each other. Callbacks observe
  only supplied operands or refuse OUT; they cannot mutate execution state.
- +063C must contain far CALL offset0B25, with its live segment binding the
  loaded StrStore body. +0691 must contain near CALL01DF. A bad reached
  binding returns CODE with the pre-call state.
- max_loopbacks counts taken outer JNE +069F transfers. glyph_loopbacks is
  a fresh +01DF budget for each call. An exhausted outer budget returns
  LIMIT before the transfer; zero allows execution through the first glyph.
- Dependency failures, including glyph CODE, PORT and LIMIT, return CALLEE
  with exact stopped CPU/memory. Diagnostic callee0 means StrStore, 1 means
  glyph, -1 means none; status is the raw dependency status. glyph_callee/
  glyph_status retain the inner UpCase diagnostic. Unexpected successful
  dependency return addresses yield TRANSFER. ARGUMENT preserves CPU/RAM/
  EGA and resets the optional diagnostic.
- Normal/386 flags; no arbitrary-PC resumption, interrupts or faults.
  All +01DF bus and hardware restrictions apply. Native mode1 evidence is
  EGA mode0Dh, read/write mode0 and full bit mask. Modes0/2's ordinary-memory
  writes do not establish CGA/Tandy hardware closure.

## Validation

**150 original-byte cases passed**, comparing CPU14, **983,040 ordinary RAM
bytes**, ordered OUTs, EGA byte-write operands and nested diagnostics with
**zero flag masks**. The corpus reached **51/51 caller starts, 8/8 caller
conditional outcomes and 472 composed starts**, recording **4,430 OUTs**
and **8,146 EGA write operands**. Stops: **123 returns, 2 CODE, 23 CALLEE
and 2 outer LIMIT**. CALLEE cases include refused OUTs, glyph budgets and
an invalid UpCase binding. No TRANSFER case was measured.

Coverage includes modes0..3, all16 colors, empty/255-byte strings, coordinate
guards, x wrap, nil font, source/font offset wrapping, stack endpoints/odd
stacks/A20, shared DS/SS, forward/backward source overlap, shared/ordinary
pages, malformed bindings, port refusals, budgets and seeded random cases.
Six dependency starts were not reached: glyph +021A/+021D/+022D/+0235/+05E2
and StrStore +0B3B. Ordinary calls supply transform1, count1 and cap255;
this is not a claim of complete transitive branch coverage or proof that
all alias-sensitive paths are unreachable. The standalone glyph evidence
covers its full caller independently.

**59 native cases passed** under DOSBox-X machine=ega, mode0Dh,
core=normal, cputype=386. Each compares **CPU14, SS1024 bytes, DS64KiB,
font-buffer64KiB, string-buffer64KiB and all four 64KiB EGA planes**, with
**zero flag masks**. Separate INT1 guest runs compare **5,236 ordered OUTs**;
plain runs supply CPU/RAM/plane evidence. Two native cases read font bytes
through EGA, including a font/destination overlap. Three pre-CALL and three
pre-OUT checkpoints use a far-JMP capture trap without altering registers,
flags or stack; reported CS:IP names the original unexecuted boundary.
Controller/latch state is not independently dumped. These are synthetic
direct executions, not saved-game reachability evidence.

Native RESULT.BIN SHA-256:
06eb18cd00537960fb196b88afaa31e6591293645dfad0fc4704798955d6522c.

Strict C11 ASan/UBSan passed **4,241 semantic checks**, including expected
EGA pixels, copy-before-guard behavior, y25/39 rejection by the inner glyph,
x wrap, forward overlap, exact budget/port/binding stops and argument
rejection without mutation. Focused CTest passed **1/1** on integration base
595d9352bd1d080d742951cdc36b62bb1d5846c9. Refreshed origin/main still matched
that tested base; no rebase or repeated oracle run was needed.

The private receipt resources/0626-evidence.json pins **23 public files and
20 private artifacts**, owner media and tool binaries. It includes the six
compiled C sources, relevant headers/imports, original/native reports,
input/output, guest harness/configuration and sanitizer/CTest logs.
Documentation is excluded to avoid a self-hash cycle. Capstone5.0.9 and
Unicorn2.1.4 were used. Receipt SHA-256:
99d44bb4e1cb0b63b06e4d527910ed12e879f962872dd7277b98607047bc072f.

## Reproduction

Requires Capstone, Unicorn, DOSBox-X, GNU as/ld and a C compiler. Owner
START.EXE must remain under resources/DOS/POOLRAD.

    python3 tools/overlay/res0709_printer_0626_cpu_audit.py --unicorn --native --output resources/0626-run.json
    cmake -S . -B resources/0626-build
    cmake --build resources/0626-build --target poolrad_resident_0709_printer_0626_cpu_tests
    ctest --test-dir resources/0626-build -R '^poolrad_resident_0709_printer_0626_cpu_tests$' --output-on-failure

## Author review

- target_function: RES_0709_0626 complete [0626,06A7), two raw direct callees.
- source_candidates: resident_0709_printer_0626_cpu.c, accepted StrStore,
  glyph/UpCase, memory bus and EGA implementations.
- signals: pinned identities, live source-order operands, complete caller
  starts/outcomes, original ordinary-RAM/I/O parity, separate native EGA
  original execution, expected-pixel and rejection tests.
- score: strong within the stated CPU/bus contract; ordinary source string
  restriction and mode0/2 hardware limits remain explicit.
- verdict: likely_match within contract; native EGA evidence only.
- review_notes: no self-credit or accounting edits. Independent reviewer/VP
  decides verification and credit. No natural-game reachability claim.

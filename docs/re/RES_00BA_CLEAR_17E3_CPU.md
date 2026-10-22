# 00BA +17E3: raw fixed rectangle clear selector

pr_r00ba_clear_17e3_cpu implements resident [17E3,1813), composing raw
03F1 +0137, 0709 rectangle +00B7, page +00A0, EGA reset +0000 and bus-aware
SYSTEM FillChar +16A8. No callback replaces their execution. This continues
the rectangle path in the assigned [16EC dependency plan](RES_00BA_16EC_PORT_HARNESS.md).
It depends on PR #1060's raw +0137 implementation. Old schema-4 snapshots
are not a current ranking or blanket closure claim. No accounting changes
or self-credit are included.

## Identity and provenance

The caller occupies unpacked START.EXE image [02383,023B3), **48 bytes and
25 instructions**, ending in RETF at +1812. SHA-256:
9b1d170fef90807823ffd669044689890a0a06446eb13e1c69b09f1c8a62b5f7.
Owner START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
The loader ledger supplied the entry lead; its coarse endpoint is not the
measured function boundary. The adjacent +1813 literal belongs outside this
body and is neither needed nor published here.

The required deja query, "00BA 17E3 0137 rectangle clear raw CPU", returned
no matches and reported zero indexed sessions. No prior-session evidence
was reused. Public [clear composition](RES_03F1_CLEAR_0137_CPU.md) supplied
the child contract, arithmetic/stack primitives, original oracle and native
harness mechanics. Its pinned rectangle/page/reset/FillChar dependencies
remain unchanged. Original media, extracted bytes and captures stay under
ignored resources/.

## Behavior and contract

There are no arguments. The wrapper pushes BP, then compares DS:49F3 to5.
It changes AL and pushes AX four times, preserving the incoming AH in each
argument word. The resulting +0137 arguments are:

| Selector | Call site | bottom | right | top | left |
| --- | --- | ---: | ---: | ---: | ---: |
|5|17F9|21|38|10|23|
|other|180C|22|38|18|1|

The child reads the low coordinate bytes and DS:4A1E mode. Mode1 clears
both EGA pages; modes0/2 use packed ordinary-memory fills; other modes draw
nothing. Arguments, selector and mode are live memory, including stack
aliases. A measured SS=DS, SP49F5 case changes the selector to77h through
PUSH BP before the comparison and takes the second branch. SP4A20 changes
the downstream mode to77h and suppresses drawing.

- Exactly1MiB A20-off RAM, attached EGA, initial IP17E3, SP[004C,FFFC].
  The lower bound reserves14 bytes before the child's minimum003E stack;
  the upper bound contains the four-byte far return pair.
- DS/SS whole segment windows exclude A0000..AFFFF. Code/bindings remain
  immutable outside EGA, writes avoid code, reached frames do not wrap.
  Other aliases remain live. CPU/bus/EGA/diagnostic objects are distinct
  from RAM and each other; callbacks observe/refuse OUT without mutation.
- Reached calls17F9/180C must contain opcode9A and offset0137. Their live
  segment operands select the loaded child body. CODE retains pre-CALL
  state. CALLEE retains the exact child status and stopped CPU/RAM/I/O.
  TRANSFER retains an unexpected successful child return; this wrapper's
  new cases do not independently exercise that path.
- Diagnostic callee0 identifies17F9,1 identifies180C,-1 means no child
  invoked. clear contains the nested clear/rectangle diagnostics.
  Both budget arguments pass to +0137; its rectangle budget is fresh for
  each rectangle. ARGUMENT preserves CPU/RAM/EGA and resets diagnostics.
- Normal/386 flags; zero comparison masks. No arbitrary-PC resume,
  interrupts or faults. All inherited hardware limitations apply.

## Measured validation

**130 original-byte cases passed**: CPU14, **983,040 ordinary RAM bytes**,
ordered OUTs, EGA byte-write operands and all six diagnostic integers,
with **zero flag masks**. Coverage: **25/25 caller starts and 2/2 caller
conditional outcomes**, **376/377 composed starts**. There were **3,426
OUT events and 208,070 EGA write operands**. Stops: **63 returns,2 CODE,
65 CALLEE**.

Cases include both fixed rectangles in every mode, adjacent/non5 selectors,
six flag profiles, eight active pages, varying AH, minimum/maximum/odd/A20
stacks, live selector/mode aliases, both direct bad bindings, eight clear
bindings, nested rectangle binding, all44 OUT refusal positions, and
separate clear/rectangle budgets. The omitted composed start is the child
mode2 reversed-row skip at +025B. Neither ordinary fixed rectangle needs
that skip; no claim is made that all possible aliases cannot reach it.
The inherited rectangle JA011A-taken outcome is impossible with its
unsigned-zero comparison. These counts are not transitive branch closure.

**61 native DOSBox-X cases passed**, machine=ega, mode0Dh, core=normal,
cputype=386. Each compares **CPU14, SS256 bytes, DS64KiB and four64KiB
planes**, with **zero flag masks**. Separate INT1 guest runs compare
**1,505 ordered OUTs**; plain runs provide state and plane evidence.
There are28 completed mode1 cases,4 no-draw cases,7 pre-CALL cases and
22 pre-OUT cases. The calls cover both wrapper sites, both child rectangle
sites, rectangle FillChar, and two packed-mode first-fill boundaries.
All22 static OUT sites in the closure have a pre-OUT checkpoint.

Far-JMP capture traps retain registers/flags/stack and report the original
unexecuted CS:IP boundary. Packed checkpoints occur before the first write;
they establish no native CGA/Tandy framebuffer parity. Native runs are
synthetic direct executions, not natural-game reachability proof. Final
controller/latch state is not independently dumped. Native RESULT.BIN SHA:
1b7100eeb0daaa42e8779e1fe10de3ac94af9887ae50a673f591072191360fdc.

Strict C11 ASan/UBSan passed **44,336 semantic assertions**: expected zeroed
rectangles and neighboring pixels on both pages, packed-bank effects,
active-page restoration, fresh rectangle budgets, AH-preserving pushes,
live selector/mode aliases, exact nested stops, and rejected-input immutability.
Focused CTest passed **1/1** on tested dependency head
bc73fd9516c253e7862522f2a0a56d876f901b36 (#1060). Refreshed origin/main
remained5438ae2f027244b0a258e9ecca5a1826de3c5a02; merge #1060 first.
All23 dependency pins remain unchanged. No dependency oracle was repeated.

Private resources/17e3-evidence.json pins **29 public files and22 private
artifacts**, including eight compiled C sources, headers/imports, original
and native reports, guest input/output/bodies/harness/configuration,
sanitizer binary/log and CTest logs. It also pins owner media and tool
binaries. Documentation is excluded to avoid a self-hash cycle. Receipt SHA:
190154e19b5f33611c6efbbd474929ca8478e71678b82e901c46b04d6fa33600.

## Reproduction

Requires Capstone5.0.9, Unicorn2.1.4, DOSBox-X, GNU as/ld and a C compiler.
Owner START.EXE remains under resources/DOS/POOLRAD.

    python3 tools/overlay/res00ba_clear_17e3_cpu_audit.py --unicorn --native --output resources/17e3-run.json
    cmake -S . -B resources/17e3-build
    cmake --build resources/17e3-build --target poolrad_resident_00ba_clear_17e3_cpu_tests
    ctest --test-dir resources/17e3-build -R '^poolrad_resident_00ba_clear_17e3_cpu_tests$' --output-on-failure

## Author review

- target_function: RES_00BA_17E3 complete [17E3,1813), two direct call sites.
- source_candidates: resident_00ba_clear_17e3_cpu.c and pinned raw child
  clear/rectangle/page/reset/FillChar, memory bus and EGA implementations.
- signals: original body identity, operand/call order, every caller start
  and branch outcome, exact ordinary RAM/I/O, native EGA planes and
  independent expected rectangle/preserved-memory assertions.
- score: strong within the stated CPU/bus contract; native hardware evidence
  is EGA-only, and transitive branch/natural-game reachability remains open.
- verdict: likely_match within contract.
- review_notes: independent reviewer/VP decides verification and credit.
  No self-credit, ledger edits or dependency credit inflation.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven in a worktree WITHOUT `resources/` (what CI has) with

```sh
python3 tools/overlay/check_checker_builds.py RES_00BA_17E3 --require-current-main
```

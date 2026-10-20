# 0709 +06A7: raw parent-frame text loop

pr_r0709_text_06a7_cpu implements resident [06A7,0714), composing raw glyph
+01DF (and SYSTEM UpCase +182A) plus CRT Delay 0A40 +029C. It removes one
remaining dependency of +0768 in the assigned [16EC plan](RES_00BA_16EC_PORT_HARNESS.md).
Both child implementations are on main: the glyph RES_0709_01DF is
strict-credited, and CRT Delay is the merged raw model
`resident_a40_delay_029c_cpu.c`. +0768 still needs its other port-bound
callees. This revision brings the merged raw body (PR #1062) to the full
evidence bar. It changes no ledger, manifest, checkpoint or strict table and
takes no self-credit; the proposed declaration rows are in the PR text.

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

The original author's deja query "Poolrad 16EC CRT timer sound next 0A40
port CPU" returned no matches. For this full-bar revision `deja` was not
installed on the host; Dunking Sheep recall for "RES_0709_06A7 0709:06A7
strict full declaration differential" found the PR #1062 session
(raw composition, 112 cases). Its claims were re-run, not reused: every
figure below is a fresh measurement on main f04d610e. The PR head is rebased
onto c1af4cbf, which changes only two queue documents. The public [glyph](RES_0709_GLYPH_01DF_CPU.md),
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
  not wrap. SS may lie in B0000..BFFFF, where the glyph's CGA/four-page
  stores can alias its own frame; those paths are modeled, not excluded. CPU/bus/EGA/diagnostic objects are distinct from RAM and each
  other. Callbacks observe/refuse OUT without mutating execution state.
- Reached +06E8 must contain E8/FAF4, targeting +01DF; +06FD requires
  9A/029C, whose live segment selects CRT Delay. CODE stops before the CALL.
  CALLEE preserves a child status and stopped CPU/RAM/I/O. TRANSFER retains
  an unexpected successful child return. Inside this contract that happens
  only when a glyph page store aliases the glyph's own far-return pair; the
  alias cases below exercise it.
- Diagnostic callee0=glyph,1=delay,-1=none; glyph_callee/glyph_status retain
  the latest glyph's UpCase diagnostic even if a later delay stops.
- max_loopbacks counts each executed backward JMP070E, including the last
  iteration before a later guard exit. LIMIT stops before that JMP.
  Glyph and delay budgets reset per invocation. Delay LIMIT retains its
  inherited target-IP convention after the exhausted taken branch; glyph
  LIMIT retains its pre-transfer convention. ARGUMENT preserves CPU/RAM/EGA
  and resets diagnostics.
- Normal/386 flags. MUL's architecturally undefined SZP bits follow its
  product's low byte and AF is cleared, matching the accepted delay-wrapper
  convention and Unicorn. Pinned DOSBox-X takes PF from all of DX:AX instead;
  the difference is visible only at the +06FD CODE stop. See the oracle
  split below. No arbitrary-PC resume, interrupts or faults are modeled.

## Media span

The body is START.EXE resident `0709:[06A7,0714)`: loader-expanded image
`[07737,077A4)` (unpack with `resident_loader_map.unpack`, never a raw file
offset), 109 bytes / 41 instructions, SHA-256 `c9674dcc…41bc5` as tabled
above. The audit asserts that the first and only far return in the span is
`RETF 2` at +0711, so the span ends at +0714. The next routine, +0714
(the nested space skip), starts on the following byte.

## Original-byte differential

**116 original-byte cases passed** under Unicorn against the C model,
comparing unmasked **CPU14**, all **983,040 ordinary RAM bytes**, the
ordered OUT stream, the ordered EGA write-operand stream and all four
diagnostic integers. The 65,536-byte aperture `A0000..AFFFF` is also accounted
for, byte-exact. The original's flat aperture must equal its initial bytes
with the ordered write operands replayed, and the model's RAM aperture must
be unchanged because the model routes that traffic into EGA planes. All
**1,048,576 bytes** are therefore checked, **flag masks: 0**. Observed
**6,140 OUTs and 11,554 EGA write operands**.

Coverage is successor-based: an outcome counts only when the instruction
executed after a conditional is that outcome's successor. Flag state is
never inferred.

| Scope | Starts | Conditional outcomes |
| --- | ---: | ---: |
| Caller `[06A7,0714)` | 41/41 | 4/4 |
| Composed (caller, glyph +01DF, SYSTEM UpCase, CRT Delay) | 468/471 | 66/68 |

The audit fails closed: every uncovered composed outcome must appear in the
exclusion table below, and every excluded outcome must stay uncovered.

### Stops, split and named

| Stop | Cases | How it is reached |
| --- | ---: | --- |
| RETURN | 62 | RETF 2 at +0711 after the end guard at +06BA |
| LIMIT | 2 | outer budget exhausted before the backward JMP at +070E (outer-limit0, index-wrap) |
| TRANSFER | 2 | a glyph return lands outside +06EB after an aliasing page store rewrote the glyph's far-return pair (alias-glyph-loop, alias-glyph-return) |
| CODE/+06E8 | 1 | near-call bytes at +06E8 are not E8 F4 FA (bad-near) |
| CODE/+06FD | 2 | far-call bytes at +06FD are not 9A 9C 02 (bad-delay, bad-delay-speed26) |
| CALLEE/glyph/CODE | 1 | glyph's UpCase call site +0207 is changed (bad-upcase) |
| CALLEE/glyph/PORT | 36 | the bus refuses the n-th OUT, n = 0..35, across the first and second glyph |
| CALLEE/glyph/LIMIT | 2 | glyph budget exhausted (glyph-limit0; alias-page-counter at budget 600) |
| CALLEE/delay/LIMIT | 8 | CRT Delay budget exhausted (delay-limit0, delaylimit1/3/7/8, zero-calibration, max-speed, delay-count-alias); delaylimit79/80 return |

ARGUMENT has no original counterpart. The strict C test exercises it with
null bus/CPU, size≠1 MiB, IP≠06A7, SP 002B and FFFB, DS A000 and SS 9FFF,
and checks that CPU, RAM and EGA are left unchanged.

### Aliases inside the contract

The contract excludes only `A0000..AFFFF` from the DS/SS windows. With SS
in `B000..BFFF`, the glyph's mode-2 (four-page) stores to `B800/BA00/BC00/
BE00:DI` land on its own frame. Three cases prove those paths are reachable
and that the model, which rereads every operand, follows them:

- `alias-glyph-loop` (mode 2, SS B000, SP 82A1): a page store rewrites
  loop byte `[bp-1]`. Count-loop continuation +05E2 → +0235 executes, which
  the caller's immediate count 1 would otherwise rule out. A later store
  in that extra pass rewrites the return pair, so this case also ends in
  TRANSFER.
- `alias-glyph-return` (SP 829B): a page store rewrites the glyph's far
  return pair. The child returns elsewhere and the caller reports
  **TRANSFER**, which the earlier suite never exercised.
- `alias-page-counter` (SP 82AE, glyph budget 600): a page store rewrites page counter `[bp-0Eh]` to a value
  outside 0..3. The +0595 JNE is then taken (no page written) and the run
  ends at the stated glyph budget.

### Exclusions

| Outcome / start | Kind | Argument | State that would make it feasible |
| --- | --- | --- | --- |
| +0201 → +021A (starts +021A, +021D) | unreachable-by-composition | Transform byte `[bp+6]` is the immediate 1 pushed at +06E4/+06E6. Between that PUSH and the read at +01FD only PUSHes to lower addresses and `[bp-2]` are written, and frames do not wrap. | `[bp+6]=0` at +01FD, i.e. a caller of +01DF passing transform 0 |
| +022B → +022D (start +022D) | unreachable-by-composition | Count byte `[bp+0Ah]` is the immediate 1 pushed at +06CC/+06CE. Before the read at +0220 only PUSHes, `[bp-2]` and SYSTEM UpCase (no RAM writes) execute. The aliasing page stores all come later. | `[bp+0Ah]=0` at +0220, i.e. a caller of +01DF passing count 0 |

No other start or outcome is excluded. In particular, +0235/+05E2 and the
+0595 JNE are covered by the alias cases rather than argued away.

### Oracle split: PF after `MUL DX` (stated, not masked)

`MUL DX` at +06FA leaves AX = speed×10, DX = 0. The model and Unicorn set PF
from AX's low byte. Pinned DOSBox-X 5a9c7c53 `MULW` sets PF from the parity of
all 32 bits of DX:AX (`PARITY16(ax)^PARITY16(dx)^FLAG_PF`). They differ
exactly when AX's high byte has odd parity (for example speed 26, AX 0104h).
The flag is overwritten by CRT Delay's `OR DX,DX` at +02A2 before any branch,
stop or memory effect. The only stop that exposes it is the +06FD CODE stop,
and that stop needs the call bytes to be mutated, which the contract
forbids. It is therefore **unreachable-by-contract**. It becomes feasible
only with code mutation at +06FD..+06FF and speed in a high-odd-parity band.
`bad-delay-speed26` runs it under Unicorn (equal). The native suite runs it
as a declared split, not a pass: the audit asserts the difference is PF
alone and equals the 16-bit parity of AX. On real 386 hardware PF after MUL
is architecturally undefined, so neither oracle is ground truth there.

## Native corroboration

**51 native DOSBox-X cases passed** (machine=ega, mode 0Dh,
core=normal, cputype=386), plus **1 declared oracle split** (above).
Each compares unmasked **CPU14, SS 1,024 bytes including the parent frame,
DS 64 KiB, font 64 KiB and four 64 KiB EGA planes**. Separate INT1 guest
runs compare **3,876 ordered OUTs**. The native sample is not 1 MiB
evidence and does not repeat the alias, limit or stack-bound cases above.

Synthetic frames and declared stubs:

- A COM program builds a synthetic parent frame at SS:EE00 and the
  call frame (`PUSH 0EE00h; CALL FAR`). Interrupts are disabled around the body.
- Relocations are applied by patching the original segment words: glyph
  +020A → the test SYSTEM segment, caller +0700 → the test delay segment.
  SYSTEM UpCase and CRT Delay execute from their original bytes; no semantic
  stub replaces a callee.
- Synthetic BIOS byte `0040:0000 = 42h` and calibration DS:84FC 1/2. This
  is control/state evidence, not elapsed time.
- Stop capture writes a far JMP to the recorder over the original
  instruction at +06E8/+06FD/+0207 (after a NOP mutation for CODE cases)
  and at +03AD/+03D5/+0470 for pre-OUT checkpoints. The JMP preserves
  registers, flags and stack and reports the unexecuted CS:IP.
- The INT1 single-step observer logs OUTs only. Plain runs supply state and
  planes.

The split case (native case 51, the +06FD pre-CALL stop, AX 0104h) records
DOSBox-X flags 0406h against model flags 0402h. The audit asserts that PF
is the only difference. DOSBox-X executable SHA-256 prefix 99d74601;
native RESULT.BIN SHA-256
537faccfb5f6c8abfe58edf588a2282ecc6b4f9e0ce3f4acd4573a18900c209e.
Combined receipt resources/06a7/final.json (ignored) SHA-256
ac759e89dd729b729b1bb618e577fd2de67be8fe3a73f4855072d214985de4ce, run
11:08–11:14 PDT 2026-10-20 on main f04d610e plus this branch's audit.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0709_06A7',)` registers the audit with the handoff gate.
`--compile-only` builds and loads the strict C sources (`-std=c11 -Wall
-Wextra -Wpedantic -Werror`) before any media access and prints
`CHECKER_BUILD_OK`. The audit is fail-closed. It stops on any composed
outcome that is neither covered nor in the exclusion table, on any excluded
outcome that becomes covered, on an incomplete caller start set, and on a
first RETF other than +0711.

Requires Capstone 5.0.7, Unicorn 2.1.4, DOSBox-X, GNU as/ld and a C compiler.
Owner START.EXE stays under ignored resources/DOS/POOLRAD. On the shared
host every heavy step runs through the global semaphore.

    python3 tools/overlay/res0709_text_06a7_cpu_audit.py --compile-only
    python3 tools/overlay/res0709_text_06a7_cpu_audit.py --unicorn --native --output resources/06a7/final.json
    python3 tools/overlay/check_checker_builds.py RES_0709_06A7 --require-current-main --output resources/06a7/checker-builds.json
    cmake -S . -B resources/06a7-build
    cmake --build resources/06a7-build --target poolrad_resident_0709_text_06a7_cpu_tests
    ctest --test-dir resources/06a7-build -R '^poolrad_resident_0709_text_06a7_cpu_tests$' --output-on-failure

Measured 2026-10-20 (11:14–11:22 PDT):

- `--compile-only`: `CHECKER_BUILD_OK` (sources built with `-Werror`).
- The C test, built separately with `cc -std=c11 -Wall -Wextra -Wpedantic
  -Werror -fsanitize=address,undefined -fno-sanitize-recover=all` over the
  test and the six sources: no warnings, **4,226 semantic checks passed**.
- CMake target `poolrad_resident_0709_text_06a7_cpu_tests`: CTest 1/1
  passed. The project's CMake does not add `-Werror` or sanitizers to this
  target, so this result makes no warning-clean claim on its own.
- `check_checker_builds.py RES_0709_06A7`: run on the PR head after rebasing
  onto current main; the result is quoted in the PR.

## Author review

- target_function: RES_0709_06A7, complete [06A7,0714). Its two direct call
  sites are +06E8 (glyph) and +06FD (CRT Delay).
- signals: pinned loader-expanded identities; first-RETF span; caller 41/41
  starts and 4/4 outcomes; composed 468/471 starts and 66/68 successor-based
  outcomes, with the two exclusions argued by composition and the feasible
  state named; 116 cases on CPU14 + 1,048,576 bytes with zero masks; nine
  named stop classes, including the first TRANSFER witnesses; native EGA
  planes; one declared, unmasked PF oracle split outside the contract.
- open limits: elapsed time; CGA/Tandy hardware (Unicorn ordinary-memory
  only); real-386 MUL PF; natural-game reachability; the absolute stack floor
  of callers. Synthetic frames are not saved-game evidence.
- verdict: likely_match within the stated contract. Verification and credit
  are the review lane's. Not a ledger target (see the declaration note in
  the PR), and no ledger, manifest, checkpoint or strict-table edits here.

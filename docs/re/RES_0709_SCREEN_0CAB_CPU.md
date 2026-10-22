# 0709 +0CAB: raw full-screen clear composition

pr_r0709_screen_0cab_cpu implements resident [0CAB,0D68), composing the
accepted +00B7 rectangle renderer and bus-aware SYSTEM FillChar. Mode1 clears
both EGA pages; modes0/2 clear packed memory banks. The existing resident
ledger identifies two overlay call sites but gives a short candidate bound;
the original bytes establish the complete 189-byte body through RETF +0D67.

This continues the assigned resident EGA work after the
[16EC dependency plan](RES_00BA_16EC_PORT_HARNESS.md). Every direct dependency
is on the tested main. Existing queue snapshots are stale; this is not a
claim of a freshly regenerated ranking. No accounting edits or credit occur.

## Identity and reused evidence

| Body | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
|0709:[0CAB,0D68)|189 / 95|af6f51fbb30aa47320a628752bd4a44cd3736314affce189245c13c068a3f42e|
|0709:[00B7,0171)|186 / 89|f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5|
|0709:[00A0,00B7)|23 / 10|6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d|
|0709:[0000,0071)|113 / 58|5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5|
|SYSTEM 0AF8:[16A8,16BC)|20 / 7|f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7|

The caller occupies unpacked load-image [07D3B,07DF8). The candidate
[loader ledger](resident_loader_ledger.csv) ends at07D74 and contains a
negative decoded count; neither value is used as this implementation's
extent. The following +0D68 Pascal prologue and terminal +0D67 RETF agree
with the hash-pinned sequential decode above. The ledger is left to the
integration/independent-review lane.

Owner START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

Reused sources: the accepted raw rectangle audit and native harness,
raw page selector/reset, bus FillChar, memory bus and EGA model. CPU/flag
primitives reuse the [printer](RES_0709_PRINTER_0626_CPU.md).
The authoring scaffolding also adapts the pending
[+0DD3 PR #1063](https://github.com/huntergdavis/poolrad-decomp/pull/1063);
no +0DD3 code is a runtime dependency, and no acceptance claim is inherited
from that pending PR. Its audit/harness was adapted to this body's no-argument
entry and actual call sites. Both required deja queries returned no matches
and reported zero indexed sessions; no prior-session claim was reused.
Media, extracted bodies, inputs and native captures remain ignored resources/.

## Behavior and API

This routine takes no arguments. It pushes BP, reads DS:4A1E once, and
selects one path:

- Mode0: FillChar zeros8000 bytes at BA00:0000, then B800:0000.
- Mode1: +00B7 fills rectangle(left0,top0,right39,bottom24), color0,
  first page0 then page1. This writes8000 bytes per page through the EGA bus.
  The child restores the selected page using live DS:4A1A.
- Mode2: FillChar zeros8000 bytes at B800,BA00,BC00,BE00, in that order.
- Other modes: return without drawing, preserving DF.

Each 8192-byte packed bank retains its last192 bytes. Ordinary EGA output
also retains page padding and all memory beyond the two drawn pages. The
mode read follows PUSH BP, so a DS/SS alias can change which branch runs.
Packed output may overwrite a reached return frame; that is a measured
TRANSFER stop, not a silently excluded alias.

The public contract requires exactly1MiB A20-off RAM, attached EGA, IP0CAB
and SP[0032,FFFC]. The minimum reserves18 bytes before the child's minimum
SP0020. Whole DS/SS windows exclude A0000..AFFFF because the accepted raw
helpers use ordinary RAM. Code/bindings remain immutable outside EGA;
writes avoid code and reached frames do not wrap. Other data/stack aliases
stay live. CPU/bus/EGA/diagnostic objects are distinct from RAM and each
other; callbacks only observe/refuse OUT without mutating execution state.
Normal/386 flags apply; interrupts, faults and arbitrary-PC resume are
outside the contract.

Far sites +0CC4/+0CD8/+0D25/+0D39/+0D4D/+0D61 require9A/16A8;
their live segment binds the loaded FillChar. Near sites +0CF7/+0D0D
require E8 with F3BD/F3A7 displacements, following PUSH CS and targeting
same-CS +00B7. CODE preserves exact pre-call state, including that pushed CS.

rect_loopbacks is fresh for each rectangle; zero executes its first row,
199 permits all200 rows. Child LIMIT/PORT/CODE propagates as CALLEE with the
stopped CPU and memory. Diagnostic callee0 is FillChar,1 first rectangle,
2 second rectangle,-1 none; rect retains the latest child diagnostic.
Unexpected successful child returns yield TRANSFER. ARGUMENT preserves
CPU/RAM/EGA and resets the optional diagnostic.

## Validation

**150 original-byte cases passed**, comparing CPU14,983,040 ordinary RAM
bytes, ordered OUTs, EGA write operands and all four diagnostic values,
with **zero flag masks**. Coverage is **95/95 caller starts,6/6 caller
conditional outcomes and259/259 composed starts**. The corpus recorded
**3,578 OUTs and1,144,080 EGA write operands**. Stops: **84 OK,16 CODE,
48 CALLEE and2 TRANSFER**. This is complete instruction-start coverage,
not a blanket claim of all transitive conditional outcomes.

Cases cover modes0/1/2 and unsupported values; all eight selected-page
values; six flag seeds; all eight caller bindings (bad opcode and operand);
all44 OUT refusal positions across both rectangles; budgets0/1/198/199;
minimum/maximum and odd stacks; A20 stack wrap; DS=SS; a mode-stack alias;
packed DS bank aliases; and two overwritten FillChar return frames.

**43 native cases passed**, comparing **CPU14,SS256 bytes,DS64KiB and
four64KiB EGA planes**, with **zero flag masks**. Separate traced executions
matched **889 OUTs**. Fourteen complete EGA cases cover selected pages0..7
and six flag seeds; two unsupported-mode cases preserve the planes.
Five pre-CALL and22 pre-OUT cases account for the other27 cases.

Native RESULT.BIN SHA-256:
878124290686d505b773c5cecc36805cb2b7446efa9fe68bff6bf848bc0bfbf7.

Strict C11 ASan/UBSan passed **65,677 semantic assertions**, including full
plane-buffer comparisons, all packed bank bytes and untouched tails, bank
write order, exact nested budget/port/binding states, live mode-stack alias,
overwritten return frames and rejected-input immutability. Focused CTest
passed **1/1** on integration base5438ae2f027244b0a258e9ecca5a1826de3c5a02.
Refreshed origin/main matched that tested base; all20 reused dependency
pins were unchanged.

Private receipt resources/0cab-evidence.json pins **26 public files and
21 private artifacts**, owner media and tool binaries. It includes the seven
compiled C sources, relevant headers/imports, original/native reports, guest
input/output/configuration and sanitizer/CTest logs. Documentation is
excluded to avoid a self-hash cycle. Capstone5.0.9 and Unicorn2.1.4 were used.
Receipt SHA-256:
d822db89db671fa64b7c90a2db6883a3d42ea850dd9cd0d63f77a80c5fcdf5b3.

Native evidence is DOSBox-X machine=ega, mode0Dh, core=normal, cputype=386.
Plain runs provide CPU/RAM/plane results; separate INT1 guest runs provide
ordered OUT traces. Five pre-CALL and22 pre-OUT cases use far-JMP capture
traps at the original boundary without changing the observed registers,
flags or stack. Two of the pre-CALL cases stop before the first mode0/2
FillChar: these do not establish native packed framebuffer behavior.
No final controller/latch dump is independently compared. Native executions
are synthetic direct calls, not natural-game reachability evidence.
Modes0/2 have original-byte ordinary-memory parity, not CGA/Tandy closure.

## Reproduction

Requires Capstone, Unicorn, DOSBox-X, GNU as/ld and a C compiler, with owner
START.EXE under resources/DOS/POOLRAD.

    python3 tools/overlay/res0709_screen_0cab_cpu_audit.py --unicorn --native --output resources/0cab-run.json
    cmake -S . -B resources/0cab-build
    cmake --build resources/0cab-build --target poolrad_resident_0709_screen_0cab_cpu_tests
    ctest --test-dir resources/0cab-build -R '^poolrad_resident_0709_screen_0cab_cpu_tests$' --output-on-failure

## Author review

- target_function: RES_0709_0CAB complete [0CAB,0D68), raw dependency chain.
- source_candidates: resident_0709_screen_0cab_cpu.c, accepted rectangle,
  page/reset, bus FillChar, memory bus and EGA implementations.
- signals: pinned identities, source-order live operands, complete caller
  paths, original CPU/RAM/I/O parity, separate native plane comparisons,
  expected cleared regions and exact rejection/alias frames.
- score: strong within the stated CPU/bus contract.
- verdict: likely_match within contract; native hardware scope is EGA only.
- review_notes: independent reviewer/VP decides verification and credit.
  Old ledger bounds are not executable extents. No self-credit, accounting
  edits or natural-game reachability claim. This does not close the
  unrelated +0BF5 keyboard/sound chain.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: `--compile-only` builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS` is declared. Proven in a worktree WITHOUT `resources/` (what CI has):

```sh
python3 tools/overlay/check_checker_builds.py RES_0709_0CAB --require-current-main
```

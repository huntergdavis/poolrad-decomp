# RES_00BA +1693: transient message, delay, and clear

The raw CPU implementation composes five original call sites through four
existing contracts. It copies at most 40 bytes from a live Pascal string into
SS:BP-41, clears row 24, prints at column 0 / row 24 / color 10, delays, clears
row 24 again, and returns with RETF 4. No callee is replaced by a callback.
This PR supplies the body and evidence; independent acceptance must add the resident readiness declaration and credit together.

## Identity and ownership

Loader-expanded START.EXE [02233,0228C), resident 00BA:[1693,16EC), is
**89 bytes / 44 instruction starts**, ending at RETF 4 at +16E9. Body SHA-256:
6db9222ad42386393622a6614d2ab37cc1f2eec0a7d65b2e943bbc2e9593581a.
The extent comes from executable bytes, not the queue row or a raw EXE offset.
The expanded image is 68,592 bytes, SHA-256
a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e.

Owner media SHA-256:

- START.EXE: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- GAME.OVR: 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

| Caller site | Original target | Composed API |
| --- | --- | --- |
| 16A7 | SYSTEM 0AF8:0B25 | pr_system_strstore_0b25 |
| 16B8 | resident 03F1:0137 | pr_r03f1_clear_0137_cpu |
| 16CB | resident 0709:0626 | pr_r0709_printer_0626_cpu |
| 16D0 | resident 0709:0DBE | pr_r0709_delay_wrapper_0dbe_cpu |
| 16E1 | resident 03F1:0137 | pr_r03f1_clear_0137_cpu |

The checker verifies the five expanded-image segment relocations at 0224A,
0225B, 0226E, 02273 and 02284. It also checks **42 overlay segment fixups**
whose original far CALL names 00BA:1693. They include U38:+0D49/+316F and
U77:+1892, the calls within the three proposed newly ready callers. All
incoming sites are emitted in the private JSON, with unit and fixup offset.
This establishes game-specific ownership, not natural saved-game reachability.

## Reused evidence

Dunking Sheep memory context for RES_00BA_1693 returned Claude session
281c8700-5acc-438b-b93d-c061e28437eb, identifying this as the U38:+3134 blocker.
That is used only as a discovery lead. The historical ledger snippet from
4d9851e1-f261-48b9-9cfa-5876d1500eb7 is not used to size the body.

The source memory/flag primitives and synthetic fixture mechanics reuse
[raw +1816](RES_00BA_DISPLAY_1816_CPU.md),
[raw clear](RES_03F1_CLEAR_0137_CPU.md),
[raw printer](RES_0709_PRINTER_0626_CPU.md), and
[raw delay](RES_0709_DELAY_0DBE_CPU.md). Their C callees remain unchanged.
The Python device bus is independent of the C EGA implementation; its
planar equations are checked against all planes, latches and indexed state.

The older graphics differentials compared only 983,040 ordinary RAM bytes,
excluding A0000:AFFFF and checking write operands separately. **That is a
64 KiB RAM comparison mask**, not a full RAM comparison. This checker uses
MMIO for the device aperture and compares the entire separate backing RAM
array, including its unchanged aperture backing bytes. It additionally
compares all EGA planes/latches/registers, ordered OUT events and EGA write
operands. No range is sliced out of the new RAM comparison.

## Contract and failures

Entry requires exactly 1 MiB A20-off RAM, attached EGA, IP1693, and
SP[016A,FFF8]. The lower bound reserves the 58 bytes between root entry and
printer entry, plus that printer's 0130 minimum. CS/DS/SS and initial source
segment windows exclude the EGA aperture because StrStore and Delay use
ordinary RAM. Code/bindings are immutable, writes avoid code, and reached
return frames do not wrap. Other aliases stay live. No arbitrary-PC resume,
interrupts, faults or timing guarantee is claimed. Port observers may refuse
OUTs but cannot mutate state.

Each far-call opcode/offset is checked when reached; its live segment binds
the dependency. CODE stops before the CALL. CALLEE preserves a dependency
failure and nested diagnostics. TRANSFER preserves a successful dependency
return to an unexpected address. The final caller RETF consumes its live
frame, including a modified external return address. Diagnostics identify
call0=StrStore, call1=first clear, call2=printer, call3=delay, call4=second clear.
Clear/rectangle/printer/glyph loop budgets reset per call; delay has its own
budget. ARGUMENT preserves CPU/RAM/EGA and resets optional diagnostics.

## Comparison scope and native caveats

The original-byte differential executes the full selected call chain under
Unicorn and compares all 14 CPU words, **all 1,048,576 backing RAM bytes**,
all 262,164 EGA state bytes, OUT events and EGA write operands. Flag masks: 0.
RAM excluded ranges: none. A separately implemented Python MMIO device
provides original-side reads, writes and controller changes; it does not call
C EGA functions. The device aperture and backing RAM are distinct state.
The comparison includes modes 0/2 ordinary-memory effects but does not model
CGA/Tandy address decoding or display timing. EGA/VGA write mode 3 is excluded
by the existing EGA contract; the oracle fails if reached.

Native DOSBox-X runs use machine=ega, video mode 0Dh, core=normal, cputype=386.
They compare CPU14, SS 1024 bytes, DS/font/source 64 KiB each, and four 64 KiB
planes. **Native RAM is partial: every byte outside those 197,632 captured
RAM bytes is masked by omission.** For the measured COM segment 0814, physical
half-open captured ranges are [16D40,17140) (SS 1024), [40000,50000)
(source 65536), [50000,60000) (DS 65536), and [70000,80000) (font 65536).
The exact native RAM masks are:

| Half-open physical range | Excluded bytes |
| --- | ---: |
| [00000,16D40) | 93,504 |
| [17140,40000) | 167,616 |
| [60000,70000) | 65,536 |
| [80000,100000) | 524,288 |
| Total | 850,944 |

These are limitations of this synthetic capture harness: DOS/BIOS, loader,
code, capture machinery and uninitialized regions outside its controlled
windows are not replayed as identical RAM fixtures. This is not a claim
that DOSBox-X cannot dump them. The A0000:AFFFF subset of the final masked
range exposes EGA device memory, not the separate unused RAM backing array;
all four device planes are captured independently. Full backing RAM parity
is supplied by the original-byte differential, not by this native dump.
No flag masking occurs. Final indexed
controller state and latches are not captured natively: four latch bytes,
sequencer index plus five register bytes, and graphics-controller index plus
nine register bytes (20 bytes total). Native plane readback itself changes
the latches and controller indices, and this harness takes no earlier
controller snapshot. The independent Python MMIO differential compares all
20 bytes; no separate physical-hardware proof for them is claimed. Native mode 0/2 cases stop before the first
clear and establish no CGA/Tandy rendering claim. BIOS byte 0040:0000 is
captured and replayed for Delay, with interrupts disabled around execution;
this is instruction-state corroboration, not elapsed-time calibration.

Native cases are synthetic direct calls with supplied data/fonts, relocated
original bodies, and synthetic entry frames. There are no successful callee
stubs. A far JMP replaces a selected reached CALL/OUT only to capture its
pre-instruction state; reported CS:IP is that original boundary. These traps
are not natural game paths. Pre-delay captures retain the drawn message
before the final clear erases it. Native runs do not independently trace OUT
ordering; the original-byte differential does.

## Validation

**172 original-byte cases passed**: CPU14, all 1 MiB backing RAM, all 262,164
EGA state bytes, 42,384 ordered OUTs and 162,066 EGA write operands. The
original device also performed 192 EGA reads. Caller coverage 44/44;
transitive coverage 896/903 starts and 81/92 conditional outcomes.

| Original stop class | Cases |
| --- | ---: |
| Return through the live external frame | 129 |
| Root CODE (one at each of five CALL sites) | 5 |
| Nested CODE (clear, FillChar binding, printer, glyph, delay wrapper) | 7 |
| Nested LIMIT (clear 1, rectangle 1, printer 1, glyph 1, CRT delay 5) | 9 |
| Refused OUT (first clear 8, printer 8, second clear 1) | 17 |
| Nested clear TRANSFER from live screen/stack alias | 4 |
| Caller TRANSFER after changed first-clear return | 1 |

Delay LIMIT checkpoints include 02C0, 02BD and 02B0; a count-zero fixture
retains the original 65536-pass semantics until its explicit budget stop.
The B800 stack cases exercise modified return pairs, including the final
external RETF frame. Source-copy overlap remains byte-by-byte live. Other
cases cover length 0/1/3/40/41/255, modes 0/1/2/3/255, flag profiles, stack
endpoints/odd stacks/A20, shared DS/SS, font/source wrap, EGA font reads and
font/output overlap, active-page profiles, equal pages, ordinary pages,
missing font, and deterministic random inputs.

**43 native cases passed**: 29 returns, 7 root CODE captures, 5 nested CODE
captures and 2 pre-OUT captures. CPU14 and 197,632 selected RAM bytes plus
262,144 plane bytes compare without flag masks; exact native RAM masks and
unobserved controller state are listed above. Native RESULT.BIN SHA-256:
1d133a3dc40de451f3aa778fa0abe7767c18051fcc3f36b06b977f0799450d54.

ASan/UBSan passed 21,424 semantic checks, including all 256 source lengths
with/without offset wrapping, forward overlap, exact copied payload and
stopped CPU state, and ARGUMENT nonmutation. Focused CTest passed 1/1;
queue regressions passed 17/17. The registered compile/load gate passed
13 actual checker builds on fetched current main 4c5b802c.

The caller itself has no conditional jump: all 44 starts, all five calls and
its terminal return are exercised. The composed inventory contains 903 starts
across existing callees as well as the caller. Seven callee starts are not
visited by these composed fixtures: clear+025B and glyph+021A/+021D/+022D/
+0235/+05E2, plus printer+064D. The report lists every unvisited conditional
outcome too. The ordinary caller supplies fixed coordinates and the printer
supplies transform 1/count 1, restricting normal callee paths. This observation
is **not** a proof that aliases make every missing path unreachable. No 100%
transitive branch-coverage claim is made; the previously accepted raw callee
contracts are reused, and their broader standalone domains remain distinct
from complete coverage of the new 89-byte caller.

## Reproduction

    python tools/overlay/res00ba_display_1693_cpu_audit.py --compile-only
    python tools/overlay/res00ba_display_1693_cpu_audit.py --unicorn --native --output resources/1693.json
    cmake -S . -B resources/build1693
    cmake --build resources/build1693 --target poolrad_resident_00ba_display_1693_cpu_tests
    ctest --test-dir resources/build1693 -R '^poolrad_resident_00ba_display_1693_cpu_tests$' --output-on-failure
    python tools/overlay/check_checker_builds.py --all --require-current-main --output resources/checker-builds.json

The registered compile-only checker builds, loads and binds the actual
shared library before touching game media. Original bytes, native binaries,
captures and full receipts stay under ignored resources/.

## Acceptance and measured readiness

The queue loader requires a resident manifest declaration to name a
strict-credited resident ID. Therefore independent acceptance must add both
rows atomically; this author PR edits neither manifest nor credit map.
Proposed partial_callee_models.csv line (text only):

    RES_00BA_1693,full,docs/re/RES_00BA_DISPLAY_1693_CPU.md

The corresponding acceptance-only strict_full_c_addresses.csv line is:

    RES_00BA_1693,docs/re/RES_00BA_DISPLAY_1693_CPU.md

A **simulated acceptance measurement, not credit**, used one private strict
map containing +1693 in both runs, and manifests differing only in that
full declaration. Fresh inventory, callgraph and validated near-call products
were generated from the pinned media. Composable rows changed **60 to 63**;
newly composable rows are OVL_0038_0CC3, OVL_0038_3134 and OVL_0077_17ED.
No row lost composability. This measures the declaration effect under the
explicit counterfactual acceptance assumption; it does not claim the current
main branch already has that readiness. Before/after products and input hashes
are retained in resources/readiness-delta.json.

Resident declarations are credit-gated; overlay full manifest rows alone
confer neither strict credit nor readiness; readiness must be re-derived from
the actual queue products rather than inferred from a checkpoint count.

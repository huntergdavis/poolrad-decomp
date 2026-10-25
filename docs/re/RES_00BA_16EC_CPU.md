# RES_00BA_16EC window message: candidate CPU model, closure blocked

This is **evidence work, not a strict unlock**. The candidate translates
`00BA:[16EC,17E3)` (247 bytes, 116 instruction starts, `RETF 0Ch` at 17E0)
and composes its display, printer, box, delay and string-copy callees.
The current refreshed base is `4b0ce9fd6bb57fc5c40691411eb750b012f39ba3`. It
adds no strict, identity, runtime-membership or REPLACED row.

The branch starts from `a611ec51b86dfdce65033dc553b2a42ad0f8f22a` and resumes
par-d2's parked `b17239fdefa85df13c7f9208aa5c1d1e20e0d878`. Reused evidence:
[16EC's identity](RES_00BA_16EC_CLOSURE_IDENTITY.md),
[the t100 NO result](RES_03F1_02E8_IDENTITY.md), and the existing raw models
named below. The original task's 1,193-byte one-hop figure is an earlier
closure price, not this body's size and not newly verified C bytes.

## Exact blocker

16EC has a passing identity argument, but its in-domain dependency
`RES_03F1_0000` is not an eligible ready-helper member. The near caller
`RES_03F1_02E8` still lacks the required argument-borne identity evidence:
t100 measured literal text at all 27 overlay and seven resident direct
sites. Game cells gate those branches; they do not supply the text object.
Composing 03F1_0000's existing candidate model does not waive that gate.
This PR therefore does not unlock 16EC or any downstream row.

## Candidate behavior and contract

The body copies the caller's Pascal string into `[BP-100h]` via SYSTEM
`0B25`, then chooses a display branch using `DS:49F3 == 5`. The other branch
selects colour 11h/12h from `DS:6DDA`; colour 11h also draws a box. Both
branches clear, display a title, and print the local message. A nonzero
`[BP+6]` calls the delay wrapper and near clear before the far return.

The seven far-call sites and three near-call sites are checked against the
original. The model checks each call opcode/offset at execution and reads
its relocated segment from RAM. Its closure uses `03F1:0137`, near `1816`,
`0709:0768`, `03F1:0000`, `0709:0DBE`, and near `17E3`. The printer's
`STOP_BF5` invokes `0709:0BF5` and resumes at `0999`; the box follows the
loaded stub into `OVL_040C_015D`. Named limits and transfer stops remain
observable; callbacks do not pretend that an unmodelled callee returned.

Entry requires 1 MiB RAM, IP 16EC, SP in 0800h..FFF0h, valid CPU/bus,
budget and environment pointers. Each own-body instruction consumes one
budget unit; separate environment limits bound the callees. Code is
immutable. Own stack/DS accesses and flat runtime string operands exclude
A0000h..AFFFFh aliases by contract, without a dynamic guard. The bus/EGA
comparison covers display accesses through bus-aware callees, not those
excluded aliases. The printer register typedef is renamed to avoid a
collision when its header is included with the existing display printer.

## Validation and negative evidence

The checker compares CPU14, every RAM byte, EGA state, ordered EGA port
writes, VRAM write operands, environment events, stop reason and remaining
budget. Flags use the existing explicitly declared pinned DOSBox-X splits
for callee shifts/MUL/DIV; there is no blanket FLAGS mask. Independent
checks cover the local string, branch colour and `RETF 0Ch` frame.

The initial full run **failed** at `m5-f0-w0-max`: the original exceeded
six million instructions on the 255-byte message. This is not a passing
comparison. The existing [printer evidence](RES_0709_PRINTER_0768_CPU.md)
explains that a final word ending at index 255 never terminates inside
06A7 because its byte index wraps; that callee has a declared LIMIT.
This is a known original-program limitation, but the t24 checker has not
yet compared a finite prefix at that callee limit. A diagnostic rerun retains per-case failures and exits nonzero
if any occur. The full 386-case set includes both display branches, wait
and colour choices, empty/long messages, stack edges, frame aliases,
flags, graphics modes, instruction-budget boundaries and seeded worlds.
Final diagnostic results will be recorded here before review is requested.

Six mutations cover mode selection, colour, copy length, wait handling,
return cleanup and budget consumption. The same eight selected cases must
pass unmodified before mutation compilation starts; the baseline is saved
first. Compiler failures are not kills. Boundary C tests check invalid
arguments and SP, budget stops, call-site guards, and a synthetic string
copy without original game media. These checks are pending in this draft.

There is **no native 16EC oracle** in this PR: `--native` fails explicitly.
The parked checker had accepted `--native` and `--mutate` without executing
either; this PR removes that misleading behavior and implements mutations.
The two new flag-helper census rows are source classifications against
DOSBox-X `5a9c7c53481c2e933948039c33256776653f4faf`, not native evidence.

## Review record

- target_function: `RES_00BA_16EC`, complete 247-byte body.
- source_candidates: `resident_00ba_window_16ec_cpu.c`, resumed from the
  parked model; no alternate source candidate is asserted.
- signals: exact decoded extent/return, seven far and three near calls,
  mode/colour constants, stack string object, composed side effects, and
  original-byte state comparisons.
- score: incomplete evidence; no numerical confidence or strict score.
- verdict: **UNVERIFIED full parity; strict closure BLOCKED**.
- review_notes: original timeout unresolved; native evidence absent;
  aperture aliases excluded; declared callee flag splits; ready-helper
  membership unresolved. Names and plausible behavior alone are insufficient.

Reproduce from the repository root with the audit Python environment:

```sh
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res00ba_window_16ec_cpu_audit.py --unicorn --keep-going --output resources/t24-16ec/diagnostic-full.json
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res00ba_window_16ec_cpu_audit.py --mutate --output resources/t24-16ec/mutations.json
/home/hunter/workspace/poolrad-heavy.sh cmake --build resources/t24-16ec/build --target poolrad_resident_00ba_window_16ec_cpu_tests poolrad_resident_0709_printer_0768_cpu_tests -j2
/home/hunter/workspace/poolrad-heavy.sh ctest --test-dir resources/t24-16ec/build -R '^poolrad_resident_(00ba_window_16ec|0709_printer_0768)_cpu_tests$' --output-on-failure
```

All executable media, derived artifacts and full reports stay under ignored
`resources/`; the resources symlink is not tracked.

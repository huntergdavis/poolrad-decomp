# U3C2 +0909 complete candidate-list caller

The full-state entry `pr_u3c2_caller_0909` composes the complete U3C2
entry 6 with its original table-pair, direction-predicate, line-check and
sort callees. It builds four origin coordinates from a shape table,
visits the party entries at DS:5F2A, tests pairs of origin/target
coordinates, retains a successful minimum-distance pair, records a
direction, and sorts the resulting three-byte candidate records.

The implementation preserves CPU14, instruction-ordered shared RAM,
actual nested call frames and returns. It replaces neither the callees
nor their memory effects with scalar answers. This is an author evidence
submission; independent review owns acceptance and credit.

## Queue ownership and provenance

This was the next uncompleted composable hard target at schema-8 rank 26,
following U25 +01B6 and U7F +0508. The schema-4 queue workflow now uses
the current schema. The prior unmerged branches
`b22dbb1900a32b3d64b1f96eac03a9d7ee89c7ca` and
`54c0dd6a0bc69a76f2dc23ac800f444b577b993c` model only two call windows
and four entry instructions. Their whole-caller parity was explicitly open.

Recall `timeout 20 deja "U3C2 0909"` found historical census/entry-gate
discussions in Codex session `01a13c11-83c`, and the Claude lane session
`281c8700-5ac` noted the old caller-prefix branch. These findings guided
the ownership check; current source and original bytes establish parity.

The complete caller is new. Its memory/arithmetic primitives are adapted
from accepted U7F +0A01 (`92db1c801c9dc5db7c12a1872d62c9a86e3e83d8`).
The fixed callee implementations are linked unchanged:

| Callee | Source provenance |
| --- | --- |
| U3C6 +0000 table pair | `73d03b250fbeddbde02cef72be215603dd12b9ac` |
| U3C2 +057B direction predicate | `39766d7494fe32241cb01ca687bc3da5b6fa607a` |
| U3C2 +0422 closed line check, including +01A0/+0000/+02CC | `343450b380f7a78e7c0f409bd42604c635e80687` |
| U3C2 +002E sort, including SYSTEM Move | `a9fc7192ecab0f1b56dddbaaa829643f63a2dfc0` |

The audit reuses the accepted line-check source auditor and the CPU
structure/memory helpers from its initializer audit. Whole-RAM and native
capture structure follows the prior U7F +0508 author submission,
`8fc4f4dfbddd414942d19d25362d3a70c3207019`.
Separate callee CPU types are copied by named fields in both directions.

## Source identity

Owner media remains under ignored `resources/`.

| Artifact | SHA-256 |
| --- | --- |
| START.EXE | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U3C2 [0909,0BD9), 720 bytes / 297 instructions | `38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed` |
| U3C2 [0000,002E), 46 bytes / 17 instructions | `5e8e5fc8a5907fee2efd922fc4f96682fd1f2e8e58cbebcd058d3f025948e5bf` |
| U3C2 [002E,01A0), 370 bytes / 158 instructions | `81e890a18c92bd83b78b50a33c1295975183a2706ec5b03b52e7cf550544ca3f` |
| U3C2 [01A0,02CC), 300 bytes / 99 instructions | `7fbacb8d9d298c72bd886321a58a8d842f2ec6dc350fc83fde6064fe78f27f40` |
| U3C2 [02CC,0422), 342 bytes / 111 instructions | `818132a151e78bff884848a874203a09a50b6d459e0ad9f5e7278de692d28337` |
| U3C2 [0422,057B), 345 bytes / 141 instructions | `f075c6dba0b31e12aea19318958a72f4caee6fe7954c53f3583dd5dd9fa8114d` |
| U3C2 [057B,0909), 910 bytes / 410 instructions | `59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae` |
| U3C6 [0000,0068), 104 bytes / 41 instructions | `05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207` |
| SYSTEM [0A3F,0A57), 24 bytes / 9 instructions | `5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa` |

The closed chain has **3,161 instruction bytes / 1,283 sites**.
The caller is U3C2 entry 6, stub 03DB:003E, GAME.OVR file
[3045B,3072B), with RETF 0Eh at +0BD6. Its far calls +092A and +09D8
resolve stub 03E0:0025 to U3C6 +0000; segment fixups are +092D/+09DB.
PUSH CS/near calls at +0AA2/+0BA4 reach +057B, +0AF5 reaches +0422,
and +0BD0 reaches +002E. The final sorter makes the original three
SYSTEM Move calls. The oracle executes every reached nested instruction.

## ABI and bounds

Enter at IP=0909 after far CALL. SS:SP contains return IP/CS.
Seven argument words follow:

| Initial SP displacement | Argument |
| --- | --- |
| +4, +6 | Map offset and segment |
| +8 | Origin shape type, low byte used |
| +10 | Direction mode, low byte used |
| +12 | Radius/distance word |
| +14, +16 | Origin Y and X, low bytes used |

RAM is exactly 1 MiB with A20 disabled; the CPU is a separate stable
host object and access is synchronous. Initial SP is [008E,FFEE].
The lower bound reserves the caller's 34-byte frame, the line-call
24-byte argument/return frame and its deepest 84-byte stack use.
The upper bound keeps the initial return/argument frame unwrapped.

All loaded modeled code and the supplied bindings are immutable, and
writes must avoid code. This is a precondition, not a general dynamic
code-write guard. U3C6 +0000 is bound by `pair_cs`. The accepted
sorter reads its SYSTEM call operands from the loaded +002E code.
Its existing CODE rejection is retained as a callee stop.

Effective offsets wrap at sixteen bits; word/LES bytes are physically
contiguous with twenty-bit wrap. The pair helper uses its explicit
WRAPPED_SS return policy. Under the reserved stack bounds, its return
frame is unwrapped, so that policy agrees with the measured native frame.
No conclusion about an initially wrapped frame is inferred from this.

All counters, coordinates, table bytes, arguments, output pointers,
candidate records and return frames remain live. Signed coordinate checks,
unsigned distance comparisons and indeterminate predicate results in
direction modes 9..254 retain the original behavior.

## Partial-state contracts

Three independent positive host budgets bound otherwise unbounded alias
behavior. They do not assert original termination.

- The caller budget counts visits before +0918, +0993, +09BA, +0A48,
  +0A61 and +0B73, shared across all six loops.
- Each +0422 invocation receives a fresh line budget counting visits
  before +04DD.
- The final +002E sort receives the specified cap on taken loop-backs;
  a declined back-edge stops at +0056 or +0072 before its INC.

Limits retain every prior CPU/RAM effect at the declined instruction.
A dependency rejection, guarded-code stop or internal unexpected return
is preserved as CALLEE. A direct callee return to an unexpected CS:IP
stops as TRANSFER without executing that target. The outer RETF uses
its actual, possibly aliased frame. Invalid initial host arguments,
entry IP, stack bounds or zero budgets reject atomically.

Logical flags use the accepted zero-AF convention. The caller's
intermediate SHL AF follows the reused normal/386 convention; subsequent
defined arithmetic overwrites it before the measured exposed boundaries.
Neither original backend masks FLAGS or substitutes emulator CPU state.

## Measured author evidence

**439 original-code comparisons pass**, each checking CPU14 and
all 1,048,576 RAM bytes. They include 421 complete returns,
12 caller limits, 3 nested line limits and
3 sort limits. No FLAGS masks or CPU substitutions are used.

Coverage is **297/297 caller sites and 34/34 caller conditional outcomes**.
The transitive chain reaches **1281/1283 sites
and 185/192 conditional
outcomes**. Full transitive coverage is not asserted. The unvisited sites
are: `caller:004E`, `caller:006A`.

The two unvisited sites are +004E and +006A within the U3C2 +002E sorter;
`caller` in the audit namespace denotes the loaded U3C2 segment.
No unexpected nested transfer or dependency rejection occurred in this suite.

The suite covers all 256 direction bytes, empty/disabled/mixed/ten-record
parties, signed coordinate boundaries, distance overflow/underflow inputs,
blocked maps, all six caller loop headers, nested line and sort limits,
odd/boundary stacks, relocated/A20 addresses, live stack/data aliases
including changed outer return targets, and 100 seeded random cases.
Detailed uncovered edges and stop counts are retained in audit.json.

**24 DOSBox-X normal/386 captures pass**, all full original-chain returns.
Each checks all fourteen CPU words, the full 65,536-byte DS segment, and
864 stack bytes: [EF00,F040), [0000,0100), [FEE0,10000).
That is **66,400 RAM bytes per capture**, with unmasked FLAGS.
Cases include empty/disabled shapes, blocked maps, mixed party records
requiring actual swaps/Move calls, direction modes 0..8/FF, out-of-range
coordinates, and initial SP=008E/008F/FFED/FFEE.

The native harness copies the pinned caller/callees into separate loaded
segments, resolves the overlay/SYSTEM call operands, disables interrupts
during capture, and executes the entire chain without callee interception.
These are synthetic fixtures; overlay loading and natural game-route
reachability are not measured.

**40 focused semantic checks pass under strict C11 ASan/UBSan**:
candidate IDs/distances/directions, actual sorting, skipped records,
empty input, caller/line/sort limits, the direction-search stop,
changed outer return frames, and atomic argument rejection.
The full recovered library builds and focused CTest passes (1/1).

Native partial stops, unexpected nested transfers, initial wrapped frames,
indirect LES operand seams and real-hardware CPU portability are not
claimed. Native RAM coverage is DS plus the listed stack windows, not
whole RAM. Transitive helper coverage is reported separately from the
complete caller coverage.

## Reproduction

Supply pinned media at `resources/DOS/POOLRAD/{START.EXE,GAME.OVR}`.
Python needs Capstone and Unicorn; native capture needs DOSBox-X and
GNU as/ld. All generated bytes, captures and reports stay ignored.

```sh
mkdir -p resources/u3c2-0909
PYTHONPATH=tools/overlay python3 tools/overlay/u3c2_caller_0909_audit.py --output resources/u3c2-0909/audit.json
PYTHONPATH=tools/overlay python3 tools/overlay/u3c2_caller_0909_native.py
cmake -S . -B resources/u3c2-0909/build -DBUILD_TESTING=ON
cmake --build resources/u3c2-0909/build --target poolrad_overlay_u3c2_caller_0909_tests
ctest --test-dir resources/u3c2-0909/build -R u3c2_caller_0909 --output-on-failure
```

The sanitizer build links the caller, its eight implementation dependencies
listed by `SOURCES` in the audit, and the focused test with
`-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined`.
The final ignored evidence receipt pins the tested head, dependencies,
reports, tool versions and native capture hashes.

## Author parity assessment

| Field | Assessment |
| --- | --- |
| target_function | OVL_03C2_0909, complete entry6 [0909,0BD9) and fixed transitive callees |
| source_candidates | overlay_u3c2_caller_0909.c and the unchanged full-state callees above |
| signals | Hash/entry/fixup/call pins, complete caller site/edge coverage, original CPU14/whole-RAM comparisons, independent native backend, alias and budget evidence |
| score | Strong source identity and tested contract parity; separate incomplete transitive coverage and no natural-route evidence |
| verdict | likely_match, pending independent review |
| review_notes | Check argument order, signed coordinates, late reloads, explicit pair RETF policy, per-call line budget, shared caller budget, actual outer frame and stated native limits; no self-credit |

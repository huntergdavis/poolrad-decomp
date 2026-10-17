# U3C2 +0422: complete line check

The new pr_u3c2_line_check_0422 API executes entry5 through its actual RETF20
using the complete +01A0 initializer, its +0000 sign helper, and the +02CC
record stepper. It replaces the unresolved callee boundary for this entry.
The original pr_u3c2_entry5_0422 callback API remains available with its
existing behavior; both APIs share one caller body.

## Selection and provenance

The schema-4 queue regenerated against main 02eb3e35 places OVL_03C2_0422
at rank 366, score 32: 141 instructions, two ready dependencies, one aligned
near caller and three fixup-backed far callers. +01A0 became composable
through merged PR #869. Higher-ranked hard entries were already implemented
or had active owners: U33+012B and U6A+019C have open PRs, U3C6+0CB9 and
U77+27DF have complete implementations, and U6C+4235 has an existing
unmerged full-state branch. The read-only operations ledger had no +0422
body event; all-ref history showed the existing callback candidate.

A target-specific deja query indexed sessions but timed out without
returning reusable session text. Reused repository evidence is explicit:

- Caller body: commits 3a4cec1f and 1cb4cb37, merged by #799; the 141
  original instruction annotations and all raw accesses remain in one body.
- Initializer: #869, source head dc29e5a381a7fbebfdb4ecde98eb48876a7631ef,
  [initializer evidence](U3C2_LINE_INIT_01A0.md).
- Stepper: [complete +02CC evidence](U3C2_ENTRY3_02CC.md), including its
  attributed released pF implementation. It is linked without modification.
- Oracle infrastructure: the checked-in +01A0 audit supplies CPU layout,
  pinned input validation, original descriptor discovery, and byte helpers.
  The new audit independently executes the full +0422 chain.

No strict manifest, accepted-body event, or credit count is changed here.
Independent verification and credit belong to the reviewing lane.

## Original identity and control flow

| Item | Value |
| --- | --- |
| START.EXE | 65231 bytes; SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 bytes; SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U3C2 descriptor | File base 0x2FB52; loaded stub segment 03DB; public slot 5 = 0422 |
| Caller extent | [0422,057B), file [196468,196813), 345 bytes / 141 instructions |
| Caller SHA-256 | f075c6dba0b31e12aea19318958a72f4caee6fe7954c53f3583dd5dd9fa8114d |
| Callee sites | 045C and 04D6 to 01A0; 0541 and 054D to 02CC; each uses PUSH SS/DI/CS then near CALL |
| Return | 0578 RETF 20 |
| Loop | 0560 back to 04DD |
| Incoming near call | U3C2+0AF5, preceded by PUSH CS |
| Incoming far calls | U31+0D1F, U38+1EFD, U5E+2F1A, each with descriptor fixup evidence |

The audit also pins all three dependency bodies:
+0000 [0000,002E), 46 bytes / 17 instructions;
+01A0 [01A0,02CC), 300 bytes / 99 instructions;
+02CC [02CC,0422), 342 bytes / 111 instructions.
Their complete SHA-256 values are checked and printed by the audit.

## Behavior and accepted machine profile

At entry, SS:SP contains far return IP/CS, followed by these arguments:

| Offset from entry SP | Argument |
| --- | --- |
| +04 | Far map pointer |
| +08 | Far pointer to a word, read as range and overwritten with step count |
| +0C | Far pointer to destination y byte |
| +10 | Far pointer to destination x byte |
| +14 | Initial y byte in a word slot |
| +16 | Initial x byte in a word slot |

The caller sign-extends coordinate bytes and initializes two stack records.
The first follows the x/y line; the second interpolates a height value.
Map indexing is base + 50*y + x + 7 with original 16-bit arithmetic. Tile
attributes are read from DS:2745+4*tile and DS:2746+4*tile. A nonzero map
header byte at +6 bypasses the height check. The range threshold is the
wrapped word 2*input_range+1, compared unsigned to the byte step count.
A height/range failure writes current x/y/count and returns AL=0; completion
writes count and returns AL=1. All pointer reloads and write ordering remain
live, so output aliases can change later pointers, locals, saved BP, or the
actual far return. The final return uses the reached frame.

The full API accepts exactly 1 MiB RAM, IP=0422 and SP in [0054,FFE8].
RAM and CPU storage must be distinct and stable, execution synchronous,
and modeled code immutable. Output writes into modeled code are outside
the contract. A guard atomically rejects reserved stack/code overlap.
The deepest reservation is 84 bytes: 60 caller bytes, eight callee argument/
far-frame bytes, and 16 initializer/sign-helper bytes. These bounds also
keep every far return frame away from the 16-bit stack wrap seam.

Effective offsets wrap at 16 bits; word and LES bytes are physically
contiguous with 20-bit wrapping. Logical/SHL undefined AF follows the
chosen zero profile. Undefined MUL status bits are overwritten before
observable stops or returns. No interrupt, concurrent mutation, or
universal hardware undefined-FLAGS claim is made.

max_iterations counts visits to the original loop header +04DD. LIMIT
stops immediately before that instruction with exact reached CPU/RAM,
including all preceding initialization and nested-call effects. Zero
allows initialization and stops at the first loop visit. This is an
explicit host stop, not an original return or a termination guarantee.
The entry API does not resume a stopped +04DD state. CALLEE propagates
reached dependency failure or an unexpected nested return. Initial
argument and reserved-code-alias rejections are atomic.

## Validation

The checked-in audit maps the same RAM backing at 0 and 1 MiB for A20
wrapping, executes all four original bodies, and compares all 14 CPU
words and all 1,048,576 RAM bytes. It does not replace original callees
with scalar stubs. A return-frame monitor stops before the outer RETF,
executes that instruction once, and compares the actual resulting CS:IP.
Loop stops occur before +04DD in both executions.

The matrix covers short lines in every direction, signed coordinate
extremes, wrapped range thresholds, height equality/failure/bypass,
step-counter wrapping, varied incoming flags, relocated/A20 code,
stack endpoints, far data crossings, and 168 output/stack alias layouts.
It includes explicit iteration budgets 0, 1, 2, 3, and 7.

The default audit passes 1,430 cases: 1,425 actual returns and five exact
iteration-limit stops. Instruction coverage is 141/141 in the caller,
99/99 in the initializer, 17/17 in the sign helper, and 111/111 in the
stepper: 368/368 across the complete chain. Every case compares all
14 CPU words and every byte of RAM without masks.

The focused C regression checks completion, range and height failure,
an output write changing the outer return IP, exact iteration stops,
and atomic invalid-argument/code-alias guards. Strict C11 warnings and
ASan/UBSan pass. The complete recovered library builds; five focused
CTests pass (new full API, existing callback API, initializer, sign helper,
and stepper). The original static audit still matches all 141 source
instruction annotations, four callee sites and the one backedge.

| Review field | Assessment |
| --- | --- |
| target_function | OVL_03C2_0422, full [0422,057B) |
| source_candidates | Shared caller body and pr_u3c2_line_check_0422 with fixed full-state callees |
| signals | Pinned descriptor/body hashes, three relocation-backed callers, near-call ABI, complete instruction execution, full CPU/RAM comparison, actual return and limit states |
| score | 1,430 full CPU/RAM cases; 368/368 chain instructions; five focused CTests; strict warnings and ASan/UBSan |
| verdict | Author evidence for independent parity review; no self-acceptance |
| review_notes | Synthetic original execution is not saved-game reachability; alias space is not exhaustive; explicit code/stack/FLAGS and iteration-stop profile applies |

## Reproduction

Use owner-provided media under ignored resources and install Capstone/Unicorn
in a local Python environment. No original bytes are checked in.

~~~sh
python3 tools/overlay/u3c2_line_check_0422_audit.py \
  --exe /path/to/ignored/resources/START.EXE \
  --ovr /path/to/ignored/resources/GAME.OVR
cmake -S . -B /tmp/poolrad-build
cmake --build /tmp/poolrad-build --target poolrad_overlay_u3c2_line_check_0422_tests
ctest --test-dir /tmp/poolrad-build --output-on-failure -R line_check_0422
~~~

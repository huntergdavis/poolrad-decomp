# RES_08AE +181A: translated, uncredited byte setter

This raw CPU body writes the low byte of its WORD argument to resident
DS:020D and returns with RETF 2. **Identity remains unresolved.** This author
change adds no strict credit, resident manifest declaration, or caller readiness.
It finishes the approved leaf after the dependency band closed on identity.

## Media and executable extent

START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Loader-expanded image: 68,592 bytes, SHA-256
a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e.

The decoded span is **08AE:[181A,1830), 22 bytes / 12 starts**, physical
[0A2FA,0A310), ending in RETF 2 at 182D. Body SHA-256:
4b13f9458c5b74c2a0dde8cd81bfd304d197c7712bf282efcf6cf4e38b88b79a.
The checker verifies the entire span, final return, instruction count and
loader relocation at physical A300 (operand CS:1820). Sizing comes from
expanded executable bytes, not a queue row.

## Ownership and six-helper verdict

GAME.OVR SHA-256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
A hash-locked scan of every overlay segment fixup finds **zero direct far
CALL fixups to each of these six entries**. The checker reproduces that scan.
It also verifies 00DC's two decoded near calls to 181A at 010E and 011D;
the original far-return idiom supplies CS before each near CALL.
Those callers establish use, not game-specific source identity.
The resident loader ledger labels parents 00DC/01A5
game_or_other_library_unresolved. Absence of direct overlay calls is not a
proof of library origin.

| Body | Verdict on current evidence | Evidence class |
| --- | --- | --- |
| 181A, 22 B | NEITHER | Local byte setter, parent identity unresolved; no direct overlay fixup calls. |
| 1742, 108 B | NEITHER | Local table writer, measured saved-route use; game/library identity explicitly unresolved. |
| 17AE, 108 B | NEITHER | Same evidence class as 1742. |
| 006C, 112 B | NEITHER | Calls 08AE:0000 twice; no direct overlay ownership or accepted environment contract. |
| 137C, 53 B | DECLARABLE candidate only | INT21 at 1386/139E; PIT OUT43/40 at 13A3/13A8/13AC. Vector/timer contract and oracle remain absent. |
| 13B1, 57 B | DECLARABLE candidate only | INT21 at 13C6, OUT40, IN/OUT61 and OUTC0; vector/port/device contract and oracle remain absent. |

None is currently creditable or readiness-qualified. The
[local-writer evidence](RESIDENT_8AE_LEAVES.md) and
[1742 evidence](RESIDENT_8AE_1742.md) explicitly separate saved-game use from
identity. The accepted three-hop unique floor is 1,932 B (1,445 B plus
487 newly measured bytes, accounting for overlapping entries); it is a
cost floor, not an established path to readiness.
Four ordinary helpers cannot be laundered through declared_boundary.
No strict row or full manifest row is proposed for 181A.

## Raw contract

The API requires exactly 1 MiB of A20-off ordinary RAM, IP=181A,
SP in [0006,FFFA], distinct CPU/RAM storage, immutable executable code and
accesses outside MMIO. It excludes interrupts, faults and arbitrary-PC resume.
It reads the live relocated segment immediate at CS:1820; code and data
segments need not equal. Initial DS is saved and restored through live RAM.

PUSH/POP, argument read, store and RETF execute in original order.
Data may alias saved registers, argument, or the external return pair:
the API consumes the resulting live values. Word accesses retain physical
A20 wrapping. The lower/upper SP bounds avoid stack-frame offset wrap;
odd SP is supported. All flags are unchanged.
ARGUMENT rejects null RAM/CPU, wrong RAM size/IP and out-of-contract SP
before changing CPU or RAM. RETURN is the only executable outcome:
the body has no conditional branches, calls, interrupts or port operations.

## Evidence

The original-byte Unicorn differential passes **276 RETURN cases**, comparing
all **14 CPU words and 1,048,576 RAM bytes**, including flags and the
A0000:AFFFF backing range. No RAM or flag masks are used. Coverage is
**12/12 starts (100%)**, zero conditional edges and zero callees. Four
separate ARGUMENT tests verify CPU/RAM nonmutation; these are API rejections,
not claimed original executable paths.

Fixtures cover all 256 stored bytes with varying high argument bytes and
flags, random registers, thirteen byte positions aliasing argument/return/
saved frames, odd and endpoint stacks, A20 stack/data aliases and random
full RAM. The original emulator maps both A20 aliases onto the same memory.
ASan/UBSan passes 1,282 semantic checks; focused CTest passes 1/1.

Native DOSBox-X normal-core/386 corroboration passes **32 synthetic direct
calls**. Original instructions are relocated to 8800:181A; the segment
operand is relocated to data segment 5000. COM entry/return frames,
registers and data are synthetic, and no callee stubs exist.
This does not establish natural gameplay reachability or device behavior.

Native comparison captures CPU14 without flag masks, a 256-byte stack
window and the full 64 KiB data segment: **65,792 RAM bytes captured;
982,784 bytes masked by omission**. For measured COM segment 0814 the
captured physical half-open ranges are [17040,17140) and [50000,60000).
Exact native RAM masks:

| Excluded half-open physical range | Bytes |
| --- | ---: |
| [00000,17040) | 94,272 |
| [17140,50000) | 233,152 |
| [60000,100000) | 655,360 |
| Total | 982,784 |

The harness does not replay DOS, BIOS, code or capture machinery as identical
RAM outside its controlled windows. This is a harness limitation, not a
DOSBox-X limitation. Full RAM comparison comes from the original-byte
differential, not this native capture. Native RESULT.BIN SHA-256:
095e78d384898e1b54b7ae8c583d59d93d9a049dcdcf22b75095cf0ad050bd2f.

## Reuse and reproduction

Dunking Sheep memory query for "08AE" "181A" reused Claude session
281c8700-5acc-438b-b93d-c061e28437eb chunks 23898/23772 as discovery leads
for the 22-byte leaf and environment operations. All extents and ownership
claims above were checked afresh against owner media.
The native COM capture mechanics adapt
res0768_pure_callees_cpu_audit.py; this note explicitly corrects that
template's "no mask" wording for its partial native RAM capture.
Shared CPU register definitions and loader expansion are reused unchanged.

    python3 tools/overlay/res08ae_byte_181a_cpu_audit.py --unicorn --native --output resources/181a-evidence.json
    python3 tools/overlay/check_checker_builds.py RES_08AE_181A --require-current-main --output resources/181a-checker-build.json
    cmake -S . -B resources/build181a -DCMAKE_BUILD_TYPE=Debug
    cmake --build resources/build181a --target poolrad_resident_08ae_byte_181a_cpu_tests
    ctest --test-dir resources/build181a -R '^poolrad_resident_08ae_byte_181a_cpu_tests$' --output-on-failure

Use the audit Python environment with Capstone and Unicorn. Compile-only
actually builds, loads and binds the production symbol before printing the
gate marker, and requires no media. The author branch includes fetched
main fa14698b. Original bytes, generated assembly/COM and capture receipts
remain under ignored resources/. Review acceptance and any later identity
decision belong to the independent reviewer/VP.

## Driver-internal declaration (class membership proposal)

This section proposes `RES_08AE_181A` as a member of the
[driver-internal class](DRIVER_INTERNAL_CLASS.md). It changes nothing above: the body stays **translated, evidenced and
uncredited**, its identity stays unresolved, and no strict, identity or resident-manifest row is proposed or implied. The
class refuses a credit row for a member by test, and this proposal adds none.

| Item | Value |
| --- | --- |
| Member id | `RES_08AE_181A` (segment-scoped to `RES_08AE_XXXX`) |
| Evidence document | this document |
| Body | 08AE `[181A,1830)`, **22 bytes / 12 starts**, SHA-256 `4b13f9458c5b74c2a0dde8cd81bfd304d197c7712bf282efcf6cf4e38b88b79a`, from the loader-expanded image (SHA-256 `a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e`), ending in `RETF 2` at `182D` |
| Merged body | the raw model, audit, tests and this evidence merged in #1185; no new translation is made here |
| Closure | **complete: nothing to compose.** The body has no calls, no jumps, no interrupts and no port operations, so every callee requirement of the class is vacuously met |
| Never a credit | yes: it is not added to any strict or identity table |

Proposed row (text only; the acceptance PR adds it to `driver_internal_models.csv` and fills the two provenance fields that
only the accepting review can supply):

    RES_08AE_181A,docs/re/RES_08AE_BYTE_181A_CPU.md,<review_pr>,<reviewed_commit>,4b13f9458c5b74c2a0dde8cd81bfd304d197c7712bf282efcf6cf4e38b88b79a,22

### Re-run on current main

The audit was re-run on main `af9e15e0` with this branch. It reproduces the figures stated above: **276 RETURN cases and
4 ARGUMENT rejections**, all 14 CPU words and all 1,048,576 RAM bytes compared with no flag or RAM mask, **12/12 starts**,
zero conditional edges, zero callees; native **32 cases**, 65,792 captured and 982,784 omitted bytes as tabulated above, and
a native `RESULT.BIN` SHA-256 of `095e78d384898e1b54b7ae8c583d59d93d9a049dcdcf22b75095cf0ad050bd2f`, **identical** to the
value recorded when the body merged. The receipt now records the DOSBox-X binary, and the harness caps the oracle run at
300 s and truncates an oversized log.

### Declared substitution: the DOSBox-X binary

The native oracle is **not** the pinned DOSBox-X source build (git `5a9c7c53481c2e933948039c33256776653f4faf`). It is the
packaged binary on PATH, `/usr/bin/dosbox-x`, `DOSBox-X version 2026.01.02 SDL2`, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, run with `core=normal`, `cputype=386`, `cycles=max`. The
pinned build was neither rebuilt nor re-run here. What could differ: the lazy-flag conventions, the LES wraparound and
stack-offset wrap at FFFF. This body is insulated from all three: it contains only `PUSH`, `POP`, `MOV` and `RETF`, writes **no
flag** (the audit compares FLAGS in all 276 cases and they are unchanged), has no `LES`, and the SP bounds `[0006,FFFA]`
exclude stack-offset wrap. The same executable hash is recorded as the native oracle in the merged
`RES_0709_TEXT_06A7`, `RES_0709_SKIP_SPACES_0714`, `U33_PARTY_ATTACH_12E1`, `U33_1403_EVIDENCE_REFRESH` and `SYSTEM_REAL_CPU`
evidence, and the 181A `RESULT.BIN` hash reproduces bit-for-bit under it. A reviewer who needs the pinned result must
re-run the native harness with that binary.

GATE_PENDING


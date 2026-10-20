# U33 +12E1: party attachment initialization through the complete resident chain

This author candidate implements U33 +12E1 with the actual resident +0BB8
closure, SYSTEM GetMem and FillChar. It follows the party list, recalculates
each character, allocates and clears its 22-byte attachment, and applies
the window/character-dependent attachment and member writes. Independent
review decides acceptance and credit.

## Original bytes

The body comes directly from DOS v1.3 GAME.OVR, not a queue extent:
unit [12E1,1403), file [55474,55764), **290 bytes / 91 instructions**.
The final one-byte RETF is at +1402; +1403 begins the next routine.
Body SHA-256:
e463fe4b8a2db622ea31bec99ecf067a6a18483547fa251d8c9379a6aea35312.

GAME.OVR is 206,105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
START.EXE is 65,231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
The static closure spans 3,709 executable bytes / 1,339 starts: this
290-byte/91-start caller, 3,153 bytes / 1,134 resident starts, 246 bytes /
107 GetMem starts and 20 bytes / 7 FillChar starts. Dynamic coverage and
unreachable runtime instructions are reported separately.

The original descriptor has segment fixups +130C, +1318 and +133B,
belonging to calls +1309 -> 00BA:0BB8, +1315 -> 0AF8:023F and
+1338 -> 0AF8:16A8. These bindings are validated before C mutates state.

## Ownership and caller

The owning list is DS:5D96/5D98, with character links at +104/+106.
Its provenance is independently established by the
[character loader](../formats/DOS_CHARACTER.md),
[party insertion](U6F_PARTY_INSERT_1446_CPU.md) and
[resident party count](RESIDENT_PARTY_COUNT.md).

For each character, +12E1 stores an allocated 22-byte attachment at
+108/+10A and writes attachment +09/+13 and member +84. Those writes
depend on member +10E, window-1 (DS:49D6) words +67C/+58C, DS:6AAF and
the DS:02F2 byte table. Public U33 entry 1 resolves to +1E31; its
PUSH CS / near CALL at +1EE6/+1EE7 reaches +12E1. This party/window
layout and its ordered writes establish game ownership independently of
the generic allocator and memory-fill calls. Precise gameplay names for
attachment fields remain unproved.

## Ordered behavior

Entry is after CALL at IP +12E1, with the live far return IP/CS on SS:SP.
There are no arguments. The six-byte local area holds the current member
pointer, an eight-bit counter, and a temporary member byte.

Starting from the party head, each iteration executes resident +0BB8,
increments the byte counter, requests 22 bytes from GetMem, stores DX:AX
in the character attachment pointer, and clears those 22 bytes through
the actual FillChar routine. If the zero-extended counter exceeds
window +67C, attachment +13 becomes 1. Attachment +09 comes from
DS:[02F2 + (byte DS:6AAF >> 1)]. When member +10E is 1, that byte is
replaced with (value + 4) modulo 8 through the original signed divide.

When member +10E is zero, attachment +13 is 1, and member +84 masked
with 7F is either zero or greater than 102, member +84 becomes the low
byte of window +58C plus 128. The live +104/+106 pointer advances the
loop. The byte counter wraps after 255. Empty lists return immediately.

The implementation retains original source order, flags, partial registers,
real CALL frames and the actual final RETF. All three callees execute their
C bodies; there are no allocation-result or recalculation stubs.

## API, host guards and stops

The public function is pr_u33_party_attach_12e1_cpu, taking one MiB of RAM,
CPU14, loaded resident and SYSTEM segments, a shared instruction budget,
a per-GetMem free-list walk cap, and an optional allocator diagnostic.
Entry IP must be 12E1 and SP must be in [0100,FFF0]. Null/size/entry/SP
errors or a mismatch in any of the three immutable far-call bindings
return ARGUMENT before changing RAM, CPU, budget or diagnostic.

After valid guards, the diagnostic is -1 until GetMem executes, then
records its exact most recent status. Each executed caller or resident
instruction consumes one shared budget unit. Existing GetMem and FillChar
instructions are uncharged; GetMem has its own walk cap, reset per call.
LIMIT preserves prior effects and leaves the next caller/resident IP
unexecuted. CALLEE preserves an allocator failure or stop, including its
CS:IP and frame; allocator walk exhaustion is CALLEE with diagnostic
GETMEM_LIMIT, distinct from the shared instruction LIMIT. Unexpected
callee returns yield TRANSFER. No stop invents cleanup or a successful
return, and no mid-call resume is promised.

## Arithmetic profile

The signed divide at +1393 has a dividend from 4 through 259 and divisor 8,
so neither divide-by-zero nor quotient overflow is possible. Its undefined
flags are observable at a host budget stop even though later instructions
overwrite them before the normal caller return. Native testing contradicted
the initial assumption that IDIV preserved those flags; the implementation
was corrected to the pinned DOSBox-X normal/386 IDIVW profile.

That profile clears AF, SF and OF; ZF is set when the remainder is zero
and the quotient is odd; CF depends on the remainder's low two bits being
1 or 2; PF is the XOR of the 16-bit remainder even-parity bit, quotient even-parity bit and 1.
Other flag bits are preserved. SHR uses the same native profile's AF=1.
The matching DOSBox-X source is IDIVW in src/cpu/instructions.h, SHA-256
7cbc4bea7e47678f0a3734a4c16c2110ef8125b09d0aa7f7a8457aab7b3f747e.
The native executable SHA-256 is
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
These are measured implementation choices for architecturally undefined
flags, not claims about every x86 CPU or emulator. The final native
checkpoint matrix and raw-Unicorn disagreements are reported separately.

## Dependency contracts and evidence limits

The resident implementation and its normal/386 arithmetic profile are
documented in [U9E +115C](U9E_CHARACTER_PAIR_115C_CPU.md).
The unchanged [GetMem model](SYSTEM_GETMEM_023F.md) includes its
rounding, free-list, heap-top and default HeapError paths. It stops at
RunError entry, at a custom HeapError call, or at its host walk limit.
The caller propagates these partial CPU/RAM states without inventing a
successful allocation or return. RunError's body and arbitrary custom
HeapError implementations are outside that dependency contract.

**Heap well-formedness (limit, added by the reviewer after independent
review):** the heap must be well formed enough that its free-list header
fields are readable. The per-call walk cap turns a cyclic or over-long free
list into an exact stop that is reported rather than a successful allocation;
heaps with corrupted header fields, and heap metadata outside the loaded
segment, are outside this profile.

GetMem and FillChar retain their existing contiguous return-frame access
contract. This candidate therefore requires active runtime frames that
do not cross the 16-bit stack boundary or suffer corruption from party,
heap or output writes; arbitrary aliasing is outside this profile. Wrapping
runtime frames are not
covered by the resident model's separate wrapped-pop support. CPU/budget/
diagnostic host storage must be disjoint from RAM and one another.
Code and loaded bindings are immutable, and asynchronous modification
is outside the model.

## Reuse

Owner #986 authorizes the +0BB8 line, U9E +115C first and U33 +12E1 next.
Dunking Sheep context found earlier U33 reconnaissance in Claude session
997d7fe9-a031-492f-9b1a-11963b6f8cc5 and ownership leads in session
281c8700-5acc-438b-b93d-c061e28437eb. The prior feasibility packet
i19056 identified the party/window provenance; this work rechecks the
original bytes and caller bindings. No older unpublished U33 implementation
is included. The actual resident, GetMem and FillChar sources are reused,
with their contract limitations retained.

The synthetic C regression passes 128 checks under ASan/UBSan, including
257-member counter wrapping, attachment contents and untouched allocation
padding, threshold/tag conditions, exact/split/top allocation, failure after
one successful member, custom HeapError and cyclic free-list stops, party
budget exhaustion, and atomic guards. The focused CMake/CTest target passes.

The final default original-byte audit passes **1,151 U33-origin cases**,
comparing all 14 CPU words (including every FLAGS bit), the entire
1,048,576-byte RAM image, status, remaining budget and allocator diagnostic.
There are no output masks, original-body replacements or oracle CPU
substitutions.

| Scope | Instruction starts | Conditional outcomes |
| --- | ---: | ---: |
| U33 caller | 91 / 91 | 14 / 14 |
| Eleven resident bodies, reached through U33 | 1,134 / 1,134 | 278 / 278 feasible |
| Entire static closure | 1,333 / 1,339 | 312 / 330 syntactic |

All six missing starts and all 18 missing syntactic outcomes are accounted
for by the proofs below. The gate rejects any other gap. The cases comprise
1,137 returns, eight shared-budget stops and six allocator stops: three
RunError entries, two custom HeapError calls and one free-list walk cap.
They include failure after an earlier successful allocation, a cyclic
party, 257-member counter wrapping, DS=SS with disjoint offsets, and high
SS/A20-wrapped physical stack locations within the permitted SP interval.

The resident data fixtures reused from the U9E audit supply
inputs only. U33's code, frames, party globals and heap are installed
independently; no prior coverage totals are imported. Original body hashes
and instructions determine all denominators.

No identity or strict-credit ledger is changed.

## Closure coverage exclusions

The caller and all eleven resident bodies are exercised through U33 entry,
rather than treating prior standalone helper coverage as new caller
coverage. The static denominator still includes all original allocator
instructions, so the six excluded runtime starts remain visible:

| SYSTEM starts | Why this composition cannot execute them |
| --- | --- |
| 0383, 0385, 0386 | Nil-size return (XOR AX,AX; CWD; RET). The caller pushes the constant 22; active-frame corruption is outside the contract. The alternate HeapError route to this return also requires AL=1. |
| 03CB, 03CD, 03D0 | Default HeapError returns AL=0, so CMP AL,1 / JB at 03C7/03C9 always takes the failure branch. JE-to-nil and reload/retry require a non-default handler, at whose call the model explicitly stops. |

The four excluded SYSTEM edges are 030A->0383, 03C9->03CB,
03CB->0383 and 03CB->03CD. Fourteen additional resident directions
contradict their preceding range checks; their exact sites and proofs
are inherited from the hash-pinned resident checker and retained in this
report. Missing instructions and outcomes are not removed from the static
denominator or reported as executed. No unknown coverage gap is accepted
by the final gate.

## Reproduction

From the repository root, use a Python environment with Capstone and Unicorn.
Owner-provided media belongs under ignored resources/DOS. The native checker
also requires GNU binutils (as, ld, nm) and DOSBox-X.

    python tools/overlay/check_checker_builds.py OVL_0033_12E1 --require-current-main
    python tools/overlay/u33_party_attach_12e1_audit.py --output resources/u33-audit.json
    python tools/overlay/u33_party_attach_12e1_native.py --output-dir resources/u33-native

The native default includes all 256 reachable table-byte inputs at the
IDIV checkpoint. Its optional --dosbox-source argument checks the matching
local src/cpu/instructions.h hash; it does not download source. The
--diagnose option retains discrepancies in its report and still fails the
final gate when any remain. Neither --quick nor --diagnose substitutes for
the default final evidence run.

The compile-only mode builds, loads and binds the same source list as the
actual differential, without requiring game media. The local current-main
gate passes all eight registered checker builds for eight rows.

## Native corroboration

The final default native run passes **290 captures**, with zero CPU or RAM
differences: 21 completed U33 calls, 256 IDIV checkpoints (all reachable
table bytes), eight SHR checkpoints, three RunError-entry interceptions,
and two custom-HeapError-call interceptions. All compared FLAGS bits are
unmasked. Native RAM comparison is **327,680 bytes per case**, the complete
five 64 KiB data/stack regions; it is not a full-1MiB native claim.

The original caller, resident and runtime bodies are loaded with their
segment relocations and synthetic direct-entry frames. Completed calls
capture 13 register words and use the executed return-label IP. Each of
the 269 interceptions captures 12 register words plus declared original
CS:IP coordinates. An explicit far JMP at the first unexecuted instruction
enters the capture stub; the script checks that this point was not visited
earlier. The native RunError/custom-handler continuations are not executed.
Flags are saved before DOS output, on a separate capture stack outside the
observed data pages. There are no hidden successful-return stubs.

Raw Unicorn differs from normal/386 at some undefined SHR/IDIV flag
boundaries. Those exact differences remain in the native report;
the C state is compared directly with native capture and is never masked
or substituted into the raw oracle. The 1,151 completed/declared-stop
differential cases above pass unmasked on their own. Native fixtures
add exact/split/top allocations, failure after a successful member, counts
255/256/257, DS=SS with disjoint offsets, and both ends of the allowed SP
interval. No naturally reached saved-game invocation is claimed.

## Author parity assessment

| Field | Finding |
| --- | --- |
| target_function | OVL_0033_12E1 with resident +0BB8, SYSTEM GetMem and FillChar |
| source_candidates | overlay_u33_party_attach_12e1_cpu.c and the unchanged declared callees |
| signals | Original extent/hash/fixups; loader-backed party and window ownership; 91 source-ordered caller starts and three real calls; complete caller/resident coverage; unmasked CPU14/full-memory differential; native boundary matrix |
| score | Strong measured agreement within the documented execution profile; not a probability or credit decision |
| verdict | match within the immutable-code, noncorrupted-frame and pinned normal/386 profile |
| review_notes | Six excluded allocator starts; 18 proved/contract-excluded outcomes; explicit RunError/custom-hook stops; native RAM subset and reconstructed interception coordinates; synthetic entry states; independent acceptance pending |

Independent source inspection rechecked ownership and all 91 caller
starts/three calls. Static harness review identified and corrected the
257-member fixture overlap and allocator-return diagnostic classification.
The native IDIV contradiction was retained and corrected before final runs.
Those reviews supplement the measured execution; they do not replace the
reviewer lane's acceptance or assign credit.

Measured report SHA-256 values (original media and captures stay private):

- u33-oracle-final.json: 00bf303f0b1c3f01c21c13e1776d09f0255bc7cbeb619b7156dd8753df9e308f
- u33-native/report.json: 98a0c1879c0b63f5d53fd2be4ab5800c7e62f634faf47aeac378a3f78aa2d4ae
- u33-ownership-review.json: 42d8592cc1948875c1381236b8c086357110000bce2c9392969fa4449afce2ed
- u33-source-review.json: 79958b320260ac28254a606561f208c40694edcfca1cde97d246ba55e9593afe
- u33-harness-review-notes.json: f0c9a20895e5964263dbba2563dc93c3d3dc87cde3482be9b8b43ba2e7d16dc3

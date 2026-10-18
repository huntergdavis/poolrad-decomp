# U9E +0160: raw composition with an explicit handler boundary

pr_u9e_sidecar_0160_cpu implements U9E [0160,02FA), composing the accepted
raw resident lookup, four slot helpers, U3C2 +0909 scan, and SYSTEM set/Move
leaves. A no-hit path completes RETF 6. A found path executes the original
U9E +0000 prefix and returns **PENDING before +001E's indirect CALL**,
with the live handler pointer, CPU14 and prepared stack intact.

This is useful partial composition, **not strict full-chain completion**.
No handler is skipped or supplied by a success callback. The new interface
does not make +0160 or its +02FA caller strictly ready, and adds no credit.

## Identity and provenance

- U9E internal [0160,02FA), GAME.OVR file [176202,176612):
  **410 bytes, 140 instruction starts**, RETF 6 at +02F7.
- Body SHA-256:
  89a45482f4f42e1f4805cc4aa2448ebb685b777dbc7b91be985da4473bb208ba.
- Set [0140,0160): 32 bytes, elements 21,45,46,49. SHA-256:
  251cddcd1fef74b67e089e40cf1dbed79db94489e43a5c00d273c5f1713d7c3b.
- Dispatcher [0000,0026): 38 bytes, 16 instructions. SHA-256:
  1572342d9be429f7a7b7f6b015b2a609726b4d5580e34235fda31eb2a43b9217.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

The audit checks the original call graph and all ten far-call fixups,
then resolves the overlay stubs through their public entry tables:

| Caller site(s) | Original target | Composed raw body |
| --- | --- | --- |
|018B,01EC|00BA:2199|resident sidecar lookup|
|01A5|0AF8:0DC0|SYSTEM set membership|
|01BE,02CF|0AF8:1685|SYSTEM Move|
|021C,022A,0242,0274|03E0:005C/0061/0066/006B|U3C6 0BE7/0C0F/0C37/0C5F|
|0250|03DB:003E|U3C2 0909|
|02F1|near 0000, preceded by PUSH CS|dispatcher prefix only|

Direct overlay calls use explicit resolved body offsets/loaded segments;
the audit applies those bindings to original bytes, preserving the far
return frame. There are no handler substitutions.

Recall before implementation found Claude session 281c8700-5ac; no old
implementation was imported. The reused behavioral provenance is
[U9E_0160.md](U9E_0160.md), the raw
[slot getter API](U3C6_SLOT_GETTERS_CPU.md),
[resident lookup](RESIDENT_SIDECAR_LOOKUP_2199_CPU.md), and
[U3C2 scan composition](U3C2_CALLER_0909.md).
The memory/arithmetic engine and original oracle scaffolding reuse
[U5E +32E6](U5E_CALLER_32E6.md); the native boundary capture follows the
[partial +0026 precedent](U9E_LIST_RELEASE_0026_CPU.md).
The old origin/codex/fullc-u9e-0160-prefix branch stops before +018B;
it is a setup prefix, not the implementation used here.
A later recall probe timed out; deja-health --repair was invoked.

## Behavior and boundary

The three argument words after the far return pair are type, member
offset, and member segment. The caller reserves E4 bytes. Its output cell,
party cursor, saved D8-byte scratch span, found flag, candidate index,
scan mode byte, and candidate count remain live stack operands.

It first looks for the type on the supplied member. A direct hit sets
found and proceeds to dispatch for **any type byte**. Only a direct miss
tests the four-member set. A type outside that set returns immediately.

The fallback saves DS:671B..67F2, walks the party head at DS:5D96/5D98,
and searches each member's sidecars. Outside mode 5, a hit suffices.
In mode 5 it obtains the member's slot fields, runs +0909 with mode/radius
6 for type49 or 1 otherwise, and searches the generated candidate records
for the original member's slot index. It follows the next-member pointer
at +104/+106 before the next loop test. The saved D8-byte span is restored;
DS:6716/6718 and candidate count DS:671A are outside that restored span.

A found path pushes type, original member, the live found-node cell and
mode byte zero, then calls +0000. Its 13-instruction prefix prepares five
handler argument words and computes DI=4*unsigned_type. **At PENDING,
CS:IP is the U9E segment:001E and the indirect CALL has not executed.**
The diagnostic records the live far pointer at DS:6828+DI.
Upper argument bytes are preserved as the original pushes AX; zero mode
does not imply a zero high byte in its word.

The four set members are a fallback condition, not a global dispatch
restriction. Tests exercise all 256 types after a direct lookup hit.
The [handler inventory](u9e_dispatch_handlers.json) distinguishes installed
primary slots from uninitialized/adjacent table regions; none of those
regions may be declared closed merely by providing this raw caller.
A strict caller needs an evidence-backed live binding/type contract and
complete reachable handlers. See also the
[+2EDA boundary analysis](U38_DISPATCH_BOUNDARY_2EDA.md).

## API limits and stop semantics

- Exact 1 MiB A20-off RAM, initial IP0160 and SP[0186,FFF6]. The lower
  bound leaves +0909's required 008E stack position after the caller frame
  and seven scan arguments. CPU, layout, limits, and optional diagnostic
  storage are distinct from RAM and each other.
- Original code and bindings are immutable, writes avoid modeled code,
  and reached return frames do not wrap. Data/stack aliases remain live.
  Layout supplies the four loaded code segments. Reached CALL operands
  are checked before execution; CODE retains that pre-call state.
- Caller party/result budgets count visits to +01C3/+026E, respectively,
  shared over the call; zero is valid. LIMIT preserves the exact state
  before the exhausted visit. Lookup and slot budgets reset per direct
  call. Positive scan/line/sort budgets retain +0909's documented meanings.
- CALLEE preserves a dependency's stopped state with its identity/status.
  TRANSFER preserves unexpected direct returns; no direct TRANSFER case
  is claimed by this corpus. A normal outer RETF uses its actual frame.
  ARGUMENT preserves RAM/CPU; diagnostic is reset on entry.
- The selected flag profile is normal/386: logical AF0 and one-bit SHL
  AF1. Interrupts/faults are outside the API. Native validation below is
  synthetic direct execution, not a saved-game reachability measurement.

## Validation

The original differential compares CPU14, all 1,048,576 RAM bytes,
stop reason and direct-callee diagnostic. It fails on unexpected PCs,
instruction ceilings, changes to code/set data, or unlisted differences.
The corrected corpus passed **609 cases**: 295 returns, 294 PENDING,
11 CODE, six CALLEE and three LIMIT stops. It covered **140/140 caller
instruction starts, 22/22 conditional outcomes**, and all 13 pre-call
dispatcher prefix starts. The reached composition totals 1,313 starts and
158 conditional outcomes; uncovered transitive sites remain in the report,
so those totals do not assert complete dependency coverage.

The only permitted Unicorn differences are exact FLAGS XOR0010 with
C AF=1 at PENDING and at bad-binding checkpoints +022A/+0250. Those two
checkpoints immediately follow a getter's SHL without intervening flag
changes. Every other CPU word, flag and RAM byte must match.
No emulator register or memory value is patched. The measured exceptions
were 294 PENDING AF differences and two binding-checkpoint AF differences.

Native validation passed **113 cases** under DOSBox-X normal/386:
**16 returns, 95 PENDING boundaries and two bad-binding checkpoints**.
Each compares CPU14, SS1KiB, DS64KiB and the character/node segment's
64KiB with **no flag masks**. The three boundary sites use a far-JMP
capture trap replacing the unexecuted CALL; it changes no general register,
flag or stack byte. The report names the original boundary in captured
CS:IP. These probes do not execute any dynamic handler.

The native corpus covers six incoming flag profiles; direct-hit types
0,1,4,21,45,46,49,139,140,214,215,255; all four fallback types in modes
0/5; found/missing candidates, nil/missing party, and varied coordinates.
The original corpus additionally covers every input byte, zero budgets,
cycles, malformed reached bindings, stack endpoints, A20, and data aliases.

The first native fixture exposed a test-initialization bug: filling all
256 slot pointers overlaps DS:6716/6718, replacing the intended map pointer
with 0070:00BE. That made native code read DOS low memory, while the
emulator/C used seeded synthetic low RAM. The corrected native fixture
restores the intended map pointer after table initialization. The original
corpus retains the old overlap as an explicit full-RAM alias case.
No C implementation or comparison mask changed to hide that discrepancy.

Native capture SHA-256:
8f884d4e8e6bcc3e89420baab27f235b8f4408ad0deb025ddb008a5cd2b340f8.

The strict C11 ASan/UBSan test passed **2,588 semantic checks**.
The focused CTest passed **1/1**. The synthetic test includes live aliasing,
argument rejection, exact boundary stack/CPU state, and scratch save/restore.

Integration base: d3a933d779a78d17d60e643ed1a7ece8c44dc30b.
The private receipt resources/0160-evidence.json pins source/header/audit
hashes, original/native reports, capture artifacts, media, DOSBox-X binary
and sanitizer/CTest logs. Documentation is excluded to avoid self-reference.
Receipt SHA-256:
9e6d04ada92354cba4bbb9c879e47184c3f0779a144e2285704c62e35fe1bf88.

## Reproduction

Owner media remains under ignored resources/DOS/POOLRAD; Capstone,
Unicorn, DOSBox-X, GNU as/ld and a C compiler are needed.

    python3 tools/overlay/u9e_sidecar_0160_cpu_audit.py --unicorn --native --output resources/0160-evidence-run.json
    cmake -S . -B resources/0160-build
    cmake --build resources/0160-build --target poolrad_overlay_u9e_sidecar_0160_cpu_tests
    ctest --test-dir resources/0160-build -R '^poolrad_overlay_u9e_sidecar_0160_cpu_tests$' --output-on-failure

## Author review

- target_function: OVL_009E_0160 complete [0160,02FA), with the explicit
  U9E +0000 pre-handler boundary.
- source_candidates: overlay_u9e_sidecar_0160_cpu.c and the accepted raw
  dependencies pinned by the audit.
- signals: exact identity/fixups, source-order live operands, caller
  path coverage, CPU14/full-RAM differential, and separate native original
  execution including both AF-exposing binding checkpoints.
- score: strong within-contract partial composition evidence; dynamic
  handler effects and natural-game type/binding invariants remain open.
- verdict: likely_match for the implemented paths; partial chain.
- review_notes: no full-chain acceptance or credit claim; no accounting
  edits. Reviewer/VP controls independent verification and credit.

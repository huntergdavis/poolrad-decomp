# U38 +27D9: conditional three-die roll and sidecar append

Public entry 84 tests DS:6819 bit 0, then bit 4. If either is set it
returns after the test. Otherwise it rolls 3d6 through the complete U9E
+0DE3/SYSTEM chain and appends a nine-byte sidecar node through U9E
+0E52/GetMem: key 66h, word equal to the roll, bytes FFh and 1, nil link.
The implementation composes the existing raw CPU callees without callbacks.

## Identity and live bindings

The complete body is U38 [27D9,2814), **59 bytes / 28 instructions**,
GAME.OVR file [0x11434,0x1146F), ending with RETF 0Ah at +2811.
The next public entry begins at +2814. There is no trailing-data adjustment.

| Input/span | SHA-256 |
| --- | --- |
| START.EXE | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U38 [27D9,2814) | 8247af44aa38ad460b80fe1491657b6a84b0228d483b706c4f91f3664180aee5 |
| U9E dice [0DE3,0E52) | 9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b |
| U9E append [0E52,0F06) | cfde5096f098f1f0c4aa15f805eb6a02c3540f56a2f0df8632b99501a6366b20 |

| Call site | Loaded stub | Raw dependency |
| --- | --- | --- |
| +27FD | 00B0:0048 | U9E +0DE3, RETF 4 |
| +280B | 00B0:0052 | U9E +0E52/GetMem, RETF 0Ch |

Both segment operands (+2800 and +280E) are loader fixups. C validates
reached LCALL opcode/offset and EA stub target offset, then reads the loaded
segments live. The oracle checks entry, extent, hash, calls, fixups and exit
against owner media before executing the original chain.

## ABI, ordering and limits

The five-word argument frame contains a far record pointer at entry
SS:[SP+0Ah]/[SP+0Ch] (offset/segment), or [BP+0Ch]/[BP+0Eh] after PUSH BP.
The other three caller words are unused. Both gate tests preserve AH and
set logical flags. Bit 0 takes precedence: an early return leaves AL=1
or AL=10h, respectively. PUSH BP happens before the gate-byte reads.

On the active path the caller pushes the record segment, offset and AX
with AL=66h. It then pushes AX with AL=3 and AL=6, retaining its incoming
AH in each word. The dice callee consumes only the final two words. The
caller clears AH, pushes the zero-extended result, then AX with AL=FFh
and AL=1. Append consumes all six remaining words. Thus the record/key
are already on the stack during the dice execution; they are not reread
or rebuilt from host values afterward. Seed, stack, record, list and
heap effects remain live and survive later failures.

The profile requires exactly 1 MiB A20-off RAM, initial IP=27D9,
SP in [0040,FFF2], immutable loaded code/bindings and synchronous execution.
CPU/limits/diagnostic storage must be distinct from RAM. SYSTEM remains at
0AF8 with the pinned dice/SYSTEM layout and live multiplier at +1245.
All reached dependency return frames must stay nonwrapping, including
after aliases change them. Offsets wrap at 16 bits, physical addresses at
20 bits, and words read/write physically adjacent bytes. Dependency flag
profiles apply, including measured normal/386 logical AF=0. Interrupts,
faults, arbitrary-PC resume and self-modifying code are outside this API.

Budgets retain their dependency meanings and may be zero: dice SYSTEM call
slices, GetMem free-list walk edges and append tail links. CODE stops before
a malformed reached direct call/stub. CALLEE retains the exact dependency
stop/status and APPEND's nested GetMem status. TRANSFER preserves a changed
direct return. ARGUMENT changes no CPU/RAM; diagnostics initialize to
NO_CALLEE/-1 and may be omitted. No stop rolls back effects. RunError and
custom HeapError execution are explicit boundaries.

## Measured evidence

- **685 original-chain cases**, CPU14 and all **1,048,576 RAM bytes** equal
  without masks: 670 returns, nine dependency stops, four binding stops,
  two changed direct returns.
- **28/28 caller starts, 4/4 branch outcomes**, and all 3d6 totals 3..18.
  The chain reaches **247/255 decoded starts** and 28 observed conditional
  outcomes. Eight unvisited allocator starts are 0AF8:0383,0385,0386,039A,
  039E,03CB,03CD,03D0. Exhaustive transitive branch closure is not claimed.
- **128 DOSBox-X normal/386 cases**: 116 returns and 12 pre-RunError 203
  captures. All CPU14 fields and **148,480 RAM bytes** (SS1,024, DS65,536,
  heap65,536, record/old-nodes16,384) match without masks.
- **4,573 semantic checks** pass ASan/UBSan, independently calculating
  the seed recurrence and three high-word remainders, checking every gate
  byte, early-return CPU effects, list payload/linking, ABI and diagnostics.
  Focused CTest **1/1 passes**.
- Native RESULT.BIN SHA-256:
  b9497eaa5ee8997c154d7f5e400132c291d95096f17525b83d5ec5add9f4490e.

The original corpus covers 400 seed cases, all 256 gate bytes, dice budgets
0/1/2/3, nil/existing/cyclic lists, exact/split/top/failing heaps, custom
HeapError, heap/list caps, binding errors, record and stack aliases, changed
returns, boundary/odd stacks, SS=DS, A20 wrap and randomized RAM. The nine
dependency stops comprise three dice caps, two list caps, RunError203,
custom HeapError, heap cap and nested GetMem binding rejection. Diagnostics
are checked for these selected stops and semantic cases, not every field
on every differential case.

Native replay uses synthetic far-entry frames, masks IRQs during execution,
and captures at a jump installed at RunError's entry. It does not execute
RunError termination or custom HeapError bodies and does not demonstrate
natural gameplay reach. COM offsets 2E40..4087 reserve the absolute SYSTEM
code footprint; generated blobs start above 4100. Original media, decoded
bytes, generated assembly/COM, snapshots and captures remain under ignored
resources/.

With owner media under resources/DOS/POOLRAD and Capstone/Unicorn installed:

    mkdir -p resources/u38-27d9
    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u38_roll_effect_27d9_audit.py --unicorn --native --output resources/u38-27d9/report.json
    cmake -S . -B resources/u38-27d9/build -DBUILD_TESTING=ON
    cmake --build resources/u38-27d9/build --target poolrad_overlay_u38_roll_effect_27d9_cpu_tests
    ctest --test-dir resources/u38-27d9/build -R '^poolrad_overlay_u38_roll_effect_27d9_cpu_tests$' --output-on-failure

Private receipt resources/evidence.json SHA-256:
b45541361da92ab16af409d1815f945d2c744feafb6c5267ea8cd7d38e99f2b7.
It pins 39 public build/tool/queue inputs, 20 private evidence artifacts,
media/tool binaries and 33 unchanged dependency files against main
5438ae2f027244b0a258e9ecca5a1826de3c5a02. The document is excluded from its own receipt.

## Reuse, queue caveat and author assessment

Pre-implementation recall, deja "U38 27D9 0DE3 0E52 composition", returned
no indexed match. The stale unpublished branch codex/fullc-u38-27d9 at
8ecb7eabb961da37d417b139f005e8373c335a4a provided body/fixup/ABI provenance;
its callback-only implementation and substituted-callee measurements are
not used as full-chain evidence. Source and harness reuse follow merged
[U38 +1730](U38_ROLL_APPEND_1730_CPU.md) and
[U9E +0E52](U9E_LIST_APPEND_0E52_CPU.md), with all ten callee C sources
linked unchanged.

The requested schema-4 queue is now exported as schema 8. This target is
rank 17, the next unmodeled composable target after U86 +059C (PR #1066).
The export used --no-git and missed the stale callback branch; explicit
remote-branch and all-state PR checks caught it. No existing +27D9 PR was
found. This readiness/ownership caveat was reported to the VP before work.

- target_function: OVL_0038_27D9, public entry 84, complete [27D9,2814).
- source_candidates: overlay_u38_roll_effect_27d9_cpu.c with unchanged raw
  U9E dice/append and SYSTEM random/seed/GetMem implementations.
- signals: original hash/entry/bounds, both call fixups, gate/argument order,
  CPU14/full-RAM differential, native replay, semantic sanitizer checks.
- score: five evidence categories (identity, structure, differential,
  native replay, contract checks); no probabilistic score.
- verdict: likely_match within the stated contract; independent review pending.
- review_notes: eight unvisited allocator starts; synthetic native frames;
  fixed SYSTEM layout, bounded dependencies, nonwrapping return frames.
  No classifier, acceptance, accounting or credit changes.

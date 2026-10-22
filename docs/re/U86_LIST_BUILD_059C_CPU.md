# U86 +059C: complete mode-selected list builder

pr_u86_list_build_059c_cpu implements the complete 397-byte entry, composing
the accepted raw +028D append, +044E sidecar and +051C gate APIs. It clears
the list head/history, builds the selected item list, and returns AL=1 when
the final live list pointer is nonzero. Every direct and nested dependency
is on the tested integration base; no pending resident PR is required.

## Selection, identity and provenance

The pinned inventory, call graph and decoded near-call audit were regenerated
from the owner media. The current schema-8 export continues the schema-4
queue with the later readiness corrections. +059C is rank 4, score 71 and the
highest composable row without a main/inline model after filtering existing
implementations. This export omits the branch/history scan; explicit branch
and open-PR checks found no +059C work, and VP reservation i18290 records
ownership. This is a work selection, not a verification or credit.

| Body | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
|U86:[059C,0729)|397 / 154|0c3e43c549277fd6e825ba20a28005b834de4be688dfe7f4cfa184f355d71b4d|
|U86:[028D,044E)|449 / 165|5f82bdbca3270a4d7dcf6a93f41fa14debd04f8f4a21d8860a9b4a64d2cd5038|
|U86:[044E,051C)|206 / 71|a6b87b8c682cc8664e5ada6c55e970983f20987a9780bb65b5bda61770c04606|
|U86:[051C,059C)|128 / 44|ab8e34e0a8a47a7cefa2fc9754079107a0a0736b8a3e596cfee5ec10ae7e9c90|

GAME.OVR file span [161204,161601), entry slot2, terminal RETF2 at+0726.
The refreshed queue records one fixup-backed far caller and no aligned
PUSH-CS caller. Nested caller validation and body hashes reuse the
accepted dependency audits. The seven direct near-call sites are
+060E/+064A/+067A/+070A -> +028D, +068E -> +044E, and +069C/+06A9 -> +051C.
Each follows PUSH CS and validates the reached opcode/displacement.

Owner START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Owner GAME.OVR SHA-256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

Reused public evidence:
[append](U86_STRING_NODE_028D.md),
[sidecar list](U86_SIDECAR_LIST_044E.md), and
[gate](U86_SIDECAR_GATE_051C_CPU.md). Their accepted SYSTEM GetMem/string
and resident lookup models remain unchanged. The implementation reuses
ordered CPU/memory/flag primitives; the oracle and native harness adapt
the gate's full-chain fixtures. Recall found no +059C implementation.
Recall separately confirmed Claude's merged +0714 work, so that earlier
lead was excluded. Original media, prefix bytes, extracted code and native
artifacts remain under ignored resources/.

## Behavior

The low byte of the word at entry SP+4 selects the path. Before selecting
it, the body writes zero to DS:4766/4768 and clears DS:476A..477E inclusive.
It discards the old list pointer without freeing nodes.

| Mode | Original behavior |
| --- | --- |
|0|Scan character offsets17h..2Bh; append bytes in1..127.|
|5|Scan the same21 bytes; append values128..255 with the high prefix bit.|
|1|For indices1..55, append the index when character[32h+index] is nonzero.|
|2|Call +044E with mode0.|
|3|Call +051C with mode0.|
|6|Call +051C with mode1.|
|4|For indices1..56, append when the signed-category matrix byte is nonzero and character[32h+index] is zero.|
|Other|Build no list after the initial reset.|

The character far pointer is DS:5D92/5D94 and is reread in source order.
For mode4, the matrix byte is at character+B1h+
3*signed8(DS:31FA+16*index)+signed8(DS:31FB+16*index). The group/category
bytes are sign-extended, not unsigned host indices. Argument reads,
byte counters, history writes, heap/list pointers and returned state stay
live under data/stack aliases. The final AL boolean tests the live
DS:4766/4768 pair, rather than a host count of successful appends.

## Contract and stopped states

Exactly 1MiB A20-off RAM, IP059C and conservative SP[016A,FFFA]. Ten bytes of
outer frame/argument/call state precede the deepest direct child's minimum
SP0160. CPU/limits/diagnostic objects are distinct from RAM and each other.
Code, prefixes and bindings are immutable; writes avoid code; all reached
return frames remain nonwrapping. Reached child contracts apply. Interrupts,
faults and arbitrary-PC resumption are excluded.

History and item budgets are positive. History visits count +05B7; item
visits count +05D9/+0627/+0663/+06BB. Budgets stop before the exhausted visit.
Ordinary history clearing takes21 visits; item paths take21,55 or56.
Gate clear/record and sidecar caller/list budgets must also be positive;
lookup/heap budgets may be zero. Each direct call gets fresh child budgets.

CODE stops before a malformed near CALL, retaining PUSH CS. CALLEE preserves
the raw child stop and all nested diagnostics. TRANSFER preserves an altered
successful direct return. LOOP_LIMIT retains the pre-visit state. No rollback
occurs. Diagnostic callee0 is append,1 sidecar,2 gate,-1 none; unused nested
diagnostics are -1 and reset with each valid direct call. ARGUMENT preserves
CPU/RAM and initializes diagnostics; a null diagnostic is allowed.

Normal/386 logical AF=0 applies. The two CL=4 shifts have their flags
overwritten before every branch, call, budget stop or return; their register
effects remain exact. No flag masks are used in the evidence.

## Validation

**413 original-byte cases passed**, comparing every CPU14 word and all
**1,048,576 RAM bytes**, with **zero flag masks**. Coverage is **154/154
caller starts and 38/38 caller conditional outcomes**, plus **631/654
composed starts and 95 observed composed conditional outcomes**. Stops:
**358 OK, 14 CODE, 34 CALLEE, 6 LOOP_LIMIT and 1 TRANSFER**.

The corpus exercises all 256 selector bytes, all active modes under six
flag seeds, empty inputs, signed category/group extremes, exact/split free
blocks, allocator exhaustion, custom HeapError boundaries, list/lookup/
heap/gate/history/item budgets, malformed direct and nested bindings,
minimum/maximum/odd stacks, A20 stack wrapping, DS=SS, character offset
wrapping, a history write that resets its own loop counter, and a heap node
that overwrites its direct return frame. The latter matches the original
TRANSFER state. Allocator diagnostics are compared at relevant stops;
focused tests check the remaining diagnostic contracts. This is not a claim
that every diagnostic field was independently compared in every case.

The 23 unreached dependency starts are resident lookup +21D9/+21DD;
SYSTEM +0383/+0385/+0386/+039A/+039E/+03CB/+03CD/+03D0 and
+0BB0/+0BB4/+0BB6; and U86 +0479/+047D/+0481/+0483/+0485/+0487/
+0489/+048E/+0490/+0494. Their absence is not an unreachability proof.
The caller is fully covered; transitive branch closure is not claimed.

**87 native cases passed**: **73 returns and 14 RunError203 stops**.
Each compares **CPU14, SS1KiB, DS64KiB, heap64KiB and node storage128KiB**
(263,168 memory bytes), with **zero flag masks**. Cases cover modes0..7
and six flag seeds, allocator exhaustion/partial heading allocation,
exact/split free blocks, empty inputs, rejected/mixed record categories,
signed matrix indices and character offset wrapping.

Native RESULT.BIN SHA-256:
3202d7de23a4a59d1f248fb074ff37704b9bb28fc035580d7f2afb2608605708.

Strict C11 ASan/UBSan passed **6,717 semantic assertions**. These check
expected item sequences, all selector bytes, prefix-bit item selection through
the composed routines, final list/nonempty result, history reset, nested
allocation diagnostics, exact pre-CALL frames, budget boundaries, live
counter aliases, overwritten returns and argument rejection without mutation.
Focused CTest passed **1/1** on integration base
5438ae2f027244b0a258e9ecca5a1826de3c5a02. Refreshed origin/main matched
that base, with all 35 reused dependency pins unchanged.

Private receipt resources/059c-evidence.json pins **41 public files and
20 private artifacts**, both owner media and tool binaries. It includes
all nine compiled C sources and headers, imported oracle modules, queue
export inputs/selection, guest input/output/assembly/configuration, and
sanitizer/CTest logs. Documentation is excluded to avoid a self-hash cycle.
Capstone5.0.9 and Unicorn2.1.4 were used. Receipt SHA-256:
b47df3c64f29c02961339e3d2f1d63256d1435ff6fa151d4c9200a32da821912.

Native runs directly execute relocated original bodies with synthetic inputs
under DOSBox-X normal/386. A far-JMP capture at SYSTEM RunError records the
original unexecuted entry; it does not implement DOS error termination or
execute a custom HeapError callback. The native corpus covers successful
returns and allocation errors, not every malformed binding or budget stop.
Those boundaries are measured against original bytes and semantic assertions.
Native evidence is not a natural-game reachability claim.

## Reproduction

Requires Capstone, Unicorn, DOSBox-X, GNU as/ld and a C compiler; owner
START.EXE/GAME.OVR stay under resources/DOS/POOLRAD.

    python3 tools/overlay/u86_list_build_059c_audit.py --unicorn --native --output resources/059c-run.json
    cmake -S . -B resources/059c-build
    cmake --build resources/059c-build --target poolrad_overlay_u86_list_build_059c_cpu_tests
    ctest --test-dir resources/059c-build -R '^poolrad_overlay_u86_list_build_059c_cpu_tests$' --output-on-failure

## Author review

- target_function: OVL_0086_059C complete [059C,0729), entry2.
- source_candidates: overlay_u86_list_build_059c_cpu.c plus accepted
  append/sidecar/gate and their raw dependencies.
- signals: pinned identity, original control flow and signed addresses,
  complete caller starts/outcomes, CPU14/full-RAM parity, native original
  execution, expected item sequences and explicit stopped-state assertions.
- score: strong within the stated CPU/heap contract.
- verdict: likely_match within contract.
- review_notes: independent reviewer/VP decides verification and credit.
  No self-credit or accounting edits. Complete caller coverage is not full
  transitive branch coverage; allocator/environment stops remain explicit.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven in a worktree WITHOUT `resources/` (what CI has) with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0086_059C --require-current-main
```

# U3C6_MOV_SP_BP_0062

Local source-only fixture under PM i5776; pre-edit milestone i5840. No native or strict full-C credit.

- target_function: U3C6 public entry 1, instruction [0062,0064).
- source_candidates: src/recovered/overlay_u3c6_mov_sp_bp_0062.c; API pr_u3c6_mov_sp_bp_0062_fixture.
- signals: pinned original instruction/owner SHA, 16-bit Capstone instruction shape, synthetic complete CPU/RAM tests, constructed loaded-byte identity and negative tests.
- score: exact static instruction identity; focused synthetic semantics passed. Native/full-function parity unmeasured.
- verdict: accepted only as the bounded supplied-state fixture described here.
- review_notes: the stop occurs before the next instruction; no producer/frame join, continuation execution, return result or gameplay behavior is established.

MOV SP,BP copies supplied BP into SP. All fourteen CPU words are compared; only SP/IP change. BP, every FLAGS bit and all RAM remain unchanged. The state is supplied independently of the earlier local-byte load; POP BP and RET are not executed.

The API requires nonnull, exactly 1 MiB RAM and a distinct host CPU object with IP=0062. Guest code is immutable and CPU storage is separate from RAM. Invalid pointer, RAM size, entry  arguments reject before mutation. The RAM argument keeps the shared raw-state contract; this instruction does not read or write it. Broad supplied states do not establish reachable game states.

Original media remains ignored under resources/. START.EXE SHA256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd; GAME.OVR SHA256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614. Descriptor 03C6, file base 03073A. Instruction length 2 bytes, SHA256 59aa786538280cb6e5de5d8b80771d9d5c21914f6dd579661adbe8b5b508f278.

The private constructed image contains only the selected original instruction at supplied CS=1404/IP=0062, with other bytes zero. Full constructed-image SHA256 8bf4f4fde0b0ef17a4f81e407e1a786fcd307b4ecbb6b0db2f9dd97dbd3ea8f3. This proves instruction-byte identity at those explicit addresses. It does not identify a game RAM snapshot or execute original instructions. The byte helper checks exact 1 MiB size, explicit 16-bit CS/IP, instruction length/offset bounds and A20 physical addresses. Image provenance is not inferred.

Validation, serialized nice 19 with 20-second per-command caps:

- Strict C11, Wall/Wextra/Werror/pedantic compile passed.
- 65,536 BP complete-CPU cases and 65,536 independent incoming FLAGS cases passed. All FLAGS states accepted. RAM matches the entire initial 1 MiB after each sweep; no Cartesian state coverage claimed.
- Eight additional complete CPU/1 MiB RAM snapshots and six unchanged argument rejections passed.
- Fourteen synthetic loaded-byte unit tests passed, including wrap, malformed domains and wrong-byte rejection.
- Original source versus CONSTRUCTED loaded instruction identity passed; flipped last byte rejects with CLI exit 1. Missing image/CS partner rejects with exit 2.

Private logs/images/binaries are ignored resources/source-gate/. No CMake configure, broad build, CTest, emulator/native oracle or GitHub outgoing operation was run. Heavy/native and publication holds remain. Both RETF CS-word wrap policies remain unresolved and unselected. Native/full-state gate is required before any full-C credit or accounting change. U3C2 entry4 and existing PR150 typed slot ownership are untouched.

Reuse: 8e9871a5237179631181fee70cce47c741f988a8 (owned predecessor and audit/unit template); reviewed 4ef6fb67332cef20ba547e45a44622faae602cdc loaded-byte helper. Bounded deja query U3C6 0062 00ED MOV SP BP MOV AL zero exited 0 with no matches across zero indexed sessions; no indexed finding reused. Ownership screen before edits found zero exact candidate implementations across 24 clone roots/115 source trees in available local scope; hidden ownership is not inferred.

Two distinct follow-on leads are pinned but unimplemented/unreserved:

- Entry1 POP BP [0064,0065), 1 byte, SHA256 cfae0d4248f7142f7b17f826cd7a519280e312577690e957830d23dcf35a3fff; stop before RET at 0065.
- Entry2 PUSH AX [00EF,00F0), 1 byte, SHA256 5c62e091b8c0565f1bafad0dad5934276143ae2ccef7a5381e8ada5b1a8d26d2; stop before MOV at 00F0.

Restack review under PM i6151/i6187: parent 75de529083b0adc8a1950f59bbc0da624d3cb697 includes merged PR531/532 and PR534 static QA foundation. Original reviewed 1126457a49bbe412ef9821d8159fce8eb015934b and separate 06456e5bf75b2576e4b96a4803e6f2f9f09fca95 remain clean/preserved. Header, C implementation, C tests, byte audit and fourteen unit tests are byte-identical to 1126457; only additive CMake registration and this evidence note adapt to main. C source SHA256 2371cfabc9813cd81e160e6dd7226b8056ce81f0587c66727f52907b3864a6a8.

Focused strict C11 ASan/UBSan passed with leak detection enabled: elapsed 0.17 seconds, user 0.14, system 0.03, max RSS 11312 KiB, exit 0. Initial sandbox execution failed at LeakSanitizer startup with its explicit ptrace-unsupported diagnostic; the same binary succeeded outside the sandbox, without disabling leak checks. Both logs are private. Fourteen byte tests passed in 0.089 seconds; original/constructed identity and flipped/missing-pair CLI checks passed again. All checks were serialized with nice 19 and 20-second caps, without native execution.

Merged static CMake source/dependency/acyclic-graph QA passed for 271 CTest registrations and 280 source paths, zero literal DEPENDS setters. Hash-bound recovered source membership passed 231/231 (230 base sources plus this fixture); all base source contents and every base CMake entry are retained. Public tracked-path hygiene, all fourteen prior-research snapshot hashes, and oracle shell syntax passed. No configure, broad build or full CTest was run. This remains an unpublished exact local review candidate; PM pre-SSH review and required remote CI remain outstanding.

Read-only follow-on preparation separately pins POP BP [0064,0065) and PUSH AX [00EF,00F0). Available-local search covered 268 tree roots/885 source directories and found only two existing documentation endpoints, no candidate entry implementation matched. No hidden ownership is inferred. Bounded deja query U3C6 0064 POP BP 00EF PUSH AX stack wrap ownership exited 0 with no matches across zero indexed sessions. No indexed finding reused. Direct original-source decode/hash checks passed. No stack contents were read, no POP/PUSH runtime fixture was added, and no stack-word/RETF wrap policy was selected. Separate runtime scope is required for those leads.

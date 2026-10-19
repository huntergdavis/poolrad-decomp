# U86 +051C record-gated sidecar collection

This completes the 128-byte U86 caller using the merged raw-CPU +323D
predicate and +044E sidecar builder. It clears 49 far-pointer slots, resets
the count, walks the character's record chain, and applies +044E only to
records whose byte at `record+2Eh`, used as an index into the category
table, selects a table entry whose category lies in 11..13. The gated value
is the table entry, not the `record+2Eh` index itself. There are no host
callbacks.
This author PR requests independent review; it assigns no strict credit.

## Identity and provenance

| Artifact | Extent | SHA-256 |
| --- | --- | --- |
| START.EXE |65,231 bytes|d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd|
| GAME.OVR |206,105 bytes|872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614|
| U86 +051C |unit[051C,059C), file[161076,161204), 128 bytes/44 instructions|ab8e34e0a8a47a7cefa2fc9754079107a0a0736b8a3e596cfee5ec10ae7e9c90|
| U86 +323D |77 bytes/29 instructions|2ec1f95931eeb474593d0733ece3c8d582ab3a2a4d1c23b9de59d8f0bb5e87cd|
| U86 +044E |206 bytes/71 instructions|a6b87b8c682cc8664e5ada6c55e970983f20987a9780bb65b5bda61770c04606|

Selected at main3464f6a7 from the schema-8 successor to the schema-4 queue:
rank23, score62, game_evidenced, two composable direct callees. The source
audit independently re-decodes both incoming PUSH-CS/CALL sites, U86+059C
at+069C/+06A9. It checks the full caller hash and extent, its direct calls,
and both dependency hashes. No inferred public entry is added.

Recall query “poolrad U86 051C” returned Claude session281c8700-5ac and the
existing predicate contract. Reused repository evidence is
[U86_RECORD_PREDICATE_323D.md](U86_RECORD_PREDICATE_323D.md) and
[U86_SIDECAR_LIST_044E.md](U86_SIDECAR_LIST_044E.md). The ordered CPU/RAM
primitives, synthetic string/heap setup and original/native harness are
adapted from +044E. The new caller is checked against its own original bytes.

## Raw behavior and limits

pr_u86_sidecar_gate_051c_cpu accepts exact1MiB A20-off RAM and the standard
14-word CPU structure. Entry is after far CALL at IP051C, with mode at
SS:[SP+4]; RETF2 consumes six bytes total. Conservative SP range is0160..FFF8.
Code, inline prefixes and bindings are immutable, reached return frames
remain nonwrapping, and reached callee contracts apply. Host structures are
distinct from RAM and each other. IRQs, faults and self-modifying code are
outside the profile.

The byte local starts at0 and clears DS:477F+4*i through i=48 inclusive,
including both words of each pointer. Stores remain ordered: a local/stack
alias can change the live index and extend the loop. DS:4843 is then zeroed.
LES of DS:5D92 supplies the character; its +C8/+CA pointer is copied to
DS:6B80/+82. Every iteration reloads that live pointer. Non-null records
call +323D; nonzero AL calls +044E with the original mode's low byte.
Afterward the current DS pointer is reloaded, and record +2A/+2C supplies
the next pointer. Neither the next record nor the mode is cached across calls.

Near-call opcode/displacement guards are checked at+0572 and+057E after
the original PUSH CS. The saved frames, callee registers and all RAM writes
are retained. A successful inner function must return to the actual expected
CS:IP before the caller continues. CALLEE preserves the nested raw status
and all four +044E diagnostic fields. TRANSFER preserves an altered direct
return. CODE stops before a malformed reached CALL. There is no rollback.

Clear and record visit budgets are positive, checked before+052B and+0560.
Ordinary clearing uses49 visits. Record visits include the final null test.
A limit can also stop aliased/cyclic states, with CPU/RAM at the next visit.
Every +044E invocation starts fresh subordinate caller/list/lookup/heap
budgets. Its caller/list limits must be positive; lookup/heap may be zero.
ARGUMENT leaves CPU and RAM unchanged and initializes the optional diagnostic.

Flags follow the inherited DOSBox-X normal/386 profile; logical AF is zero.
No flag masks or undefined-flag exceptions were needed in this caller corpus.

## Measured verification

| Gate | Result |
| --- | --- |
| Original-byte differential |361 cases:338 returns,2 CODE,15 CALLEE,2 TRANSFER,4 caller limits|
| Comparison |all14 CPU words and all1,048,576 RAM bytes; no masks|
| Caller coverage |44/44 instruction starts;6/6 conditional outcomes|
| Composed coverage |491 instruction starts;66 conditional outcomes|
| Native DOSBox-X normal/386 |100 cases:93 returns,7 RunError203; no masks|
| Native footprint |CPU14, SS1KiB, DS64KiB, heap64KiB, node region128KiB|
| C validation |27,671 ASan/UBSan checks; focused CTest1/1|

Cases exercise all256 category values, modes, one-to-three-record chains,
mixed accepted/rejected records, empty/cyclic chains, signed gates, lookup
cycles, string/list and heap paths, all budget layers, malformed direct and
nested bindings, odd/boundary SP, A20 wrap, record/character offset wrap,
SS=DS, seeded background, local-index aliases, cleared saved BP/outer return,
and heap aliases changing direct/nested return frames.

Native fixtures perform real far CALLs with synthetic inputs. Only SYSTEM
RunError entry is redirected to capture. The initial native node window was
too small for wrapped-pointer cases; it was expanded to cover physical
50000..6FFFF in both the input and output. Final captures include every
mutable fixture region; original-byte comparison independently checks all
RAM. Native inputs use fixed relocated original call operands, never C
callbacks. Local generated assembly/COM, original bytes and captures stay
under ignored resources/.

Native synthetic reachability does not prove a natural saved-game route.
Alias-induced transfer and loop-limit cases have original-byte differential
coverage, but are not claimed as native cases. Existing callee coverage
does not imply every transitive instruction is visited by this corpus.

Combined receipt SHA-256:
34cea76a08a471d3d45e792ab71cd2a367678cf909b71df023fefd5005dcd4b5.
Native RESULT.BIN SHA-256:
f87a0d73356681d63a6899f46e279f174e8ad2538ca9f71b8a00f329690d7560.

## Reproduction

Owner media belongs under ignored resources/DOS/POOLRAD.

    PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u86_sidecar_gate_051c_audit.py --unicorn --native --output resources/u86-051c-evidence.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_overlay_u86_sidecar_gate_051c_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_overlay_u86_sidecar_gate_051c_cpu_tests$' --output-on-failure

The audit compiles its isolated C dependency chain with strict compiler
warnings. Sanitizer checks compile the same SOURCES list plus the focused
test with -fsanitize=address,undefined -fno-omit-frame-pointer.

## Parity review record

- target_function: OVL_0086_051C.
- source_candidates: overlay_u86_sidecar_gate_051c_cpu.c, composing existing
  overlay_u86_record_predicate_323d.c and overlay_u86_sidecar_list_044e.c.
- signals: exact extent/hash, aligned callers, complete local coverage,
  independent original execution, native CPU/RAM captures, negative stops.
- score: high behavioral evidence under the declared raw profile.
- verdict: likely_match, pending independent reviewer acceptance.
- review_notes: inspect live pointer/local aliases, propagated diagnostics,
  visit-budget semantics, native footprint and synthetic reachability limit.
  No classifier, strict-map, identity, accounting or credit edits are included.

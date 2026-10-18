# U7F +0508 resource-list transfer and clear

The complete CPU14/shared-RAM entry `pr_u7f_clear_0508` implements U7F public
entry 2, including the accepted +0020 record subtractor and SYSTEM FillChar.
It reads the list head from DS:5D96/5D98, sets DS:6DC0, and walks the live
next pointers at record+0104/+0106. For record type 0 or B3 at +0084 it adds
seven unsigned resource words at +0088..+0094 to the seven 32-bit DS totals
at 67F4, subtracts each live amount from record+0102 through +0020, then
clears those fourteen resource bytes through the original FillChar.
Other record types are traversed without transferring or clearing.

This is an author evidence submission. Independent review owns acceptance
and credit. No ledger/accounting changes are included.

## Ownership, recall, and reused source

Selected schema-8 queue rank 10, the next available composable hard target
after the rank-8 U25 +01B6 submission. The user's schema-4 queue workflow
continues through its current schema. The queue labels +0020 as modeled;
its actual accepted API exposes all fourteen CPU words and ordered RAM
effects, so this call chain is composable without a semantic callback.

`timeout 20 deja "U7F 0508 0020 full CPU composable ownership"` completed
without matches. No historical session findings were reused.

Memory/flag primitives are adapted from
`overlay_u7f_resource_subtract_0a01.c`, commit
`92db1c801c9dc5db7c12a1872d62c9a86e3e83d8`.
The +0020 callee is linked unchanged from
`overlay_u7f_record_subtract_0020.c`, last source change
`5e3f814cd62f4cd0b094c06ccc4dfeb9469cba69`.
FillChar is linked unchanged from `system_runtime_leaves_cpu.c`,
commit `cfa24e99f63f91a7a7aa058e87d0651e28c3790d`.
Its separate CPU type is copied by named fields in both directions.
The audit/native capture structure follows the U25 +01B6 author tooling,
commit `881a0a7e2b6c98eea1b27a8e3bd2a6d83922ebbe`; this submission does not
require that caller's implementation or acceptance.

## Source identity

Owner-provided inputs, kept under ignored `resources/`:

| Input | SHA-256 |
| --- | --- |
| START.EXE | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Body | Bytes | Instructions | SHA-256 |
| --- | ---: | ---: | --- |
| U7F [0508,05E0), file [25191,25269) | 216 | 79 | `a9f79cf66963228814271af27cb8b03f11fd48cab64f778db4f2a2a35efb7efd` |
| U7F [0020,003A) | 26 | 9 | `f1e091b372c0401a7b90dac9e6a32f12c64b898c8f0039a69df380669e24e13a` |
| SYSTEM [16A8,16BC) | 20 | 7 | `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7` |

The closed chain contains **262 instruction bytes / 95 instruction sites**.
The caller's final RETF is +05DF, with no arguments. Entry 2 is stub
008F:002A. The direct PUSH CS / near CALL at +05A8 targets +0020 and
returns to +05AB. The far CALL at +05C1 targets SYSTEM 0AF8:16A8;
its segment word has the overlay fixup +05C4 and returns to +05C6.
The source auditor pins the owner entry, bytes, extent, fixup and both calls;
the reused +0020 auditor independently pins its three direct public callers.
The queue records three far callers of entry 2: U23 +0657, U1E +0DE8,
and U20 +105E.

## API and partial-state contract

- Enter after the caller's far CALL with IP=0508, the return IP and CS at
  SS:SP and SS:SP+2, and no arguments.
- RAM is exactly 1,048,576 bytes, A20 disabled. The CPU object is separate
  from RAM. CPU/RAM access is synchronous.
- Initial SP is in [0014,FFFC]. Twenty bytes reserve the caller's frame and
  deepest nested stack. Initial wrapped return frames are outside this API.
- Loaded caller/helper code and the supplied SYSTEM segment binding are
  immutable, and writes must avoid that code. This is a precondition,
  not a dynamic code-write guard.
- Effective offsets wrap at sixteen bits; bytes within a word or LES operand
  are physically contiguous with twenty-bit address wrap. Next-pointer words
  at +0104 and +0106 are two separately addressed original MOVs.
- Positive node and item budgets count visits before +0520 and +054D.
  The node budget includes the final null-pointer test. The item budget is
  shared across records. Cyclic lists and altered loop counters can stop
  with exact CPU14/RAM before the declined instruction.
- An overwritten internal return target produces TRANSFER immediately after
  the actual RETF, retaining its effects; execution at that changed target
  is not claimed. The outer RETF returns to its actual frame contents.
- Null arguments, incorrect RAM size, wrong entry IP, invalid initial SP,
  or zero budgets are rejected atomically.

Record pointers, next pointers, indices, resource values, totals and stack
locals are read in instruction order. Aliases are not replaced by a snapshot.
In particular, the amount pushed at +05A2 is reloaded after the accumulator
writes, and the next pointer is loaded after FillChar. Arithmetic preserves
defined x86 flags; the adapted SHL primitive retains the accepted native
normal/386 AF=1 convention. Later defined arithmetic overwrites SHL flags
before every exposed stop/return in this audit, so neither oracle needs a mask.

## Measured author evidence

**596 original-code comparisons passed**, each checking all fourteen CPU
words and all 1,048,576 RAM bytes:

- 571 complete returns.
- 12 node-limit stops at +0520.
- 11 item-limit stops at +054D.
- Two changed internal returns, retained as 3000:05AA and 2FFF:05AB.
- 79/79 caller sites and 95/95 chain sites.
- 8/8 conditional outcomes.
- Zero FLAGS masks and zero emulator CPU substitutions.

Fixtures include every type byte, empty and multi-node lists, arithmetic
carry/overflow boundaries, cyclic lists, both budgets, odd/boundary stacks,
word and record seams, A20 wrap, relocated code, data aliases, stack/local/
return-frame aliases, and 250 deterministic randomized cases. RAM code
integrity is checked after original execution. These are synthetic fixtures,
not natural game-route observations.

**23 DOSBox-X normal/386 captures passed**, all complete original-chain
returns. Each compares CPU14, the entire 65,536-byte DS segment, and
256 stack bytes: [EFC0,F040), [0000,0040), [FFC0,10000).
Total RAM compared per capture is **65,792 bytes**. FLAGS are unmasked.
Captures include empty/eligible/skipped/mixed lists, five-node lists,
arithmetic extremes, SP=0014/0015/FFFB/FFFC, and resource writes aliasing
the totals or list-head globals. The harness loads hash-pinned original
code, relocates the SYSTEM segment word, disables interrupts for capture,
and runs the original nested callees without interception.

**103 semantic checks passed under strict C11 warnings and ASan/UBSan**.
They assert transferred totals, record subtraction and clearing, skipped
records, preserved CPU fields, defined final flags, cycle/iteration stops,
changed nested returns, and atomic invalid-input rejection.
The full recovered library and focused CTest also pass.

Native partial stops, initial wrapped return frames, indirect LES operand
seams, hardware CPU portability, and a natural game route are not claimed.
Native coverage is the DS segment plus stated stack windows, not whole RAM.
The independent reviewer must assess the exact submitted head.

## Reproduction

Install Capstone and Unicorn in the local Python environment, and provide
the pinned owner media at `resources/DOS/POOLRAD/{START.EXE,GAME.OVR}`.
DOSBox-X, GNU as/ld, a C11 compiler and CMake are also needed.

```sh
mkdir -p resources/u7f-0508
PYTHONPATH=tools/overlay python3 tools/overlay/u7f_clear_0508_audit.py   --output resources/u7f-0508/audit.json
PYTHONPATH=tools/overlay python3 tools/overlay/u7f_clear_0508_native.py
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined   -I include tests/recovered/test_overlay_u7f_clear_0508.c   src/recovered/overlay_u7f_clear_0508.c   src/recovered/overlay_u7f_record_subtract_0020.c   src/recovered/system_runtime_leaves_cpu.c -o resources/u7f-0508/test-clear
resources/u7f-0508/test-clear
cmake -S . -B resources/u7f-0508/build -DBUILD_TESTING=ON
cmake --build resources/u7f-0508/build --target poolrad_overlay_u7f_clear_0508_tests
ctest --test-dir resources/u7f-0508/build -R u7f_clear_0508 --output-on-failure
```

Raw binaries, extracted instructions, captures, logs and the final receipt
remain under ignored `resources/u7f-0508/`.

## Author parity assessment

| Field | Assessment |
| --- | --- |
| target_function | OVL_007F_0508, entry 2, complete [0508,05E0) and both callees |
| source_candidates | overlay_u7f_clear_0508.c plus accepted +0020 and SYSTEM FillChar |
| signals | Pinned media/body hashes, entry and relocation, exact calls, full site/edge coverage, whole-RAM oracle, independent native backend, ordered-alias tests |
| score | Strong source identity and tested contract parity; no natural-route evidence |
| verdict | likely_match, pending independent review |
| review_notes | Check budget boundaries, late amount/next-pointer reloads, overwritten return handling, immutable-code precondition, and native scope; no acceptance or credit asserted |

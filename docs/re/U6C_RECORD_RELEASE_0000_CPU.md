# U6C +0000 record release: complete raw CPU composition

This implements the complete U6C internal helper [0000,00E2), composing the
merged raw SYSTEM FreeMem implementation without callbacks or skipped heap
effects. It releases an optional sidecar, two lists, and the outer record.
See the [source](../../src/recovered/overlay_u6c_record_release_0000_cpu.c),
[ABI](../../include/poolrad/recovered/overlay_u6c_record_release_0000_cpu.h),
[original-byte audit](../../tools/overlay/u6c_record_release_0000_audit.py),
and [native harness](../../tools/overlay/u6c_record_release_0000_native.py).

This is author evidence for independent review, not an accounting change.
The queue classifies this as a provisional helper_candidate,
glue-runtime-only, with no exported entry slot. Its two verified internal
call sites belong to U6C +3291. No identity promotion, merge, or credit is
claimed here.

## Identity and extent

| Item | Measured value |
| --- | --- |
| Target | OVL_006C_0000, descriptor unit 006C, memory segment 0078 |
| Executable span | [0000,00E2), 226 bytes, 73 instructions |
| GAME.OVR file span | [104831,105057) |
| Body SHA-256 | 89b9da1045a0ae35910cc989734674536cd458aa8f7d74389f5460635f9e6983 |
| Exit | RETF 4 at +00DF |
| Direct calls | +002F, +0076, +00BE, +00CF to SYSTEM 0AF8:0254 |
| Segment fixups | +0032, +0079, +00C1, +00D2 |
| Internal callers | U6C +3291, PUSH CS / CALL at +334A and +33B2 |
| Queue bound | 353 bytes / 137 decoded instructions, ending at next entry +0161 |

The queue bound includes 127 trailing data bytes [00E2,0161). They are
excluded from the recovered body, coverage, and identity claim. The audit
verifies media hashes, descriptor, fixups, internal calls, exact return,
and body hash before execution.

| Private media | SHA-256 |
| --- | --- |
| START.EXE | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |

The unchanged FreeMem closure is pinned by the reused FreeMem audit:

| SYSTEM span | Bytes / instructions | SHA-256 |
| --- | --- | --- |
| [0254,026E) | 26 / 11 | 47b372b606692fcf3038cd6734cf3f3c600d9dff7298dce2a50d2d0a03196c96 |
| [03D3,0432) | 95 / 42 | 828388f0c2e899b6aaaf00c9c294397c879075f524eff052e73d26cde7e79311 |
| [0432,0491) | 95 / 45 | b839d3ecc1ecd5e593a51dbb7c22231ad96156638c4676c7a3753bccff5bb7c9 |
| [0491,04A2) | 17 / 8 | 440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d |

## Behavior and profile

After a far call, the stack contains return IP/CS followed by record
offset/segment. The function copies that pointer to SS locals, then:

1. Frees a nonnil far pointer at record+108 with size 0016.
2. Walks record+C8, reading next at node+2A before freeing size 003F.
3. Walks record+7F, reading next at node+5 before freeing size 0009.
4. Rereads the original stack argument and frees size 011D, clears that
   stack argument, restores BP, and returns with RETF 4.

The last step does not clear an external caller-owned pointer. Source fields
are not proactively reset. FreeMem overwrites freed nodes, including the +5
link of a nine-byte node; read-before-free ordering is essential. Stack
locals and original arguments stay live. The final free does not substitute
the initially copied record pointer.

The profile is exact 1 MiB A20-off RAM, entry IP=0000, initial
SP in [0040,FFF8], immutable code/bindings, and nonwrapping reached return
frames. CPU, limits and diagnostic objects are distinct from RAM and each
other. Words and LES pairs use physically contiguous bytes, including
offset FFFF. Interrupts, faults, arbitrary-PC resume and self-modifying code
are excluded. Logical AF follows DOSBox-X normal/386 zero. Relocated copies
of the same FreeMem closure are supported through live call segment words.

The shared list budget counts taken +007B->0047 and +00C3->008F edges.
Zero permits the first node's actual free, then LIMIT at the next loop head.
The heap budget is per FreeMem invocation. CODE stops before a reached
malformed call opcode/offset. CALLEE retains FreeMem's RunError204, LIMIT,
TRANSFER or ARGUMENT state. A changed far return yields TRANSFER.
No state is rolled back; ARGUMENT leaves CPU/RAM unchanged.
Diagnostics identify the last reached call site and last FreeMem status.

## Measured evidence

Unicorn 2.1.4 executes the original caller and FreeMem bytes. Every CPU word
and every byte of the 1 MiB RAM image is compared, with zero masks or
emulator substitutions.

- 425 cases: 395 returns, 15 RunError204 callee stops, 3 list LIMITs,
  2 heap LIMITs, 8 CODE stops, 1 changed far return and 1 changed
  FreeMem near return.
- Caller: 73/73 instruction starts and 6/6 conditional outcomes.
- Combined closure: 179/179 starts and 39/40 conditional outcomes.
  The sole uncovered outcome is FreeMem +03D5->0431 (zero-size no-op);
  this caller pushes fixed nonzero sizes 16h, 3Fh, 9 and 11Dh.
- 2,750 FreeMem invocations across the corpus.

Cases cover optional sidecar, empty/populated lists, lengths 0..6,
offset-0/8 blocks, adjacency merges, heap-top shrinking, invalid/nil/out-of-
range/duplicate pointers, cycles, budgets, four malformed call sites,
unreached malformed sites, relocated callees, random RAM, register/flag
patterns, SS=DS, A20-wrapped stack, stack bounds and heap/stack aliases.

With SS=3000 and SP=2000, freeing sidecar 3200:0000 changes the original
stack argument to 0001:0008. The final free rereads it and stops at
RunError204. Using the saved record pointer would incorrectly free
3100:0000. Other aliases exercise changed root, far-callee and near-callee
return frames.

DOSBox-X normal/386 synthetic replay passes:

- 128 cases: 117 returns and 11 stops before RunError204.
- All 14 CPU words, 1,024 SS bytes, full 65,536-byte DS segment and
  65,536-byte heap arena agree, with zero masks: 132,096 RAM bytes.
- RESULT.BIN SHA-256:
  986d56f0f6b6b3d6393bec5a301f3b559769f530b22032710ea3fffe09897de9.

The native harness patches only the external RunError entry to capture its
boundary, not the compared closure. This is synthetic execution, not
natural gameplay reachability or full-memory native evidence. Original
alias/budget cases are not claimed as native coverage.

The C suite passes **3,481 checks under ASan/UBSan**: free-list order,
rounded sizes, preserved source fields, stack-copy clearing,
read-before-free links, limits, final-argument reload, transfer stops,
malformed calls and unchanged invalid arguments. Focused CTest passes 1/1.
Two C sources are compiled. FreeMem source/header are unchanged from base
5438ae2f027244b0a258e9ecca5a1826de3c5a02.

## Reproduction and private receipt

Provide owner-supplied media in ignored resources/DOS/POOLRAD/.
Capstone distribution 5.0.9 reports binding version 5.0.7; Unicorn is 2.1.4.
No original/extracted bytes or generated capture binaries are committed.

    PYTHONPATH=resources/pythondeps:tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u6c_record_release_0000_audit.py --unicorn --output resources/original.json
    PYTHONPATH=resources/pythondeps:tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u6c_record_release_0000_audit.py --native --output resources/native.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_overlay_u6c_record_release_0000_cpu.c src/recovered/overlay_u6c_record_release_0000_cpu.c src/recovered/system_freemem_0254_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_u6c_record_release_0000_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_u6c_record_release_0000_cpu_tests$' --output-on-failure

Private evidence remains in /tmp/poolrad-u6c-0000/resources/.
The evidence.json receipt pins compiled sources, local Python imports and
headers, queue inputs, tool binaries, media, logs, and native artifacts.
Native files are under resources/oracle-runs/u6c-0000-synthetic/.
The PR handoff records the receipt digest and exact committed head.

## Selection, reuse and author assessment

The requested schema-4 queue now exports schema 8. The pinned no-git export
ranks this row 362, score 25, ready 1/1 with SYSTEM FreeMem. Higher
substantive ready targets U86+059C and U38+27D9/+11F9/+1276 are pending in
#1066–#1069. Other higher unmodeled ready rows are five-byte stubs or
U86+3092's 16-byte upper-bound leaf. This is the next substantive composition.
Remote branch/open-PR checks supplemented the no-git export; reservation
i18331 was sent to the VP. U6C +3291 retains its unready U3CE +014E edge,
so this helper alone does not make that caller composable.

Recall query U6C 0000 SYSTEM 0254 returned no matches. No recalled
implementation was reused. Memory/flag idioms follow the merged
overlay_u9e_list_release_0026_cpu.c. SYSTEM FreeMem is composed unchanged.
Fixture and capture patterns derive from the merged FreeMem/GetMem audits;
local imports are pinned in the receipt.

| Assessment field | Author finding |
| --- | --- |
| target_function | OVL_006C_0000, [0000,00E2) |
| source_candidates | New raw caller; unchanged SYSTEM FreeMem closure |
| signals | Body/fixup/caller identity, full-state original differential, native replay, semantic heap invariants |
| score | 4/4 evidence categories present; not a probability or independent approval |
| verdict | likely_match within the documented bounded CPU profile |
| review_notes | Provisional internal-helper identity; trailing data excluded; zero-size callee edge unexercised; synthetic native frames; no accounting edits or self-credit |

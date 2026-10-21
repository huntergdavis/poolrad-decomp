# OVL_006F +0EC4: translated, evidenced monster-record loader (evidence only; no credit proposed)

This raw CPU body, `OVL_006F_0EC4` (unit 0x6F, entry slot 6, run-time stub `007C:003E`), builds three file names from the digit string of `DS:[5376]`, loads three
blocks through the resident block loader `0802:0716`, copies the first into the caller's 11Dh-byte record, clears four far-pointer fields, derives a class byte,
rolls five dice into the record, and chains the second and third blocks into 9-byte and 3Fh-byte cells. It is the **program** the 0BF5 evidence change named as its
parent: its direct dependency `RES_0709_0BF5` is on main (#1353). This change adds the model, its checker, its native harness and this document. It proposes **no
strict row, no identity row and no manifest row**; par-e accepts or refuses.

## Media and executable extent

START.EXE SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd; GAME.OVR SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614. Loader-expanded image SHA-256
a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e.

The decoded span is **unit 0x6F [0EC4,1262), 926 bytes / 341 instructions**, GAME.OVR file offset 126,634 (0x1EEAA), ending in the only return, `RETF 6` at 125F.
Body SHA-256 `1160665fef6397a88b922ce115836e5cf4b55f09b5c4db04e765e8a02f08f747`. The span was taken from the GAME.OVR bytes, never from a queue row. The checker
asserts the hash, the 341 starts and their extent, **16 conditional branches (7 jne, 5 je, 2 jb, 2 jae) and 5 unconditional jmp**, the single `RETF 6`, the
absence of any interrupt, port or near call, the 32 far calls with their segment words, and that **exactly 32 unit relocations fall inside the span, each the
segment word of one far call** (the unit's `lcall_segment` fixup table). Each segment word is read live by the model.

The six name constants and the message literal live just before the span, at **+0E98..+0EC4 (44 bytes, plain data, no relocation, SHA-256
`c8f784380312a0bb469beaae250edf2c50bd5898da328bdf4d4f01e6d40d366b`)**. They are data the string loads read, not executable bytes, and are not part of the span or
any hash-bound executable entry. Their text stays in the ignored media.

## What the body does

Frame: a far call leaves `[SP]` IP, `[SP+2]` CS, `[SP+4]/[SP+6]` the far record R (offset, segment), `[SP+0Ah]` the block id (a word slot, read as a byte). RETF 6.

1. `PUSH BP; MOV BP,SP; SUB SP,11Ah`; **Str** `0AF8:1366` makes the digit string of `DS:[5376]` (Width 0, maximum length **1**, so the name holds the first digit only).
2. Block 1: name := c0E98 + digit + c0E9C (StrLoad, StrConcat, StrConcat), extension := c0EA0, then `0802:0716` (id, `&size [BP-2]`, `&block [BP-6]`).
3. **Size 0:** the message c0EA5 through StrLoad, `0709:0BF5` (colour 0Fh, waits for a key), then `0899:0000` (QUIT). Neither returns on its own: QUIT ends in Halt.
4. Otherwise `Move(11Dh, block, R)`; R+C8/CA, +7F/81, +104/106, +108/10A := 0; the class byte R+C0 := 1 when the loaded byte at R+2E is 1, 3 or 5, := 2 when it is
   2, 4, 6 or 7, else it keeps the loaded value; **five rolls** through the stub `00B0:0048` (count 1, sides 6) store `((r+8)<<4)+r` (low byte) at R+C1, C2, C3, then R+C4 := C4h,
   then R+C5, C6; `FreeMem(block, size)`; **`DS:[557C] := 1`**.
5. Block 2 (name c0E98 + digit + c0EBC): size 0 skips; otherwise 9-byte cells chained from R+7F/81 (GetMem 9, Move 9, link at +5/+7, do-while the running offset is below the size); FreeMem.
6. Block 3 (c0EC0): the same with 3Fh-byte cells chained from R+C8/CA, link at +2A/+2C.
7. **`DS:[557C] := 0`**; `0899:0104` (the idle prologue); `MOV SP,BP; POP BP; RETF 6`. No value is returned.

## The roll stub 00B0:0048, declared exactly

The two roll calls are **real, fixup-backed far calls**, not patches: the segment words at `0FEE` and `103B` are `lcall_segment` entries of unit 0x6F's table, both reading **00B0**,
which is **unit 0x9E's run-time stub segment** (the unit table derives it from the next descriptor, `memory_segment_sequential_ok` true). The offset **0048 = 20h + 5 x 8** is
entry 8 of that stub table, and unit 0x9E's entry 8 is `+0DE3`. In the game the overlay manager builds that table at run time as five-byte stubs that dispatch through INT 3Fh. **The expanded
image does not contain the run-time stub**: the five bytes at 00B0:0048 in the loader-expanded image are `0e 10 0f 0d 0c`.

The substitution, identical in the oracle and in the C model: **the INT 3Fh overlay-manager dispatch is not executed.** The five bytes at 00B0:0048 are replaced by a far JMP
(`EA E3 0D <lo> <hi>`) to the loaded unit 0x9E `+0DE3`; the original unit-0x9E bytes `[0DE3,0E52)` (SHA-256 pinned by the merged 0DE3 audit) run in the oracle at their own load
segment, with their SYSTEM call segment word relocated by the unit's own fixup table. The C model reads the stub segment word live, requires the stub entry to be `EA` + offset `0DE3`
(else CODE, with IP at the call and before its return frame), takes the target segment from the stub's own bytes and enters the strict `0DE3` composition. **What this pins:** the call
frame (count 1, sides 6), the draw order and the seed. **What it does not pin:** the overlay manager itself, its stub construction, or that unit 0x9E is resident when a real game calls.

**The five draws are part of the claim.** They are made in this order and stored at R+C1, C2, C3, C5, C6 (C4 is the constant C4h, not a draw), from `RandSeed` at `DS:[4428]/[442A]`, which is composed
state: the checker compares the seed and every stored byte, and an independent statement (the SYSTEM LCG `seed := seed x 08088405h + 1`, `r := (seed >> 16) mod 6 + 1`, stored byte `((r+8)<<4)+r`)
agrees in every case that returns with a full record.

**Finding on the merged 0DE3 model (not changed here).** `overlay_u9e_entry8_system_call_0e0e.c` fixes the SYSTEM segment at `0AF8` (`SYSTEM_CS`) instead of reading the call's relocated segment word, so
it is correct only when the image is loaded at segment 0. With the image at base 1000h its Random reads the multiplier at `0AF8:1245`, finds fill bytes (5A5Ah), and draws a different seed (observed). This audit declares
a **shadow**: a byte copy of exactly `0AF8:[11C0,1250)` (0x90 bytes) is placed in both runs; the oracle executes the relocated original and never touches it. It is a fact about that model for its reviewer.

## DS:[557C], the disk-prompt flag

`DS:[557C]` is set to **1 mid-routine** (at 107B, after block 1 has been loaded and freed) and **cleared at the end** (at 1252, before the idle call). It is not touched on any other path, so on EXIT,
RUNERROR and STALLED paths it is left as it stands. Its meaning is `0802:0716`'s, measured here: with the flag **0** and the block's file absent, the loader's insert-disk prompt appears and the run waits for keys
(cases `block1-absent-prompt-then-ctrl-c`, `block1-absent-prompt-never`, `block1-absent-prompt-then-appears`); with the flag **1** an absent file silently yields size 0. Block 1 is loaded under the **entry** value
(random in the cases: 0 or 1), blocks 2 and 3 are loaded with it forced to **1**, so a missing second or third file is skipped without a prompt (`block2-file-absent`, `block3-file-absent`, `blocks23-absent`: RETURN, the
chain fields 0). On the returning path it leaves **0 regardless of its entry value** (independent check, every RETURN case).

## Role (identity is not claimed)

The role is the game's **monster-record loader**: its subject is the 11Dh-byte monster record, its class byte, five random bytes, and the special and item chains. Measured ownership facts: **one fixup-backed far caller**
(`0025:0444`, inside `OVL_0025_0411`, the LOAD MONSTER wrapper) and **one near caller** (unit 0x6F, function 1262, site 1291, aligned `PUSH CS` and exact prologue), from the regenerated export. This note makes no claim about identity: the two-factor test is the reviewer's.

## Membership chain

Facts only; nothing here admits a member.

- The regenerated queue (2026-10-21, before #1353) priced `OVL_006F_0EC4` at **2,706 bytes / 13 bodies**, with no uncovered SYSTEM endpoints and no declared boundaries. The body's 11 call targets were: `OVL_009E_0DE3` (strict),
  `RES_0709_0BF5` and `RES_0802_0716` (both then `resident_unmapped`), `RES_0899_0000` and `RES_0899_0104` (resident covered) and six SYSTEM leaves (runtime covered: Str, StrLoad, StrConcat, Move, GetMem, FreeMem).
- `RES_0709_0BF5` is now on main as an evidence-only model (#1353); this change composes it. Its three display callees and ReadKey are merged bodies.
- `RES_0802_0716` is on main as a model with its cluster composed (#1343): twelve bodies, 1,813 bytes. Per the t30 record, **six of the cluster's nine identity candidates (0040, 0105, 0193, 050F, 05CD, 06E7) are near-only residents** whose `RES_` aliases exist but whose classification has not been granted, and 0175, 0000 and 0020 are side (ii) non-credit. **This document grants and requests nothing for them.**
- The closure's model side here is **63 C sources, this body included** (the checker's `SOURCES`: the 0716-composed list, the 0BF5 list, the SYSTEM leaf models, the idle, QUIT, and the 0DE3 chain).

## Raw contract and stops

`pr_u6f_entry6_0ec4_cpu(bus, size, cpu, env, diagnostic)`: exactly 1 MiB of A20-off RAM, IP 0EC4, **entry SP in [09F4,FFF4]**, immutable code, distinct CPU/bus objects, an attached EGA. The bound is measured and
explained: the binding callee is `0802:05CD`, entered from 0716 at its first open, which needs SP >= 066Ah; 066Ah plus 0716's frame and pushes (258h) plus the 132h this body has pushed when it calls 0716 is 09F4h. A scan at every SP from
0432h to 1200h over ten scenarios found nested refusals up to exactly 09F3h and none from 09F4h; the bound is the measured minimum over those scenarios, and a nested refusal below it (CALLEE) is a stop of the model, not an
architectural state. Every access is physical flat RAM: the record, the blocks, the heap and the string windows exclude A0000..AFFFF, the one aperture being the EGA that 0BF5 reaches through the bus. No interrupt, fault, self-modification
or arbitrary-IP resume. Every flag writer of the body is executed; the shift and three adds whose flags are overwritten before any callee are folded.

Every stop, with its class, where it comes from and the check that pins it:

| Stop | Class | Origin | Pinned by |
| --- | --- | --- | --- |
| RETURNED | RETURN | `RETF 6` at 125F | Unicorn: C and the original agree on CPU14, all 1 MiB, EGA, file service and logs in 107 of 139 cases; native: 31 of 33 |
| EXIT | EXIT | Halt `0AF8:00E9`: the message path's QUIT (0F49), QUIT or ReadKey's hotkey inside 0716 or 0BF5 | Unicorn: 23 cases, same state at the Halt boundary on both sides |
| BREAK | EXIT (Crt Ctrl-Break) | ReadKey's break path inside the 0BF5 prompt | Unicorn: 1 case |
| RUNERROR | EXIT-class abort | RunError `0AF8:00E2` from GetMem, FreeMem or IOCheck (heap exhausted) | Unicorn: 7 heap-size cases (3 inside the first 0716 load, 4 later); native: 2 |
| STALLED | HANG | 050F's proven fixed point inside 0716's decode: the original never returns | Unicorn: 1 case; the original reached the same 051F arrival and ran 1,000 further arrivals unchanged |
| DIVIDE | EXIT-class abort | BlockRead's DIV by record size 0 inside the closure | **not reached by any case** (BlockRead's record size comes from the file record the open sets, and no case corrupts it); stated, not excluded |
| LIMIT | LIMIT (host cap) | a free-list walk (100,000 steps), the 0DE3 slice cap (256), 050F's arrivals (1,000,000), ReadKey's drain (64) | **measured 0** in the differential (a LIMIT would be a non-architectural stop) |
| SERVICE | SERVICE | a declined DOS or keyboard service | **measured 0**; in the oracle an undeclared service is a hard environment refusal |
| ARGUMENT | ARGUMENT | null/size/IP or entry SP outside the contract | C test (164 checks, ASan/UBSan): CPU, RAM and EGA untouched |
| CODE | ARGUMENT-class | a call's opcode or offset, or the roll stub entry, is not the original; IP at the call, before its return frame | C test (the 0716 call word, the first Str call opcode, the StrLoad offset); **0 in the differential** (code is immutable there) |
| CALLEE, TRANSFER | non-architectural | a nested model stopped otherwise, or returned elsewhere than the instruction after its call | **measured 0** |

The final run has **0 non-architectural stops** (ARGUMENT, CODE, LIMIT, SERVICE, CALLEE, TRANSFER): they are counted per case, not asserted.

## Evidence

**Original-byte Unicorn differential on the composed closure: 139 cases, 0 mismatches.** The whole loader-expanded image is loaded at base 1000h with the loader's relocation table, the two unit spans are placed at their own load segments with their own fixup tables applied,
and the stub is the declared trampoline. Every original byte of the closure runs: this body, Str, StrLoad, StrConcat, Move, GetMem, FreeMem, the 0716 cluster, 0BF5 with 05EB/0626/01DF, ReadKey and QUIT, the idle, unit 0x9E +0DE3 and SYSTEM Random.
Environment, declared and identical in oracle and C callbacks: the 0716-composed audit's DOS file service **routed by name over three archives** (the three suffix constants are read from the bytes at run time; each case chooses which archives exist and which hold the id),
its keyboard contract, the logging INT 10h stub, ReadKey's services, and the EGA ports (an independent Python device on the A000 window). After every case, **unmasked**: CPU14 (**dirty incoming flag words 0202, 0602, 0A87, 0402 and 0E57 are in the case set**, on the full path and the message path),
**all 1,048,576 RAM bytes** (`ram_masked_bytes = 0`), the file-service state and its ordered log, the ordered environment log, the full EGA state, the ordered EGA OUT events and VRAM write operands, and the end state.

Outcomes: **107 RETURN, 23 EXIT, 7 RUNERROR, 1 BREAK, 1 STALLED**. **Starts 341/341 (100%)**; **conditional outcomes 32/32** (both successors of all 16 conditional branches were taken). 5,073 far calls and 555 rolls were composed.

Cases: all three blocks present; class bytes 0..0Fh and 20h, 7Fh, 80h, FEh, FFh; block 2 sizes 0, 1, 8, 9, 10, 17, 18, 19, 27 and 300 (cell boundaries at 9) and block 3 sizes 0, 1, 62, 63, 64, 126, 127, 128 and 400 (boundaries at 3Fh); each of
blocks 2 and 3 with its file absent and with its id missing; record sizes 1, 2Eh, 2Fh, 11Ch, 11Dh, 11Eh and 200h; digits 0, 1, 9, 10, 99, 100 and 255; block 1 absent in five ways (id missing, file absent, absent with the flag 0 and prompt then ctrl-C, never appearing, the file appearing after a prompt); the message path in game display modes 0..3, three page pairs, plain, extended
and type-ahead keys, ctrl-S and ctrl-D hotkeys, ctrl-C with and without `STING`, and Ctrl-Break; the 050F stall archive; twelve heap sizes from 2 to 200h paragraphs; the record aliasing the heap, the heap middle and the live stack frames; the record offset wrapping at 64 KiB; SP at its lower bound, upper bound and odd; SS = DS; the six clean and five dirty flag
images on the full path and the dirty five on the message path; four seeds; 20 random registers/flags/sizes/ids/seeds.

### Claim-to-check

| Claim | Check |
| --- | --- |
| the C model equals the original closure | the Unicorn differential above (CPU14, 1 MiB, EGA, file state and logs, stop) |
| the body hands each callee the frame the listing says, in the order the listing says | independent Python statement of the call sequence and of every argument word (Str, both name loads, both concatenations, the 0716 frame with P, U, id, extension and name pointers, Move's count and pointers, GetMem sizes, FreeMem's size and pointer, the roll's count and sides, the message's pointer and colour), compared at each callee's entry as its own bytes run: every case except the stall and the three aliasing cases; the sequence is a prefix of the statement on every non-returning case |
| the record, the class byte, the five rolls and the chains are right | independent statement from the case alone (record bytes, the class rule, the LCG and stored byte, a chain walk of exactly ceil(size/unit) cells ending in nil with the first bytes of each cell equal to the block's), checked on every returning case with a full record |
| `DS:[557C]` is 0 on return and the seed advanced by exactly five draws | independent check on every returning case |
| the model's stops are the original's | stop compared per case; C-only stops are asserted in the C test |
| the checker detects a wrong model | 14 deliberate mutations of the C model (wrong class value, a field cleared at the wrong offset, a roll loop one too long or short, a wrong C4 constant, a wrong link offset, a wrong unit size, a wrong `DS:[557C]` value twice, a wrong Move count, a wrong message colour, a wrong Str length, a missing class 7): **13 of 13 non-equivalent mutants caught**; the fourteenth (dropping the INC byte flags) **survives, and is equivalent**: its flags are overwritten by the roll callee before any stop can observe them |

### Declared environment and substitutions

1. The INT 3Fh overlay-manager dispatch at 00B0:0048 is replaced by a far JMP to unit 0x9E +0DE3, identically in both runs.
2. A shadow copy of `0AF8:[11C0,1250)` (see the finding above), identically in both runs.
3. The environment services above are scripted or modelled, not real; every unlisted access is an environment refusal (counted, zero).
4. The EGA is an independent Python device, not a real one; the BIOS and DOS are the scripted services.
5. The three name constants and the message literal are read from the bytes; no game text is in the repository.

### Native corroboration (partial; packaged DOSBox-X, not the pinned build)

DOSBox-X normal core, 386, **33 synthetic cases that end in RETURN or RUNERROR** (all three blocks, class bytes, chain sizes at and around the cell boundaries, absent and id-missing blocks, two record sizes, two heap sizes, two seeds, and the flag images 00D7, 08D7, 0402, 0893 and 0003; the images with IF set are Unicorn only). The three archives of each case are **real files** in the mounted directory, named by what the code builds (a one-letter `DS:[547C]` prefix + the first block constant + the case digit + each suffix constant + the extension), so
**every file operation is real DOS**. All initial memory the case needs is written to STATE.BIN and read back into place by real INT 21h reads; the entry frame's return words are written by the oracle program with its own return address; RunError's entry is replaced by a capture jump (the boundary the C model stops at). Compared with the C model on the identical state:
all 14 CPU words and **1,234,918 bytes** (the heap-variable block with the RNG seed, `DS:[557C]`, the 32 KiB heap area, the 240h-byte record, and the 1,020h-byte stack window, minus the 24 bytes real DOS uses below each INT 21h's SP). **Result: 31 RETURN and 2 RUNERROR, 0 mismatches.**
**Not native:** the message path (EGA, ReadKey, QUIT), the prompts, the keyboard, STALLED, the aliasing and SP/SS variants, flags with IF set, the OUT events. The native run used the host's **packaged `/usr/bin/dosbox-x` 2026.01.02** (SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`), **not the pinned `5a9c7c53` build**. STATE.BIN SHA-256 `1e43838a5851cf64a6f98fa0bd6c61cc82b8ac8ccd85a37e6198111f6a2aa465`; RESULT.BIN SHA-256
`1c99efd5dc162083d6ee83e97d2fa3eaa11bad67c60d2b7e6b657bd955505141`; IMAGE.BIN SHA-256 `02bac6e8cd3c3e6450f1f67d87ebc450bdaaf506261fcfca6537082e1507b2fe`. Frames are synthetic; this does not establish natural reachability.

## Not established

- Real BIOS, DOS or EGA behaviour beyond the 33 native cases; the message path natively.
- The overlay manager's INT 3Fh dispatch and stub construction, and that unit 0x9E is resident at every real call.
- A DIVIDE stop: no case reaches it.
- The entry-SP bound is the measured minimum over ten scenarios; a path outside them could need more, and below it the model may stop CALLEE.
- Interrupts, faults, self-modification, arbitrary-IP resume.
- Identity, credit and the membership of any cluster body: the decisions are the reviewer's.
- The prior-session memory tool (`deja`) was unavailable; nothing was reused from earlier sessions. The 0716-composed and 0BF5 audits and native harnesses were reused as templates, and every figure above was measured on this change.

## Reproduction

    python3 tools/overlay/ovl_006f_0ec4_cpu_audit.py --unicorn --output resources/0ec4-evidence.json
    python3 tools/overlay/ovl_006f_0ec4_cpu_audit.py --native --output resources/0ec4-native.json
    python3 tools/overlay/ovl_006f_0ec4_cpu_audit.py --mutate --output resources/0ec4-mutation.json
    python3 tools/overlay/ovl_006f_0ec4_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --all --require-current-main
    cmake -S . -B resources/build0ec4 -DCMAKE_BUILD_TYPE=Debug
    cmake --build resources/build0ec4 --target poolrad_overlay_u6f_entry6_0ec4_cpu_tests
    ctest --test-dir resources/build0ec4 -R '^poolrad_overlay_u6f_entry6_0ec4_cpu_tests$' --output-on-failure

Use the audit Python environment (Capstone, Unicorn). Original bytes, generated assembly/COM and capture receipts stay under ignored `resources/`.

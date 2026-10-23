# OVL_007F +0A88: translated, evidenced capped money transfer from the DS:67F4 pool, composed over +0054, +003A, StrLoad and resident 1693 (strict body proposed; credit is par-e's)

This raw CPU body, `OVL_007F_0A88` (unit 0x7F, offset 0A88h, no entry slot), takes an index byte I, a word AMOUNT and a far record pointer. It asks [`OVL_007F_0054`](OVL_007F_0054_CPU.md) whether adding AMOUNT to the record's word at `+0102h` would pass the class limit V (the `+0000` value). If it would, the body **shows a 10-byte message through resident `00BA:1693`** and returns with nothing else changed. Otherwise it **clamps AMOUNT to the 32-bit pool at `DS:[67F4h+4I]`**, subtracts it from the pool, adds it to the record's word at `+88h+2I` and calls [`+003A`](U7F_RECORD_WORD_ADD_003A.md), which adds it to `+0102h`.

This change adds the model, its C test, its checker, the native harness for the no-display branch, two census rows and this document. It extends the shared unit-0x7F world (`ovl_007f_world1693.py`) with this span and with the declared AF-after-SHL split. It also carries the stranded PM nit commit `47153cb4` (flag_census `carry_from` dead imports), cherry-picked as described in the PR. **par-e decides credit.**

## Claim table

| Item | Value |
| --- | --- |
| Row | `OVL_007F_0A88` (unit 0x7F +0A88), queue class **game_evidenced**, grade B, composable, priced at 267 B / 1 row |
| Executable extent | **[0A88,0B5A), 210 bytes / 87 instructions**, ending in the only return `RETF 8` at 0B57. The queue's 267-byte bound runs on into the unit's string data at 0B5A; those 57 bytes are not code and are not modelled |
| Model | `src/recovered/overlay_u7f_0a88_cpu.c`, `include/poolrad/recovered/overlay_u7f_0a88_cpu.h` |
| C test | `tests/recovered/test_overlay_u7f_0a88_cpu.c` (CTest `poolrad_overlay_u7f_0a88_cpu_tests`, asserts on) |
| Checker | `tools/overlay/ovl_007f_0a88_cpu_audit.py`, `CHECKER_ROWS = ('OVL_007F_0A88',)`, `--compile-only` prints `CHECKER_BUILD_OK` |
| Native | `tools/overlay/ovl_007f_0a88_native.py` (not-above branch; packaged DOSBox-X) |
| Census | two new helper rows, `overlay_u7f_0a88_cpu.c:arith16` and `:shl1`, classified (below); `flag_census.py --check`: 0 gaps |
| Callees in the composed closure | `+0054`: **its merged C model** (#1421: evidence only, uncredited; the queue calls it `modeled`; par-a's U7F_IDENTITY_ARGUMENTS gives it factor 1 and factor 2 PASS), over `+0000` (merged model) and the resident 13AB/11EA closure; `+003A` (strict, `strict_full_c_addresses.csv`); SYSTEM StrLoad `0AF8:0B0B` (runtime-covered model); resident `00BA:1693` (`resident_covered`, its own audit) |
| Identity | two-factor argument below; par-a writes the U7F entry |

**Role line:** moves up to AMOUNT of money kind I from the 32-bit pool `DS:[67F4h+4I]` into the active character's money word at `+88h+2I` (and its `+0102h` total through `+003A`), clamped to what the pool holds. When `+0054` says the character's `+0102h` word would pass the class threshold, it is refused with a message instead.

## Identity (two factors)

**Factor 1: one caller, in game_evidenced code, inside the caller's extent.** The only site is a near `PUSH CS; CALL 0A88` at `+0F25/+0F26` in `OVL_007F_0CEB`, which is `game_evidenced`, grade A, with extent [0CEB,0F7E). That makes it a near site inside the caller's extent. There is no fixup-backed site, and 0A88 is not one of the unit's entry slots. The callers were found by scanning the unit for `E8` displacements that reach 0A88. The caller pushes, at `+0F15..+0F24`:
- the record `[5D94]:[5D92]`, the active-character pointer ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md));
- the amount `[BP-116h]`, the value `+0294` returned at `+0F0E`, whose maximum the caller pushed as `DS:[67F4h+4i]` at `+0F00` (see [U7F_NUMBER_0294_CPU.md](U7F_NUMBER_0294_CPU.md));
- the index byte `[BP-2]`.

**Factor 2: the body USES game state, not just forwards it.**
- It reads the pool dword `DS:[67F4h+4I]`/`[67F6h+4I]`, which [U7F_RECORD_SUBTRACT_0020](U7F_RECORD_SUBTRACT_0020.md) documents as the dwords the record's money words are added into.
- It **compares** the pool, signed 32-bit, with AMOUNT (`CMP DX,BX; JG; JL; CMP AX,CX; JBE` at 0AD2..0ADA), **clamps** AMOUNT to it (0AE6/0AEA) and **writes back** pool − AMOUNT (0B1A/0B1E).
- It read-modify-writes the active character's word `ES:[record+88h+2I]` (0B2D/0B42), one of the record's money words at +88h, +8Ah … that U7F_NUMBER_0294's factor 2 cites.

**Limits:**
- the meaning of the seven money kinds is not named here;
- the 10-byte message is not reproduced;
- natural reachability of the call (a live route into `0CEB`) was not measured.

## Media and executable extent

START.EXE SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd; GAME.OVR SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

Body SHA-256 `3e0eb89c3eb4beb862487bf9646c68f61bebcb70b203ce4578379c289759070a` (210 bytes). The checker asserts:
- the hash and the 87 instruction starts;
- **four call sites**: `0A9D CALL 0054`, `0AAE LCALL 0AF8:0B0B`, `0AB3 LCALL 00BA:1693`, `0B51 CALL 003A`;
- the jumps `JE 0ABB`, `JMP 0B54`, `JG`/`JL`/`JBE` (the pool compare), and the single `RETF 8`;
- **exactly two relocations in the span**, the segment words at 0AB1 and 0AB6. The model reads both live.

The 10-byte message at `CS:0A7D` (length byte plus text, no relocation) is plain data. The model never reads it; StrLoad does, in both runs. **The game text is not reproduced here.**

## What the body does

Frame after the call: `[SP]` IP, `[SP+2]` CS, `[SP+4]` the index word (only its low byte is read, sign-extended by `CBW`), `[SP+6]` AMOUNT, `[SP+8]`/`[SP+10]` the record pointer. `RETF 8`. Locals: the message temporary at `[BP-0Dh..BP-3]` and the excess word at `[BP-2]`, which `+0054` writes and this body never reads.

1. `PUSH BP; MOV BP,SP; SUB SP,0Eh`. `+0054(record, extra = AMOUNT, out = &[BP-2])`; `OR AL,AL; JE 0ABB`.
2. **Above** (AL nonzero): `StrLoad(&[BP-0Dh], CS:0A7D)`; `LCALL 00BA:1693` shows it (clear row, print, delay, clear row); `JMP 0B54`. The pool and the record are untouched.
3. **Not above**:
   - P = the dword at `DS:[67F4h+4I]`.
   - If `(int32)P < AMOUNT`, then `[BP+8] := low word of P`. The compare is `XOR DX,DX; CMP DX,BX; JG clamp; JL keep; CMP AX,CX; JBE keep`. A negative pool clamps to its low word.
   - `P := P − AMOUNT` (`SUB/SBB`).
   - `[record+88h+2I] += AMOUNT`, through `LES DI,[BP+0Ah]; ADD DI,2I`.
   - `+003A(record, AMOUNT)`.
4. `MOV SP,BP; POP BP; RETF 8`.

The index is scaled four times with `SHL DI,1; SHL DI,1` and twice with `SHL AX,1`. These are the body's only shifts, and the source of the declared AF split below.

## Raw contract and stops

`pr_u7f_0a88_cpu(bus, size, cpu, budget, limits, diagnostic)`:
- exactly 1 MiB of A20-off RAM behind a `pr_bus`; the above branch needs an attached EGA and a ports environment for the 1693 closure;
- IP 0A88;
- **entry SP in [0200h,FFF4h]**. The upper bound keeps the 12-byte entry frame below FFFFh. The lower bound is +0203's, and this body's frame (0Eh) is shallower than 0203's (2Ah) at every callee entry.

Budget:
- each instruction of this body, of `+0054`, `+0000` and the 13AB closure consumes one unit;
- the instructions of StrLoad, `+003A` and the 1693 closure do not. Their merged models have no budget; 1693 has its own loop-back limits, passed through `limits`;
- LIMIT leaves the next unexecuted counted instruction in `cpu->cs:ip`.

| Stop | Class | Origin | Pinned by |
| --- | --- | --- | --- |
| RETURNED | RETURN | `RETF 8` to the entry frame's return address | Unicorn: 382 of 732 cases; native (not-above branch only): 37 of 37 |
| LIMIT | LIMIT (host cap) | the budget ends at the next unexecuted counted instruction | Unicorn: 343 cases, equal CPU, RAM, EGA, OUT events and VRAM operands at the same arrival |
| TRANSFER | control transfer | the body's own store through the record reached its return address, or a callee returned elsewhere | Unicorn: 7 cases, all in the `money-at-bp*` / `g102-at-bp*` alias families, the oracle stopping at the same garbage address |
| ARGUMENT | ARGUMENT | null/size/IP or entry SP outside [0200,FFF4] | C test (ASan/UBSan): CPU, RAM and budget untouched |
| CODE | ARGUMENT-class | a call site is not the original; IP at the call, before its return frame | C test (0A9D, 0B51, 0AAE); 0 in the differential |
| CALLEE | non-architectural | a callee stopped otherwise | C test (a corrupted far call inside `+0000`); 0 in the differential |

The 1693 closure's reachable stops through this body's inputs follow +0203's argument ([OVL_007F_0203_CPU.md](OVL_007F_0203_CPU.md), PM condition i21437), with one change: the message is the **fixed 10-byte literal at `CS:0A7D`**, below StrStore's 40-byte cap.
- AMOUNT, I and the record decide only whether the call happens.
- **In the above branch the body makes no store through caller memory at all**, so it cannot rewrite 1693's frame.
- The 1693 LIMIT paths are reachable only through the display globals. They are exercised here across 4 modes, 3 pages and 3 delay settings, with generous limits.

## Pointer census

| Pointer | Source | The body's own access | Bus? | Also dereferenced by |
| --- | --- | --- | --- | --- |
| the record (argument `[BP+0Ah]`/`[BP+0Ch]`) | the caller (`[5D92]` in 0CEB) | **`[record+88h+2I]` read, added to and written** (not-above only) | **yes, through the bus** (both the read at 0B2D and the write at 0B42) | `+0054`: `[+102h]` through the bus, `+10h`/`+16h` flat via +0000/13AB; `+003A`: `[+102h]` flat RMW |
| the pool `DS:[67F4h+4I]` | a DS global table, signed index | flat read (three times), flat write of both words | no, flat DS | none |
| the message | code, `CS:0A7D` | none | no | StrLoad (flat) |
| the frame, the message temporary, `[BP-2]` | created here / caller | flat SS | no | StrLoad writes the temporary; 1693 reads it; `+0054` writes `[BP-2]` |

A record aimed into the A0000..AFFFF aperture splits across three conventions: this body and `+0054` go through the bus, while `+0000`/13AB and `+003A` read flat. That is the declared contract limit, shared with `+0054` and `+0203`. The C test pins the bus path with an EGA attached:
- record in the aperture;
- plane 0 holding `34h`/`12h` at `+8Ch`/`+8Dh`;
- the not-above branch writes `39h`/`12h` to **all four planes** and leaves the RAM under the aperture untouched.

## Evidence

**Original-byte Unicorn differential on the composed closure.**
- Every body runs as the original bytes at its own segment.
- The relocated segment words are installed from the unit's fixup table.
- The 1693 world is the 1693 audit's, with the independent Python EGA.

Compared after every case, **with no masks**: CPU14 (dirty flag words in the set), **all 1,048,576 RAM bytes**, the EGA planes, latches and registers, the ordered OUT events, the ordered VRAM operands, the stop and the remaining budget.

**732 cases, 0 mismatches.**
- Outcomes: **382 RETURNED, 343 LIMIT, 7 TRANSFER**; 0 ARGUMENT, CODE or CALLEE stops.
- **Starts: 87/87 in this body** (1,159 across the composed closure).
- **241 returning cases carry the independent statement.**
- Split arrivals, counted and reported: **2** under the pinned-MULW split and **34** under the AF-after-SHL split. There are no masks.
- 268.9 s on the heavy wrapper.

Cases:
- `+0054`'s boundary: five class bytes x four values of W x {V−1, V, V+1, V+8000h};
- the pool compare: five high words (0, 1, 7FFFh, 8000h, FFFFh) x five low words around AMOUNT = 5, so JG, JL and both JBE outcomes are taken, plus AMOUNT 0/1/FFh against pools at, and one below, AMOUNT;
- the index: 0..7, 7Fh, 80h, FFh, FEh, 40h, each also with garbage in the index word's high byte;
- money values 0000h/7FFFh/FFFEh/FFFFh;
- the display world: four modes, three pages, three delay settings;
- record placement: offsets that wrap, segments FFFFh/FFF0h/0;
- six entry SPs and ten flag images, each on both branches;
- frame aliases: the money word on each byte from BP−12h to BP+0Fh, and W the same on both branches;
- the money word over the pool dword itself (4 cases);
- three budget sweeps: above 0..129 plus 150/200/260; not-above with clamp 0..199; not-above without clamp 150..199;
- 40 random cases.

**Declared splits (shared helper `tools/overlay/pinned_shift_flags.py`, never a mask):**
1. **pinned MULW**, inside `+0000`'s 13AB closure, as in +0203;
2. **AF after SHL**. The model's `shl1` sets AF as the pinned core does (`get_AF` = count & 1Fh, `flags.cpp:172-177`, stored `763-771`); raw Unicorn leaves it clear. When the original's last FLAGS writer is a non-zero shift (a LIMIT arrival right after one of the index-scaling SHLs), the C flags must equal Unicorn's with AF set. Otherwise they are compared exactly.

The shared world now counts these arrivals separately (`SHIFT_SPLIT_ARRIVALS`). On every completed call the last FLAGS writer is `+003A`'s ADD or the 1693 closure, both compared exactly.

### Claim-to-check

| Claim | Check |
| --- | --- |
| the C model equals the original closure | the Unicorn differential (CPU14, 1 MiB, EGA, OUT events, VRAM operands, stop, budget) |
| the span and the callees are the original bytes | `source()` asserts the SHA-256 and decode of this body, the four call sites, the jump set, the two relocations, and every callee span |
| the branch, the clamp, the pool, the money word, W and the message temporary are as the listing says | **independent statement** computed from V (the original `+0000` alone), W, AMOUNT, I, the pool and the initial words, on the returning cases whose record does not overlap the stack, the pool or the world's data and does not straddle FFFFh |
| SP (+12), BP and CS:IP are restored | the same independent statement |
| the money RMW goes through the bus | the C test with an EGA attached; the differential cannot see it |
| the index is a signed byte | `index-80/ff/fe` cases plus the C test (I = FFh selects 67F0h and +86h) |
| the stops are the original's | stop compared per case; C-only stops asserted in the C test |
| the checker detects a wrong model | **22 deliberate mutations: 20 caught (19 by the differential, 1 by the C test) and 2 equivalent, each with its proof** |
| the census covers the new helpers | `flag_census.py --check`: 0 without a row, 0 unclassified |
| the checker builds on current main | `CHECKER_ROWS` registered; `--compile-only` prints `CHECKER_BUILD_OK`; `check_checker_builds.py --require-current-main` |

### Mutation

**22 mutants: 20 caught, 19 by the Unicorn differential and 1 by the C test; 2 equivalent.** The run is on the 46-case mutation set (`MUTANT_CASES`).

**Caught by the differential (19):**
- JE inverted;
- the message at 0A7Eh (the string then loaded overruns the 10-byte temporary: TRANSFER);
- the money word at +8Ah;
- **JG as unsigned above** (`pool-8000-0000`);
- **JBE as JB** (`pool-0000-0005`);
- **SBB without the borrow** (`pool-0001-0000`);
- the pool's high word stored at 67F4;
- **the index zero-extended instead of CBW** (`index-80`);
- the index byte read from `[BP+7]`;
- the clamp keeping AMOUNT (`pool-0000-0004`);
- `+003A` given the index word;
- RETF popping 6;
- `SUB SP,10h`;
- the excess local at BP-4;
- **the return-address (TRANSFER) check dropped** (`money-at-bp+02`);
- the `+0054` call-site displacement;
- the `+0054` and the 1693 calls not charged to the budget;
- **SHL leaving AF clear**, caught by the declared split itself at `budget-not-150`: the split demands AF set after a non-zero shift, so it is a check, not a mask.

**Caught by the C test (1).** The money read bypassing the bus survives the differential, as predicted, because no differential case puts the record in the aperture. It is caught by the C test's EGA case.

**Equivalent (2), with proofs:**
- **SHL without OF.** DI and AX are `CBW` of a byte, so they lie in [FF80h,007Fh]. After one doubling they lie in [FF00h,00FEh]. In both ranges bits 15 and 14 are equal before each of the ten SHLs (0AC1, 0AC3, 0AE2, 0AE4, 0AFC, 0AFE, 0B16, 0B18 on DI; 0B26, 0B3B on AX), so OF = bit15(result) ^ CF = 0 every time. A model that never sets OF agrees on every input.
- **`OR AL,AL` testing 16 bits.** This is +0203's proof: AL is the 0/1 byte `+0054` returns, so SF is always clear.

The first mutation run had two defects of its own, fixed before this run:
- "the index read as a word" was itself equivalent, because `CBW` overwrites AH. It was replaced by the `[BP+7]` mutant.
- The bus mutant did not compile (an unused helper under `-Werror`). A compile error is not counted as a catch.

### Census rows (FLAG_SEMANTICS_CENSUS.tsv)

| Helper | Rule | Verdict |
| --- | --- | --- |
| `shl1` (SHL DI,1 / SHL AX,1) | CF = old bit 15, OF = result bit 15 ^ CF, **AF = 1**, SZP | **agree** with SHLw (`flags.cpp:719-727`), same as the 00A8/28BE row |
| `arith16` (CMP, SUB/SBB of the pool, ADD DI,AX, ADD AX,[BP+8], SUB SP) | standard word arithmetic, PF from the low byte | **disagree on PF only**, the established stored-vs-lazy PF path split; the body has no JP/JNP, and its branches after `arith16` are JG/JL/JBE |

`logic` (OR AL,AL / XOR DX,DX) hashes to +0203's existing row.

### Native corroboration (partial; packaged DOSBox-X, not the pinned build)

DOSBox-X normal core, 386, **37 synthetic cases, all on the NOT-ABOVE branch**. `+0A88`, `+0054`, `+0000`, the 13AB/11EA closure and `+003A` run as original bytes under a real DOS. The branch that shows a message through 1693, StrLoad and the EGA is **not** run natively.

Cases:
- the pool compare: high words 0/1/8000h/FFFFh x low words 0/4/5/6 against AMOUNT 5, so JG, JL and both JBE outcomes are taken;
- indices 0, 1, 3, 6, 7Fh, 80h, FFh, plus garbage in the index word's high byte;
- money 0000h/7FFFh/FFFFh;
- five flag images;
- SP at both bounds and an odd SP;
- W at the class edge;
- a record at offset FF00h.

Against the C model on identical state, all 14 CPU words and **12,125,196 bytes** (five 64 KiB regions per case plus the CPU words) match: **0 mismatches, no masks, 0 skipped.** IF is cleared in every native case.

The host's **packaged `/usr/bin/dosbox-x`** (SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`) was used, **not the pinned `5a9c7c53` build**. Frames are synthetic, so this does not establish natural reachability.

### Declared environment

1. No stubs stand in for original behaviour: there is no INT 3Fh dispatch and no environment service. The whole composition runs as original bytes in the oracle.
2. The 1693 closure runs with generous loop-back limits and no refused OUT; its own LIMIT, refusal and CODE stops stay with its own audit.
3. The shared world gains this span, the `CS:0A7D` constant, the two relocation words, this model's source, and the shift split. The **+0203 audit was re-run on the changed world**: `--unicorn --quick` gives **758 cases, 0 mismatches, 53/53 starts**, the same 2 pinned-MULW split states, 444 RETURNED / 302 LIMIT / 12 TRANSFER. +0203 has no SHL, so the new shift branch never fires there.
4. TF is excluded from the random flag words; FLAGS bit 1 is always set.

## Not established

- **The above branch natively.** As for +0203, only the not-above branch ran under DOSBox-X. Composing the 1693 native machine is the same one-lane-day gap.
- A record in the A0000..AFFFF aperture, in the differential or natively; the bus claim rests on the C test.
- The 1693 closure's own stops inside this composition.
- Real BIOS, DOS or EGA behaviour beyond the native cases; the pinned DOSBox-X build.
- Natural reachability: the caller is real, but no live route was measured.
- Credit: par-e's.

## Reproduction

    python3 tools/overlay/ovl_007f_0a88_cpu_audit.py --unicorn --output resources/7f0a88-evidence.json
    python3 tools/overlay/ovl_007f_0a88_cpu_audit.py --mutate --output resources/7f0a88-mutation.json
    python3 tools/overlay/ovl_007f_0a88_cpu_audit.py --native --output resources/7f0a88-native.json
    python3 tools/overlay/ovl_007f_0a88_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --require-current-main OVL_007F_0A88
    cmake -S . -B resources/build7f0a88 -DCMAKE_BUILD_TYPE=
    cmake --build resources/build7f0a88 --target poolrad_overlay_u7f_0a88_cpu_tests
    ctest --test-dir resources/build7f0a88 -R '^poolrad_overlay_u7f_0a88_cpu_tests$' --output-on-failure

Use the audit Python environment (Capstone, Unicorn). Original bytes, generated assembly/COM and capture receipts stay under ignored `resources/`.

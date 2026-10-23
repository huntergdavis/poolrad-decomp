# OVL_007F +044C: translated, evidenced capped transfer between two records with a message, composed over +0000, +0020, +003A, StrLoad and resident 1693 (evidence only; no credit proposed)

This raw CPU body, `OVL_007F_044C` (unit 0x7F, offset 044Ch, no entry slot), takes a byte INDEX, a word EXTRA, and two **far pointers to far pointers** to records: PP (destination P) and QQ (source Q). It asks [`OVL_007F_0000`](OVL_007F_0000_CPU.md) for P's class value V and reads P's word W at `+0102h`.
If W + EXTRA (16-bit) is **above** V it **shows a message through resident `00BA:1693`** (StrLoad of a ten-byte literal at `CS:0441`, then the display) and returns, changing no record word. Otherwise it **subtracts** EXTRA from Q's array word at `+88h + 2*sext(INDEX)`, calls `+0020(*QQ, EXTRA)`,
**adds** EXTRA to P's array word at `+88h + 2*sext(INDEX)` and calls `+003A(*QQ, EXTRA)`. This change adds the model, its checker, its native harness, a C test and this document, on the shared world harness of [`OVL_007F_0203_CPU.md`](OVL_007F_0203_CPU.md) (#1440). It proposes **no strict row, no identity row and no manifest row**; par-e accepts or refuses.

## Media and executable extent

START.EXE SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd; GAME.OVR SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

The decoded span is **unit 0x7F [044C,0506), 188 bytes / 71 instructions**, ending in the only return, `RETF 000Ch` at 0505. Body SHA-256 `1f9525dc814d360d5e1938b7d7fcaf1782ad65128922575603d402f4cad70a06`. The checker asserts the hash and the 71 starts, **three near calls (`045D CALL 0000`, `04AD CALL 0020`, `04E9 CALL 003A`)**, **two far calls (`04F8 LCALL 0AF8:0B0B` StrLoad, `04FD LCALL 00BA:1693`)**,
the `JA 04EE` at 0472, the single `RETF 000Ch`, and **exactly two relocations in the span, the segment words at 04FB and 0500**; the model reads both live. The ten-byte message at `CS:0441` is plain data; the model never reads it (StrLoad does, in both runs) and **the game text is not reproduced here**.

The composed closure is this body (71) + `+0000` (12) + resident 13AB (89) and 11EA (62) + `+0020` + `+003A` + StrLoad (12) + the 1693 closure (the 1693 audit's pinned bodies). The checker also pins the `+0020`, `+003A` and `0203` spans of the shared world by SHA-256.

## What the body does

Frame after the call: `[SP]` IP, `[SP+2]` CS, `[SP+4]` INDEX (a byte in a word slot), `[SP+6]` EXTRA, `[SP+8]/[SP+10]` PP, `[SP+12]/[SP+14]` QQ. `RETF 000Ch`. Locals: the string temporary at `[BP-0Bh..]`.

1. `PUSH BP; MOV BP,SP; SUB SP,0Ch`. `LES DI,[BP+0A]`, push `ES:[DI+2]`, `ES:[DI]` and CS: **`+0000(*PP)`**, whose result is V (AX), kept in DX.
2. `LES DI,[BP+0A]; LES DI,ES:[DI]; MOV AX,ES:[DI+0102]; ADD AX,[BP+8]; CMP AX,DX; JA 04EE`. (The compare uses the 16-bit sum, so a sum that wraps compares small.)
3. **Not above**: `AL := INDEX; CBW; SHL AX,1`, then through `QQ`: `ES:[DI+88h+AX] -= EXTRA`; `+0020(*QQ, EXTRA)`; through `PP`: `ES:[DI+88h+AX] += EXTRA`; `+003A(*QQ, EXTRA)`. Each pointer chain is re-read from the stack before every use.
4. **Above**: `LEA DI,[BP-0Bh]; StrLoad(DI, CS:0441); LCALL 00BA:1693`.
5. `MOV SP,BP; POP BP; RETF 000Ch`.

## Role (identity is not claimed)

The role is a **capped transfer of an amount between two records' array slots, with an overflow message**: the destination is filled only while its `+0102h` word plus the amount does not exceed its class limit, the source is debited, and the two records' bookkeeping callees run. Measured ownership: **42 fixup-backed incoming call sites across 11 units** (0077: 11, 002F: 6, 005E: 5, 006C: 5, 007F: 4, 006A: 3, 0023: 2, 0031: 2, 0038: 2, 001E: 1, 0086: 1), recorded by the checker.
This note makes no claim about identity or about what the array words, `+0102h`, `+0020` and `+003A` mean: the two-factor test is the reviewer's.

## Membership chain

Facts only; nothing here admits a member. The row's callees are `+0000` (modelled), `+0020` and `+003A` (strict, merged), StrLoad, resident 1693 (`resident_covered`) and covered SYSTEM runtime. **This document grants and requests nothing for any row.**

## Raw contract and stops

`pr_u7f_044c_cpu(bus, size, cpu, budget, limits, diagnostic)`: exactly 1 MiB of A20-off RAM behind a `pr_bus` **with an attached EGA and a ports environment** (the 1693 closure needs both), IP 044C, **entry SP in [0200h, FFEEh]**.
Each instruction of this body, of `+0000` and the 13AB closure consumes one unit of the budget; **the instructions of `+0020`, `+003A`, StrLoad and the 1693 closure do not** (their merged models have no budget; 1693 has its own loop-back limits, passed through `limits`). **LIMIT leaves the next unexecuted counted instruction in `cpu->cs:ip`.**

| Stop | Class | Origin | Pinned by |
| --- | --- | --- | --- |
| RETURNED | RETURN | `RETF 000Ch` to the entry frame's return address | Unicorn: 614 of 880 cases; native (transfer branch only): 39 of 39 |
| LIMIT | LIMIT (host cap) | the budget ends at the next unexecuted counted instruction, and alias cases whose stores rewrote the body's own state and did not finish within 30,000 counted instructions | Unicorn: 259 cases, equal CPU, RAM, EGA, OUT events and VRAM operands at the same arrival |
| TRANSFER | control transfer | **the body's own stores through a record reached its return address** (the array word at `BP+2` or `BP+4`), or a callee returned elsewhere; CPU left at the popped CS:IP | Unicorn: 4 cases (`parr`/`qarr` at `bp+02` and `bp+04`, not-above), the oracle stopping at the same address; the mutant that drops the check is caught by `parr-at-bp+02-not` |
| ARGUMENT | ARGUMENT | null/size/IP or entry SP outside [0200,FFEE] | C test (ASan/UBSan): CPU, RAM and budget untouched |
| CODE | ARGUMENT-class | a call site (opcode, displacement or offset) is not the original; IP at the call, before its return frame | C test (a corrupted call site on the transfer path and on the message branch); **0 in the differential** |
| CALLEE | non-architectural | a callee stopped otherwise | C test; **0 in the differential** |
| outside the contract | precondition | **the original's own stores reached resident code** (`qcell-at-bp-2e-not`, `-20-not`, `-14-not`: the Q pointer cell placed on the callee frames the `+0000`/13AB chain writes), so the original goes on executing modified bytes | counted and named; not compared after the write (see below) |

The final run has **0 ARGUMENT, CODE or CALLEE stops**, counted per case, not asserted.

**Outside the contract, 3 cases.** `code_immutable` is a precondition of every model in this family: the model runs the literal original instructions, the original runs whatever bytes are in RAM. In these three cases the Q pointer cell sits where the `+0000` / 13AB call chain writes its frames, so the **original overwrote its own code** (`code written (w 044C)`) and then diverged (it took the message branch where the model took the transfer branch). The harness
classifies a case this way **only after a failed comparison and only when the original's final RAM shows changed code in a pinned span**; every other failure still raises. The 3 cases are reported, not hidden, and 877 cases are compared unmasked.

### Stops reachable through the 1693 closure (PM condition, i21437)

1693's own caps and refusals are its audit's; **whether 044C can reach them is argued here from the bytes.** 044C's only input to 1693 is the **fixed ten-byte literal at `CS:0441`** (StrLoad copies it to `[BP-0Bh]`, 1693 is called with that pointer) plus the display globals (mode, active page, delay factor and count). **No 044C argument (INDEX, EXTRA, PP, QQ, any record word) reaches the message length, the rectangle or the delay**; the only way into 1693 is the JA, whose condition the arguments decide.
The inherited stops are therefore reachable only through the display globals, which the differential sweeps: **4 modes, 3 active pages, 3 delay settings on the above branch (`mode*-above`, `page*-above`, `delay-*-above`), and 183 cases whose names end `-above`** reach the 1693 closure. **Inherited stops observed: 0** (no CALLEE, CODE or refusal); the closure's own LIMIT/refusal paths are not exercised here, and not reachable through 044C's arguments.

## Pointer census

Every pointer the body dereferences that it did not create, and whether the access goes through the bus (`pr_bus`: with an EGA attached, a byte access to A0000..AFFFF goes to the planes):

| Pointer | Source | The body's own access | Bus? | Also dereferenced by |
| --- | --- | --- | --- | --- |
| PP (`[BP+8]/[BP+10]`), QQ (`[BP+12]/[BP+14]`) | caller's frame | the two words of each are read flat from SS | no, flat SS | none |
| the cell `*PP` (far pointer P) | caller memory | `PUSH ES:[DI+2]`, `PUSH ES:[DI]` (0455/0459), then **`LES DI,ES:[DI]`** before every later use (the four bytes read contiguously) | **yes, through the bus** | `+0000` reads P through the pushed pointer, flat |
| the cell `*QQ` (far pointer Q) | caller memory | `LES DI,ES:[DI]` before each use | **yes, through the bus** | `+0020`, `+003A` (flat) |
| P's word `+0102h` | P | `MOV AX,ES:[DI+0102]` | **yes** | `+0000`/13AB read P's class bytes flat; `+003A` flat RMW |
| P's and Q's array words `+88h+2*sext(INDEX)` | P, Q | **read, add/subtract, write** | **yes** | `+0020`, `+003A` (flat) |
| the string temporary `[BP-0Bh..]`, the entry frame | created here / caller | flat SS | no | StrLoad writes it; 1693 reads it |
| the message `CS:0441`, the two segment words at 04FB/0500 | code | the segment words read live | no | StrLoad (flat) |

**Where the exposure lives:** every access through PP, QQ, `*PP`, `*QQ` and the data words that **this body** makes goes through the bus; the callees (`+0000`, 13AB, `+0020`, `+003A`, StrLoad) keep their own flat contracts. **A record in the aperture therefore splits across conventions**; that is the declared contract limit, shared with `OVL_007F_0054_CPU.md` and `OVL_007F_0203_CPU.md`,
and no differential case places a pointer there.

### What pins the bus claims

The differential's oracle has **no planar EGA behind the unit's own accesses**, so a model that bypasses the bus there cannot be seen by it. The **C test attaches `pr_ega`** and hand-checks plane bytes: the **PP cell in the aperture** (plane 0 holding the pointer to P while the RAM bytes under it are `A5h`; the transfer still happens, and **the two words pushed for `+0000` at `[BP-0Eh]`/`[BP-10h]` are P's, read off the stack after stopping at the seventh instruction**),
and **P's array word in the aperture** (plane 0 holds 0200h at +8Ch; the add writes 0210h to **all four planes** and leaves the RAM bytes untouched). **Three mutants bypass the bus (the array store, the `LES DI,ES:[DI]` cell read, the two pre-`+0000` pushes); all three survive the differential, as predicted, and all three are caught by the C test.**

### Adversarial fixtures (pointer-reached memory placed on memory the body itself creates or reads)

Placed at `BP+k` for `k = -30h..+0Eh` in steps of 2, each on the above and not-above branch (alias families, `ALIAS_BUDGET` 30,000):

- **`pcell-at-bp*`, `qcell-at-bp*` (69 each):** the pointer cells over the body's locals, saved BP, return address and arguments.
- **`parr-at-bp*`, `qarr-at-bp*` (64 each):** the array word (INDEX 2, `+88h+4`) over the same bytes, so the read-modify-write changes the frame (**TRANSFER at `bp+02` and `bp+04`**).
- **`pw-at-bp*` (64):** P's word `+0102h` over the frame.
- **The pointer cell over the return slot itself** (`pcell-at-bp+00/+02`): the entry return address is then whatever the cell holds; the oracle and the model both take **the return address from the entry stack**, not a constant. (My first oracle compared against a constant, so these cases showed `C RETURNED / original TRANSFER`; that was the oracle, and is fixed in the shared harness.)
- Word-straddle placements (`p-off-*`, `q-off-*`, `pcell-off-*`: a word at offset FFFFh): differential only; the independent statement is skipped on them, because the corpus and Unicorn read contiguous and the pinned DOSBox-X wraps.

## Evidence

**Original-byte Unicorn differential on the composed closure, re-run on the tree rebased onto main `328ed22c` (main has since moved only by one unrelated C test file, `test_overlay_u7f_roll_composed_0fd2.c`): 880 cases, 0 mismatches among 877 compared; 3 outside the contract (above).** Every body is the original bytes at its own segment; relocated segment words are installed from the unit's fixup table; the 1693 world is the 1693 audit's, with the independent Python EGA behind A0000..AFFFF. After every compared case, **unmasked** (one declared exception below): CPU14, **all 1,048,576 RAM bytes**, the EGA planes, latches and registers, the ordered OUT events, the ordered VRAM operands, the stop and the remaining budget.

Outcomes: **614 RETURNED, 259 LIMIT, 4 TRANSFER**. **Starts: 71/71 in this body (1,117 across the composed closure).** **Non-architectural stops: 0.**

Cases: boundary cases for three class bytes x four values of W x {V-1, V, V+1, V+8000h} (48), INDEX values 0, 1, 5, 7Fh, 80h, FFh, 41h on both branches (14), five array-word pairs x three amounts (15), three Q-word values, the four display modes, three active pages, three delay settings, record offsets and segments that wrap (18), six entry SPs on both branches (12), ten flag images, the alias families above (330 `*-at-bp*` cases plus the offset and segment placements), **two budget sweeps (0 to 179 on the above branch, 0 to 199 on the not-above branch)** (380), and 40 random cases.
**The independent statement** (172 cases): the branch, the array words, Q's `+0020` effect, SP, BP and CS:IP are recomputed from V (the original `+0000` alone), W, EXTRA, INDEX and the initial words on returning cases whose records do not overlap the stack or the world's data and do not straddle FFFFh.

**The one declared split (pinned MULW), shared with `+0000`, `+0054` and `+0203`:** the C models compute the pinned MULW flags (`instructions.h:605-619`: SF and AF kept, ZF from AX alone, PF over DX:AX, CF=OF=DX!=0); raw Unicorn leaves SF, ZF, AF and PF at its own values. The checker uses the shared `tools/overlay/pinned_shift_flags.py` (`LastFlagWriter`, `declared_flags_split`): whenever the last FLAGS writer in the original is a word MUL the C flags must equal Unicorn's outside the six arithmetic bits and the pinned value inside them; every other state is compared exactly. **2 states of the 880 cases were compared under the split**, counted and reported; there is no mask.
**The AF-after-shift split (shared, declared on main by `+0A88`).** The body's four `SHL AX,1` set AF as the pinned core does (`get_AF` = count & 1Fh); raw Unicorn leaves it clear. The shared harness declares it the same way (the last FLAGS writer is a non-zero shift: the C flags must equal Unicorn's with AF set; everywhere else exact). My first version of this model cleared AF and passed the exact comparison it was written against; **after the rebase onto the shared declaration it failed at `budget-not-103` (arrival 047A), and `shl1` now sets AF**. The run's JSON counts only the MULW states (2); the shift states are counted by the shared harness counter, not reported separately here.

### Claim-to-check

| Claim | Check |
| --- | --- |
| the C model equals the original closure | the Unicorn differential (CPU14, 1 MiB, EGA, OUT events, VRAM operands, stop, budget) on 877 cases |
| the span and the callees are the original bytes | `source()` asserts the SHA-256 and decode of this body, the five call sites, the two relocations, the pinned `+0020`/`+003A`/`0203` spans, and every callee span |
| the branch, the array words and the Q-side effect are as the listing says | independent statement on **172** returning cases |
| SP, BP and CS:IP are restored | independent statement on the same cases |
| every pointer access of the body goes through the bus | **the C test with an EGA attached** (PP cell, pushes for `+0000`, array word); the differential cannot see it |
| the body tolerates pointers into its own frame | the `*-at-bp*` families; TRANSFER at `bp+02`/`bp+04` |
| the return-address check (TRANSFER) is needed | the 4 TRANSFER cases; the mutant that drops it is caught by `parr-at-bp+02-not` |
| the model's stops are the original's | stop compared per case; C-only stops asserted in the C test |
| the model is exact at every instruction | the two budget sweeps (380 cases) compare the CPU at every arrival |
| the checker detects a wrong model | **20 deliberate mutations: 19 caught (16 by the differential, 3 by the C test) and 1 equivalent, with its proof** |
| the checker builds on current main | `CHECKER_ROWS` is registered (`OVL_007F_044C`); `--compile-only` prints `CHECKER_BUILD_OK`; `check_checker_builds.py --require-current-main` |

### Mutation

**20 mutants: 19 caught (16 by the Unicorn differential, 3 by the C test), 1 proven equivalent, 0 survivors.** Caught by the differential: JA inverted, **`SHL` not setting AF (the pinned rule; the budget sweep arrival at 047A)**, the message at 0442h, the source word read at `+8Ah`, the source word getting an ADD, the destination word getting a SUB, INDEX zero-extended, the first SHL skipped, `+0020` given INDEX instead of EXTRA, `+003A` given P instead of Q, RETF popping 0Ah, `SUB SP,0Eh`, **the return-address (TRANSFER) check dropped (`parr-at-bp+02-not`)**, the `+0000` call-site displacement, the `+0000` call not charged to the budget, and `LES DI,ES:[DI]` swapping the pointer words.
**The three bus-bypass mutants survive the differential, as predicted, and are caught by the C test.**
**One mutant is equivalent: `SHL AX,1` leaving OF clear.** Every SHL in the body (0478, 0490, 04B4, 04CC) directly follows a CBW, so its operand is a sign-extended byte, bits 15 and 14 are equal, and OF (= CF xor the new bit 15) is always 0. The checker proves it exhaustively over all 256 INDEX values and reports it as `proven_equivalent`; the proof, not the survival, is the claim.
Two earlier survivors were **missing fixtures, not equivalents**: the TRANSFER-check mutant (the mutation set lacked the case that rewrites the return slot) and the pre-`+0000` pushes mutant (the C test did not look at the pushed words). Both are now pinned; a compile error is not counted as a catch.

### Declared environment and substitutions

1. There are none that stand in for original behaviour: no stub, no INT 3Fh dispatch, no environment service. The whole composition runs as original bytes in the oracle.
2. **Scope of the 1693 closure:** it runs with generous loop-back limits and no refused OUT; its own stops are not exercised, and are argued unreachable through this body's arguments above.
3. The shared module `ovl_007f_world1693.py` (#1440) now takes the entry frame's return address **from the entry stack**; for the ordinary frames of `+0203` the value is unchanged.
4. TF is excluded from the random flag words, and FLAGS bit 1 is always set.

### Native corroboration (partial; packaged DOSBox-X, not the pinned build)

DOSBox-X normal core, 386, **39 synthetic cases, all on the NOT-ABOVE (transfer) branch**: three class bytes x three values of W x {V-1, V}, INDEX 00/01/05/7F/80/FF, four array-word pairs, three Q-word values, five flag images and SP at both bounds and an odd SP. The data, stack and record segments are written to files and loaded by real DOS reads; the oracle program provides the entry frame's return address; **IF is cleared in every native case**.
Compared with the C model on identical state: all 14 CPU words and **12,780,612 bytes**, **0 mismatches, no masks.**
The host's **packaged `/usr/bin/dosbox-x` 2026.01.02** (SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`) was used, **not the pinned `5a9c7c53` build**. **Not native: the above branch** (the display path through StrLoad and 1693 has no native corroboration here; a gap for credit, not a limit), the frame aliases, the budget sweeps, TRANSFER and the aperture. Frames are synthetic; this does not establish natural reachability.

### Which way verdicts move under a convention split

- **Word accesses that straddle FFFFh:** the `*-off-*` placements put a record word at offset FFFFh; the corpus and Unicorn read the second byte at the next physical address, and the pinned DOSBox-X build is recorded as wrapping it. Those placements are the only ones that depend on it. Not run on the pinned build.
- **A pointer in the aperture:** the record would be split across the conventions above; the verdict moves from reproducible to not on a machine whose plane reads differ from flat bytes. No differential case places a pointer there; the C test pins the bus side.

## Not established

- **The above branch natively** (needs the 1693 native harness's EGA machine); only the transfer branch ran under DOSBox-X.
- **A pointer in the A0000..AFFFF aperture, in the differential or natively;** the bus claims rest on the C test.
- **The 1693 closure's own stops** inside this composition (argued unreachable through the arguments, not run).
- **The 3 outside-the-contract cases** are not compared after the original's code write.
- Real BIOS, DOS or EGA behaviour beyond the native cases; the pinned DOSBox-X build.
- Natural reachability: 42 fixup-backed callers exist, but no saved-game or live-route measurement was made.
- Identity, credit and closure membership: the reviewer's.
- The prior-session memory tool (`deja`) was unavailable (it failed to connect with ENOENT); nothing was reused from earlier sessions. The `0203`, `0054` and 1693 audits were the templates, and every figure above was measured on this change.

## Reproduction

    python3 tools/overlay/ovl_007f_044c_cpu_audit.py --unicorn --output resources/7f044c-evidence.json
    python3 tools/overlay/ovl_007f_044c_cpu_audit.py --native --output resources/7f044c-native.json
    python3 tools/overlay/ovl_007f_044c_cpu_audit.py --mutate --output resources/7f044c-mutation.json
    python3 tools/overlay/ovl_007f_044c_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --require-current-main OVL_007F_044C
    cmake -S . -B resources/build7f044c -DCMAKE_BUILD_TYPE=Debug
    cmake --build resources/build7f044c --target poolrad_overlay_u7f_044c_cpu_tests
    ctest --test-dir resources/build7f044c -R '^poolrad_overlay_u7f_044c_cpu_tests$' --output-on-failure

Use the audit Python environment (Capstone, Unicorn). Original bytes, generated assembly/COM and capture receipts stay under ignored `resources/`. The Unicorn run took 588 s.

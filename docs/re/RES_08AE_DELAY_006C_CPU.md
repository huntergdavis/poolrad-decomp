# RES_08AE +006C: translated, evidenced driver-internal delay loop (never a credit)

This raw CPU body waits until a 32-bit time source reaches *(first reading + a WORD
argument)* and returns with RETF 2. It is a candidate **`driver_internal`** member
([DRIVER_INTERNAL_CLASS.md](DRIVER_INTERNAL_CLASS.md)): translated and evidenced to the
credited-body bar, composing as ready once accepted, and **never a credit**. This author
change proposes **no strict row, no identity row, no manifest row and no member-table
row**; admission to the class is the reviewer's (par-e) act. It is evidence only.

## Media and executable extent

START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Loader-expanded image: 68,592 bytes, SHA-256
a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e.

The decoded span is **08AE:[006C,00DC), 112 bytes / 46 instructions**, physical
[08B4C,08BBC), ending in the only return, `RETF 2` at 00D9. Body SHA-256:
`f03f1ed3f490c274e18fa550c29af9b1bb41112ad6cb741781582721fed52446`. The return was located
by decoding forward from the entry and requiring that no earlier direct branch lands
past it, then the extent was taken from the expanded executable bytes, not from a queue
row. The checker asserts the hash, the 46 starts, the single return, that every
conditional target lies inside the span, the two call sites, and that **no loader
relocation falls inside the span** (the body has no segment-word operand).

## What the body does

Arguments: one WORD at `[BP+6]` (n, zero-extended to 32 bits). Locals `[BP-4..-1]` target,
`[BP-8..-5]` current.

1. `PUSH CS; CALL 0` (`0072/0073`): read the time source, DX:AX, as t0.
2. target := t0 + n (32-bit add with carry), then loop from `0089`:
3. `PUSH CS; CALL 0` (`0089/008A`): current := reading.
4. If `(target - current)` is greater than n (signed high word, then unsigned low word) the
   clock is treated as wrapped and `current := target + 1`.
5. Exit when `current >= target` (signed high, then unsigned low); otherwise repeat.

`CALL 0` is the near call to **08AE:0000**, which pushes only IP; the preceding
`PUSH CS` supplies the CS word, so 0000's `RETF` returns here. 08AE:0000 is a **separate
body** (108 bytes) that reads DOS time through the Turbo Pascal DOS unit
(`0A32:00A2`, INT 21h) and the long-multiply helper `0AF8:0A5B`.

## Ownership facts (measured, not a claim of identity)

The checker's media scans found: zero direct far-call fixups from any overlay to 08AE:006C;
one decoded near call inside the 08AE code, at `08AE:0162`. This establishes use inside the
driver segment. It establishes nothing about authorship, which is the class's premise.

## Raw contract

`pr_r08ae_delay_006c_cpu(ram, size, cpu, time, context, max_steps)`: exactly 1 MiB of
A20-off ordinary RAM, IP=006C, SP in [000E,FFFA] (the saved BP, eight locals, two call
frames and the argument stay inside one SS offset space), distinct CPU/RAM storage,
immutable executable code, no MMIO, no interrupts, faults, self-modification or
arbitrary-IP resume. Word accesses keep physical A20 wrapping.

**The time source is a hook, not 08AE:0000.** The model pushes CS and the return IP,
sets IP=0000 and calls the hook, which must set the registers the callee leaves (DX:AX is
the 32-bit time; BX, CX, DI and the flags are callee-owned, because the real callee
clobbers them) and may write RAM below SP. The model then performs the `RETF` from the
live stack words. This PR does **not** compose the real 08AE:0000.

Named stops (not summed):

| Stop | Meaning | State left |
| --- | --- | --- |
| RETURN | the original return at 00D9 | CPU/RAM as the original leaves them |
| TIME | the hook returned nonzero (script exhausted in the checker) | frame pushed, IP=0000, nothing popped |
| CODE | `0E E8 rel16` with target CS:0000 not present at a call site | stops at the `PUSH CS` (IP=0072 or 0089), nothing pushed |
| LIMIT | `max_steps` dispatch steps executed (`PUSH CS; CALL` is one step) | stops before the next step |
| ARGUMENT | null/size/IP/SP out of contract | CPU and RAM untouched |

## Evidence

**Original-byte Unicorn differential: 488 cases, 0 mismatches**, comparing all **14 CPU words
and all 1,048,576 RAM bytes after every case, with no mask of any kind**
(`masked_ram_bytes=0`). The original 006C bytes execute; two mappings of one backing give
the A20-off aliases. Outcomes: **480 RETURN, 8 TIME**, plus separately asserted API stops:
4 ARGUMENT, 6 CODE (three corruptions, of the `PUSH CS` byte, the `CALL` opcode and its displacement, at each of the two call sites, each stopping at the `PUSH CS` with nothing pushed), 1 LIMIT.

- **Starts: 46/46 (100%).**
- **Conditional successors: 12/12.** Both successors of each of the six conditionals were
  observed: `00AA` to `00B2`/`00AC`, `00AC` to `00C4`/`00AE`, `00B0` to `00C4`/`00B2`, `00CD`
  to `00D6`/`00CF`, `00CF` to `0089`/`00D1`, `00D4` to `0089`/`00D6`. The checker fails if any
  successor is missing.
- 1,317 stub calls. Scripts cover: climbing to the target in steps, landing exactly on it,
  overshooting, clock far below the target (wrap guard), exactly n and n+1 below it, every
  signed high-word class (8000, FFFF, 0, 1, 7FFF), random readings, first reading 0,
  arguments 0, 1, 2, 7FFF, 8000, FFFE, FFFF and random, 32-bit target overflow, random
  registers, nine initial flag words, SP at both bounds, SS:SP across the A20 wrap, CS=FFFF
  and a random full-RAM background.
- **Sensitivity:** three deliberate model mutations were run. Dropping the carry-in of `ADC`
  and weakening `JBE` were each caught (AX and flags mismatches). Setting AF after the
  `XOR DX,DX` was **not** caught, because both `XOR`s are followed by an `ADD`/`SUB` that
  rewrites every arithmetic flag before any read; that bit is unobservable, not unchecked.

C test: **28 semantic checks**, clean under `-fsanitize=address,undefined` (manual build with `-Wall -Wextra -Wpedantic -Werror`) and 1/1 in the Debug CTest build, covering the exit paths, the 32-bit
overflow loop, TIME with the frame state, LIMIT, the three CODE conditions at each site,
the ARGUMENT rejections and a missing hook.

### Declared substitution

The differential and the native run **both replace 08AE:0000**: Unicorn puts a lone `RETF`
at CS:0000 and a code hook at that address loads DX:AX, BX, CX, DI and the flags from a
per-case script; the C hook does the same from the same script. The substitution is
identical on both sides, so the comparison proves the body's own behavior given any time
source obeying the contract. It does not prove anything about the real 0000. A caller that
composes the real 08AE:0000 must show that body leaves the registers and stack words the
hook contract assumes (it also writes locals below SP).

### Native corroboration (partial, with a declared stub)

DOSBox-X normal core, 386, **32 synthetic direct calls** (8 take more than one extra
reading), with the original 112 bytes relocated to segment 8800 and a stub at 8800:0000
that loads the scripted registers and flags and returns. The stub restores the stack word
its `PUSH/POPF` used, so it leaves no residue. All 14 CPU words and a 256-byte stack window
match the C model. **1,048,320 of 1,048,576 RAM bytes are not compared natively**; the
full RAM comparison is the Unicorn differential. The native run used the host
`/usr/bin/dosbox-x` (SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`),
**not** the pinned `5a9c7c53` build. Script cases always terminate, so the TIME stop is
**not** corroborated natively. RESULT.BIN SHA-256:
`9fd1ebb1f98aae8a5de2671e6a7c937cdc29b9792fba15cab0355a690f4041d6`. Frames are synthetic,
so this does not establish natural reachability or timing behaviour. The COM harness
adapts [RES_08AE_BYTE_181A_CPU.md](RES_08AE_BYTE_181A_CPU.md)'s mechanics and corrects
nothing in it.

## Not established

- The real 08AE:0000 (and through it INT 21h time and the helper at 0AF8:0A5B) is not
  modelled here, so 006C's closure is **not complete** until that member exists.
- No wall-clock behaviour: the model does not know the source's tick rate.
- No interrupt, fault, self-modification or arbitrary-PC resume.
- The prior-session memory tool (`deja`) was unavailable (it failed to start), so nothing
  was reused from earlier sessions; the 181A checker and native harness were reused as
  templates and every figure above was measured on this change.

## Reproduction

    python3 tools/overlay/res08ae_delay_006c_cpu_audit.py --unicorn --native --output resources/006c-evidence.json
    python3 tools/overlay/res08ae_delay_006c_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --all --require-current-main
    cmake -S . -B resources/build006c -DCMAKE_BUILD_TYPE=Debug
    cmake --build resources/build006c --target poolrad_resident_08ae_delay_006c_cpu_tests
    ctest --test-dir resources/build006c -R '^poolrad_resident_08ae_delay_006c_cpu_tests$' --output-on-failure

Use the audit Python environment (Capstone, Unicorn). Compile-only builds, loads and binds
the production symbol and needs no media. Original bytes, generated assembly/COM and capture
receipts stay under ignored `resources/`.

## Reviewer acceptance record (added by the review lane, not the author)

Member row: `RES_08AE_006C` in `driver_internal_models.csv`.

**This body's own evidence**, reproduced by par-e on main `ab790918` (2026-10-20):
- **Unicorn:** 488 cases (480 RETURN, 8 TIME) plus 4 ARGUMENT, 46/46 starts and 12/12 conditional outcomes, 14 CPU words and
  all 1,048,576 RAM bytes with no mask.
- **Native:** 32 cases, RESULT.BIN `9fd1ebb1…` (matches).
- **C:** 28 checks under strict `-Werror` ASan/UBSan.

**Composition evidence (VP i20169/i20176).** This document's model takes 08AE:0000 as a hook, which is the gap it states
under "Not established". The composition is proven instead by the `RES_0899_0000` composed run (#1291,
[RES_0899_QUIT_0000_CPU.md](RES_0899_QUIT_0000_CPU.md)). That run chains the members through their real interfaces,
including this body with the real `RES_08AE_0000`, and compares the result against the original image executing the real
bodies. Reproduced by par-e: 112 cases, all EXIT, CPU14 + 1 MiB, flag masks 0, with this body **46/46** starts executed
and `RES_08AE_0000` **48/48**. Its closure callee `RES_08AE_0000` is a member. Never a credit.

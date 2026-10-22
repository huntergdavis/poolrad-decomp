# Resident 0709:0714: raw-CPU nested space skip

`pr_r0709_skip_spaces_0714_cpu` is a raw-CPU, full-state model of the
START.EXE resident routine `0709:[0714,074E)`. It is a pure, call-free leaf
of the RES `00BA:16EC` closure: a nested helper of the port-bound
`0709:0768`, which reaches it through its parent's frame pointer. See
[the port-harness plan](RES_00BA_16EC_PORT_HARNESS.md#plan).

This revision brings the merged body (PR #1040) to the full evidence bar.
Measuring the stack-offset edges found one model defect, which is fixed
here (see *Stack-offset edges*). No ledger, manifest, checkpoint or strict
table is changed, and no credit is taken.

## Media span

| Item | Value |
| --- | --- |
| Span | `0709:[0714,074E)`, loader-expanded image `[077A4,077DE)` |
| Size | 58 bytes, 19 instructions |
| First RETF | `RETF 2` at +074B, the only return in the span (asserted by the audit) |
| SHA-256 | `67b1ce78e79b1e281051f3bc73e634213d904cedd5ec265a4bdf72f502af799c` |
| Owner START.EXE | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Data follows the span: a Pascal string with length byte 19h. The image is
unpacked with `resident_loader_map.unpack`; a raw file offset is never used.

## Behaviour

The routine is entered after a far call with the parent's frame pointer P
at `[SP+4]`. After `PUSH BP; MOV BP,SP`, every use rereads P from
`SS:[BP+6]`. The parent frame holds an index byte at `P-103h`, a limit byte
at `P-101h`, and text from `P-100h`. While the index is below the limit
(unsigned `JAE`) and the character at `SS:[P-100h+index]` is a space
(`CMP 20h; JNE`), `INC BYTE SS:[P-103h]; JMP 0717` repeats. It ends with
`POP BP; RETF 2`.

On return the flags are those of the last `CMP`. AL is the index read last,
and AH is 0 once a character was tested. DI is P, or P plus the index.
Every operand is reread in source order. The `INC` can land on the stack
word that holds P, so `max_loopbacks` caps the taken `JMP 0717` and stops at
+0717 with `LIMIT`.

## Contract

- Exactly 1 MiB of A20-off RAM; IP = 0714h; code at its original offsets.
- Every stack and frame word (PUSH BP, P at SS:SP+4, POP BP, RETF IP and CS)
  is addressed at `SS:(offset mod 64K)`. The two bytes of a word that
  straddles offset FFFF are linear-contiguous (`SS*16+FFFF`,
  `SS*16+10000h`). Both rules are measured on DOSBox-X below.
- **Limit (unreachable-by-contract):** neither the PUSH BP nor the INC may
  write the routine's own bytes `CS:0714..074D`, because the model does not
  refetch code. That becomes feasible only if SS:(P-103h) or SS:(SP-2)
  aliases CS:0714..074D. No case violates it.
- **Unmodeled hardware limit:** on a real 386 a word at offset FFFF raises
  INT 13 (segment overrun). Neither oracle faults (measured), and the model
  follows the oracles.

### Entry-SP evidence (stated, not closed)

- The only call is +0768 → +0714 at +095F..+0961 (`PUSH BP; PUSH CS; CALL
  0714`). +0768's prologue is `PUSH BP; MOV BP,SP; SUB SP,126h`, so with a
  balanced stack the entry SP is BP₀₇₆₈−12Ch and P = BP₀₇₆₈. A straddling
  entry SP (0001, FFFB, FFFD, FFFF) needs BP₀₇₆₈ ∈ {0127, 0129, 012B, 012D}.
  That means +0768's own frame lies within 12Dh bytes of SS:0000.
- The program stack is set once by the EXEPACK stub: SS = load base + 14F2h,
  SP = 4000h (even). The unpacked image contains no `MOV SS`.
- An exhaustive byte-pattern search, which over-approximates real
  instructions, finds **zero odd-immediate `ADD/SUB SP`** in START (68,592 B)
  or GAME.OVR (206,105 B).
- **Open gap:** register-form SP writers are not individually discharged:
  `POP SP`, `XCHG` with SP, `MOV SP,imm`, `MOV SP,reg` (raw byte hits
  START 41/13/15/0 and GAME.OVR 197/306/83/1, including data and ASCII), and
  `MOV SP,BP` (START 11). So an odd entry SP is unlikely on the evidence but
  not proven impossible. The absolute stack floor of +0768's callers is
  likewise not established. The straddling SPs are therefore covered as
  real cases, not excluded.

## Stack-offset edges (the defect and its fix)

The merged model read RETF's CS at the physically next bytes `(SS:SP)+2`.
The merged fixture also wrote each frame word twice, segment-wrapped and
physically contiguous. That made the two readings indistinguishable, so 156
passing cases never exercised the difference. The fixture now writes the
frame at `SS:(SP+2i) mod 64K` only.

Measured 2026-10-20, 11:02–11:07 PDT, with the wrapped and contiguous copies
given different values:

| Entry SP | DOSBox-X normal/386 | Unicorn 2.1.4 | Merged model |
| --- | --- | --- | --- |
| FFFE | CS from SS:0000 (wrapped) | CS from SS*16+10000h | contiguous |
| FFFF | IP straddles (contiguous); CS from SS:0001 (wrapped) | CS contiguous | contiguous |
| 0001, FFFB, FFFD | straddling PUSH BP / P / CS word linear-contiguous, no INT 0Dh | same | same |
| 0000, 0002, FFFA, FFFC | words wrap mod 64K | same | same |

The VP decided (i19555) that the model follows DOSBox-X, the project's
native oracle. Intel's RET-far pseudo-code also pops CS with a 16-bit
stack-address-size SP; that is a reading of the manual, not a measurement on
a 386. The fix is one line: CS is read at `SS:((SP+2) mod 64K)`.

The Unicorn cases at SP FFFE and FFFF are a **declared oracle split**.
Unicorn reads that CS word contiguous. The audit asserts that the model's
CS is the wrapped frame value, that Unicorn's CS is the contiguous word, and
that nothing else differs: all 1 MiB and the other 13 CPU words. Any other
difference fails the run. This is the second known Unicorn deviation of
this shape; the first is the MUL PF split in
[RES_0709_TEXT_06A7_CPU.md](RES_0709_TEXT_06A7_CPU.md).

## Validation

Receipt run 2026-10-20 11:30:47–11:31:37 PDT on the shipped model
(resources/0714/final.json, ignored, SHA-256 `25fe15c3…5fa8`).

**Unicorn differential: 163 original-byte cases equal, plus 2 declared
CS-only splits.** Each compares unmasked CPU14 and all 1,048,576 RAM bytes.
Coverage is successor-based: **19/19 instruction starts, 4/4 conditional
outcomes** (JAE +0727 and JNE +073E, both sides), so no start or outcome is
excluded. Stops: RETURN 162, LIMIT 1 (`cap-two`, stopping at +0717), and the
2 split cases (SP FFFE/FFFF), which return. ARGUMENT is covered by the C
test (IP ≠ 0714, null RAM, size ≠ 1 MiB; nothing changed). Cases: 150
random frames (index, limit, P and SP 0800..3000 varied), all-spaces to
limit 255, an INC that rewrites P itself (`index-wraps-p`), SS across A20,
DF set, random 1 MiB background, and the seven agreed stack edges SP
0000/0001/0002/FFFA/FFFB/FFFC/FFFD (straddling words included).

**Native DOSBox-X** (normal/386, binary SHA-256 `99d74601…0df3`):

- The merged 32-case synthetic oracle passes unchanged, with RESULT.BIN
  `052cd242…f457` identical to PR #1040's. Each case compares CPU14, the
  512-byte stack window holding the parent frame and the 64 KiB data
  segment, with no mask. Those frames sit just below SS:F000, away from any edge.
- **Stack-offset edge suite: 9/9 equal to the model** at SP 0000, 0001, 0002,
  FFFA, FFFB, FFFC, FFFD, FFFE and FFFF. It compares the landing CS:IP, AX,
  DI, BP, SP, the LAHF flag byte and five stack words at the wrapped and
  contiguous locations. No INT 0Dh fault occurred. RESULT.BIN
  `92d24721…99f1`, COM segment 0814.
- Synthetic setup, declared: a COM program places the original 58 bytes at
  8800:0714. It builds frames at SS 4000h with distinct wrapped and
  contiguous copies (IP 1100h/1200h, CS S/S+100h, P 8000h/9000h with
  different parent texts) and enters by far JMP with interrupts disabled.
  Four DS-relative landing pads record the state. INT 0Dh is hooked to
  record a fault. Nothing in the body is stubbed.
- **Defect shown, then gone:** with the pre-fix CS line temporarily
  restored (11:31:46–11:31:58), the edge suite fails at SP FFFE: DOSBox-X
  CS 0814h against model CS 0914h, all other fields equal. With the fix,
  9/9 pass.

**C test:** `cc -std=c11 -Wall -Wextra -Wpedantic -Werror
-fsanitize=address,undefined -fno-sanitize-recover=all` builds without
warnings, and **14 checks pass**. They include the new SP FFFE case, in
which a decoy CS 0BADh at SS*16+10000h must not be read. `--compile-only`
prints `CHECKER_BUILD_OK` (the audit's library build uses `-std=c11 -O2`
without `-Werror`; the warning-clean claim rests on the strict build above).

## Checker and reproduction

`CHECKER_ROWS = ("RES_0709_0714",)` registers the audit with the handoff
gate. `--compile-only` builds and loads the C source before any media access
and prints `CHECKER_BUILD_OK`. Requires Capstone 5.0.7, Unicorn 2.1.4,
DOSBox-X, GNU as/ld and a C compiler. Heavy steps run through the host
semaphore.

    python3 tools/overlay/res0709_skip_spaces_0714_cpu_audit.py --compile-only
    python3 tools/overlay/res0709_skip_spaces_0714_cpu_audit.py --unicorn --native --output resources/0714/final.json
    python3 tools/overlay/check_checker_builds.py RES_0709_0714 --require-current-main
    ctest --test-dir <build> -R poolrad_resident_0709_skip_spaces_0714_cpu --output-on-failure

## Author review

- target_function: RES_0709_0714, complete `[0714,074E)`, call-free.
- signals: pinned loader-expanded identity; first-RETF span; 19/19 starts
  and 4/4 successor-based outcomes; stack-offset edges measured on both
  oracles; one model defect found and fixed to the native oracle, with the
  Unicorn split declared and asserted.
- open limits: real-386 INT 13 on straddling words; the register-form SP
  writers and the callers' absolute stack floor (above); natural-game
  reachability. Synthetic frames are not saved-game evidence.
- verdict: likely_match within the stated contract. Verification and credit
  are the review lane's. Not a ledger target (acceptance #24 recorded it as
  an internal closure leaf).

## Grant record: `RES_0709_0714` (added by the review lane, par-e, VP i20406/i20573/i20671)

Member row: `RES_0709_0714`, resident strict credit with `GAME_SPECIFIC_TARGETS` identity under the reviewed near-only
name (#1350). Ground: **the factor-2 clause in the 01DA shape (with its limit)**. Only the text of one game_verified
caller is claimed.

- **Evidence, reproduced by me on current main.**
  - **Unicorn:** 163 cases, starts 19/19, outcomes 4/4, CPU14 + 1 MiB, masks 0. The SP `FFFE`/`FFFF` declared oracle
    split is as stated above.
  - **Native:** 32 synthetic cases, all equal (RESULT.BIN `052cd242…` equals this document). The SP-edge suite is 9/9
    with 0 faults (`92d24721…`). The audit's `skipped: 10` means 10 of the 32 cases **skipped at least one blank**: it
    is a behaviour count, NOT omitted cases (audit line 299).
  - **ASan/UBSan:** **14 checks**, which matches this document.
  - **Checker build:** passes; `CHECKER_ROWS = ("RES_0709_0714",)`.
- **Factor 1 (chain), verified by me from the media:** the only near call is `0961`, inside the printer
  `0709:[0768,09E3)`, and a raw `E8` scan finds no other site. `0768` has a direct PASS verdict.
- **Factor 2: the clause, 01DA shape, verified by me from the media.**
  - **What the body does:** it has no absolute-DS operand. It takes 0768's frame pointer (`[BP+6]`) and advances 0768's
    index over blanks in **0768's local copy of its string argument**.
  - **The text, for a game_verified caller:** `OVL_001C_0A09` pushes `MOV DI,70C8h; PUSH DS; PUSH DI` immediately
    before `LCALL 0709:0768` at **both `+0A69` and `+0A91`**. That string is far `DS:0x70C8`, the ECL **string buffer
    1**, documented in [U1C_PRINT_0A09.md](U1C_PRINT_0A09.md) (row `+0x0A52..+0x0A6E`) and
    [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md).
  - **Limit, kept:** only that caller's text is claimed. Other callers of 0768 are not argued here.
- **Role line:** skips the leading blanks of the message the printer `0768` is drawing; for the game_verified caller
  `001C_0A09`, that message is the game's ECL string buffer 1 at `DS:70C8`.

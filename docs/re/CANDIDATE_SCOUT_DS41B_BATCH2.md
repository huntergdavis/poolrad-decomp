# Candidate scout batch 2 - C-verified ECL run loop (OVL_001C_3171)

Status: **proposal, no credit, no accounting change.** The C draft is compiled and
compared against the original under the Unicorn differential, not merely byte-copied.

## Candidate: OVL_001C_3171 - ECL run loop

| field | value |
| --- | --- |
| body | `OVL_001C_3171` |
| unit:offset | `0x1c:0x3171` |
| file offset | `16055` |
| length | 66 bytes `[0x3171,0x31B3)` (byte `+0x31B3` is padding) |
| span SHA-256 (pre-filter: proves DIFFER, never SAME) | `4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489` |
| instruction count | 22, ending `RETF 2` |
| model | `src/recovered/overlay_u1c_run_3171_cpu.c` |
| header | `include/poolrad/recovered/overlay_u1c_run_3171_cpu.h` |
| audit | `tools/overlay/u1c_run_3171_cpu_audit.py` |

The span is identified by `GAME.OVR` sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, the file offset,
the length and the span digest above. Original game bytes are not quoted here.

Semantic reading (corrected; the two earlier omissions were found by the sweep):

```c
ax := [BP+6]; PC ([0x49ED]) := ax; stop byte [0x442E] := 0;
while ([0x442E] == 0 && [0x49FF] == 0) {
    [0x6F7E] := [0x6F7F];                        /* previous opcode */
    ax := [0x49ED];                              /* reload PC every iteration */
    di,es := far pointer [0x49DE]; di += ax;     /* ADD DI,AX writes DI, not AX */
    al := es:[di + 0x6700]; [0x6F7F] := al;      /* fetch and latch, AL only */
    push cs; call near +2F2D;                    /* dispatcher */
}
[0x442E] := 0;
```

The body clears `[0x442E]` on entry and exit but does **not** clear `[0x49FF]`, so a
preset `[0x442E]` is cleared and the loop runs, while a preset `[0x49FF]` skips the loop.

## The check that counts

```
$ python3 tools/overlay/u1c_run_3171_cpu_audit.py
OVL_001C_3171 span 3171..31B3 file 16055 sha256 4055cdf54a816923...
  AGREE [adversarial] advance-then-stop
  ...
  AGREE [adversarial] es-6000
  AGREE [adversarial] flags-nonfinal
  AGREE [adversarial] of-collision-7ffe
  AGREE [adversarial] of-collision-7fff
  AGREE [adversarial] of-collision-8000
  AGREE [regression] regress-entry7-stop2-1
  AGREE [regression] regress-entry7-stop2-1-zero
  AGREE [regression] regress-bufoff-advance
  AGREE [regression] regress-bufoff-advance-zero
  ... 64 sweep cases ...
  mutant bias: killed on 45/82 cases
  mutant ax_entry: killed on 26/82 cases (incl. regress-entry7-stop2-1)
  mutant add_ax: killed on 22/82 cases (incl. regress-bufoff-advance)
  mutant flags_xor: killed on 47/82 cases (incl. advance-then-stop)
  mutant flags_pcodd: killed on 42/82 cases (incl. flags-nonfinal)
  mutant of_nonterminal: killed on 45/82 cases (incl. of-collision-7fff)
PASS: 82 cases (14 adversarial, 4 regression, 64 sweep), 106 dispatches, full 1 MiB RAM + 14 CPU words
```

What the audit does:

1. Reads the span from the hash-pinned `GAME.OVR` and asserts the extent, instruction
   count, the single near `call` to `+2F2D`, and the `RETF 2` ending.
2. Runs the **original bytes** under Unicorn (16-bit, 1 MiB RAM) with a full caller frame
   (`[BP+2]=return IP`, `[BP+4]=return CS`, `[BP+6]=entry argument`).
3. Runs the **compiled C** model from the same initial RAM and CPU.
4. Places the same deterministic dispatcher double at `+2F2D` on both sides (Unicorn code
   hook on the original side; the same compiled function in C). The double is a harness,
   not a claim about the real dispatcher. It records the **ordered** arithmetic-FLAGS word
   for every dispatch (masked to `08D5h`) in 64 slots at DS:0F00h..0F7Fh plus the
   observation count at DS:0F80h, so a change on any dispatch **and the sequence length**
   are compared directly. This replaced a 16-bit rolling hash, which admitted a
   demonstrated modular cancellation (`33*0x800-0x800 = 0x10000`); the OF-flip collision is
   a permanent regression control. The records are audit instrumentation, not body
   behaviour. A digest printed by the audit is a **pre-filter**: it proves DIFFER, never
   SAME.

   **External-trace caveat:** external mode compares RAM EXCEPT the 130-byte
   instrumentation region (DS:0F00h..0F82h). It must not replace normal full-RAM row
   acceptance. The mask does not distinguish intended instrumentation writes from model
   writes; a wrong value written inside that window is invisible to external final-state
   comparison. Normal mode keeps the full 1 MiB compare. Permanent controls:
   `MaskBoundaryTest.test_external_mask_boundary_controls` (normal rejects 4/4; external
   rejects the two outside and accepts the two inside).
5. Compares **all 1,048,576 RAM bytes, all 14 CPU words (unmasked), the dispatch count
   and the final `CS:IP:SP`** per case.
6. Runs four mutants and requires each to die; the two regression mutants must be killed
   by their own regression case.

## How every case was chosen

Every case is labelled `adversarial`, `regression` or `sweep` in the audit output.

**Adversarial (14)** - one distinct semantics shape each (buffer segment, a non-final
dispatch, and the 7FFE/7FFF/8000 buffer-offset boundary controls), with measured dispatch
counts:

| case | dispatches | what it actually exercises |
| --- | ---: | --- |
| advance-then-stop | 2 | opcode 2 advances PC, then a 00 stop |
| stop-immediately | 1 | first fetched opcode is 00 |
| second-stop-byte | 2 | 0xFF sets `[0x49FF]` on the second dispatch |
| stop-byte-preset | 2 | entry clears `[0x442E]`, so the loop runs; 00 then stops it |
| second-stop-preset | 0 | preset `[0x49FF]` is not cleared, loop skipped |
| nonzero-entry | 2 | entry 7, two dispatches |
| latch-carried | 2 | a nonzero `[0x6F7F]` is copied to `[0x6F7E]` |
| many-steps | 13 | 12 advancing opcodes plus the 00 stop |
| wrap-pc | 2 | PC goes FF00 -> FF05; the *effective address* wraps, PC does not |
| es-6000 | 2 | buffer segment 6000h with offset 0100h |
| flags-nonfinal | 2 | first ADD has odd PC, last has even; the ordered sequence sees a first-dispatch change |
| of-collision-7ffe / 7fff / 8000 | 3 each | review-d's OF-flip boundary controls; the 7FFF case used to cancel in a 16-bit rolling hash |

**Regression (4)** - the reviewers' exact fixtures, both shapes kept:
`regress-entry7-stop2-1` (script `[3,4,0]`) and `regress-entry7-stop2-1-zero` (script
`[0]`), both entry 7 with `[0x49FF]` preset (0 dispatches, AX must be 7);
`regress-bufoff-advance` (advance-then-stop) and `regress-bufoff-advance-zero` (script
`[0]`), both buffer offset 0100h (AX must not receive DI+PC).

**Sweep (64)** - the small cross-product derived from the instruction semantics:
`entry in {0,1,7,0xFFFF} x stop [0x442E] in {0,1} x stop2 [0x49FF] in {0,1} x buffer
offset in {0,1,0x100,0xFF00}`, each with script `[1,0]`. Preset stops may make the script
unreached; that is the point of including them.

**Axes and how each is varied** (a constant that cannot vary is a hole no number of cases
fills):

| axis | values exercised | coverage |
| --- | --- | --- |
| entry | 0, 1, 7, 0xFFFF | sweep + adversarial (`nonzero-entry`, `wrap-pc`) |
| stop byte `[0x442E]` | 0, 1 | sweep + adversarial (`stop-byte-preset`) |
| stop2 `[0x49FF]` | 0, 1 | sweep + adversarial (`second-stop-preset`, regressions) |
| buffer offset | 0, 1, 0x100, 0xFF00 | sweep + regression (`regress-bufoff-advance`) |
| buffer segment | 0x5000, 0x6000 | adversarial (`es-6000`); per-case parameter |
| latch `[0x6F7F]` | 0, 0x5A | adversarial (`latch-carried`) |
| script length | 0, 1, 2, 3, 13 | adversarial (`stop-immediately`, `many-steps`) + sweep |

**Mutants (each must be killed):**

| mutant | defect | killed by |
| --- | --- | --- |
| bias | fetch bias +0x6700 -> +0x6701 | any of 39 cases |
| ax_entry | omits `MOV AX,[BP+6]` | `regress-entry7-stop2-1` |
| add_ax | `ADD DI,AX` wrongly writes AX | `regress-bufoff-advance` |
| flags_xor | XORs the arithmetic flags after ADD | `advance-then-stop` (flags observed at dispatcher entry) |
| flags_pcodd | XORs only when the ADD operands differ in bit 0 | `flags-nonfinal` (ordered sequence catches a non-final dispatch) |
| of_nonterminal | flips only OF before dispatch on nonterminal opcodes (review-d's repro) | `of-collision-7fff`, with 7FFE and 8000 positive controls |

## What this does NOT verify

- The real dispatcher (`+0x2F2D`) and its 61 cases. It is deliberately a callback
  boundary with a test double on both sides.
- The ECL script buffer's provenance or any caller; the audit synthesises both.
- Any credit, strict-map, or accounting claim. None is made.

The 75 cases are systematic over the stated domains, not an exhaustive input space; the
sweep does not enumerate script contents beyond `[1,0]`.

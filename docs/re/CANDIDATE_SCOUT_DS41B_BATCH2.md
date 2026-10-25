# Candidate scout batch 2 - C-verified ECL run loop (OVL_001C_3171)

Status: **proposal, no credit, no accounting change.** This batch carries the check the
VP required for the metric: the C draft is compiled and compared against the original
under the Unicorn differential, not just byte-copied.

## Candidate: OVL_001C_3171 - ECL run loop

| field | value |
| --- | --- |
| body | `OVL_001C_3171` |
| unit:offset | `0x1c:0x3171` |
| file offset | `16055` |
| length | 66 bytes `[0x3171,0x31B3)` (byte `+0x31B3` is padding) |
| span SHA-256 | `4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489` |
| instruction count | 22, ending `RETF 2` |
| model | `src/recovered/overlay_u1c_run_3171_cpu.c` |
| header | `include/poolrad/recovered/overlay_u1c_run_3171_cpu.h` |
| audit | `tools/overlay/u1c_run_3171_cpu_audit.py` |

Original bytes (verbatim, `GAME.OVR` sha256 `872964a3...928614`):

```bytes
55 89 e5 8b 46 06 a3 ed 49 c6 06 2e 44 00 80 3e
2e 44 00 75 24 80 3e ff 49 00 75 1d a0 7f 6f a2
7e 6f a1 ed 49 c4 3e de 49 03 f8 26 8a 85 00 67
a2 7f 6f 0e e8 85 fd eb d5 c6 06 2e 44 00 5d ca
02 00
```

Semantic reading:

```c
PC ([0x49ED]) := entry; stop byte [0x442E] := 0;
while ([0x442E] == 0 && [0x49FF] == 0) {
    [0x6F7E] := [0x6F7F];                        /* previous opcode */
    ax := PC; di,es := far pointer [0x49DE]; di += ax;
    [0x6F7F] := es:[di + 0x6700];                /* fetch and latch */
    push cs; call near +2F2D;                    /* dispatcher */
}
[0x442E] := 0;
```

## The check that counts

```
$ python3 tools/overlay/u1c_run_3171_cpu_audit.py
OVL_001C_3171 span 3171..31B3 file 16055 sha256 4055cdf54a816923...
  AGREE advance-then-stop
  AGREE stop-immediately
  AGREE second-stop-byte
  AGREE stop-byte-preset
  AGREE second-stop-preset
  AGREE nonzero-entry
  AGREE latch-carried
  AGREE many-steps
  AGREE wrap-pc
mutant killed on 7/9 cases
PASS: 9 cases, 26 dispatches, full 1 MiB RAM + 14 CPU words
```

What the audit does:

1. Reads the span from the hash-pinned `GAME.OVR`, asserts the 66-byte extent, the 22
   instructions, the single near `call` to `+2F2D`, and the `RETF 2` ending.
2. Runs the **original bytes** under Unicorn (16-bit, 1 MiB RAM) from the unit's load
   segment, with a full caller frame (`[BP+2]=return IP`, `[BP+4]=return CS`,
   `[BP+6]=entry argument`).
3. Runs the **compiled C** model from the same initial RAM and CPU.
4. Places the same deterministic dispatcher double at the `+2F2D` boundary on both
   sides. The original side reaches it through a Unicorn code hook that emulates the
   far callee's `RETF`; the C side calls the same compiled
   `pr_u1c_3171_dispatch_double`. The double is a harness, not a claim about the real
   61-case dispatcher.
5. Compares **all 1,048,576 RAM bytes, all 14 CPU words (unmasked), the dispatch count,
   and the final `CS:IP:SP`** per case.
6. Compiles a **mutant** (fetch bias `+0x6700` -> `+0x6701`) and requires it to differ on
   at least one case, so the differential is shown able to fail. It dies on 7/9.

Cases cover: normal advance then `0x00` stop, immediate stop, `0xFF` second stop, each
stop byte preset (loop skipped), a nonzero entry, a carried opcode latch, 12 fetch
iterations, and an entry that wraps the 16-bit PC.

## What this does NOT verify

- The real dispatcher (`+0x2F2D`) and its 61 cases. It is deliberately a callback
  boundary with a test double on both sides.
- The ECL script buffer's provenance or any caller. The audit synthesises the buffer and
  caller frame.
- Any credit, strict-map, or accounting claim. None is made.

Batch 1's `OVL_001C_0292` and `OVL_001C_0EBC` remain **C-unverified drafts**: their byte
spans are identity-checked, but they do not count toward proposals-that-verified until a
differential like this one exists for them.

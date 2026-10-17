# U3C6 entry 16 (+0x0CB9): four-step neighbour scan (CPU model)

This note covers GAME.OVR unit `0x3C6` + `0x0CB9`, public entry 16, as a
whole function. The run-time stub is `0x3E0:0x70`.

- **Status.** The CPU/RAM model is source-only. It is uncompiled under the
  host hold and not counted.
- **Ownership.** Separate relative CLEAR from all 18 queried lanes (PM i14419).
  Local source GO: PM i14426.

| Input | SHA-256 |
| --- | --- |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit `0x3C6` code (4,864 bytes, file 198,458) | `9ce9da56407e068deff83afea3d5f3f91eeb2705630124a1f9c6770cd2eddba6` |
| Body `[0CB9,0E11)`, 344 bytes, file `0x313F3` | `63f0434e2d8cadf426464601eed98ae42fa5d13b708303f45ac5440faf95d43b` |
| Folded entry 15 `[0C5F,0CB9)`, 90 bytes | `c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41` |
| Folded entry 1 `[0000,0068)`, 104 bytes | `05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207` |
| Folded entry 6 `[04BE,052B)`, 109 bytes | `bf1db9df7666fe7c5615d826c14442fd10e08f768289f65ebfd3ac10daf2027e` |

## Shape

- **Decoding.** 139 instructions, all 344 bytes reached, 19 leaders. No jump
  leaves the span.
- **Frame.** Far Pascal with 14 argument bytes:
  - far byte pointer P6 at `[BP+6]`;
  - far byte pointer P10 at `[BP+0xA]`;
  - byte k at `[BP+0xE]`;
  - far pointer Q at `[BP+0x10]`.

  `sub sp,0xC`; `mov sp,bp; pop bp; retf 0xE`.
- **Callers** (fixup-validated far calls to stub slot 16): unit `0x2F` +
  `0x0BDC`, `0x31` + `0x06DA`, `0x33` + `0x150E` and `0x9E` + `0x0B0E`. There
  is also one near call inside the unit at `+0x111A`.
- **Callees.** All are `push cs; call near`, call-free and fully decoded:
  entry 15 (`retf 4`, called once), entry 1 (`retf 0xC`) and entry 6
  (`retf 0xC`), the last two once per step. Pushed argument bytes equal each
  RETF immediate (4, 12, 12). Every callee frame lies at or below `BP-0xE`,
  outside the locals `[BP-0xC,BP)`.

## Behaviour

1. **Initialise.** `*P10 := 0`, `*P6 := 0x17`, best rank `[BP-5] := 1`.
2. **Find the slot.**
   - `s := entry15(Q)`: the first slot 1, 2, … whose far pointer at
     `DS:0x65B9 + 4s` equals Q, stopping when the byte counter equals
     `[0x5F2A]`; 0 when none.
   - When `[0x5F2A]` is 0, the counter wraps and all 256 slots are compared,
     with slot 0 last.
   - The base position is `(x0, y0) := ([0x5F27 + 4s], [0x5F28 + 4s])`.
3. **Four steps.** For `i = 0..3`:
   1. Call `entry1(class = [0x5F2A + 4s], i, &[BP-0xA], &[BP-0xB])`. When the
      class is non-zero, it stores the pair at `DS:0x2844 + 8·class + 2i` and
      returns 1 iff the first byte is signed non-negative. When the class is
      0, it stores nothing and returns 0. A 0 result skips the step.
   2. Compute the column `[0x2736 + k] + [BP-0xA] + x0` and the row
      `[0x273F + k] + [BP-0xB] + y0` (CBW sums truncated to a byte).
   3. Call `entry6(column, row, &cellB, &cellA)`:
      - in range (column 0..0x31, row 0..0x18, signed): cellA is the byte at
        `+7` of the 50-wide map behind the far pointer `DS:0x6716`, and cellB
        is `[0x60DB + 50·row + column]`;
      - otherwise both are 0.
   4. If `cellB = s`, set cellB := 0. If cellB > 0, then `*P10 := cellB`
      (the last write wins).
   5. Update `*P6`, re-read live through LES:
      - if cellA = 0 or `*P6` = 0: `*P6 := 0`;
      - else if cellA = 0x1E or `*P6` = 0x1E: `*P6 := 0x1E`;
      - else if `[0x2744 + 4·cellA]` ≥ best (unsigned): best := that byte and
        `*P6 := cellA`.
4. **Return.** Every path ends with `cmp byte [bp-1],3` equal, so the
   final arithmetic flags are PF and ZF.

## CPU model

Files:
- `include/poolrad/recovered/overlay_u3c6_0cb9_cpu.h`;
- `src/recovered/overlay_u3c6_0cb9_cpu.c`, entry point
  `pr_u3c6_0cb9_cpu_run(ram, ram_size, cpu)`.

The model uses the full-state precedent conventions: 1 MiB A20-off RAM and a
14-register CPU. All instructions of the body and of the three folded callees
run in original order on live state. Execution includes the real `push cs`,
the CALL return pushes, the callee frames, the flags and the RETFs.

- **Hooks.** None; no resident, SYSTEM or DOS call is reachable.
- **Returns.** After each folded RETF the model checks that CS:IP is the pushed
  return. Otherwise the status is `RETURN_CHANGED`, with the popped values in
  CS:IP.
- **Alias stops.** Every write, including pushes, is checked per byte against
  `[0000,0068)`, `[04BE,052B)` and `[0C5F,0E11)` at the entry CS. The model
  stops before an aliasing byte, with IP naming the instruction.
- **Live reads.**
  - P6 and P10 are re-read through LES at every original site, and the frame
    words are live.
  - When `*P6` or `*P10` aliases the frame (for example the pointer words or
    the counter), the original's behaviour follows.
  - This includes non-termination when a store rewrites the step counter
    `[BP-1]` on every step. The model, like the original, has no step bound.
- **Undefined flags.** These follow the pinned core's lazy flags
  (`src/cpu/flags.cpp` sha `3ea36c4c…d893`, `src/cpu/instructions.h` at
  revision `5a9c7c53`):
  - AF = 0 after XOR and OR (`get_AF` returns false for `t_OR*`/`t_XOR*`);
  - after SHL by a non-zero count, AF = 1 (`get_AF` returns the masked
    count) and OF = MSB(result) XOR MSB(operand);
  - MUL on the 386-type normal core sets ZF = (AX = 0) and PF from all 32
    product bits, keeps SF and AF, and sets CF = OF = (DX ≠ 0).

  In this body these values are overwritten before any return. They become
  visible only in an alias-stop state, at the write that follows `shl di,1`
  at `+0CED`, `+0CFF`, `+0D1A` or `+0DF6`.

## Controls (source only, not yet run)

`tests/recovered/test_overlay_u3c6_0cb9_cpu.c` builds synthetic state with no
game bytes. It is not wired into CMake and has not been compiled:

- **Seeded runs.** 240 seeded table sets are compared with an independent
  high-level reference: `*P6`, `*P10`, slot, best rank and step counter. Each
  run also checks the frame: CS:IP is the return, SP is the entry SP + 4 +
  0xE, BP and the saved BP are intact, and the flags are `0x0046`.
- **Write set.** Every changed byte must be inside `SS:[0x7FC0,0x7FEE)` (the
  frame and callee scratch) or be `*P6`/`*P10`.
- **Code-alias boundaries.** P6 or P10 is placed on the first and last byte
  of each modelled code range and just outside it. Inside, the model stops at
  `+0CC9` (P6) or `+0CC2` (P10) with the code byte unchanged; outside, it
  returns.
- **Negative control.** P6 points at the low byte of its own frame word. The
  `+0CC9` store rewrites the pointer, and the later `+0DB5` store must land at
  the new address. A cached-pointer model would write back over the frame
  instead.
- **Arguments.** NULL pointers, a wrong size or a wrong IP give `ARGUMENT`
  with the CPU unchanged.

## Unresolved

- **Profile.** The LES/LDS rule (segment word at linear EA+2, minus 0x10000
  when the offset is above 0xFFFC) is the pinned DOSBox-X normal core:
  - revision `5a9c7c53481c2e933948039c33256776653f4faf`;
  - `CPU_OMIT_8086` absent;
  - `do_lds_wraparound` = true;
  - `cpu.code.big` = false.

  Word accesses are physically consecutive (high byte at linear+1). Main's
  U3C6 `+0000` model leaves its RETF stack-wrap policy unmeasured; here
  physically consecutive access is a profile assumption, not a native finding.
- **Conventions.** The undefined-flag conventions above.
- **Overlay dispatch.** The overlay stub dispatch and the caller side.
- **Evidence.** No compile, test, native or oracle evidence yet. The read-only
  trace gate attempted on 2026-10-16 was interrupted with no output, and it
  is neither PASS nor FAIL.
- **Credit.** Callee credit stays with its owners: main's U3C6 `+0000` model,
  the U3C6 slot C for entries 12–15, and the separate `+04BE` head.

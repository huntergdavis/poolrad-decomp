# U3C6 entry 6 (+0x04BE): range-checked 50 x 25 cell lookup (CPU model)

This note covers GAME.OVR unit `0x3C6` + `0x04BE`, public entry 6, as a whole
function. The run-time stub is `0x3E0:0x3E`.

- **Status.** The standalone model now has author-run original-byte and
  native parity evidence; see [the parity report](U3C6_04BE_PARITY.md).
  Independent verification and credit remain with the reviewer. Its explicit
  return policies and four unsupported LES boundaries are retained.
- **Ownership.** Relative CLEAR from pF, pN, pP, p5, p9, pV, p6, pC, pG,
  pD, pR and pH (i13226..i13553). PM source-only GO: i13565.

| Input | SHA-256 |
| --- | --- |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit `0x3C6` code (4,864 bytes, file `0x3073A`) | `9ce9da56407e068deff83afea3d5f3f91eeb2705630124a1f9c6770cd2eddba6` |
| Body `[04BE,052B)`, 109 bytes, file `0x30BF8` | `bf1db9df7666fe7c5615d826c14442fd10e08f768289f65ebfd3ac10daf2027e` |

## Shape

- **Decoding.** 42 instruction starts, all 109 bytes reached, seven basic
  blocks, no calls, no loop.
- **Arguments and frame.** Far Pascal with arguments far `A` `[BP+6]`, far
  `B` `[BP+0xA]`, byte `y` `[BP+0xE]` and byte `x` `[BP+0x10]` (signed; high
  bytes unread). `push bp; mov bp,sp`, then `pop bp; retf 0xC`. No locals.

**Callers:**

- far, through stub slot 6: unit `0x5E` `+1C67`/`+2ECC`, and unit `0x86`
  `+1C7D`/`+28DB`/`+297B`/`+2A8B`;
- near: unit `0x3C6` `+025A`/`+0325`/`+0572`/`+0D85`.

## Behaviour

| Offset | Effect |
| --- | --- |
| `+04C1..+04D7` | Signed byte tests: `x` < 0 or > `0x31`, or `y` < 0 or > `0x18` → `+0519` |
| `+04D9..+04F7` | `i = cbw(y) * 0x32 + cbw(x)` (MUL; DX = 0 in range). `*A := (far DS:[0x6716])[i + 7]` |
| `+04FA..+0514` | `i` is recomputed; `*B := DS:[0x60DB + i]` |
| `+0519..+0523` | Out of range: `*B := 0`, then `*A := 0` |

**Exit state:**

- In range: AX = (the high byte of `y * 0x32`, the `B` byte), DX = the
  high word of the second MUL, CX = `cbw(x)`, ES:DI = `B`, and the flags of
  `add di,cx` at `+050B`. DX is 0 only with a stable frame. If the first
  `*A` store aliases the `y` byte, the second CBW/MUL reads the new value;
  for example `y` = 0x80 gives DX:AX = 0x0031:0xE700. The model reproduces
  this, because every read is live (review i13648, C6-3).
- Out of range: AX, CX and DX are unchanged, ES:DI = `A`, and the flags of
  the failing CMP.
- BP is restored and SP = entry SP + 0x10.

MUL's flags never reach the exit, a store or any test: each MUL (`+04E6`,
`+0507`) is followed by an ADD (`+04EC`, `+050B`) before the next write.
The model still follows the pinned 386-type normal core (`MULW` in
`src/cpu/instructions.h` sha
`7cbc4bea7e47678f0a3734a4c16c2110ef8125b09d0aa7f7a8457aab7b3f747e`;
`CPU_CORE` = 386 in `core_normal.cpp`, revision `5a9c7c53`):
- ZF = (AX = 0);
- PF from all 32 product bits;
- CF = OF = (DX ≠ 0);
- SF and AF kept.

Before self-finding i14647 the model kept ZF and PF.

## CPU model

Files: `include/poolrad/recovered/overlay_u3c6_04be_cpu.h` and
`src/recovered/overlay_u3c6_04be_cpu.c`, entry point
`pr_u3c6_04be_cpu_run(ram, ram_size, cpu, retf_policy)`. It uses the
full-state precedent conventions: 1 MiB A20-off RAM and a 14-register CPU. All
42 instructions run in order on live state, with no hooks.

- **Writes.** The BP push and both stores are alias-checked against the
  loaded body; an aliasing write stops before it.
- **MUL flags.** The pinned 386-type normal core (`MULW`) sets ZF = (AX = 0),
  PF from all 32 product bits and CF = OF = (DX ≠ 0), and keeps SF and AF.
  This agrees with the section above. Both MULs are followed by an ADD, so
  none of this can be observed.
- **LES.** It reads the offset word at EA and the segment word at EA+2.
  - The `DS:0x6716` operand has a fixed offset and never reaches the seam.
  - The stack operands `[BP+6]` and `[BP+0xA]` (`+04F4`, `+0511`, `+0519`,
    `+0520`) at offsets above 0xFFFC fall in an unmeasured profile seam. The
    pinned DOSBox-X source would read EA+2−0x10000 there, but no receipt
    exists for executing that LES. The new parity evidence measures the
    pre-instruction stop, not execution through the seam. The model stops
    **before** that LES with status
    `UNSUPPORTED_LES_SEAM`, IP at the LES offset and earlier effects kept
    (PM i16879, after pV i16876 F3).
- **Stack words.** PUSH BP, POP BP and the return-IP word are read and written
  physically contiguous (high byte at linear+1), even across SS:`FFFF`. This
  rule now matches selected native frames, including the BP word at
  SS:FFFF with entry SP=1; see the parity report.
- **RETF policy.** The `RETF 0xC` at `+0528` follows one caller-declared
  policy with the values and meaning of the public U3C6 `+0000` model
  (PR #629), as in `+0CB9` (PR #833).
  - `UNMEASURED` stops at `+0528` after `POP BP`, with `RETURN_PENDING` and
    both stores kept.
  - `WRAPPED_SS` reads CS at SS:(SP+2) with the offset wrapped.
  - `CONTIGUOUS_PHYSICAL` reads CS two linear bytes above the IP word.
  - WRAPPED_SS matches the measured native return frames.
    CONTIGUOUS_PHYSICAL remains the explicit Unicorn alternative. They
    differ only when the return IP word sits at SS:`FFFE`/`FFFF`.
- **Alias stops.** An aliasing store stops with IP at that store's offset
  (`+04F7`, `+0514`, `+051C` or `+0523`; review C6-2). An aliasing BP push
  stops at `+04BE` with nothing changed.

## Controls (synthetic C test)

`tests/recovered/test_overlay_u3c6_04be_cpu.c` is registered in CMake as
`poolrad_overlay_u3c6_04be_cpu_tests`. It uses synthetic state only and
counting, NDEBUG-safe checks. All 257 checks pass under strict C11 warnings,
ASan and UBSan, and through the registered CTest.

- **In range.** Corner and middle cells under both synthetic profiles. The
  test checks both stores, the full exit registers, and the flags of the
  final ADD against an independent formula.
- **Out of range.** Five cases, including x = 0x80. Both stores are 0, in
  order; AX, CX and DX are unchanged; the flags are those of the failing CMP.
- **Live re-reads.**
  - The `*A` store aliasing the row byte gives DX = 0x0031 and a 16-bit
    wrapped B index.
  - The `*A` store aliasing B's offset word gives B at the rewritten offset.
- **LES seam stops.**
  - In range: at `+04F4` with nothing stored, and at `+0511` after `*A`.
  - Out of range: at `+0519` with nothing stored, and at `+0520` after
    `*B := 0`.
  - The `FFFC` neighbour runs.
- **Stack edge.** Entry SP 1 puts the BP word at SS:`FFFF` plus linear
  +0x10000, and BP is restored.
- **Code-alias stops** at each store, at the BP push (including a word that
  straddles into the body), and at the neighbours.
- **`UNMEASURED`** gives `RETURN_PENDING` at `+0528`.
- **Return frames** at SS:`FFFD`/`FFFE`/`FFFF`, using decoys.
- **Arguments.** NULL pointers, a wrong size, a wrong IP or an undeclared
  policy give `ARGUMENT`, with the CPU and RAM unchanged.

## Evidence and remaining limits

- **Evidence.** The standalone entry is now compared against original bytes
  and native captures in [the parity report](U3C6_04BE_PARITY.md). The folded
  +0CB9 implementation has separate evidence (PR #933); those caller tests
  were not treated as standalone +04BE verification.
- **Profile assumptions.** The stack-operand LES seam is unsupported (fails
  closed), not resolved. Native observations support the selected
  contiguous stack-word and WRAPPED_SS cases; they do not establish every
  processor configuration. CONTIGUOUS_PHYSICAL remains an explicit alternative.
- **Overlay dispatch.** The overlay stub dispatch into the entry.

- **Profile switches.** LES offsets `FFFD`–`FFFF` are not modelled.
  - The stack operands stop before the LES with `UNSUPPORTED_LES_SEAM`.
  - The `DS:0x6716` operand is a fixed offset.
  - Every reachable LES reads its segment word at EA+2 with no wrap.
  - What a CPU or emulator does at the seam depends on its profile (for
    example DOSBox-X `do_lds_wraparound`). The new native receipt measures
    states before these LES instructions; it does not resolve their execution
    semantics or supersede the fail-closed decision (PM i13899 and i16879).

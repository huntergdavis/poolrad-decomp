# U3C6 entry 6 (+0x04BE): range-checked 50 x 25 cell lookup (CPU model)

This note covers GAME.OVR unit `0x3C6` + `0x04BE`, public entry 6, as a whole
function. The run-time stub is `0x3E0:0x3E`.

- **Status.** Source-only, uncompiled under the host hold, not counted.
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
`pr_u3c6_04be_cpu_run(ram, ram_size, cpu)`. It uses the full-state precedent
conventions: 1 MiB A20-off RAM and a 14-register CPU. All 42 instructions run
in order on live state, with no hooks.

- **Writes.** The BP push and both stores are alias-checked against the
  loaded body; an aliasing write stops before it.
- **MUL flags.** CF = OF = (DX ≠ 0). SF, ZF, AF and PF are left unchanged;
  they are architecturally undefined and are overwritten by the next ADD.
- **LES.** It follows the pinned DOSBox-X normal core (revision `5a9c7c53`):
  the segment word is read at linear EA+2, minus 0x10000 when the offset is
  above 0xFFFC. Only offset 0xFFFD differs from seg:(off+2) (review i13648,
  C6-1). A physical 8086 or other emulator profile may differ.
- **Alias stops.** An aliasing store stops with IP at that store's offset
  (`+04F7`, `+0514`, `+051C` or `+0523`; review C6-2).

## Unresolved

- No compile, test, checker, native or oracle evidence yet.
- MUL's undefined flags (unobservable here) and the LES wrap profile
  conventions.
- The overlay stub dispatch into the entry.

- **Profile switches.** The LES/LDS rule above holds only for the DOSBox-X
  normal core at revision `5a9c7c53481c2e933948039c33256776653f4faf`,
  built with `CPU_OMIT_8086` absent and run with `do_lds_wraparound` = true
  (the `lds wraparound` default) and `cpu.code.big` = false (16-bit code
  segment). No build, configuration or native receipt has been recorded for
  this profile (PM i13899, after pN i13852/i13860/i13866).

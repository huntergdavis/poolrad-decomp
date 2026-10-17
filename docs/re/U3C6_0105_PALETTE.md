# U3C6 entry 3 (+0x0105): gated palette register pair (CPU model)

This note covers GAME.OVR unit `0x3C6` + `0x0105`, public entry 3, as a whole
function. The run-time stub is `0x3E0:0x2F`.

- **Status.** Source-only, uncompiled under the host hold, not counted.
- **Ownership.** Relative CLEAR from pF, p5, p9, pG, pV, pC, pR, p6, pP,
  pD, pN and pH (i13466..i13682).

| Input | SHA-256 |
| --- | --- |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Body `[0105,0127)`, 34 bytes, file `0x3083F` | `94976ef2a24d913b603b17093f996d1099bf152b8af84e2e9e8a1730ead65644` |
| Resident `0x464:+2A0D`, 61 bytes (START image `0x0704D`) | `9a5d907528d68e38983cc795b891dfbc5bb20aa0f8053494f47e553edbfff0a8` |

## Shape and callers

- **Decoding.** 17 instruction starts, three blocks, `push bp` ... `pop bp;
  retf`, no arguments.
- **Callers** (far, through the stub): unit `0x2F` `+0064` (p5's
  external-callback caller), unit `0x6C` `+3F28` and unit `0x77` `+0AC1`.

## Behaviour

When `DS:[0x4A1E]` = 1, the body calls the resident palette setter
`0x464:+2A0D` twice, with (register, value) = (0, 0) and then (8, 8). The
arguments are pushed as words with AH carried. Otherwise it returns at once.

The callee is documented on main in `RESIDENT_PALETTE_2A0D_GATE.md`. For
each call it does the following:

1. It writes `DS:[0x4A03]` = 0x10 and `[0x4A02]` = 0.
2. It writes `[0x4A05]` = value; for value ≥ 8 it is `(value + 8) & 0xFF`,
   computed through `xor ah,ah; add ax,8`.
3. It writes `[0x4A04]` = register.
4. It pushes AX (AL = 0x10) and the far pointer `DS:0x4A02`, then calls the
   DOS-unit Intr routine `0xA32:+000B`.
5. It ends with `pop bp; retf 4`.

## CPU model

Files: `include/poolrad/recovered/overlay_u3c6_0105_cpu.h` and
`src/recovered/overlay_u3c6_0105_cpu.c`, entry point
`pr_u3c6_0105_cpu_run(ram, ram_size, cpu, hooks)`. It uses the full-state
precedent conventions (1 MiB A20-off RAM, 14-register CPU).

- **Folding.** The entry body and `0x2A0D` are both executed exactly, with
  registers, arithmetic flags and stack/DS writes.
- **Hook.** The DOS-unit Intr (`0xA32:+000B`) is the one hook. It receives
  the full CPU and RAM after its call pushes, and execution continues only
  at `0x464:2A46`.
- **Preconditions.** The live call operands must be the original offsets,
  and both `0x2A0D` calls must target the same segment.
- **Safety.** Every write is alias-checked against both bodies, and both
  bodies are snapshotted after the hook.
- **AF after `xor ah,ah`** is written as 0.
- **LES/LDS.** The body has no LES or LDS, so the DOSBox-X boundary profile
  does not arise.

## Unresolved

- The Intr/BIOS INT 10h effects.
- The overlay stub dispatch.
- No compile, test, checker, native or oracle evidence.

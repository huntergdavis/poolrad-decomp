# U3C6 entry 3 (+0x0105): gated palette register pair (CPU model)

This note covers GAME.OVR unit `0x3C6` + `0x0105`, public entry 3, as a whole
function. The run-time stub is `0x3E0:0x2F`.

- **Status.** Source and focused test, not yet built or run (host hold), not
  counted. No native, full-parity or strict claim.
- **Ownership.** Relative CLEAR from pF, p5, p9, pG, pV, pC, pR, p6, pP,
  pD, pN and pH (i13466..i13682).

| Input | SHA-256 |
| --- | --- |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Body `[0105,0127)`, 34 bytes, file `0x3083F` | `94976ef2a24d913b603b17093f996d1099bf152b8af84e2e9e8a1730ead65644` |
| Resident `0x464:+2A0D`, 61 bytes (START image `0x0704D`) | `9a5d907528d68e38983cc795b891dfbc5bb20aa0f8053494f47e553edbfff0a8` |
| Intr `0xA32:[000B,006C)`, 97 bytes (START image `0x0A32B`) | `08854b63986cc7a20465b9594ceb0ac8d86c7467dccdcf49d894a04b79dccde9` |

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

Files: `include/poolrad/recovered/overlay_u3c6_0105_cpu.h`,
`src/recovered/overlay_u3c6_0105_cpu.c` and
`tests/recovered/test_overlay_u3c6_0105_cpu.c` (CTest
`poolrad_overlay_u3c6_0105_cpu_tests`). The entry point is
`pr_u3c6_0105_cpu_run(ram, ram_size, cpu, retf_policy, code_policy, hooks)`.
It uses the full-state conventions: 1 MiB A20-off RAM, a 14-register CPU,
per-byte alias stops, live re-reads, and a declared contiguous push/pop word
rule. Neither body has an LES or LDS.

### Code policy

- **`CODE_PINNED` (default).** Before anything runs, the model checks every
  byte of three ranges against the originals:
  - the entry body;
  - resident `[2A0D,2A4A)` at the live lcall segment;
  - Intr `[000B,006C)` at the live segment from `2A44`.

  Each range is compared through a 64-bit FNV-1a pin computed with its
  loader-fixup segment words skipped. Those words are entry `0118` and
  `0123` (raw `0x0464`) and resident `2A44` (raw `0x0A32`). They must satisfy
  `[0118] == [0123]` and `[2A44] - [0118] == 0x05CE`, so all of them were
  relocated by one load base. Otherwise the result is `CODE_NOT_ORIGINAL`
  with nothing changed.
- **Fixup evidence.** The derivation script is kept in ignored local
  evidence.
  - The decodes show that the only segment-valued fields are the three far
    call segments.
  - In native RAM images, 2A0D was loaded at segment `0x0C88`, and its only
    differing bytes are the `2A44` word (`0x1256`).
  - Intr was loaded byte-identical at segment `0x1256`, which satisfies the
    relation.
  - The U3C6 overlay was not loaded in those images. Its two words are fixed
    by the decode only.
- **`CODE_SYNTHETIC` (synthetic tests only).** This policy checks only the
  three call sites; every other instruction byte is unverified. It carries no
  parity meaning. The focused test uses it for every behavioural case,
  because no game bytes are in the repo, and exercises `CODE_PINNED` only as a
  rejection.

### Execution and Intr

- **Folding.** The entry body and `0x2A0D` run exactly, including registers,
  arithmetic flags and stack/DS writes. The argument reads (`[bp+6]`,
  `[bp+8]`) are live, so record stores that alias them feed the later reads.
  AF after `xor ah,ah` is written as 0, and `jae` is unsigned.
- **Intr policy.**
  - `INTR_UNMEASURED` is the default, and also applies with no hooks. The
    model stops at `0x464:2A41` before the far call, with the three argument
    words pushed (`INTR_PENDING`).
  - `INTR_HOOK_BOUNDED` adds checks at `2A41`, where S is the SP at Intr
    entry (entry SP − 0x16):
    1. The live argument words must be DI = `0x4A02`, the current DS and
       int `0x10`.
    2. The record `DS:[4A02,4A16)` must miss `SS:[S,S+10)`; otherwise the
       result is `INTR_FRAME`.
    3. The record must miss all three code ranges.
    4. For the caller-declared handler stack depth D, the region
       `SS:[S-20-D,S)` must miss every code range, the record, `IVT[0x10]`
       (`0000:0040..0043`) and the live frame `SS:[S, entry SP+4)`. Offsets
       wrap at 16 bits. Otherwise the result is `INTR_STACK_ALIAS`, stopped
       at `2A41` with nothing else changed.

    Intr's own scratch is `[S-20,S)`; the handler is entered at S−10.

    The frame check covers only `SS:[S,S+10)`. A record that overlaps the
    caller frame above S+10 is modelled live and is not pre-checked: the
    model's own record stores rewrite that frame, and the run ends wherever
    the rewritten frame leads. The wrap test case ends in `SYSTEM_TRANSFER`.
    This is modelled behaviour, not a parity claim.
- **Hook contract.** The hook includes the interrupt and Intr's RETF 6. The
  model accepts its result only if all of these hold:
  - it returns to `0x464:2A46` with SS unchanged and SP = S + 10;
  - every code byte is unchanged;
  - it leaves the outputs of Intr's original tail (`+0045..+006B`):
    - DS and BP are restored, ES = DS and DI = `0x4A16`;
    - AX = the stored Flags word `[4A14]`, and FLAGS = AX with DF clear
      (PUSHF precedes CLD);
    - BX, CX, DX and SI equal their record words.

  Any other result gives `CALLEE_TRANSFER`, `CODE_CHANGED` or
  `CALLEE_OUTPUT`.
- **Provenance.** A run through a hook is a modelled subset whose provenance
  is unknown. The model makes no claim about:
  - RAM below S after Intr;
  - BIOS data area, hardware or BIOS effects;
  - handler register effects beyond the relations above;
  - the hook's live inputs: the stale record words `[4A06..4A13]`, the
    interrupt number word and the vector read.
- **RETF policy.** Both RETFs (`+2A47 retf 4` and `+0126 retf`) follow the
  public U3C6 `+0000` values: `UNMEASURED` (stop before the first one
  reached), `WRAPPED_SS` or `CONTIGUOUS_PHYSICAL`.

## Unresolved

- The Intr/BIOS INT 10h effects, the handler stack depth, and the BIOS data
  area and hardware state.
- The overlay stub dispatch; first-view reachability is unproven.
- The overlay fixup words, which have no loaded-image check.
- No build, test-run, checker, native or oracle evidence yet.

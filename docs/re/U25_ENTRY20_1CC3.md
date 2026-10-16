# Unit 0x25 +0x1CC3: provisional full-state public entry 20

`pr_u25_entry20_1cc3` translates the complete U25 public entry 20 (far stub
`2B:0084`) over caller-owned A20-off 1 MiB RAM and a fourteen-word CPU
frame. Its three SYSTEM Real48 helpers are explicit fail-closed
register-ABI call-site callbacks. **No Real48 arithmetic is implemented
here, game identity is not asserted, and no strict full-C, identity, count
or native credit is requested.**

## Original boundary and ownership

Ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x25` starts at file `0x74D3`. The routine is exactly file
`[0x9196,0x92C3)`, unit `+1CC3..+1DEF`, **301 bytes / 81 instructions**,
SHA-256 `cfffb7e933808123e58434886f10226994973bdb120d358f09cfcf97f2ea90ca`.
It has no jumps and no locals, and its only return is `+1DED RETF 0xA`. It
follows a `RETF 2`, and `+1DF0`'s prologue follows directly.

The callers are two far calls through `2B:0084` at U1C `+1E1F` and `+1E6E`.
pG released this interval. No source, branch or sibling-clone file owned it
at `45bc73b`.

## SYSTEM Real48 helpers

The helpers were decoded from `START.EXE`. The image expanded by
`resident_loader_map.unpack` places SYSTEM `AF8` at image `0xAF80`, which
matches raw file base `0xAC76`. All three take their inputs in registers,
take no stack arguments, and end in a plain `RETF`.

| Entry | Effect | Failure |
| --- | --- | --- |
| `119A` → near `10D9` | LongInt `DX:AX` → Real48 `DX:BX:AX` (AL exponent, DH bit 7 sign); clobbers CH | always returns |
| `1186` → near `0F33` | Real48 `DX:BX:AX *= CX:SI:DI`; clobbers CX, SI and DI | CF → `11B4`: `AX=0xCD` (205), `JMP 00E2` |
| `119E` → `MOV CH,0`, near `1118` | Trunc Real48 → LongInt `DX:AX` (tiny exponents give 0) | CF → `11AE`: `AX=0xCF` (207), `JMP 00E2` |

`00E2` is the runtime-error exit. It pops the error address, sets `DS=0C7C`,
stores the exit code at `[441C]` and terminates, so it never returns to the
caller.

## Behavior

Arguments are the far record pointer `[BP+6]` and a 6-byte Real48 factor
`[BP+0xA..0xF]`. For each word k at record `+0x88`, `+0x8A`, …, `+0x94`:

1. `LES DI,[BP+6]`, load the word into AX, then `XOR DX,DX`.
2. Call `119A`.
3. Load the factor into CX, SI and DI. This is reloaded for every word,
   since the multiply clobbers those registers.
4. Call `1186`, then `119E`.
5. `LES DI,[BP+6]` again and store AX.

The pointer and the factor are re-read from live stack RAM for every word,
so writes by a hook or by aliasing take effect in source order. The record
can even overlap the saved BP, the return words or the factor.

The 21 call sites are `+1CD0`, `+1CDE` and `+1CE3`, plus `0x2A·k`.

The hook returns 1 when the helper returns, and the wrapper then continues
at the next instruction. A hook that returns 0 means the helper did not
return (the RunError exits, or a host failure). The routine then stops with
`PR_U25_1CC3_NO_RETURN`, with IP at that call site and nothing after it
executed.

The only flag writer in the body is `XOR DX,DX`. It clears CF, OF and SF,
sets ZF and PF, and leaves AF 0 as in the reference emulator, since AF is
architecturally undefined. The `119A` hook observes these flags. The return
carries the last `119E` flags.

## Verification

`test_overlay_u25_entry20_1cc3.c` uses synthetic register stand-ins, not
Real48, that clobber CX, SI, DI and ES. It checks:

- all 21 call sites in order
- the per-word factor reloads, the seven stored words and the untouched
  neighbouring words
- the return frame, ES:DI and flags
- `NO_RETURN` at each of the 21 calls, with the completed blocks stored and
  nothing after the failing call
- argument rejection

It passes strict C11 `-Wall -Wextra -Wpedantic -Werror` and ASan+UBSan.

`check_overlay_u25_entry20_1cc3.py` pins:

- the file and body hashes, the 81 instructions and 21 call targets
- no jumps and the sole `RETF 0xA`
- the neighbouring bytes
- the three helpers' plain `RETF` endings

Its source pins have run statically. Its Unicorn differential has **not**
run and waits for a PM serial slot. When run, it compares the C against
original-byte execution with the same deterministic register stand-ins over
69 cases:

- word patterns × factors × stack positions
- record-overlaps-frame aliasing
- 40 seeded cases

For each case it compares all 1 MiB of RAM, the CPU words and every call
trace, and requires every instruction start. Only returning paths are
compared. Exact Real48 semantics and native caller parity remain open.

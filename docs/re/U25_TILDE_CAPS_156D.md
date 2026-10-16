# Unit 0x25 +0x156D: provisional full-state tilde-caps string filter

`pr_u25_tilde_caps_156d` translates the complete U25 routine `+156D` over
caller-owned A20-off 1 MiB RAM and a fourteen-word CPU frame. All seven
SYSTEM runtime calls are explicit fail-closed call-site callbacks. The name
describes the observed string effect only; **game identity is not asserted,
and no strict full-C, identity or count credit is requested.** Whole credit
remains for PM review.

## Original boundary and data ownership

Ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x25` starts at file `0x74D3`. The executable routine is exactly file
`[0x8A40,0x8B37)`, unit `+156D..+1663`, **247 bytes / 98 instructions**,
SHA-256 `eb9f88fcfcdfff0e1b561428c0dedaa730b51b400a64e2aa66d8954e9d8a21b8`.
Its only return is `+1661 RETF 8`, and all jumps are internal.

The ledger CSV's 279-byte bound (255 decoded, 102 instructions) counts the
32 bytes `+1664..+1683` as code. They are a set constant owned by `+1684`.
Its `MOV DI,1664` at `+1717` is the only reference to them in the unit, and
`+1684`'s prologue follows them.

This routine's own set constant is the 32 bytes `+154D..+156C`, just before
it. They are referenced only by its `MOV DI,154D` at `+15A4`. Turbo Pascal
places each typed set constant immediately before its user. The two tables
happen to be byte-identical. Neither table is embedded in this repository:
the set-test callee reads the constant from `CS:154D` in caller RAM.

The callers are two near `PUSH CS; CALL +156D` sites: `+16BF` in `+1684`
(public entry 13) and `+17C3` in `+1788` (public entry 14). A scan of all U25
ledger functions found no other caller.

## SYSTEM callees

Each was decoded from `START.EXE` at SYSTEM `AF8` (file base `0xAC76`).

| Call | Decoded effect | Return |
| --- | --- | --- |
| `0DC0` | set membership: ZF clear when the char's bit is set | `RETF 6` |
| `0CBC` | `Delete(s, index, count)` through SYSTEM Copy, Concat and Store | `RETF 8` |
| `182A` | `UpCase`: `a..z` minus `0x20` in AL | `RETF 2` |
| `0B0B` | StrLoad, destination retained | `RETF 4` |
| `0C3B` | CharStr, destination retained | `RETF 2` |
| `0B98` | StrCat, capped at 255, destination retained | `RETF 4` |
| `0B25` | StrStore under the cap word | `RETF 0xA` |

## Behavior

Arguments are the far destination string `[BP+6]` and the far source string
`[BP+0xA]`. The frame is `SUB SP,0x202`: index `[BP-1]`, original length
`[BP-2]`, a character string at `[BP-0x102]` and a temporary at
`[BP-0x202]`.

1. Clear the destination length and read the source length once.
2. If the length is zero, return. The last flags are those of `CMP 1,len`.
3. Otherwise, for i = 1 up to the original length, ending when i equals it:
   - If `src[i]` is in the set, add `0x20` to it in place.
   - If `src[i]` is now `~`:
     1. `Delete(src, i, 1)`.
     2. `src[i] = UpCase(src[i])`.
     3. `dest = dest + src[i]`, built in the temporary and stored with cap
        `0x28`.

The loop keeps running to the original length after deletions. It then
reads and rewrites stale bytes past the shortened source string, and can
append them too. The translation keeps this exactly. Every pointer, index,
length and string byte is re-read from RAM where the original reads it.
Callee or alias writes therefore take effect in source order.

Flags are exact wherever a callee or the return observes them:

- **Set-test call:** the `ADD DI` before it.
- **Delete:** the `XOR AH,AH` before it.
- **UpCase, StrLoad and CharStr:** the `ADD DI` before each.
- **Concat and StrStore:** the preceding callee's returned flags.
- **Return:** the `CMP 1,len` or the final equal `CMP i,len`.

The `ADD AX,0x20` and `INC` flags are dead. `XOR` uses the reference
emulator's cleared AF, which is architecturally undefined.

## Verification

`test_overlay_u25_tilde_caps_156d.c` uses a synthetic runtime with the
decoded effects. It checks:

- a worked example with three tildes
- 300 seeded strings of length 0 to 39 against an independent reference
  model of the whole 256-byte source buffer and the destination, including
  the stale-tail effects
- zero length and its final flags
- an abort at each of the seven call sites
- argument rejection

It passes strict C11 `-Wall -Wextra -Wpedantic -Werror` and ASan+UBSan.

`check_overlay_u25_tilde_caps_156d.py` pins:

- the file, body and StrStore hashes
- all 7 call targets, internal jumps and the sole `RETF 8`
- the unique `154D` and `1664` references and `+1684`'s prologue
- all seven SYSTEM return encodings

Its source pins have run statically. Its Unicorn differential has **not**
run and waits for a PM serial slot. When run, it covers 109 cases:

- 11 strings × the original, upper-case and random sets × stale or constant
  tails
- SS-alias and wrapped-frame cases
- 40 seeded cases

For each case it compares the C against original-byte execution over all
1 MiB of RAM, the CPU words and every call trace. It requires every
instruction start and every conditional outcome to be hit.

No reached native `+156D` frame was searched. Runtime and caller native
parity remain open.

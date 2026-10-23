# Unit 0x1c +0x0234: ECL RANDOM (0x08), bounded model

Opcode `0x08` ([ECL_CONTRACTS.md](ECL_CONTRACTS.md), 2 operands) dispatches to unit `0x1c`
`+0x0234..+0x0291`, file `0x0F7A..0x0FD7`, 94 bytes, 35 instructions, four bytes of locals,
`retf`. SHA-256 of the span
`a634c69e97fe005c2041e9d18d0874d0eb8ede7f4a59833bfb642918b7c5d979` (GAME.OVR, 206,105 bytes,
`872964a3…7928614`). The next routine (`+0x0292`) is SAVE. **Classification: bounded model**;
the original hook API is retained, and an explicit-seed adapter now covers the
three runtime calls for the reached byte range. Lane-A core hooks remain,
and the seed cannot be derived from the save alone. No new strict full-C,
identity, or end-to-end saved-Up parity claim is made. The earlier `deja` search found no matching trace; the
new search `deja "Poolrad ECL RANDOM unit 0x1C +0x0234 SYSTEM 0xAF8 11C0 119A 119E seeded runtime trio"`
also found no exact match. Reused and checked [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md)
for the seed recurrence and [U9E_ENTRY8_ROLL.md](U9E_ENTRY8_ROLL.md) for
the hash-locked generator, low-byte range, and caller mapping.

## Original order (all 35 instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x023A..+0x023D` | loader(2), `0x2B:0x2A` |
| `+0x0242..+0x024A` | `v := AL` of slot value(1), `0x2B:0x25` |
| `+0x024D..+0x0253` | `cmp byte v, 0xFF; jae +0x0256`, else `inc byte v` (saturating at 0xFF) |
| `+0x0256..+0x0263` | `dest := word([0x7087], [0x7047])` via `0x2B:0x4D` (slot 2) |
| `+0x0266..+0x026C` | push the word `v` (`xor ah, ah`); `lcall 0xAF8:0x11C0` |
| `+0x0271..+0x0273` | `xor dx, dx`; `lcall 0xAF8:0x119A` |
| `+0x0278..+0x027D` | `lcall 0xAF8:0x119E`; `[bp-2] := AL` |
| `+0x0280..+0x0289` | push `dest`, push the zero-extended AL; `lcall 0x2B:0x52` (store word) |
| `+0x028E..+0x0291` | return |

## Model

`pr_u1c_op_random` (`overlay_u1c_random_0234.{h,c}`): the three `0xAF8` calls are separate
fallible hooks in this order (NULL = HOOK, nonzero = CALLBACK, neither stores anything).
The legacy hook ABI is only what the instructions show: the first gets the stack word,
the second the DX value (0), the third supplies AL. That API does not convey AX/DX
between hooks; the seeded adapter below supplies their reached behavior. The store word reports its status after the
attempt (HOOK = word written). Loader and slot-value BOUNDS/UNRESOLVED stop before any
hook; a class-1 slot read runs to completion then reports HOOK (PICTURE pattern).

Tests (`test_overlay_u1c_random_0234.c`) are fully synthetic: every slot byte 0..255 for the
saturating increment, hook order and arguments, the zero-extended store; a failing and a
missing hook at each step with nothing stored; destination classes (window 2, window 0,
exempt and non-exempt class 1, unsupported class); loader and slot failures; a class-1
slot read; a word slot value (low byte). The fabricated hook result is not a generator.

## Decoded SYSTEM chain and seeded adapter

`pr_u1c_op_random_seeded` calls the existing handler with private,
deterministic implementations of its three runtime hooks. It takes the
caller-owned DWORD `RandSeed` at DOS DS:`0x4428..0x442B`; the save does not
contain that seed. It reuses `pr_u9e_entry8_roll` for one `+0x11C0` call,
then applies the proved conversion identity below. The original public
hook API remains available.

The ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Exact loaded-image spans, file ranges (exclusive end), and hashes:

| SYSTEM span | Loaded image / file | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| `+0x119A` wrapper | `0xC11A..0xC11E` / `0xBE10..0xBE14` | 4 / 2 | `e962b3c25deea98a57e98aac984c325f9cfc8dacebb4c508fdd5943756fad95a` |
| `+0x119E` wrapper | `0xC11E..0xC126` / `0xBE14..0xBE1C` | 8 / 4 | `072143d5fffbb6d2bdebaf88a86cbff0b593feaf31aaae45fbe5c312144926cc` |
| `+0x10D9` encode body | `0xC059..0xC098` / `0xBD4F..0xBD8E` | 63 / 30 | `64b69941b112ea462aea3848edd98e4f3ec85523f1b631a38fda67942f1c9251` |
| `+0x1118` decode body | `0xC098..0xC0F4` / `0xBD8E..0xBDEA` | 92 / 44 | `7ccd6539f95a342fa34f081470130d1ed34749d7761369cb05dc02a5723c1234` |
| `+0x11C0` random | `0xC140..0xC156` / `0xBE36..0xBE4C` | 22 / 10 | `e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9` |
| `+0x120F` seed | `0xC18F..0xC1C5` / `0xBE85..0xBEBB` | 54 / 22 | `f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242` |

At `+0x11C0`, `+0x120F` first advances the seed as
`(seed * 0x08088405 + 1) mod 2^32`. With a nonzero WORD range,
`DX:AX = 0:high16(new_seed)` is divided unsigned by that range; after
`xchg DX,AX`, **AX is the remainder** and **DX is the quotient**.
`div` writes the pinned DIVW flags (via the shared SYSTEM random model). Pinned DOSBox-X `DIVW`/`IDIVW`/`DIVB` write all six arithmetic flags (AF=SF=OF=0, ZF=(rem==0 && quo odd), CF=((rem&3) in 1..2), PF over rem:quo; `instructions.h:635-761`), measured natively in [U33_PARTY_ATTACH_12E1_CPU.md](U33_PARTY_ATTACH_12E1_CPU.md) (256 IDIV checkpoints, all FLAGS bits unmasked); the model writes those values. With range zero, division is
skipped: seed still advances, AX=0, DX=high16(new seed), and the preceding
`or bx,bx` cleared carry. This also holds for a large WORD range such as
`0xFFFF`: the result is `high16(new_seed) mod range`.

The ECL handler never sends range zero or greater than 255:
`+0x024D..+0x0253` increments the slot's low byte unless it is 255;
`+0x0269` clears AH. AX from `+0x11C0` is therefore `0..254`.
The handler's `xor dx,dx` at `+0x0271` discards the quotient and clears
carry before `+0x119A`. That wrapper calls `+0x10D9`, encoding the
nonnegative DX:AX longint into Turbo Pascal Real register form. The
`+0x119E` wrapper sets CH=0 (truncate), calls `+0x1118`, and branches
on carry to a runtime error only for conversion failure. For every
AX in `0..254` with DX=0, the encode/decode shift paths return the
same AX, DX=0, carry clear: zero takes the zero path; positive values
normalize into exponent `0x88-k` with `k=0..7`, then shift right
`8+k` places with a zero last carry. Thus the runtime error branch is
unreachable for the handler's supported values. At `+0x0286`, the
handler clears AH and stores the zero-extended AL via `0x2B:0x52`.

`tools/overlay/overlay_u1c_random_runtime_audit.py` locks the handler
and all six SYSTEM spans, checks call/store sites, and restates the
reached register shift path for all **255** AX values `0..254`.
Synthetic seeded tests invert the odd multiplier to force each possible
range-255 result, checking the next seed and stored WORD. They also cover
range 1, a supplied range-14 seed, reached-only missing seed, truncated
operands before RNG, and a store hook after RNG.

The independent native [one-Up RANDOM sample](SLOT_A_ONE_UP_NATIVE_RANDOM_SEED.md)
measured seed **3140872353 → 3238563622** for pushed range 14,
`+0x11C0` AX=10, `+0x119A` input AX=10/DX=0, its output
AX=132/CX=5/DX=8192, `+0x119E` output AX=10, and the window-2
`0x980F` word **16 → 10**. This is live evidence for one reached path.
The new synthetic fixture supplies that measured seed and checks the same
seed transition and stored word in the isolated C handler. It does not
replay the full Up route or derive the seed from the save. The adapter
remains a bounded model because loader, slot, and store can report
unresolved lane-A hooks. No new function-count credit is claimed.

## Earlier private supplied-state probe (documented, not committed)

Hash-pinned slot A (`abca2483…c08f1`), run entry `0x9914`, search entry `0x9975`; handlers
bound: SAVE, COMPARE, IF, GOTO, AND/OR, EXIT, PICTURE, PRINT/PRINTCLEAR, ON GOTO/GOSUB,
PARTYSTRENGTH (explicit empty host list) and this one; explicit `[0x84E2] = 0`, PICTURE DS
bytes 0. **The RANDOM result is an explicit host input (AL = 0), not a saved or generated
value.** At `0x9B3A`: slot byte 13, argument 14, three hook calls, AL (host) stored as a
word to the window-2 word `0x980F` (OK). The later native sample above measured AL=10 for
one Up; this earlier supplied-AL probe remains only a model reachability check.

**Next stop**: dispatch 25, run status HANDLER / handler HOOK, PC `0x9B46`, opcode `0x03`
COMPARE (previous opcode `0x08`), whose class-1 operand is a word load from `0x6DCA` (load
hook, not exempt; not audited). Supplied-state probe: no DOS reachability, display or
one-Up parity.

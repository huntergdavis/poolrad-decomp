# Slot-A authentic party input and the RANDOM input at the search entry

This note records how the slot-A world save builds its loaded party, a private numeric
PARTYSTRENGTH over that party, and what the runtime RANDOM input at saved PC `0x9B3A`
requires. Names and record bytes stay in ignored `resources/`; only counts, hashes, addresses
and aggregate numbers are published. **No strict full-C, count, identity, DOS/native parity or
one-Up claim is made.** `deja` found only the earlier save-frame, character and
PARTYSTRENGTH notes; they were reused ([DOS_SAVGAM](../formats/DOS_SAVGAM.md),
[DOS_CHARACTER](../formats/DOS_CHARACTER.md), [SLOT_A_NATIVE_SAVE_ADAPTER](SLOT_A_NATIVE_SAVE_ADAPTER.md),
[RESIDENT_PARTY_COUNT](RESIDENT_PARTY_COUNT.md), [U1C_PARTY_STRENGTH_12C6](U1C_PARTY_STRENGTH_12C6.md)).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| slot-A world save | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| record for slot 0 | 285 | `d7ba1321e1fca8464d1220e77c26ce92173d60cb67b0f155c71868926cfa6c8e` |
| record for slot 1 | 285 | `6e82edf77f8c213f76b184f0c0eb2af6a42d2ed5bd7150d330031f78edb44a12` |
| record for slot 2 | 285 | `0dd9737e34c75c0a3c41bb1ba3f60a627d00c391e4e261751369961ce9fd726d` |
| record for slot 3 | 285 | `a8596f9be05c439a674f4c420053a9d0d3f27e5cb2106c2b17a496c1847e012a` |
| record for slot 4 | 285 | `0dd24d3201adc5d50db4aa4c2ab121d55eaf1bcc1a0a491a93743f8871d56062` |
| record for slot 5 | 285 | `51a2a026381993f4d2fb8a1a3bb9254eb43450e4061a34640433a619dd2e45b8` |

## The original loader (unit `0x6f` entry 8, `+0x1602`), checked against the bytes

* **Count and slots.** After the ten transfers (`+0x17D5..+0x18DA`) the local character count is
  world-save byte `0x3208` (**6** for slot A) and the eight 41-byte basename slots start at
  `0x3209` (stride `0x29`, `mov dx,0x29; mul dx`, `+0x194D..+0x1950`). Slots 0..5 each hold an
  8-byte Pascal basename that names one of six ignored character files; slots 6 and 7 are not
  used with this count. The slot array is the loader's local `[bp-0x171 .. ]` with the loop
  variable starting at 1 (`+0x1926..+0x1931`), so iteration `i` reads slot `i-1`.
* **Order.** For `i = 1..count` (`+0x1937..+0x19F4`): compose the basename and the `.sav` literal
  (`cs:0x15E9`), test existence (`lcall 0x802:0x2EB`, `+0x19A8..+0x19AF`), `GetMem(0x11D)`, call the
  record loader `+0x0AEC`, then the party insertion `+0x1446`, then `call +0x02AE`.
  Insertion appends at the **tail** (follow links at `+0x104/+0x106`, store the new pointer into
  the last link, `+0x147A..+0x14B9`), so the party order is the slot order.
* **Pointer clears.** The record loader clears the far-pointer slots `+0x7F..+0x82`,
  `+0xC8..+0xCB` and `+0x104..+0x10B` right after the 285-byte read (`+0x0C54..+0x0C8A`).
  In the supplied files these slots hold nonzero stale values, so the clear matters.
* **Slot allocation.** `+0x1446` sets `+0xBF := 0xFF`, marks the slots used by the list, and picks the
  first free index below eight (`+0x153E..+0x1563`): `0, 1, 2, 3, 4, 5` for six present members.
* **Counter.** The window-1 word at script address `0x6E3E` (`[0x49D6]+0x67C`) is zeroed before the
  loop (`+0x1917`) and incremented per inserted member (`+0x1569`): **6**, equal to the value saved in the
  world file (saved word `0x6E3E` = 6), a consistency check.
* **Not covered.** The unresolved resident far call `0xAC:0x25` after each insertion (record far
  pointer pushed, `+0x1574`) and `call +0x02AE` (468 bytes) are not modelled. A search of the decoded
  instructions of `+0x02AE`, `+0x0482`, `+0x0AEC` and `+0x1446` finds no write to record offsets `0x96`,
  `0x9B`, `0x110`, `0x111` or `0x11B` (only `+0xBF` is touched, in `+0x0482` and `+0x1446`); but
  `0xAC:0x25` is undecoded, so a write there is **not excluded**. The sidecar (`.itm`/`.spc`) loads are not
  modelled.

## Host model and private numeric result

`overlay_u6f_party_load.{h,c}` (bounded host model; synthetic test
`test_overlay_u6f_party_load.c`) parses the count and Pascal slots, applies the pointer clears and the
`+0xBF` allocation, appends in order, skips absent files (taking no slot) and builds the
`pr_u68_0236_node` chain the merged PARTYSTRENGTH model consumes.
`tests/replay/check_slot_a_party_input.sh <ignored dir>` hash-pins the world save and the six records
and prints only aggregates. For slot A it reports:

```text
count=6 present=6 loaded=6 slots=0,1,2,3,4,5 strength_status=0 visited=6 strength=10 saved_window1_counter=6
```

PARTYSTRENGTH reads `+0x11B`, `+0x111`, `+0x110`, `+0x09B`, `+0x096` (none of them cleared or
rewritten by the loader pieces decoded above). The six per-member quotients sum to **10**, stored as
the word `10` to the operand `0x9808`; an independent Python restatement of the instruction formula
over the raw bytes gives the same value.

## RANDOM input at saved PC 0x9B3A (audit)

* **Operands.** The opcode is `08` with slot 1 the literal byte `13` (`0x0D`) and slot 2 the window-2
  word `0x980F`. The handler (`+0x0234`) increments the byte (saturating) to **14**, calls
  `0xAF8:0x11C0` with the word 14, then `0x119A`, `0x119E`, and stores AL zero-extended to `0x980F`.
* **Runtime contract** (resident segment `0xAF8`, START.EXE image offset `seg*16 + off`):
  `0x11C0` calls the generator step `+0x120F` (32-bit `RandSeed` at `DS:0x4428..0x442B`;
  `seed := seed * 0x08088405 + 1`, the multiplier built from the word `0x8405` at `cs:0x1245` plus
  shifts) and returns `high word of the new seed mod range` (`xchg dx,ax; div bx`, remainder in AX);
  range 0 returns 0. `0x119A` is the longint-to-Real conversion (`+0x10D9`) and `0x119E` the
  truncating Real-to-longint conversion (`+0x1118`, `ch = 0`), an exact round trip for 0..13.
  So **AL is in 0..13**. Values 14..255 cannot occur from this call (this is a decode inference, not a DOS
  observation).
* **Seed.** `Randomize` (`0xAF8:0x1247`: `mov ah,2Ch; int 21h` into `RandSeed`) has one call site in
  GAME.OVR, unit `0x36` `+0x0602` (file `0xEBD5`). The seed is therefore the DOS clock at start-up,
  advanced by every earlier generator use: not in the save, not derivable from it.
* **Route sensitivity** (private supplied-state probe, authentic party, every AL 0..255 supplied):
  the script compares the stored AL with 12 (`COMPARE 0x980F, 12` at `0x9B60`, then `IF`). **AL 0..12
  reach the EXIT at `0x9B67`** (run status OK, 31 dispatches, `[0x5E24] = 1`, `[0x5E25] = 0x11`). **AL 13
  (the only reachable value above 12) reaches opcode `0x06` (DIVIDE, `+0x01A3`, 3 operands) at PC `0x9B68`**, which is
  unmodelled; AL 14..255 give the same stop but cannot occur. The route to this point also assumes the
  other supplied inputs (`[0x84E2] = 0`, PICTURE DS bytes 0).
* **Authentic capture.** What a DOS oracle must provide is `RandSeed` (`DS:0x4428..0x442B`) at the moment
  before the `0xAF8:0x11C0` call at this opcode, or the AL result; the DOSBox-X harness here
  (`tools/oracle/`) only drives keys and screenshots, so reading that memory needs its debugger (a
  breakpoint at the call) and a controlled DOS clock. **Not attempted**, and it additionally needs the
  separate, still unproven fact that the DOS one-Up run reaches the search entry. No generator is
  implemented here and no AL is invented: the probes above are sweeps over supplied values.

## Next gate

Model opcode `0x06` DIVIDE (`+0x01A3`, 3 operands) only if the AL = 13 branch is wanted; and obtain the
authentic `RandSeed` (or AL) from a private DOS oracle run before claiming which branch the real game
takes. The undecoded `0xAC:0x25` call after each party insertion is the remaining gap in the party
input.

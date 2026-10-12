# Unit 0x9E entry 8: byte-count random roll

`GAME.OVR` unit `0x9E`, public entry 8 at `+0x0DE3..+0x0E2D`, is a
75-byte, 29-instruction routine. Its next public entry begins at `+0x0E2E`.
The C translation is `pr_u9e_entry8_roll`. It includes the called SYSTEM
random step as an explicit caller-owned mutable 32-bit seed, so there is no
arbitrary random-result callback. This is an isolated routine translation;
the saved one-Up route, initial DOS clock seed, and strict function accounting
remain unproven here.

Before decoding, `deja "Poolrad unit 0x9E entry 8 +0x0DE3 0xAF8:11C0 roll RNG"`
found no exact prior session. Reused and independently checked the unit-stub
mapping and call context in [U68_ROLL_GATE_0294.md](U68_ROLL_GATE_0294.md) and
[U68_MOVEMENT_FLAG_04DF.md](U68_MOVEMENT_FLAG_04DF.md). The earlier
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md) identified the SYSTEM seed
recurrence and its DOS clock initialization; this audit re-decoded the two
SYSTEM spans and checked that recurrence against 16-bit register emulation.

## Hash-locked extent and ABI

Ignored DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Span | File range, exclusive end | Size / instructions | SHA-256 |
| --- | --- | --- | --- |
| unit `0x9E:+0x0DE3` | `GAME.OVR` `0x2BCCD..0x2BD18` | 75 / 29 | `7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba` |
| SYSTEM `0xAF8:11C0` | `START.EXE` `0xBE36..0xBE4C` | 22 / 10 | `e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9` |
| SYSTEM `0xAF8:120F` | `START.EXE` `0xBE85..0xBEBB` | 54 / 22 | `f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242` |

The Pascal far-call frame has count WORD at `[bp+8]`, range WORD at
`[bp+6]`, and `retf 4`. The code reads only the low byte of each WORD.
At `+0x0DED`, zero effective count jumps to `+0x0E1F`: AL=0, no random call,
and no seed effect. For count 1..255, the loop at `+0x0E00..+0x0E1D`
pushes the zero-extended range byte, calls `0xAF8:11C0`, increments AX,
adds its low byte to a byte accumulator, and repeats exactly count times.
The return AL is that accumulator modulo 256. The count-loop comparisons
are unsigned byte comparisons; the `+0x0DFE` upper guard cannot fire after
the first nonzero count snapshot but is covered by the same finite loop.

SYSTEM `+0x11C0` calls `+0x120F` **before** testing the range. The generator
reads and rewrites DS words `0x4428` and `0x442A` as a little-endian DWORD:

```text
new_seed = (old_seed * 0x08088405 + 1) modulo 2^32
random_word = 0 if range == 0 else high16(new_seed) modulo range
```

The multiplier's low word is at CS:`+0x1245`; the shifts and adds in
`+0x120F` produce the upper half. At `+0x11C0`, unsigned `div bx`
leaves the remainder in DX and `xchg dx,ax` returns it in AX. Range zero
still advances the seed once and returns zero. The caller's range is at
most 255, so each returned word is at most 254 before the increment.
The only nonstack writes in the entry 8 body are through these SYSTEM seed
words; it returns its byte result in AL.

The audit counts **122 fixup-confirmed far call sites** to `0xB0:0x0048`
in 15 overlay units. Unit `0x68` contributes 13: 12 in the roll-gate body
and `+0x0513` in the movement-flag body. The older callgraph fan-in of 127
also includes five raw near-call candidates within unit `0x9E`; those are
not counted as confirmed callers here. Static call sites do not establish
which branch a particular saved one-Up run took.

## Host behavior and verification

The API takes count/range WORDs, `uint32_t *rand_seed`, and an AL output.
The seed pointer represents the DOS DS DWORD; the host must provide its
actual starting value. If effective count is zero, a NULL seed is allowed.
For positive count, a missing seed stops before the first generator step;
missing output stops before any effect. Those are host bounds, not DOS
branches. The report contains effective bytes, number of generator calls,
and final seed when supplied.

Synthetic tests cover count 0/1/2/3/255, range 0/1/6/255, low-byte WORD
argument truncation, AL wrap, intermediate seed after each single roll,
missing seed and output, and an optional report. The expected values come
from an independent 16-bit register emulation of the decoded generator.
`tools/overlay/overlay_u9e_entry8_roll_audit.py` verifies both input hashes,
all three span hashes and contiguous instruction counts, ABI and branch
sites, the SYSTEM reads/writes and multiplier, and the fixup-confirmed
caller aggregate. It emits only addresses, hashes, and counts.

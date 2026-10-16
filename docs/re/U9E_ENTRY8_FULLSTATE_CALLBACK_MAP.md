# U9E entry8 full-state callback ownership map

This map describes what a U7F B0:0048 callback must reproduce before
the conditional caller composition can represent the original callee.
It is a source and ownership gate, not a U9E full-state C translation.
The bounded `deja` query for U7F composition and U9E entry8 callback
timed out without a finding. The exact spans and fixup are reused from
the checked-in [entry8 audit](U9E_ENTRY8_ROLL.md) and were decoded again
from the ignored owner-supplied images. The [entry9 callback evidence](U9E_ENTRY9_ROLL_LATCH_0E2E.md)
is a separate wrapper precedent.

| Original span | File bytes | Size / starts | SHA-256 |
| --- | --- | ---: | --- |
| U9E entry8 `+0DE3..+0E2D` | `GAME.OVR [0x2BCCD,0x2BD18)` | 75 / 29 | `7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba` |
| SYSTEM random `+11C0..+11D5` | `START.EXE [0xBE36,0xBE4C)` | 22 / 10 | `e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9` |
| SYSTEM seed `+120F..+1244` | `START.EXE [0xBE85,0xBEBB)` | 54 / 22 | `f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242` |

The complete `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
the complete `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The existing audit confirms unit stub paragraph `0xB0`, public entry
index 8 at `+0DE3`, the entry9 boundary `+0E2E`, and the far-call
fixup at U9E `+0E0E` to SYSTEM `0AF8:11C0`. The local audit also
reports 122 fixup-confirmed far callers of B0:0048.

## Frames and control transfers

At the U7F caller entry let the original SS:SP be `S`. The merged
[prefix](U7F_ROLL_PREFIX_0FD2.md) saves BP, reserves two local bytes,
then pushes count and range WORDs while retaining incoming AH in their
high bytes. Thus before the call, SP is `S-8`. The original `LCALL`
at U7F `+0FDE` pushes return CS and IP, entering B0:0048 at `S-12`:

| Stub-entry SS offset | Source-owned value |
| --- | --- |
| `SP` | return IP `+0FE3` |
| `SP+2` | U7F caller CS |
| `SP+4` | range WORD, low byte `0x14` |
| `SP+6` | count WORD, low byte `1` |

All offsets wrap at 16 bits; each word's bytes are physically
contiguous with A20 disabled. The B0:0048 dispatch maps to U9E public
entry8, but the loaded code segment and dispatch side effects must be
established by the future callback implementation. At U9E `+0DE3`,
`PUSH BP` places `[BP+6]` over the range WORD and `[BP+8]` over count.
U9E reserves four local bytes. It snapshots the count into `[BP-4]`,
keeps the accumulating sum at `[BP-3]`, loop index at `[BP-2]`, and
final AL copy at `[BP-1]`.

For each nonzero-count iteration, U9E `+0E08` rereads the range byte
from `[BP+6]`, zeroes AH, pushes that range WORD and far-calls SYSTEM
`0AF8:11C0` at `+0E0E`. The SYSTEM call frame holds return IP
`+0E13`, U9E return CS, then the range WORD. SYSTEM `+11C0` calls
`+120F` before its range-zero branch and ends with `RETF 2`, returning
to U9E `+0E13` while discarding the range argument. U9E finishes with
`MOV SP,BP; POP BP; RETF 4` at `+0E28..+0E2B`, returning to the U7F
`+0FE3` continuation and discarding U7F's two pushed WORDs. An abort
or other destination must remain an explicit transfer in the U7F API.

## Effect ownership

| Effect | Checked-in coverage | Outstanding full-state owner |
| --- | --- | --- |
| U7F prefix, literal far-call frame, return classification | [prefix](U7F_ROLL_PREFIX_0FD2.md), [call adapter](U7F_ROLL_CALL_0FDE.md), [continuation](U7F_ROLL_RETURN_0FE3.md), and merged conditional composition PR630 | The B0:0048 callback supplies all callee effects. |
| Overlay stub and U9E loaded-entry transfer | Descriptor/fixup mapping in [entry8 audit](U9E_ENTRY8_ROLL.md) | Stub dispatch and resulting live CS:IP, RAM and CPU14 state. |
| U9E count/range/AL aggregate | [entry8 helper](U9E_ENTRY8_ROLL.md) computes low-byte count/range and returned AL | Every U9E instruction's stack locals, registers, flags, branch and RETF4 effects. |
| SYSTEM generator arithmetic | Entry8 helper computes seed recurrence and AL aggregate | Exact +11C0 and +120F stack, AX/BX/CX/DX, FLAGS, DS RAM writes and return effects. |
| DS seed DWORD at `4428/442A` | Entry8 helper accepts a detached `uint32_t *` | Reads and writes in the same 1 MiB RAM as SS frames, in source order, including aliases. |

The generator advances the DS seed as
`seed = seed * 0x08088405 + 1 (mod 2^32)` and derives the random WORD
from `high16(seed) % range`, or zero when range is zero. Count zero
skips SYSTEM entirely and leaves the seed untouched. This arithmetic
is already verified for the [entry8 helper](U9E_ENTRY8_ROLL.md), but
its `uint32_t *` interface does not carry CPU14 or the live stack.
The original body rereads range on every iteration; a DS seed write
can alias a range argument, stack local, or return word in the 1 MiB
address space. A fixed range argument or detached seed copy is not a
general full-state callback. Likewise, final FLAGS and non-AL register
state depend on executed U9E/SYSTEM instructions and must be measured,
not filled from the AL helper.

The existing [entry9 wrapper evidence](U9E_ENTRY9_ROLL_LATCH_0E2E.md)
has 96 full-state comparisons with a hooked entry8 callback, 16 cases
that execute original U9E entry8 while hooking SYSTEM, and eight native
synthetic wrapper frames with a deterministic callee. Those results do
not provide a natural U7F frame or full CPU14 parity for the original
U9E plus SYSTEM chain. Entry9's callback leaves the frame for its
wrapper to emulate RETF4; the U7F +0FDE callback contract instead owns
RETF4 before returning to U7F +0FE3. These interfaces cannot be used
interchangeably without an explicit frame adapter.

## Gates before callback closure

1. Preserve a separate U9E full-state source candidate: U9E 75 bytes,
   SYSTEM +11C0 22 bytes, and +120F 54 bytes with exact original-byte
   frame and alias tests. Start with a PM-approved small controlled
   probe under the current host hold; do not infer CPU14 from AL/seed.
2. Verify B0:0048 dispatch and loaded-entry state, then compose the
   callback with U7F's existing full-state hook. Check returning,
   abort and transfer paths with full RAM/CPU14 and stack-wrap decoys.
3. Seek a natural reached U7F frame only when native execution is
   permitted. Current host hold prohibits a new DOSBox-X run. No
   natural reach, complete U7F/U9E path, identity, or strict full-C
   credit follows from this map or the synthetic U7F composition.

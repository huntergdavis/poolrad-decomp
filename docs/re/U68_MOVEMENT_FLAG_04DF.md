# Unit 0x68 internal +0x04DF: list-gated movement flag

`pr_u68_movement_flag` models the entire DOS v1.3 `GAME.OVR` body at unit
`0x68:+0x04DF..+0x05AA`. It walks the far list at DS:[0x5D96], invokes
an external roll once per reached node, chooses the first node whose
threshold admits the roll and whose byte `+0x10C` is zero, clears
DS:[0x6DDF], and on a hit invokes local `+0x0109` twice. The two callees
remain explicit hooks; this is a **bounded model**, not a strict full-C
translation. No game-identity or strict count is claimed, and no saved
one-Up execution through this routine is established.

Before decoding, `deja "Poolrad unit 0x68 +0x04DF +0x05AA entry1 movement flag helper"`
and the shorter `deja "Poolrad 04DF movement flag"` found no exact prior
session. Reused and checked the unit-0x68 entry-1 caller context in
[the coordinate-writer note](U68_COORDINATE_WRITER_073B.md) and the
unit-0x9E stub mapping from the [overlay function ledger](overlay_function_ledger.csv).
The ignored binary was decoded independently for every claim below.

## Hash-locked boundary and ABI

The ignored `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x68` begins at file `0x166CB`; this routine is file
`0x16BAA..0x16C75` inclusive, **204 bytes / 81 contiguous instructions**,
SHA-256 `2d93ca5d0c9a3f1a5d609fc04bdc3a5ab66a33444d2dc4f2e9ee4aa08c2362b1`.
Its `retf` ends at `+0x05AB`, exactly the next prologue, which belongs
to the separately merged record scan. It has no stack arguments,
returns its local flag in AL, and uses a plain `retf`. Entry 1 calls it
with `push cs; call +0x04DF` at `+0x0ED7`; `+0x0EDA` stores AL in its
movement local. That path follows `AL=0x50` at `+0x0ED2..+0x0ED4`.
The entry-1 local is tested later at `+0x1027`, with a nonzero value
calling coordinate writer `+0x073B` at `+0x102E`. These are static
links, not a guest-PC trace of the saved one-Up case.

| Span | Decoded path or effect |
| --- | --- |
| `+0x04E5..+0x0504` | Set local found=0, copy DS:[0x5D96/0x5D98] as far current node; exit the loop when both pointer words are zero or found is nonzero. |
| `+0x050D..+0x051F` | Set AL to `1` and push AX; set AL to `0x64` and push AX; far-call `0xB0:0x0048`. AH can retain bits from the prior pointer-word OR and is not established by those instructions. Compare returned AL **unsigned** with ES:[node+0x78]; `JA` skips the candidate. |
| `+0x0521..+0x052C` | When AL is at most the threshold, byte ES:[node+0x10C] must be zero; then found=1. |
| `+0x0530..+0x0543` | Read the next far pointer from node `+0x104/+0x106` and loop. This read occurs **even after a hit**; the next loop test exits on found. |
| `+0x0545..+0x054E` | Store zero at DS:[0x6DDF] for empty, exhausted, and found paths. Only found enters the two map calls. |
| `+0x0550..+0x055D` | Pass low bytes DS:[0x6AAD] x, DS:[0x6AAE] y, DS:[0x6AAF] facing to `push cs; call +0x0109`. |
| `+0x0560..+0x059B` | Re-read facing, x and y after that call. Sign-extend table bytes DS:[0x2736+facing] and DS:[0x273F+facing], add them to sign-extended x/y, and call `+0x0109` again with the low bytes of those sums and selector `(facing+4) mod 8` (signed `idiv` of a nonnegative word). |
| `+0x059E..+0x05AA` | Return AL=found: zero for no candidate, one for a candidate. |

The `0xB0:0x0048` stub maps to unit `0x9E` entry 8 at `+0x0DE3`.
Its 75-byte body reads only count **byte** `[bp+8]` and range **byte**
`[bp+6]`, calls runtime `0xAF8:0x11C0` with the zero-extended range
byte, increments each AX, accumulates low bytes, and returns AL. The
effective bytes here are count=1 and range=100; the full pushed AX
words have unproved high bytes. The runtime random-state semantics
remain unresolved, so the model accepts that call's AL through
`roll_9e_entry8` and does not assert a distribution. Local `+0x0109`
is a 301-byte map-field updater:
it range-checks the x/y argument bytes, uses the selector byte to
choose a two-bit mask, and accesses a far map pointer at DS:[0x6A5C].
Its map buffer and side effects stay behind `mark_0109`; the callback
receives the exact low-byte arguments at each reached call.

## Bounded host behavior and tests

The C state maps reached ES selectors to synthetic 64 KiB views and
provides caller-owned visited scratch. Missing views/hooks, cycles,
scratch exhaustion, and a word read beginning at offset `0xFFFF` return
host statuses; DOS has no corresponding exits. Each hook is checked
only when its call is reached. On a found node, DS:[0x6DDF] is cleared
before a missing map-hook status; the direction-table guard comes after
the first map call. AL output is untouched on a host error. The first
hook may mutate the supplied state; x/y/facing are read again before
the second, as in the DOS instructions.

The synthetic test covers empty head, threshold equality and unsigned
miss, nonzero `+0x10C`, second-node hit, first-hit self next pointer,
nonzero-segment/zero-offset node, state mutation between map calls,
and reached-only hook, direction-table, cycle, capacity, and missing-view
guards. No game data or decoded content is embedded.

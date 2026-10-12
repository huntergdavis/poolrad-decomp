# Slot-A party-loop handoff after the save transfers

`pr_slot_a_party_entry_begin` models the first party-loop decision in DOS
v1.3 `GAME.OVR` unit `0x6F`, immediately after the merged
`+0x18EA..+0x191D` post-transfer step. This is a **bounded prefix**, not a
translation of the world-save loader, record loading, sidecar loading, menu
completion, or the saved one-Up route. It earns no strict full-C or identity
credit.

Before decoding, `deja "Poolrad u6f after +191D party sidecar menu completion
C bridge"` found no exact session. Broader `deja "u6f party"` found a Claude
session (`3e3ff494-f86`) that called out unresolved u6f record initialization;
this boundary is retained. The merged [post-transfer step](SLOT_A_POST_TRANSFER.md),
[party-input model](SLOT_A_PARTY_INPUT.md), and
[key-dispatch bridge audit](SLOT_A_KEY_DISPATCH_BRIDGE_AUDIT.md) supplied the
transfer layout and remaining route gates. Their count/slot interpretation was
rechecked against the bytes below.

## Byte and branch evidence

| Ignored input/span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| unit `0x6F:+0x191E..+0x1964`, file `0x1F904..0x1F94A` | **71 bytes, 29 contiguous 16-bit instructions** | `e609e0ce57d0eeba1936bb2c5ce6bcbde32d6f55e5dea90484c29f2e8da1a4b7` |

The local byte `[BP-0x1F9]` is the ninth world-save transfer: the saved
character count. `+0x191E..+0x1925` copies it to `[BP-0x212]`.
`+0x1926..+0x192E` tests unsigned `1 <= count`; zero jumps to `+0x19F7`.
For a nonzero count, `+0x1931` starts the one-based index at 1, and
`+0x1947..+0x1959` computes `BP-0x171 + 0x29*index`. At index 1 this is
`BP-0x148`, the first byte of the transferred eight-slot name array. The
first external call starts at `+0x1965` (`0xAF8:0x0B0B`, Pascal string load);
the C step stops immediately before it. It reports the borrowed slot pointer
and the next PC but does not emulate the stack arguments or scratch strings.
The 29 instructions have no external call or persistent DS/window write.
An independent pC byte audit (inbox i676) confirmed the span, hash,
count branch, slot arithmetic, and first-call boundary.

The DOS count is an unchecked byte loop bound. This prefix still selects
slot 0 for count values 9..255, because the first iteration is in range;
later source indices may exceed the eight transferred slots and are outside
the host model. A zero count reports `+0x19F7` as the next PC; the later
party-head check and post-loop effects are outside this prefix too.

## Saved case and next boundary

The hash-pinned save transfers count **6**. The strict C check selects the
first slot and reports next PC `+0x1965`; it does not read or publish that
slot's text. pC's independent caller/ABI audit (inbox i678) pins the next
three calls, `+0x1965..+0x1973`, to 15 bytes at file
`0x1F94B..0x1F959`, SHA-256
`8caf30b0059e878ceaacb6f7fc0ec602ddd1838f9410c950805ad63d3c4e5126`:

| Site | Supported data flow | Boundary |
| --- | --- | --- |
| `+0x1965` SYSTEM `0xAF8:0x0B0B` | Load private one-character Pascal `CS:+0x15E7` into local `[BP-0x214]`; retain destination pointer. | Literal is host-supplied, never embedded in public code. |
| `+0x196A` resident `0x0802:0x0356` | Input local `[BP-0x214]` and selected Pascal name slot; output `[BP-0x314]`, retaining destination pointer. | First presently unmodeled callee effect; loaded `0x08376..0x08444`, 207 bytes/82 instructions, SHA-256 `ab5aee9717d1a2e5f472234bc2198528c02792bec1098de0dbb7be0fef4ea889`. |
| `+0x196F` resident `0x0802:0x0425` | Input that intermediate, output `[BP-0x414]`, retaining destination pointer. | Loaded `0x08445..0x0852E`, 234 bytes/91 instructions, SHA-256 `28c69009d8bd4218b1eb50754c3f32a6f36ef40d3927d630fc8481d10b99e5a3`. |

For a future direct `0x0802:0x0425` model, pC's instruction audit (inbox
i680) also finds ten indexed bytes at `DS:+0x0D19` as live inputs to its
conversion loop, then an eight-character prefix, uppercase pass and
bounded store. The table and private literal must be supplied by the host,
not frozen into public source.

After those string transforms, the caller reaches the file test, the
record loader/party insertion, and `+0x02AE` after a present insertion.
The existing `pr_u6f_party_build` is a bounded host model of the resulting
chain, but it does not execute those intervening sidecar and post-load
effects. The actual far window-1 word `+0x67C` before and after the DOS
record loop awaits a separate native watch; this prefix makes no party-count
parity claim. A faithful C-native saved one-Up route cannot skip the later
effects.

`bash tests/replay/check_slot_a_party_entry.sh` compiles the C11 test with
`-Wall -Wextra -Wpedantic -Werror` and sweeps all 256 count bytes, including
zero, six, eight and values above eight. Passing the ignored save path
checks both input hashes, this exact code span, and the saved numeric
handoff. No original bytes, names or records are checked in.

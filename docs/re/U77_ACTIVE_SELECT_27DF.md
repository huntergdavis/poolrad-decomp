# GAME.OVR unit 0x77:+27DF complete C translation

## Original source and callers

The owner-supplied `resources/DOS/POOLRAD/GAME.OVR` is 206105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The function is exactly file bytes `[145433,145656)`, 223 bytes, SHA-256
`461b9a087f9daa616a7bb52320c801cb4e7608f279f0b1105dca3c08746a9b1c`.
All 71 instruction starts decode contiguously from `+27DF` through `RETF 2`
at `+28BB..+28BD`; every branch target is inside this body, and it has no
calls. It is public entry 10 of unit `0x77`, called through `0x85:0x52`.

Sixteen aligned original `LCALL 0x85:0x52` byte patterns occur in `GAME.OVR`,
in callers from units `0x1E`, `0x20`, `0x23`, `0x25`, `0x68`, `0x6A`, and
`0x6C`. Multiple callers compare a key against `0x47` (`G`) or `0x4F` (`O`)
immediately before the call. All 16 signatures align when decoded from their
containing function starts:

| Caller unit | Original GAME.OVR file call offsets |
| --- | --- |
| `0x1E` | `0x545A`, `0x5469` |
| `0x20` | `0x6345`, `0x6354`, `0x66ED`, `0x670A` |
| `0x23` | `0x741D`, `0x742C` |
| `0x25` | `0x8BCE` |
| `0x68` | `0x17254`, `0x173C3` |
| `0x6A` | `0x18C85`, `0x1901E`, `0x194E2`, `0x19762` |
| `0x6C` | `0x19C9E` |

The merged [alias audit](FUNCTION_ALIAS_AUDIT.md)
supplied the candidate lead; its original bytes and caller sites were checked
again here. A bounded `deja` lookup timed out, so no prior-session result was
reused.

## Behavior

The routine copies the party head `DS:[5D96/5D98]` into a four-byte stack
local. If the head is null, it stores that null pointer as the active
character `DS:[5D92/5D94]`. Otherwise it reads its byte argument at
`SS:[BP+6]`:

- `G` selects the predecessor of the active character; when active is the
  head, it walks to the last linked record. If active is absent from a
  terminated list, it can select the null end.
- `O` selects the active record's next pointer, wrapping to head when the
  next pointer is null.
- Other values select head.

The routine writes the selected offset, then segment, to the active pointer
and returns through `RETF 2`. Next pointers are at record offsets `+104` and
`+106`. The C model retains every source-order read of locals, DS globals,
and linked records, all tracked CPU registers and defined flag effects, stack
frame writes, and far-return cleanup. A cyclic chain may loop as in the
source.

## Verification and limit

With original media and the private captured RAM available locally:

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_overlay_u77_active_select_27df.py
```

Twenty-six cases execute the original bytes under a 16-bit emulator and the
C translation from identical states. Cases cover null head, single and
three-node chains, head/middle/tail and absent active pointers, all key
branches, same-segment/different-offset comparisons, offset and stack
wrapping, single-word far pointers, a link overlapping the active-pointer
table, a nonzero argument high byte, and two previously captured RAM input
states. Each compares the full 1 MiB RAM image
and every returned CPU register; the auxiliary flag is masked when it is
undefined after `OR`. All 26 cases pass. The CTest smoke test checks input
validation, predecessor selection, frame/return state, and returned flags.

The captured RAM originates from another native route. No real `+27DF`
call/return frame or route-specific pointer state was measured. The static
caller evidence does not settle game identity or strict full-C accounting;
this translation is provisional pending independent review.

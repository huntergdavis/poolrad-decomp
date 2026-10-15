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

With owner-supplied original media available locally:

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_overlay_u77_active_select_27df.py \
    --output resources/u77-active-select-parity/differential.json
```

The refreshed oracle executes 975 synthetic original-byte/C cases and visits
all 71 instruction starts. Cases cover null head, single and
three-node chains, head/middle/tail and absent active pointers, all key
branches, same-segment/different-offset comparisons, offset and stack
wrapping, single-word far pointers, a link overlapping the active-pointer
table, every argument byte with a nonzero high byte, stack/table overlap,
A20-off pointer accesses, and 128 seeded list/offset/flag states. Each compares
the full 1 MiB RAM image and all 14 returned CPU words. AF is masked only
when the final flag-writing instruction is OR; defined CMP AF is checked.
The emulator maps high physical addresses onto the same backing memory and
single-steps RETF separately, so aliased or same-body return addresses do not
silently execute extra instructions. The focused strict C test also covers
an active-pointer write overwriting the far return and contiguous RETF reads
across offset FFFF. No C body is duplicated or replaced.

An optional --include-capture flag adds the two historical, hash-pinned
unrelated-route RAM inputs; the 975-case result does not require those private
captures. The old 26-case result included them.

The reproducible native harness is tools/overlay/u77_active_select_27df_native.py.
Run it with the same PYTHONPATH. It generates the copied body, COM assembly,
binary and DOSBox-X outputs only under ignored resources/u77-active-select-parity/native/.
Twelve fresh cases execute an actual far CALL and compare all 14 CPU words,
48 caller/local stack bytes, active/head globals and all three next links.
Null/head/middle/tail states and G/O/other byte arguments pass. The result
SHA-256 is 62a7bbb4a29a698ea2cca1c731aaf3c53e3f6269ea17289688fee21d98b4833e;
DOS allocation segments can make this output hash environment-dependent.
This reuses the COM-oracle method from tools/overlay/u25_load_word_0e0c_native.py.
Interrupts are disabled from frame initialization through capture (caller
FLAGS=0002), then enabled for DOS I/O. This isolates the routine's stack
effects from asynchronous interrupt-handler writes observed while building
the longer U33 native oracle. All 12 isolated native cases pass.

The captured RAM originates from another native route. No real `+27DF`
call/return frame or route-specific pointer state was measured. The static
caller evidence does not settle game identity or strict full-C accounting;
this translation remains subject to independent identity/accounting review.
The new native frames are synthetic and do not close that reached-game gap.

## Review refresh

Target_function: OVL_0077_27DF. Source_candidates: the existing #395
pr_u77_active_select_27df implementation only. Signals: pinned exact extent,
all internal branches and zero calls, 16 aligned static callers, 975 full-RAM/CPU
comparisons with 71/71 coverage, and 12 DOSBox-X synthetic far-call comparisons.
Score: 5/5 listed behavioral evidence checks pass; this is a checklist, not a
probability or ownership score. Verdict: match under the documented immutable
code, A20-off, entry-frame contract. Review_notes: cyclic chains retain source
nontermination; no real reached caller frame was obtained; no new strict credit
or identity is recorded here.

Before this refresh a bounded 15-second deja lookup reported a damaged index
and rebuilding, without a completed finding. Reused repository findings are
the original #395 implementation, this document's pinned body/caller evidence,
and FUNCTION_ALIAS_AUDIT.md. Neither target address was strict-credited in the
current-main manifest at base cdb4c31851e8dd7cb2753f785e211e1f0280c30a.

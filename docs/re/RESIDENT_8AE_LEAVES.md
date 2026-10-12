# Resident 0x8AE +1742 and +17AE local writers

This note records exact local procedure boundaries and memory effects. Their
use on the saved slot-A one-Up route is conditional on unsampled resident
state; no game-specific identity or C translation count follows from it.

## Prior work and input

Before decoding, `deja "Poolrad resident 8AE 1742 17AE slot A one Up
reached callee mode active argument"`, the individual offset queries, and
table-offset queries found no prior matching session. The merged
[RES_08AE:00DC note](RESIDENT_8AE_00DC.md) supplies its verified caller ABI
and branch sites. The [native coordinate watch](SLOT_A_ONE_UP_NATIVE_COORDINATE_WATCH.md)
observed execution of unit `0x68:+0752` in the saved one-Up run. In the
straight-line [coordinate writer](U68_COORDINATE_WRITER_073B.md), the
`+0742` far call to RES_08AE:00DC precedes that store; thus the dispatch
call was reached and returned in that run. No live value for the dispatch
mode, active word, or argument was measured there.

The ignored DOS v1.3 START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
`tools/overlay/resident_loader_map.py` hash-checks it and recovers loaded
source provenance. Both spans below are consecutive copied bytes, have
complete x86 16-bit disassembly, and terminate with `RETF 2`.

| Target | Loaded image | START file | Size / instructions | SHA-256 |
| --- | --- | --- | --- | --- |
| `0x8AE:+1742..+17AD` | `0x0A222..0x0A28D` | `0x09F1F..0x09F8A` | 108 / 43 | `6128c5126be22f692937ccbb5e5eb2b07c85081a04f086e9bb84579486258484` |
| `0x8AE:+17AE..+1819` | `0x0A28E..0x0A2F9` | `0x09F8B..0x09FF6` | 108 / 43 | `8bb73768c1c77c46b1695935e45b9ab162b9c7863c94529cbbffa248001c52a8` |

Each has no calls and three jumps. They have the same control flow and
one WORD argument at `[BP+6]`; all saved registers and DS are restored.
They switch DS to `0x8AE` and make the following segment-local writes.
The words are little-endian and effective offsets use 16-bit wrap.

| Step | +1742 | +17AE |
| --- | --- | --- |
| Record argument | `[0x01FF] = argument` | same |
| Enter four-row update | `[0x01D8] = 1; [0x01D9] = 4` | same |
| For row `i=0..3` | read word `[0x055F + 8*argument + 2*i]` | read word `[0x0607 + 8*argument + 2*i]` |
| If row word is nonzero | clear 0x30-byte record at `0x022D + 0x30*i`, from high word to low; write word `record+2 = row word`, byte `[0x0209+i] = low(argument)`, word `record = 1` | same, with record at `0x03C7 + 0x30*i` |
| After each row | advance record/table offset; decrement byte `[0x01D9]` | same |
| Exit | `[0x01D8] = 0`; `[0x01D9]` ends at 0 | same |

A zero row leaves its corresponding 0x30-byte record and selection byte
untouched. The source table, record area, and control bytes share the same
segment; in-place operation order matters for 16-bit wrapped or aliasing
arguments. The target names are address-based: table or device meaning is
not established. Their link to dispatch calls that also reach DOS vector
and PIT/speaker I/O makes a sound-driver role plausible, but this is an
inference, not a verified game-specific classification.

## Saved-route gate still open

For the observed `+0742` dispatch call, `[0x5372] == 2` would bypass both
leaves. Otherwise, argument 0 reaches both, argument 1 reaches neither,
argument `0x00FF` reaches both, and other arguments select `+1742` for
reread mode 1 or `+17AE` for reread mode 0. A positive active word
`[0x5374]` also creates a late `+17AE(0)`, then `+1742(0)` pair. These
are byte-verified conditional routes, not measured one-Up leaf execution.
The next live check must capture both mode reads, both active-word reads,
and the dispatch WORD argument on the saved Up call.

A hash-locked linear instruction scan of all 34 overlay unit code ranges
found six direct DS:[0x2628] reads: unit `0x01C:+2D88,+34CF`,
`0x05E:+0872`, and `0x068:+073E,+0B10,+0B55`. It found no direct
overlay write to that displacement. This bounded scan cannot exclude
indirect or resident writes and does not determine the saved argument.
The same scan found a direct `[0x5374]` write at unit
`0x036:+045D`, where AX is zero immediately before a far call to
`0x8AE:+01A5`. This is an initialization lead only; its execution
and any later resident writes were not established for slot A.

## Bounded +17AE segment-memory model

`pr_rec_res_08ae_17ae` represents the complete call-free +17AE body's
memory effects on an explicitly supplied 64 KiB resident segment. It
rejects a null or short view before writing, preserves 16-bit effective
offset wrap, and reads each source WORD immediately before its row update.
The in-place order matters if an unusual argument aliases the source
table with a prior output record. This C API does not expose x86 register
or flag state and is **not** a live one-Up reachability claim.

The synthetic test covers all-zero rows, separated nonzero rows, untouched
miss records, full-WORD argument storage versus low-byte selection, a
source WORD crossing `0xFFFF`, and source/output aliasing across rows.
It is a bounded resident writer model pending native route and PM parity
review, with no game-specific identity or strict full-C credit claimed.

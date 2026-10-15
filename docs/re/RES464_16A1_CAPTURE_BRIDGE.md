# First-page +16A1 capture-state bridge

The [bounded C bridge](../../tools/replay/res464_16a1_capture_bridge.c)
parses a **future** pre-loaded-SYSTEM:16A1 native state record and passes it to the
[explicit one-byte model](../../tools/replay/res464_16a1_explicit_byte_model.h).
The model was extracted unchanged from the earlier
[one-byte fixture](RES464_16A1_EXPLICIT_BYTE_PROJECTION.md) so that the
synthetic behavior checks and bridge use the same projection. There is
still **no native +16A1 hardware/plane capture** and therefore no native
first-byte, page, RGB, or full-frame parity claim.

## Required input and boundary

The record is bounded to 4096 bytes, one unique ASCII key=value field per
line, with exact-width hexadecimal values and no unspecified fields. All 29
fields are required:

| Group | Fields | Meaning |
| --- | --- | --- |
| Stop | phase=pre_16A1, pc=loaded-SYSTEM:16A1 | Exact stop before the first REP MOVSB source read; expected segment comes from the pinned loaded far caller |
| CPU | ds, si, es, di, cx, df | Must be A200, 03C3, A000, 03C3, 000B, 0 respectively |
| EGA registers | gc6, seq4, seq2, gc4, gc5 | Explicit current register values, never inferred from earlier writes |
| EGA mapping | handler, compatible_chain4, ignore_odd_even, memmask, bank_read, bank_write | Explicit handler and memory-bank state |
| Planar addresses | source_planar, page_planar | Must equal the model's GC6-dependent planar offsets |
| Source planes | source_p0 through source_p3 | Four bytes at the source planar offset |
| Page planes | page_p0 through page_p3 | Four old bytes at the page planar offset |

Only handler=unchained, GC6 04 or 06, Seq4 00 or 04, GC5 01,
valid read-plane and Seq2 mask, compatible-chain4 and odd/even flags clear,
memory mask FFFF, and zero banks are modeled. A missing, duplicate,
malformed, wrong-frame, unsupported-hardware, or inconsistent-address field
causes a typed stop before the caller's parsed input or projection changes.
The bridge cannot establish that a supplied text file really came from the
saved route; its native capture provenance and RAM/VRAM hashes must be checked
separately. The [loaded-PC audit](../../tools/replay/res464_16a1_loaded_pc_audit.py)
hash-checks the ignored flush-entry RAM, checks the far-call opcode and
`+1685` destination offset at loaded caller `0C88:+1DA0`, and supplies its
relocated segment to the C bridge. The bridge has no globally hardcoded
loaded segment. Its synthetic fixture uses an invented segment.

The ignored DOS START.EXE hash is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
The [read-only audit](../../tools/replay/res464_16a1_explicit_byte_audit.py)
also pins the first-flush RAM and GC record, three original code spans, nine
selected operands, and the conditional A200:03C3 to A000:03C3, count-11
first Move. The static register contract in local head f27862b did not
measure GC6, Seq4, handler, banks, or planes at the native +16A1 stop.
Prior one-byte fixture head 7a4c149 is the exact dependency of this local
bridge branch. The bounded pre-implementation deja searches timed out; the
reused evidence is the local one-byte fixture and hash-locked operand audit.

## Verification and next stop

No emulator was started. The bridge's **invented synthetic** record exercises
the complete parsed projection, each of 29 missing fields, duplicate and
malformed fields, the wrong segment/IP, an unsupported GC6 mode, and mismatched
planar addressing. The original five-projection one-byte fixture still
passes after sharing the model. To check a future private state record,
place it under ignored resources/, compile the bridge under ignored resources/,
and run the loaded-PC audit with `--bridge` and `--record`; the wrapper passes
the hash-derived segment to the C bridge. Direct bridge invocation requires
`--record ignored-state.txt --loaded-cs HEX4`; an arbitrary supplied segment
has no provenance. The output is a local projection, not a native parity
result. Until the handler, memory mask, and banks are measured, missing or
unsupported fields stop before projection.

Run the hash audit with python3 -B tools/replay/res464_16a1_explicit_byte_audit.py.
Compile the original fixture with a strict C11 warning build against
src/formats/ega.c, then compile the bridge the same way. Both binaries belong
under ignored resources/. The bridge with no arguments runs its synthetic
checks; with --record followed by an ignored private text path it parses and
projects the supplied stop.

The next native gate is the exact pre-+16A1 stop with current graphics
registers, handler/mapping state, banks, and all eight plane bytes. Only after
that comparison can the first byte be called native-matched; the subsequent
+16A3 DS restore and +16A5 return remain separate gates.

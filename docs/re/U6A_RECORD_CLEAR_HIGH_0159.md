# Unit 6A +0159 record high-byte clearer

## Extent and game-role evidence

The complete call-free body is U6A:+0159..019B inclusive: 67 bytes,
24 contiguous instructions, GAME.OVR file [96650,96717), SHA-256
a130919171e4e4b45af9155e9cd7e16abc1027eb83e11fbdf801fca7a0d3c099.
It has one unconditional jump, two conditional branches and RETF 4 at +0199.
Every branch remains in this span; there are no calls, interrupts or fixups.
The next function starts at +019C.

Pinned owner-supplied inputs are START.EXE, 65231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd,
and GAME.OVR, 206105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Unit 6A has file base 0x17831 and resident stub segment 0075.

Three aligned PUSH CS / near CALL pairs target this leaf: +0235/+0236
in the +0214 caller, and +0972/+0973 and +0AA6/+0AA7 in +090A.
The +0214 caller traverses records from DS:[5D96/5D98], follows the far
link at record +104/+106, and invokes this leaf on each nonnull record.
The +090A caller passes DS:[5D92/5D94], the active-character pointer
independently established by [U6F insertion evidence](U6F_PARTY_INSERT_1446.md).
The same 21-byte record list is examined by the separately recovered
[U6A +025C helper](U6A_RECORD_REMAINING_025C.md).

The source audit reconstructs the descriptor and walks a structural CFG
from public entry 1, +1E6F, to +1ECA -> +0214 -> +0236 -> +0159.
This is an instruction-aligned path with calls assumed to return, not a
proof of branch feasibility or a naturally reached game route. The
canonical +0214/+090A callers are game_evidenced; this leaf remains
helper_candidate pending independent identity review. No specific spell,
inventory or attribute meaning is assigned to the list or final byte.

## Complete raw contract

pr_u6a_record_clear_high_0159 enters after CALL with return IP/CS at
SS:SP and a far record pointer at SS:SP+4. It saves BP, sets BP to SP,
reserves two bytes and initializes byte [BP-1] to zero. [BP-2] is unused.
For indices 0..20 it reads the index, zeroes AH, reloads the far argument,
adds AX to DI, and compares record[17+index] to 7F unsigned. Values
00..7F are retained. Values 80..FF cause another index/argument reload
and a zero store at that same effective field. This clears the whole byte,
rather than removing its high bit. The loop increments its live local byte.

After the final index comparison, the source reloads the far argument
again and clears record[2C]. AX is 0014, DI is the final argument offset,
ES is its segment, and BX/CX/DX/SI/DS/SS are preserved. The last CMP
against 14 defines all arithmetic flags: PF and ZF set, CF/AF/SF/OF clear.
Other FLAGS bits are preserved. Subsequent LES/MOV/POP/RETF do not alter flags.

All locals, argument words, records and return state share 1 MiB RAM.
Each source reload is retained: for example, clearing the argument's low
offset byte redirects later iterations. No pointer is cached across stores.
A final clear can overwrite saved BP or the return IP. The actual RAM
frame supplies restored BP and CS:IP. MOV SP,BP and RETF 4 advance the
entry SP by eight bytes overall. Effective offsets and SP wrap at 16 bits;
multi-byte operands and the four-byte far return use contiguous physical
bytes with A20 disabled. Code must remain immutable. The C model needs
no original instruction bytes. Null CPU/RAM or short RAM is rejected
before mutation; valid RAM returns OK. There is no callback or host loop cap.

## Measured parity and reproduction

The original-byte audit passed **647 cases**, comparing all 1048576 RAM
bytes and all 14 CPU words. All 24 instruction sites and both outcomes
of both conditional branches (four edges) were reached. No FLAGS bit
is masked. Cases cover all 256 byte values, threshold cases at each of
21 positions, 77 byte-aligned record/frame aliases, stack and record offset
wrap, A20 wrap, and 128 seeded whole-RAM backgrounds and flag variations.

The DOSBox-X normal-core/386 oracle passed **260 cases**. It copies the
exact original +096A..0975 suffix, including argument pushes from the
active-character pointer, PUSH CS and near CALL. Both caller and leaf
run at their original offsets within an ignored synthetic COM, so the
original relative CALL operand is unchanged. It does not run the earlier
caller conditions or the game loader, and does not prove live game reach.

Native cases cover all uniform list byte values, a mixed threshold list,
argument redirection, a final saved-BP clear, and a final return-IP clear.
The last case changes original return IP 0976 to 0900. Separate synthetic
capture landings record which return was actually reached, preserving flags.
Every case compares all 14 CPU
words, 320 stack/alias bytes and 64 record bytes, covering all writes.
No flag mask is used. IRQs remain disabled through setup/execution/capture
and are enabled for DOS I/O. Incoming arithmetic flags and DF vary.
Native output SHA-256:
83ac1a6722819b63a3df1367eb167f6f94b1981ad4cc5758d0e7b1e4139cbbd3.
The header includes the DOS allocation segment, so the hash may differ
in another environment while preserving parity.

With owner media, Capstone 5.0.7 and Unicorn 2.1.4:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6a_record_clear_high_0159_audit.py --output resources/u6a-0159-parity/differential.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6a_record_clear_high_0159_native.py

The focused strict C test checks input guards, threshold/neighbor bytes,
the final clear, unused local, registers/FLAGS, argument redirection and
saved-BP aliasing. Research hashes, oracle shell syntax, tracked-media
exclusion and whitespace checks pass. Original bodies, caller suffix,
assembly, COM, disassembly, native output and logs stay ignored in resources/.

## Review and reuse

target_function: OVL_006A_0159. source_candidates: the new raw C model.
signals: pinned exact extent/ABI and CFG; aligned party/active-record callers;
647 full-state cases with complete site/edge coverage; 260 original-caller
native cases, including redirected pointer and return-frame aliases.
score: 4/4 listed checks pass, a checklist rather than a probability.
verdict: match under the immutable-code, A20-off, entry-frame contract.
review_notes: field meanings and natural game reach remain unmeasured;
game identity and strict credit require PM's independent exact-head review.
The canonical classifier and strict manifest are unchanged.

A bounded 15-second deja lookup yielded no completed finding. Reused
findings are the U6A +025C list/caller notes, U6F native active-record
pointer mapping, the U86 raw CPU/RAM audit utilities and the prior
IRQ-isolated oracle method. Extent, source hash, three callers, shortest
public-entry path and all parity counts were measured again on this branch.

The branch began at main cc3b27f84d91ea7b8aeedea9a915c9d0d49a2dbe.
U2F +122B is the disjoint follow-on reserved with PM, unimplemented here.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_006A_0159 --require-current-main
```

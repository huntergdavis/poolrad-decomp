# U5E +263D reconnaissance checkpoint

Status: reconnaissance only, paused before implementation on 2026-10-17.
No C body, executable parity result, strict credit, or denominator change.

## Verified identity

| Artifact | SHA-256 |
| --- | --- |
| START.EXE, 65,231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR, 206,105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U5E caller, file [0x14E38,0x14EAB) | 7983c5f0344f78278aa7227b2a7ae964a5ab77cfe2be70a42a91bf179cd3c297 |
| U3C2 predicate, file [196813,197723) | 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae |

The existing overlay inventory parser maps U5E public entry 18, runtime
stub 0068:007A, to +263D. Independently decoded caller extent is
[+263D,+26B0): 115 **bytes**, 41 instructions, ending in RETF 8 at +26AD.
The ledger records seven aligned callers; those callsites have not yet
been independently audited in this checkpoint.

## Behavior and dependencies

The caller reserves six local bytes. It reads two slot fields for each
of two far-pointer arguments, storing four returned bytes at BP-3 through
BP-6. It sets a byte counter at BP-2 to zero, passes the four fields and
counter to a predicate, and increments the counter until the predicate
returns nonzero AL. It returns the counter through BP-1/AL.

| Caller site | Original far target | Resolved body |
| --- | --- | --- |
| +2649, +2665 | 03E0:005C | U3C6 entry 12, +0BE7 |
| +2657, +2673 | 03E0:0061 | U3C6 entry 13, +0C0F |
| +2693 | 03DB:0034 | U3C2 entry 4, +057B |

U3C6 entries 12/13 call entry 15 (+0C5F), which searches the far-pointer
table at DS:65B9 with a wrapping byte index starting at one. The readers
select DS:5F27/5F28 plus four times the resulting slot. Existing portable
C is in overlay_u3c6_slot.c; full CPU/RAM folded variants also occur in
overlay_u3c6_1298_cpu.c. Neither was executed for this checkpoint.

U3C2 +057B is a call-free 910-byte predicate ending at +0909. It checks
signed coordinate ranges 0..49 and 0..24, uses signed byte displacements
from DS:2736/273F, and dispatches direction cases 0..8. Argument FF is
rewritten to 8. Equal or adjacent coordinates can accept before direction
dispatch. Invalid coordinates return zero, which can leave the caller
looping indefinitely. The caller counter wraps at 256. Direction values
above 8 can leave the predicate result local unwritten; do not invent a
zero initialization when translating the complete predicate.

Capstone labels opcode 98 as CWDE in this environment, but these are
16-bit CBW instructions; preserve signed AL-to-AX extension.

## Reuse, parity review, and next work

Prior-session recall was attempted before analysis with the deja query
"OVL_005E_263D 005E 263D" and MCP recall. Both stalled during this work;
recall is incomplete, not evidence of no prior match. No recalled session
finding was reused.

Repository evidence reused: [function ledger](overlay_function_ledger.csv),
[slot helper evidence](U9E_0160.md), the whole-predicate hash and explicit
incomplete-function boundary in [the +0621 fixture](U3C2_CMP_AL_STACK_0621_FIXTURE.md),
and the full-state audit pattern in [U5E +2879](U5E_PARTY_VALUE_2879.md).
The descriptor resolution, caller/predicate hashes, and disassembly were
independently checked against local original media. All raw disassembly
and original bytes remain under ignored resources/.

Parity review: target_function = OVL_005E_263D; source_candidates = none
implemented; signals = exact original hash, entry descriptor, decoded
control flow, resolved callees; score = not scored (no candidate);
verdict = unclear. Review notes: no original-byte execution or native
reachability measurement yet, and no complete U3C2 +057B implementation
was found in this checkout.

Resume by checking current ownership and merged work. Translate the caller
and its dependencies with explicit RAM/CPU and far-return semantics;
retain stack/table alias ordering and byte wrap. Add a host iteration
budget with a precise reached-state stop for nontermination. Differentially
test original bytes against C over full RAM/registers, including all
direction branches, invalid coordinates, missing slots, count zero,
relocated code, stack wrap, and supported aliases. Claim parity only for
the measured domain and keep native reachability separate.

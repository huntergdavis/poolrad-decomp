# U3C2 +057B direction predicate

This leaf accepts a direction and two grid coordinates through five stack
words. It validates signed-byte X in 0..49 and Y in 0..24, adds the selected
signed displacement bytes from DS:2736/273F to the origin, and tests the
target against the selected directional region. Original or displaced
coordinate equality returns one before direction dispatch. Mode 8 returns
one; FF rewrites the argument byte to 8. Modes 9..254 can return the
preexisting SS:[BP-1] byte, including values other than zero or one.

The [C translation](../../src/recovered/overlay_u3c2_entry4_057b.c) preserves
all fourteen CPU words and shared 1 MiB RAM. Its address-labelled basic
blocks preserve the arithmetic order, signed branches, low-byte writes,
stack frame, and RETF 10. It has no callback or interpreter dependency.
The [API](../../include/poolrad/recovered/overlay_u3c2_entry4_057b.h) states
the memory and entry contract. Table data remains caller supplied. The two
256-entry table windows overlap in RAM; they are not independent host arrays.

## Identity and evidence

GAME.OVR U3C2 public entry 4, loaded-stub identity 03DB:0034, body
[057B,0909), file [0x300CD,0x3045B): 910 bytes and 410 instructions.
Body SHA-256:
59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
The full media hashes and descriptor checks are recorded in
[the composed caller report](U5E_DIRECTION_263D.md).
There are no CALL instructions or external effects hidden behind hooks.

The byte 98 instructions decode as CBW in the 16-bit execution profile;
the disassembler's CWDE spelling must not turn these into 32-bit sign
extensions. The result local must not be initialized at entry. Bounds
failure occurs before FF normalization and displacement reads, so it
preserves argument FF and the untouched local bytes.

## Validation and parity review

The shared [original-byte audit](../../tools/overlay/u5e_direction_263d_audit.py)
executes the complete leaf on synthetic memory, comparing every RAM byte
and all fourteen CPU words. It covers every mode byte, signed coordinate
bounds, early equality, adjusted-coordinate equality, and seeded random
displacements. The final run passed 506 complete-state cases with 410/410 original
instructions exercised, including sixteen stack/displacement-table aliases.
Both focused CTest targets and AddressSanitizer/UndefinedBehaviorSanitizer
runs passed.
The [C regression test](../../tests/recovered/test_overlay_u3c2_entry4_057b.c)
also checks inherited result bytes, argument high-byte preservation, bounds
ordering, far return state and atomic API rejection.

Parity review: target_function = OVL_03C2_057B; source_candidates =
pr_u3c2_entry4_057b; signals = exact source hash, descriptor identity,
branch/stack transcription, complete-state original-byte differential;
score = four evidence classes; verdict = match within the documented contract.
Review notes: synchronous immutable code and CPU/RAM separation are
preconditions. Physical addressing wraps at 20 bits, effective offsets at
16 bits, and word bytes remain physically consecutive. Native stack-wrap,
A20-boundary profiles and saved-route reachability are not established.
The final flags come from comparisons, overwriting the intermediate
architecturally undefined XOR AF. No strict-credit change is made here.

## Reuse

DS4's ownership handoff i17131 and VP confirmation i17132 supplied the
agreed ABI, original body extent, and warning about unwritten result modes;
the implementation was independently checked against the complete body.
The required pre-implementation query, deja "u3c2 057b", timed out after
15 seconds without results; no recalled session implementation was reused.
The physical RAM, arithmetic flags, CBW and stack helper conventions are
adapted from the repository's overlay_u3c2_entry5_0422.c. Its parity status
does not establish this leaf's parity. Original disassembly and media remain
under ignored resources/.

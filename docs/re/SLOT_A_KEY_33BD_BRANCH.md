# Slot-A U1C +33BD key-dispatch decision

The PM assigned a separate alternate key-route screen in durable inbox
i4917. The required targeted `deja` CLI query could not open its index
lock in this sandbox; the MCP recall query for `RES_08AE +006B RETF
+008D caller return +33BD alternate key route` found no matching prior
session. The merged `U1C_LOOP_3387.md` already owns the high-level
comparison and branch semantics. This is the first separately bounded
instruction-level CPU/RAM gate for that decision; it does not translate
either successor body or claim a new natural route.

Original DOS v1.3 `GAME.OVR` file `0x4103..0x410A` and saved loaded
RAM physical `0x38BDD..0x38BE4` both contain
`[scrubbed: span=SLOT_A_KEY_33BD_BRANCH-1 sha256=3ac45bdf90c77cb28ac33e47374a54322c23d4cf81b392c7738140d1b917f3be bytes=8 offset=UNRESOLVED]`, SHA-256
`3ac45bdf90c77cb28ac33e47374a54322c23d4cf81b392c7738140d1b917f3be`.
The first six bytes at `3582:33BD` compare WORD `ES:[DI+0594h]` with
one; `77 0D` at +33C3 is `JA +33D2`. An unsigned value at most one
falls through to +33C5. The audit pins complete GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
and saved +33BD RAM SHA-256
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`.
At the saved stop, `ES:DI=3A16:0000` and the compared word is zero.
The documented capture does not supply all 14 CPU words or a direct
post-branch stop. Original media and RAM remain ignored under `resources/`.

`pr_rec_u1c_33bd_branch` accepts explicit 14-word CPU and complete
640 KB RAM at +33BD. It computes the six CMP arithmetic flags,
preserves every other CPU word and RAM byte, then stops at +33C5 or
+33D2 before either successor body. It guards an effective-offset
word crossing 16-bit segment wrap and an operand outside the supplied
image; those unmeasured cases preserve state and remain unresolved.

Focused tests cover zero, one, two, and signed-overflow operand values,
all CPU words, full RAM, and rejected mappings. The actual saved RAM
with explicitly synthetic unknown CPU words chooses +33C5. A separate
two-byte mutation of that RAM (word `0002h`) chooses +33D2. The latter
is a **source-only alternate input**, not a natural second-key state.
Both modeled runs leave RAM byte-identical to their respective inputs.
Source audit, strict C11 focused test, ASan/UBSan focused test, and
CTest registration pass. No native execution or broad build was run.

## Parity review

- **target_function:** U1C +33BD..+33C4 CMP/JA, stopping at +33C5
  or +33D2 before successor instructions.
- **source_candidates:** `pr_rec_u1c_33bd_branch` models the original
  compare flags and unsigned jump from explicit CPU/RAM.
- **signals:** exact original/loaded source bytes, saved zero operand,
  independent high-level decode, full-state synthetic tests.
- **score and verdict:** 2/5 direct signal groups; **source-only
  conditional match** pending complete native CPU and branch stops.
- **review_notes:** the alternate `word > 1` branch is not observed in
  the saved run. This gate earns no full-function or gameplay credit.

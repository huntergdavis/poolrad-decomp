# U3C2 +01A0: complete line-record initializer

Public entry 2 initializes a line-stepping record and composes the complete
+0000 sign helper at both original call sites. The implementation preserves
all CPU words, every stack write, repeated pointer loads and 16-bit
arithmetic effects. It requires no callback or host-supplied sign result.

## Original identity

| Artifact | Extent | SHA-256 |
| --- | --- | --- |
| START.EXE | 65,231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206,105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Caller | U3C2 [01A0,02CC), file [195826,196126), 300 bytes, 99 instructions | 7fbacb8d9d298c72bd886321a58a8d842f2ec6dc350fc83fde6064fe78f27f40 |
| Sign helper | U3C2 [0000,002E), file [0x2FB52,0x2FB80), 46 bytes, 17 instructions | 5e8e5fc8a5907fee2efd922fc4f96682fd1f2e8e58cbebcd058d3f025948e5bf |

The descriptor maps public entry 2 to +01A0. Two aligned PUSH CS / near
CALL sites in +0422 reach it at +045C and +04D6. Two fixup-backed far
calls in U86 reach stub 03DB:002A at +2A4B and +2B4C. The audit checks
all four edges against the pinned media. The new caller's two PUSH CS /
near CALL instructions are +01F9 and +0213, both targeting +0000.

Before implementing, the required recall command was run:
~~~sh
timeout 15 deja "U3C2 01A0 full state"
~~~
It indexed changed sessions but produced no usable session excerpt before
the timeout. No indexed-session finding was reused. Fetched history,
source references and read-only operations-ledger checks found no complete
+01A0 body or authored/accepted record, including offset ID variants.
The queue's inline-model evidence points to the unresolved callback in
+0422 rather than an implementation of this body.

The new entry directly reuses pr_u3c2_sign_entry1_0000 and its CPU type;
[the sign-helper evidence](U3C2_ENTRY1_SIGN_0000.md) and existing synthetic
sign audit establish that dependency. Local memory/flag helpers and the
exact-return oracle pattern are adapted from the
[+2574](U5E_DIRECTION_MATCH_2574.md) and
[+17F6](U5E_DIRECTION_GATE_17F6.md) composition work. Neither is a
build/runtime dependency. Existing recovered APIs and
dependency implementations are unchanged. The new implementation is
pr_u3c2_line_init_01a0 in overlay_u3c2_line_init_01a0.c.

## Record effects

For ordinary disjoint record/stack storage, the layout is:

| Offset | Meaning after initialization |
| --- | --- |
| +00, +02 | Original start x/y words, retained |
| +04, +06 | Original end x/y words, retained |
| +08 | Initial decision word |
| +0A, +0C | Wrapped signed absolute x/y differences |
| +0E, +10 | Diagonal and straight decision increments |
| +12, +13 | Sign bytes of the wrapped x/y differences: FF, 00 or 01 |
| +14, +16 | Current x/y, copied from the start words |
| +18 | Cleared byte |

The arithmetic resembles a Bresenham-style initializer, but the C model
follows the actual word operations rather than substituting a generic line
algorithm. Each difference is computed modulo 65536 before CWD/XOR/SUB
absolute value and before the separate sign call. Absolute value of 8000
remains 8000. The comparison of the two resulting difference words is
signed, so 8000 is treated as negative when selecting the major axis.
All doubled terms and decision increments wrap as original 16-bit words.

For example, start (0,0) and end (8000,1) leave x difference 8000 and y
difference 1. The signed comparison selects the y branch, and the final
words are decision FFFF, diagonal FFFE, straight 0000. A host absolute
value over an unbounded integer would select a different path.

Every original LES reload is retained. Record writes can alias the local
far-pointer words, coordinate inputs, later output fields or the outer
return frame. Such writes can redirect later accesses, so the disjoint
layout description is not substituted for execution in alias cases.

## API boundary

- Enter at IP=01A0 after far CALL; SS:SP+4 holds the far record pointer.
  Initial SP is in [0010,FFF8]. The deepest sign-helper frame is 16 bytes
  below entry SP. The outer return executes RETF 4.
- RAM is exactly 1 MiB, with distinct stable CPU storage and synchronous
  access. Effective addresses and SP wrap at 16 bits; word/LES operand
  bytes are physically contiguous with 20-bit/A20 wrapping.
- Code is immutable and record writes must not overlap modeled code.
  Invalid arguments and reserved stack/code overlap reject atomically.
- The accepted initial stack range avoids wrapped far frames in the sign
  helper, whose public RETF profile uses separately wrapped word pops.
  This composition does not change that profile or claim native wrapped
  RETF behavior. Actual outer return words, including aliases, are retained.
- A dependency rejection exposes reached CPU/RAM. TRANSFER exposes a changed
  nested return. No artificial loop limit is needed for this finite body.
- Undefined logical and single-bit-shift AF is chosen zero, consistent with
  the selected Unicorn profile. Native undefined-flag behavior is not claimed.

## Validation and reproduction

The audit executes original caller and sign-helper instructions, with no
result stubs. It compares all 14 CPU words and every byte of 1 MiB RAM.
Shared backing memory implements both sides of the A20 boundary. The outer
RETF is stepped separately, allowing comparison even if record aliases
change the return address. Four original incoming edges and the descriptor
entry are checked before running the corpus.

All 2,889 original-chain/C cases passed and executed the outer RETF 4.
Coverage reached all 99 caller instructions and all 17 sign-helper
instructions. No nested TRANSFER was observed. The recovered library,
three focused CTests (initializer, sign helper and adjacent record step),
strict compiler warnings and AddressSanitizer/UndefinedBehaviorSanitizer
checks passed. Original media and derived byte listings remain ignored.

The corpus includes the Cartesian product of seven boundary values for
all four coordinate words, randomized full-width coordinates, signed
absolute-value overflow, both axis branches, sign paths, stack bounds,
every record offset near FFFF, A20 wrapping, record/stack overlap and
preserved incoming flag bits. Focused C tests assert explicit expected
records, return ABI and atomic argument/code-alias rejection.

~~~sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
python3 tools/overlay/u3c2_line_init_01a0_audit.py \
  --exe resources/oracle-runs/slot-a-ui-second-cycle-callers-p9-025/POOLRAD/START.EXE \
  --ovr resources/oracle-runs/slot-a-ui-second-cycle-callers-p9-025/POOLRAD/GAME.OVR
~~~

## Evidence for independent parity review

- target_function: OVL_03C2_01A0, public entry 2.
- source_candidates: overlay_u3c2_line_init_01a0.c composed with the unchanged
  overlay_u3c2_sign_entry1_0000.c.
- signals: exact body hashes and extents, descriptor entry, four original
  incoming edges, both real sign calls, repeated pointer loads, signed
  arithmetic/branch behavior, output field structure, stack effects and
  complete original-instruction CPU/RAM comparison.
- score: strong behavioral evidence within the explicit contract; no
  numerical confidence probability is inferred.
- verdict: match in the measured differential corpus; independent acceptance
  remains pending.
- review_notes: the queue currently classifies this function as
  game_or_other_library_unresolved. The line algorithm alone does not prove
  game ownership. Independent verification, strict-credit acceptance and
  native saved-route reachability remain separate gates. +0422 still needs
  its callback composition wired and audited; this PR supplies its missing
  initializer rather than claiming that caller's completion.

## Pinned flag split after #1433 (2026-10-23, no claim moves)

After #1433 the model sets AF after a non-zero shift per the pinned rule; raw Unicorn leaves it clear, so the audit failed on main at `coordinates-0` (`flags C=7256, original=7246`, AF).

The audit now declares the split through the shared `tools/overlay/pinned_shift_flags.py` (#1446): it tracks the last FLAGS-writing instruction the original executed and requires C to equal Unicorn **exactly**, except that after a word `DIV`/`IDIV` the six arithmetic bits must equal the pinned `instructions.h` values, and after a shift with a non-zero count AF must be **set**. A model that reverted to the Unicorn value fails. Nothing is masked.

Run (`--exe/--ovr`): **2,889** full CPU/RAM cases (all returns); the split was used at **all 2,889** stops, every one after a shift (the body ends in one).

**Per-credit sentence (OVL_03C2_01A0): evidence did not depend on AF after a shift**; every stop holds C to AF=1 and all other bits are exact. Credit stands.

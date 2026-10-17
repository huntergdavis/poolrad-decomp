# U5E +17F6: linked-record direction update

The complete public entry 11 increments a linked record's byte at +0F,
composes the complete +263D direction chain, and updates the linked record's
byte at +12 using its heading byte at +09. All reads, writes, stack locals,
flags and register effects remain on the supplied 1 MiB CPU/RAM state.
There are no result callbacks or host substitutions for callees.

## Identity and reuse

| Artifact | Extent | SHA-256 |
| --- | --- | --- |
| START.EXE | 65,231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206,105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U5E +17F6 | [17F6,1884), file [81905,82047), 142 bytes, 53 instructions | 0d16445d9e2c355857b7dc440d7f87f55001c9b6cbe79750c541a6cb76b430ef |

The descriptor maps public entry 11 to +17F6. Three aligned PUSH CS / near
CALL sites reach it from U5E +0627:+06E0, +0E82:+1040 and +2C3D:+2D1E.
Two fixup-backed far calls, U2F +0D45:+0DD4 and U31 +0B3C:+0EA2, target
stub 0068:0057. The audit pins all five caller bodies and verifies the
instructions and far-call relocation entries. These are static identity
and reachability signals, not a measured saved-game route.

Before implementing, the required recall command was run:
~~~sh
timeout 15 deja "U5E 17F6 full state"
~~~
It indexed changed sessions but returned no usable session excerpt within
the timeout. No indexed-session finding was reused.

The implementation directly reuses the merged
[U5E +263D full chain](U5E_DIRECTION_263D.md), its public CPU/layout/status types,
and its complete U3C2 +057B dependency. Local memory/flag helper semantics
and the synthetic fixture/source loader come from that implementation and
its audit, tools/overlay/u5e_direction_263d_audit.py. Existing dependency
files are unchanged. The new C entry is
pr_u5e_direction_gate_17f6 in overlay_u5e_direction_gate_17f6.c.

## Observable behavior

The first far-pointer argument at SS:entry-SP+4 names a record. Its +108
far pointer selects another record. The selected +0F byte increments with
8-bit wrap before the direction call; the updated counter remains visible
if the dependency reaches a host iteration limit. Both original far-pointer
arguments are pushed in original order for +263D.

After the call, the routine reloads both pointer levels, reads +09, and
calculates a signed remainder of (direction - heading + 8) / 8. It stores
the remainder's low byte in a stack local. If that byte is unsigned greater
than four, it replaces it with the low byte of 8 - value. It reloads the
pointer chain, adds +12, calculates another signed remainder by eight,
reloads the pointer chain again, and writes the result to +12.

This preserves behavior outside ordinary direction values. For example,
direction 0 and heading 255 produce signed remainder -7, stored as byte
249, then folded to byte 15. With initial +12 equal to seven the result is
six. Treating the first division as unsigned would incorrectly produce zero.

The return executes RETF 8. BX and SI are untouched; the other registers,
flags, stack residue and record effects are part of the full-state result.
Repeated pointer reads are intentional, including when data aliases the
stack or pointer storage.

## API boundary

- Enter at IP=17F6 after far CALL, with two far-pointer arguments and
  entry SP in [0032,FFF4]. The deepest composed frame is 50 bytes below
  entry SP. All accepted initial frame windows stay within 16-bit offsets.
- RAM is exactly 1 MiB; physical addresses wrap at the A20 boundary.
  Word and LES operand bytes are physically contiguous. Effective-address
  arithmetic and SP use 16-bit wrapping.
- CPU, layout and RAM are distinct stable objects. Code is immutable;
  data writes must not overlap modeled code. Reserved stack/code overlap
  and invalid initial arguments reject atomically before the counter update.
- The existing layout supplies loaded U3C6 and U3C2 code segments. Overlay
  loading itself remains outside the routine.
- max_iterations counts +263D slot-search iterations and predicate attempts.
  LIMIT exposes the exact next +0C71 or +267F state. It is a host boundary,
  not a DOS return or a claim that cyclic behavior terminates.
- A non-OK callee result exposes its reached state. A changed nested return
  produces TRANSFER. OK means the outer RETF 8 executed, including when
  a data alias changed the outer return address.
- Logical AF is chosen as zero; IDIV preserves its incoming flags, matching
  the selected Unicorn profile. The native ISA leaves division flags
  undefined. This profile is explicitly not a universal native FLAGS claim.

## Validation

The owner-media audit runs original instructions for the new caller,
+263D, U3C6 +0BE7/+0C0F/+0C5F, and U3C2 +057B. Far calls execute loaded
EA stubs; no result-only dependency stubs are used. It compares all 14
CPU words and every byte of the 1 MiB RAM, including exact limit and return
boundaries. Two mapped views share the same backing RAM for A20 aliases.

The final audit passed all 2,794 cases: 2,790 outer returns and four exact
iteration limits. Coverage reached 53/53 caller instructions, 41/41 +263D
instructions, 34/34 slot-reader instructions, 32/32 slot-search instructions,
and 352/410 predicate instructions. The corpus includes forced direction
eight, counter aliasing the linked-pointer low byte, and counter aliasing
its segment low byte. Standalone coverage of all 410 predicate instructions
remains in the dependency's existing audit; this caller's corpus does not
claim to reach every standalone predicate state. No nested TRANSFER was
observed in this corpus.

The recovered library build, all three focused CTests, strict compiler
warnings, and the new test under AddressSanitizer/UndefinedBehaviorSanitizer
passed. No original media or derived byte listings are committed.

The corpus covers all heading, accumulator and counter byte values,
compass paths, signed-negative remainders, stack bounds, zero-count and
missing slot searches, relocated segments, DS/SS aliases, record/stack
aliases, LES offset wrapping, A20 wrapping, and seeded random fixtures.
The separate focused C test checks counter wrap, the signed-remainder
regression, the pre-limit increment, return ABI, and atomic rejection.

Run with owner-supplied media kept under ignored resources/:
~~~sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
python3 tools/overlay/u5e_direction_gate_17f6_audit.py \
  --exe resources/oracle-runs/slot-a-ui-second-cycle-callers-p9-025/POOLRAD/START.EXE \
  --ovr resources/oracle-runs/slot-a-ui-second-cycle-callers-p9-025/POOLRAD/GAME.OVR
~~~

## Adversarial parity review

- target_function: OVL_005E_17F6, public entry 11.
- source_candidates: overlay_u5e_direction_gate_17f6.c plus the unchanged
  full-state +263D and +057B dependencies.
- signals: exact body hash/bounds, descriptor entry, five verified call
  sites, one internal call, ordered pointer traversal, signed divisions,
  stack effects, branch coverage and independent original-instruction
  CPU/RAM comparison.
- score: strong behavioral correspondence within the stated contract;
  no numerical confidence probability is inferred.
- verdict: match within the documented CPU/RAM contract.
- review_notes: the current queue labels this identity
  game_or_other_library_unresolved. Its linked game-record effects and
  direction-chain dependency add ownership evidence but do not change the
  classifier or strict ledger here. Native saved-route execution, division
  FLAGS characterization and strict-credit review remain separate gates.

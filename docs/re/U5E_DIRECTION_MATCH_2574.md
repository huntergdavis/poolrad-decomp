# U5E +2574: record gates and direction match

This complete routine checks two optional record pointers and a signed
record byte, then conditionally composes the full +263D direction chain.
It returns its predicate in AL while preserving the original effects on
AH, all other registers, flags and stack memory. It has no result callback.

## Identity and reuse

| Artifact | Extent | SHA-256 |
| --- | --- | --- |
| START.EXE | 65,231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206,105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U5E +2574 | [2574,263D), file [85359,85560), 201 bytes, 64 instructions | 81c15df9fdd2f0693d2a4f1dd7e55ba75f049a8a785bba01b6c092ed98e3b101 |

Three aligned PUSH CS / near CALL sites identify this internal routine:
U5E +0191:+01EB and +1406:+15AD/+1650. The audit pins both complete
upstream caller bodies and verifies all three call instructions and their
preceding PUSH CS. Its sole call is +2619 -> +263D. These are static
reachability signals; they do not establish a native saved-game route.

Before implementation, the required recall command was run:
~~~sh
timeout 15 deja "U5E 2574 full state"
~~~
It indexed changed sessions but returned no usable session excerpt before
the timeout. No indexed-session finding was reused. Fetched remote refs,
all-ref file history and the primary/Claude/DS4 source directories were
also checked for an existing +2574 translation; none was found.

The implementation composes the merged
[U5E +263D full-state chain](U5E_DIRECTION_263D.md) through its existing CPU,
layout and status types. That includes the complete U3C6 slot readers and
slot search, plus U3C2 +057B. Existing dependency files are unchanged.
The local memory/flag helpers and exact return-boundary audit mechanism
are adapted from the +17F6 work in PR #867; that PR is not a build or
runtime dependency. Both audits reuse the merged +263D synthetic fixture
and hash-pinned source loader. The new entry is
pr_u5e_direction_match_2574 in overlay_u5e_direction_match_2574.c.

## Branches and observable state

The second far-pointer argument, SS:entry-SP+8, names the record used for
these gates. The routine copies its +CC pointer pair into unaligned stack locals.
If that pair is not raw zero, it loads the pointed record's +2E byte into
another local. When the pointer is null, that local remains unwritten;
the later null test bypasses its use. It then copies the +D4 pointer pair
into another pair of unaligned stack locals.

The record gate requires all of the following, in original branch order:

- The +9C byte is signed positive (1 through 127).
- The +D4 pointer is raw null, or its pointed +2E byte equals 50.
- The +CC pointer is raw null, or its pointed +2E byte is 7, 8, 35, 36 or 37.

Null means both stored pointer words are zero. A nonzero far-pointer pair
that physically aliases address zero is still non-null. The C model
retains the original word reads and OR flags rather than replacing the
pointer check with a normalized host pointer.

If these gates pass, the first argument's +108 far pointer selects another
record. Its +0F byte must exceed one as an unsigned byte. Only then does
the routine push the original arguments and execute +263D. After return,
it reloads both levels of the first pointer and compares the returned AL
with the selected record's +09 byte. Equality returns AL=1; all rejection
paths return AL=0.

AH is not cleared when writing the return byte. False paths can therefore
return a nonzero AX word with AL=0. Stack pointer pairs, the condition byte,
the optionally unwritten type byte and the result byte remain in RAM.
The complete function executes RETF 8. It does not directly modify the
record fields; dependency effects and stack writes remain observable.

## API contract

- Enter at IP=2574 after far CALL, with two far-pointer arguments and SP
  in [003C,FFF4]. The deepest composed stack frame is 60 bytes below
  entry SP. Initial frame windows do not cross the 16-bit offset boundary.
- Supply exactly 1 MiB RAM and distinct stable CPU/layout/RAM objects.
  Effective offsets and SP wrap at 16 bits; word and LES operand bytes are
  physically contiguous with A20 wrapping.
- Code is immutable and data writes must not overlap composed code.
  Invalid initial arguments and reserved stack/code overlap reject
  atomically. The layout describes already-loaded overlay code segments;
  overlay loading remains outside this function.
- max_iterations counts +263D slot-search iterations and predicate
  attempts. A failed outer gate does not consume a dependency iteration.
  LIMIT exposes the exact next +0C71 or +267F state, not a DOS return.
- Non-OK dependency status exposes reached CPU/RAM. TRANSFER identifies a
  changed nested return. OK means the outer RETF 8 executed, including
  any initial-data alias that changes the outer return frame.
- Logical AF is chosen as zero, consistent with the dependency and selected
  Unicorn profile. No universal native undefined-FLAGS behavior is claimed.

## Validation

The audit executes original instructions for the caller and complete
+263D/U3C6/U3C2 chain. Loaded EA stubs implement the far jumps; no dependency
is replaced by a result-only stub. Every case compares all 14 CPU words
and all 1 MiB RAM. Shared backing RAM maps both sides of the A20 boundary,
and RETF is stepped separately at outer-return or changed-return boundaries.

All 2,301 original-chain/C cases passed: 2,297 outer returns and four
exact iteration limits. Instruction coverage reached 64/64 for +2574,
41/41 for +263D, 34/34 for the slot readers, 32/32 for the slot search,
and 320/410 for +057B. The dependency's standalone audit covers all 410
predicate instructions; this caller's corpus does not claim that coverage.
No nested TRANSFER was observed. Strict compiler warnings, the recovered
library build, three focused CTests and AddressSanitizer/UndefinedBehaviorSanitizer
checks passed. Original media and derived byte listings remain ignored.

The corpus exhausts each signed gate byte, both pointed type bytes, the
counter and heading byte; combines gate boundary values; and exercises
raw nulls versus equivalent physical pointers, compass/heading combinations,
iteration limits, relocated segments, stack bounds, stack/table/record
aliases, independent far-pointer word offset wrapping, LES wrapping and
A20 wrapping. Focused C tests additionally assert the signed-byte,
nullable-pointer, allowed-type and early-return semantics.

Run with owner media under ignored resources/:
~~~sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
python3 tools/overlay/u5e_direction_match_2574_audit.py \
  --exe resources/oracle-runs/slot-a-ui-second-cycle-callers-p9-025/POOLRAD/START.EXE \
  --ovr resources/oracle-runs/slot-a-ui-second-cycle-callers-p9-025/POOLRAD/GAME.OVR
~~~

## Adversarial parity review

- target_function: OVL_005E_2574, internal routine [2574,263D).
- source_candidates: overlay_u5e_direction_match_2574.c and its unchanged
  +263D/+057B full-state dependencies.
- signals: body hash and instruction extent, three aligned original call
  sites, the sole +263D call, signed/unsigned branch distinctions, raw
  pointer comparisons, unaligned locals, return ABI, and independent
  original-instruction CPU/RAM comparison.
- score: strong behavioral correspondence within the stated contract;
  no numerical confidence probability is inferred.
- verdict: match within the documented CPU/RAM contract.
- review_notes: the current queue classifies this identity as
  game_or_other_library_unresolved. Record accesses and the full direction
  dependency add ownership signals, but classification and strict-ledger
  credit are left to independent review. Native saved-route reachability
  remains open. This work does not claim whole-game executability.

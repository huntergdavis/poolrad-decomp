# U5E +263D direction search composition

The original caller takes two far-pointer arguments, reads their two slot
coordinates through U3C6 entries 12/13, and tests byte directions starting
at zero through U3C2 entry 4. It returns the first accepted direction in AL.
The C composition retains the complete CPU and shared 1 MiB RAM, including
every frame write and raw far-pointer comparison. It does not substitute
host pointers, separated arrays, or result-only callees.

Implementation: [caller and slot helpers](../../src/recovered/overlay_u5e_direction_263d.c),
[API](../../include/poolrad/recovered/overlay_u5e_direction_263d.h),
[focused test](../../tests/recovered/test_overlay_u5e_direction_263d.c),
and [original-byte differential audit](../../tools/overlay/u5e_direction_263d_audit.py).
The [U3C2 +057B leaf](U3C2_ENTRY4_057B.md) is included in this delivery
following the DS4/VP ownership handoff. Every composed callee executes C.

## Loaded-state precondition

Added 2026-10-22 by par-e (credit authority), VP i21329, ruling i21377. This states a condition the credit always
assumed; the credit is unchanged.

This credit holds **only while every overlay unit the body far-calls is resident**.
The C now reads each listed live `LCALL` binding and requires its stub to hold a
five-byte far `JMP` (`EA`) to the expected entry. Explicit callee-segment
arguments must agree with the jump target; U6A's predicate uses the live jump
segment. If a required unit is absent or a binding is malformed, **the C
refuses with `ARGUMENT` before changing CPU14 or RAM**. The caller no longer
has to enforce this listed precondition on the C's behalf.

Bindings and code must remain immutable throughout the synchronous call.
The overlay manager's DOS loading path is still outside this C model.
Loaded-state differentials execute real far-JMP stubs; separate host checks
exercise cold stubs and malformed bindings and verify atomic refusal. Host
checks are not original executions and add no parity-case or credit count.
Updated validation results are pending; see
[the guard work report](../LOADED_STUB_GUARDS.md).

| Credit | Stub | Target | Call sites |
| --- | --- | --- | --- |
| `OVL_005E_263D` | `03E0:005C` | unit `0x3C6` entry 12 `+0BE7` | `+2649, +2665` |
| `OVL_005E_263D` | `03E0:0061` | unit `0x3C6` entry 13 `+0C0F` | `+2657, +2673` |
| `OVL_005E_263D` | `03DB:0034` | unit `0x3C2` entry 4 `+057B` | `+2693` |

Stubs were resolved from the original bytes: each `LCALL` segment is matched to its unit's `memory_segment`, and
the slot `0x20 + 5 * entry` gives the entry. SYSTEM (`0AF8`) calls are resident and need no stub.

## Source identity

Original media: START.EXE, 65,231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd;
GAME.OVR, 206,105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

| Unit span (end exclusive) | GAME.OVR start | Bytes / instructions | SHA-256 |
| --- | --- | --- | --- |
| U5E [263D,26B0) | 0x14E38 | 115 / 41 | 7983c5f0344f78278aa7227b2a7ae964a5ab77cfe2be70a42a91bf179cd3c297 |
| U3C6 [0BE7,0C37) | 0x31321 | 80 / 34 | 747df8db81cfc6b78ff0ac95e85882142ec90197519513779fa46d82de19e479 |
| U3C6 [0C5F,0CB9) | 0x31399 | 90 / 32 | c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41 |
| U3C2 [057B,0909) | 0x300CD | 910 / 410 | 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae |

The audit verifies the entry descriptors and all five caller relocation
sites. U5E public entry 18 is stub 0068:007A. Its four coordinate calls
target 03E0:005C/0061; its predicate call targets 03DB:0034. The slots use
PUSH CS followed by near CALL +0C5F, then RETF 4. The predicate returns
with RETF 10, and the outer caller with RETF 8.

## Memory and execution contract

The slot helpers search DS:65B9 with a byte index incremented before each
comparison, comparing both far-pointer words. A zero slot-count byte means
the search can visit all 256 entries, including slot zero. Coordinate
loads use DS:5F27/5F28 plus four times the returned index. Reads are not
cached across original writes, preserving stack/table alias behavior.

The caller stores four coordinates at BP-3 through BP-6 and its direction
counter at BP-2. Each predicate attempt pushes the live AX words, including
their high bytes, in source order. Invalid coordinates can make every
predicate attempt return zero; the original caller then loops and its
byte counter wraps. The API does not replace that behavior with a result.

Each slot-search iteration and predicate attempt consumes one unit of a
positive host budget. Exhaustion returns LIMIT before U3C6 +0C71 or U5E
+267F, with the reached CPU and RAM. This is not an original DOS return.
The initial SS stack window must not wrap: 34 <= SP <= 0xFFF4. A modeled
reserved stack window overlapping a composed code span is rejected before any
mutation. Other supported data/stack aliases use shared RAM. Memory
addresses use 16-bit effective offsets and A20-off physical addressing;
word bytes are physically consecutive. The audit's current cases remain
below the physical 1 MiB boundary.

The caller supplies actual loaded code segments, not overlay identifiers.
This assumes stable far-jump stub bindings and immutable original code;
the overlay loader, asynchronous changes, and self-modifying code are
outside the contract. The audit executes synthetic EA stubs with actual
loaded targets and relocates only the original call segment words. C
performs the same control transfer directly. Undefined logical/shift AF
is zero by convention; matching Unicorn does not establish a native AF
policy. Stack-wrap/RETF boundary profiles and native saved-route
reachability remain unmeasured.

## Validation and parity review

The original-byte differential passed 307 complete-chain cases, comparing
all fourteen CPU words and all 1,048,576 RAM bytes. All 41 caller
instructions were exercised and returned direction bytes 0 through 8 were
observed. Cases include signed bounds, slot-count zero, missing pointers,
physically equivalent but unequal raw far pointers, relocated code/stubs,
accepted stack limits, shared DS/SS, table aliases, byte-counter wrap,
exact budget stops, and 200 seeded random coordinate/displacement cases.

The standalone dependency differential passed 506 cases and exercised all
410 predicate instructions: all 256 modes, signed bounds, both early
equality paths, sixteen stack/displacement-table aliases and 200 seeded
random cases. Both focused CTest targets passed, as did strict-warning
compilation and AddressSanitizer/UndefinedBehaviorSanitizer runs.

Parity review: target_function = OVL_005E_263D; source_candidates =
pr_u5e_direction_263d; signals = exact source hash, descriptor/fixup
resolution, ordered shared-memory effects, all-C original-byte CPU/RAM
comparison; score = four evidence classes; verdict = match within the
documented execution contract. Review notes: this is a complete C chain
with an explicit host iteration boundary and stable loaded-code binding
preconditions. Native route reachability and stack/A20 boundary profiles
remain separate. No strict-credit or denominator change is claimed before
independent review.

## Reuse and reproduction

The pre-analysis deja/MCP queries and independently decoded call chain
are recorded in [the reconnaissance checkpoint](U5E_263D_RECON.md).
Existing [slot helper evidence](U9E_0160.md) and the folded helpers in
overlay_u3c6_1298_cpu.c supplied comparison points for stack order and
slot semantics. The audit organization follows
[U5E +2879](U5E_PARTY_VALUE_2879.md). None of their prior parity claims is
treated as execution of this composition.

To reproduce with private owner-supplied media:

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u5e_direction_263d_audit.py \
  --exe /path/to/private/START.EXE --ovr /path/to/private/GAME.OVR
```

Only source, synthetic tests, counts, addresses, and hashes are public.
Original bytes, disassembly, and private oracle artifacts stay ignored.

## Declared AF split (2026-10-22, no claim moves)

After #1433, the model follows the pinned DOSBox-X rule that `SHL` with a non-zero count sets AF (`times_four`). Unicorn
leaves AF clear. One budget case (`predicate-budget`) stops while the last arithmetic-flag writer is `times_four`'s
`SHL DI,1`, so the audit failed on main with `flags C=7212, original=7202` (VP i21571).

The audit now declares that split, and nothing else:
- FLAGS may differ in **bit 4 only**;
- **only** when the last flag writer the original executed before the stop is a shift with a non-zero count (decoded
  from the original bytes at run time);
- **only** with C's AF = 1.

Every other bit and case is compared exactly. The run: predicate 506/506; chain 307/307, all 14 CPU words and all 1 MiB
of RAM equal, the split used in **1** case; caller coverage 41/41.

**What the split cannot see:** a model that reverted to AF = 0 would match Unicorn and pass. Pinning AF = 1 itself is
the job of the pinned-rule census (`FLAG_SEMANTICS_CENSUS.tsv`, shift-AF rows) and of DOSBox-X native runs. The credit
and its evidence are otherwise unchanged.

## Registered checker (2026-10-23, no claim moves)

The audit now has a `--compile-only` path. Without media, it builds and binds the C library with the audit's own
compiler command, prints `CHECKER_BUILD_OK`, and exits. It also declares `CHECKER_ROWS = ("OVL_005E_263D",)`, so CI
builds it on every change (VP i21672).

`credit_evidence_kind.csv` is regenerated in the same change. **The credit and its evidence are unchanged.** The full
media run is unchanged too: predicate 506/506, chain 307/307, the declared AF split used in 1 case.

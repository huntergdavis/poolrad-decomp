# U5E +1BF8: full-state neighbour candidate selection

This implements the complete 542-byte U5E entry-13 caller with its real
slot-reader/search, cell-lookup and SYSTEM set-membership effects. It uses
one live 1 MiB RAM image and a 14-word CPU state. No callee result, frame
cleanup or memory write is replaced by a semantic callback.

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
| `OVL_005E_1BF8` | `03E0:005C` | unit `0x3C6` entry 12 `+0BE7` | `+1C1D` |
| `OVL_005E_1BF8` | `03E0:0061` | unit `0x3C6` entry 13 `+0C0F` | `+1C3C` |
| `OVL_005E_1BF8` | `03E0:003E` | unit `0x3C6` entry 6 `+04BE` | `+1C67` |

Stubs were resolved from the original bytes: each `LCALL` segment is matched to its unit's `memory_segment`, and
the slot `0x20 + 5 * entry` gives the entry. SYSTEM (`0AF8`) calls are resident and need no stub.

## Source identity and composition

| Body | Bytes | Instructions | SHA-256 |
| --- | ---: | ---: | --- |
| U5E [1BF8,1E16) | 542 | 193 | e4d40ee52d77251769257793d73634d80def7043b5264c6e050ea98902c11e1a |
| U3C6 [0BE7,0C37), entries 12/13 | 80 | 34 | 747df8db81cfc6b78ff0ac95e85882142ec90197519513779fa46d82de19e479 |
| U3C6 [0C5F,0CB9), entry 15 | 90 | 32 | c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41 |
| U3C6 [04BE,052B), entry 6 | 109 | 42 | bf1db9df7666fe7c5615d826c14442fd10e08f768289f65ebfd3ac10daf2027e |
| SYSTEM [0DC0,0DE1), set IN | 33 | 13 | 753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c |

The caller starts at GAME.OVR file offset 82931, unit 0x5E entry 13,
runtime stub segment 0x68. The closed chain has 854 instruction bytes and
314 instruction starts. Its 32-byte set at caller CS:1BD8 is read live,
not embedded in C; SHA-256
bd8b38d0accf12b85906b44b8948209968e09cbada69948cc188e6e7b8daa876.

The original-byte tools pin START.EXE (65231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd)
and GAME.OVR (206105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614),
unit/entry identities, every body, complete decoding and call topology.
The caller's four far calls are +1C1D to stub 03E0:005C, +1C3C to
03E0:0061, +1C67 to 03E0:003E, and +1D73 to SYSTEM 0AF8:0DC0.
All four segment operands have original relocation fixups. The two slot
readers call +0C5F with PUSH CS / CALL near. No further call is reachable.

The C reuses the complete slot/search instruction models from
overlay_u5e_direction_263d.c, calls the standalone
pr_u3c6_04be_cpu_run with its explicit CONTIGUOUS_PHYSICAL return policy,
and calls pr_system_set_in_0dc0. CPU types are copied field by field at the
public API boundaries. Every callee runs on shared RAM with its actual
pushed return, argument and saved-BP words. Unexpected internal returns
preserve the observed CPU/RAM and report TRANSFER.

## Behavior

Entry SS:SP contains far return IP/CS, the address of a writable far-pointer
result, and the subject far pointer. The caller reserves 14 local bytes and
returns with RETF 8.

For each of nine live displacement pairs at DS:2736/273F, the caller looks
up the subject's slot twice, reads that slot's coordinate bytes, adds the
signed displacement bytes and truncates to byte coordinates. The composed
cell helper produces the map byte and occupant index.

A nonzero occupant index selects its raw far pointer from DS:65B9. That
pointer is written through the caller's output argument immediately, before
candidate checks. The record bytes at +010E must match the subject's; the
candidate byte at +011B must be below its byte at +0032. A lower +011B
replaces the current best. A candidate whose raw far pointer equals the
subject can also replace it when +011B is below half of +0032. Comparisons,
pointer reloads and intermediate output writes remain in source order.

For an unoccupied cell whose map byte is 31, the caller scans records 1
through the captured DS:6715 count. Each seven-byte record starts at
DS:66D6+7*i and contains a far pointer and coordinates. A nonnull record
whose byte at +010C is absent from the original set and whose coordinates
match the current position updates the remembered fallback index. The last
matching fallback wins.

After the nine positions, the best candidate wins when its saved +011B is
below 8, or when no fallback index exists. Otherwise the fallback record's
far pointer wins. The final pointer is reloaded live, its two words ORed,
and AL returns 1 iff that OR is nonzero. The meanings of the record fields
are intentionally left unnamed.

## API boundaries and profiles

pr_u5e_neighbor_1bf8 accepts RAM, CPU, a loaded-segment layout and a positive
iteration budget. The layout names loaded U3C6 and SYSTEM segments. The
tools execute actual far-jump overlay stubs with only the four declared
link segment words relocated; loading an overlay is outside this entry.

Initial SP must be in [002A,FFF4]. The deepest nested frame reaches 42
bytes below entry SP. This restriction keeps all initial/nested stack
frames and stack LES operands away from offset wrap. A reserved stack
window overlapping any modelled body or the set data rejects atomically.
RAM, CPU and layout must be distinct stable objects; loaded code, set data
and bindings must stay immutable and compatible.

Every reached outer-loop header +1C17, fallback-record header +1D42 and
slot-search header +0C71 consumes one shared budget unit. LIMIT stops
before that instruction with the reached CPU/RAM. It never substitutes a
successful return. Live aliases can change loop counters or tables, so
termination is not assumed.

The six output-word writes guard both physical bytes before storing. A
code/set alias reports CODE_ALIAS at the original write instruction. The
first word remains stored if only the second word aliases protected bytes.
Invalid arguments and reserved-stack/code overlap are atomic rejections.
A nested cell/runtime rejection or partial stop is exposed as CALLEE with
the nested CPU state retained.

Effective offsets wrap at 16 bits; word and LES bytes are contiguous at
20-bit physical addresses. Return frames use that contiguous profile;
accepted initial/nested frames do not straddle the segment end. Arbitrary
indirect LES seams are the declared Unicorn profile, not a universal
native CPU claim. The standalone cell helper's fail-closed LES behavior
is retained, though the accepted caller stack window avoids its seams.
Logical/shift AF is zero by convention; MUL follows the DOSBox-X
normal/386 undefined-flag profile. No callback or guessed field value is
used to hide a profile difference.

## Original-byte differential evidence

tools/overlay/u5e_neighbor_1bf8_audit.py compares status, all 14 CPU words
and all 1,048,576 RAM bytes against the complete original chain. Every
original RETF is executed and checked against its actual expected return
site; the outer RETF is observed wherever its live frame returns.

The full corpus passes 587 cases, reaching 314/314 instruction sites and
54/54 conditional outcomes. It includes candidate ordering and ties,
group/capacity rejection, raw-pointer comparison differences, fallback
presence/type/coordinate filters, signed coordinate boundaries, slot
search failure/count wrap, accepted stack-edge frames, live output aliases,
all six protected output-write sites, every budget-stop site and 400
seeded cases. There are zero FLAGS masks and zero emulator-state
substitutions.

| Outcome | Cases |
| --- | ---: |
| Complete return | 560 |
| Protected output-write stop | 14 |
| Budget stop | 13 |

The original oracle observes the same declared stop boundaries before the
declined instruction. Those status codes are a host API contract, not
features attributed to the DOS executable. Code/set immutability is checked
after each original run. Both AL=0 and AL=1 complete returns are measured.

## Native replay

tools/overlay/u5e_neighbor_1bf8_native.py creates synthetic DOS programs
which load the original caller, slot/cell, runtime and far-jump stub
segments separately. Original bytes, COM files and captures stay under
ignored resources/. DOSBox-X uses normal core and 386 CPU; executable
SHA-256 is 99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.

Each capture compares all 14 CPU words, 240 stack bytes and DS:[0000,8000)
(32768 bytes): 33008 RAM bytes total, with zero FLAGS masks. Stack windows
are EFC0..F01F, 0000..004F and FFC0..FFFF. The native comparison is this
declared RAM subset, not whole-1-MiB equivalence.

The corpus contains 30 complete original-chain returns and four partial
captures. Complete cases cover candidate/fallback decisions, type filters,
coordinate bounds, count-zero/slot-255 search, two output aliases and the
accepted SP=002A/FFF4 edges. The slot-255 case receives 8192 budget units,
enough for all 18 searches; the differential corpus also measures stopping
that case at the ordinary 4096 budget.

The four partial captures observe the first slot-search header (budget 1),
the first fallback-record header (budget 3), the final best-pointer code
write and the final fallback-pointer code write. For these only, a far jump
replaces the first instruction the model declines, and the observer records
logical CS:IP at that boundary. They are not complete returns. The observer
saves CPU state before copying RAM; its scratch PUSHF at SS:7FFE lies
outside the captured stack windows.

No native wrapped return frame, indirect LES operand seam, overlay-loader
operation or natural game route is claimed measured.

## Reproduce and checks

Make the owner media available at resources/DOS/POOLRAD, with Capstone and
Unicorn on PYTHONPATH:

    python3 tools/overlay/u5e_neighbor_1bf8_audit.py --output resources/u5e-1bf8/audit.json
    python3 tools/overlay/u5e_neighbor_1bf8_native.py

The focused C test has 35 checks covering independently expected outputs,
register/frame results, accepted stack bounds, exact budget boundaries,
live return-frame overwrites, partial two-word code-alias writes and atomic
argument/stack rejection. It passes strict C11 warnings, ASan and UBSan.
The complete recovered library and its registered focused CTest are also
built and run. The ignored evidence receipt records the exact author head,
source/tool hashes, capture hashes and validation-log hashes.

## Review and queue readiness

- target_function: OVL_005E_1BF8.
- source_candidates: overlay_u5e_neighbor_1bf8.c with the reused
  full-state slot/search, standalone cell and runtime set-IN implementations.
- signals: pinned complete call chain; real frames/live aliases; complete
  instruction and branch coverage; full-RAM original comparisons;
  independent native complete and partial captures.
- score: 5/5 listed evidence categories present for the declared contract,
  a checklist rather than a probability or acceptance decision.
- verdict: likely_match pending independent review.
- review_notes: budgets, immutable bindings, stack bounds and profile
  limitations must remain visible in upstream composition. Finite
  fixtures and branch coverage do not prove every input or CPU profile.

Schema-8 rank-2 U1C +0126 was skipped after checking actual APIs: its U25
+1815/+18EB compare helpers still expose semantic ecl_helpers interfaces,
and string comparison transitively needs SYSTEM +0C10. Their queue
readiness is not sufficient for raw CPU composition. This rank-3 caller
can instead use the full-state slot/search instruction models already
exercised in the accepted +263D chain. That caveat and this ownership
claim were sent to VP/reviewer before authoring.

Prior work reused: overlay_u5e_direction_263d.c and its original-chain
audit; standalone U3C6 +04BE (PR #937); system_runtime_leaves_cpu.c;
the native capture structure from u3c6_cell_04be_native.py; shared CPU/RAM
helpers from u86_procvars_3413_audit.py. Local deja recall returned no
matches; no recalled session finding was reused. No acceptance or credit
ledger is changed; reviewer owns independent verification and credit.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven in a worktree WITHOUT `resources/` (what CI has) with

```sh
python3 tools/overlay/check_checker_builds.py OVL_005E_1BF8 --require-current-main
```

## Pinned flag split after #1433 (2026-10-22, no claim moves)

After #1433 the model's times-four shift sets AF per the pinned rule; raw Unicorn leaves it clear, so the audit failed on main at `code-alias-(24576, 7160)` (`FLAGS C=0212, Unicorn=0202`, AF).

The audit now declares the split through the shared `tools/overlay/pinned_shift_flags.py`: it tracks the last FLAGS-writing instruction the original executed (decoded from the original bytes at run time) and requires C to equal Unicorn **exactly**, except that after a word `DIV`/`IDIV` the six arithmetic bits must equal the pinned `instructions.h:655-673/743-761` values, and after a shift with a non-zero count AF must be **set** (pinned `get_AF` = `count & 1Fh`). A model that reverted to AF=0 fails. Nothing is masked; the audit prints how many stops used the split.

Run (PR #1446): **587** cases (560 returns, 14 protected-write stops, 13 budget stops), `masked_flags=0`; the split was used at **6** stops, all after a shift (0 after a divide).

**Per-credit sentence (OVL_005E_1BF8): evidence did not depend on AF after a shift**; those 6 stops hold C to AF=1 and every other stop is exact. Credit stands.

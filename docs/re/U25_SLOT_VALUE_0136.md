# U25 +0136: complete slot-value composition

The new pr_u25_slot_value_0136 API closes the existing entry-1 caller body
with fixed implementations of both callees: word assembly +06AF and the
complete +0E0C load-word/classifier/load-hook/party-index chain. The public
API accepts CPU and RAM only. It preserves every reached effect and stops
on an unexpected nested return, without asking a host callback for results.

Source: [overlay_u25_slot_value_0136.c](../../src/recovered/overlay_u25_slot_value_0136.c).
Contract: [overlay_u25_slot_value_0136.h](../../include/poolrad/recovered/overlay_u25_slot_value_0136.h).
The existing callback API signature remains available. Its far-return boundary
correction is described below. The new implementation
uses a private fixed adapter to execute the two C callees through that body;
it exposes no replacement callback to callers. The two distinct CPU types
are copied member by member, avoiding alias-unsafe struct casts.

## Identity and reused evidence

GAME.OVR is 206,105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Its associated START.EXE SHA-256 is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
The audit reads the owner-supplied overlay and verifies every composed span.

| Body | Unit span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| Entry 1 | [0136,01B6) | 128 / 57 | 0dd25983c54922e1ebe6f9f1b693f32c6a1c2b69199ddf110fc7d040134ec42b |
| Word assembly | [06AF,06D3) | 36 / 16 | 53565df24104754f01dff68b10cf0dda3fe72ad394a5253c611ceb594d93e862 |
| Classifier | [06FB,0756) | 91 / 28 | e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347 |
| Party index | [0756,07BC) | 102 / 35 | b307e1dd310b369eeb068ef1e0fd7b84667e59382144bec02e5295b974c3fcb9 |
| Load hook | [07BC,0A74) | 696 / 233 | 6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451 |
| Load word | [0E0C,0F53) | 327 / 125 | e8f999e62ee3f04b194908f90ce7c0e15b0478a2af16a3843a1a076c86c67ce5 |

U25 begins at GAME file offset 74D3; entry 1 occupies [7609,7689).
Its two near calls at 018E and 01A7 target 0E0C and 06AF, each preceded by
PUSH CS. The outer RETF 2 is at 01B3. The published callable ledger records
52 far callers and grade-A game-evidenced identity; this PR changes no
classification or strict-credit CSV.

Required recall 'deja "0136 full state"' timed out after 15 seconds without
a session result. A subsequent RETF-boundary recall query also produced no
completed session result within 15 seconds. No indexed-session finding was reused. Reused repository
evidence is the [entry-1 source gate](U25_ENTRY1_0136_SOURCE_GATE.md), the
[complete load-word report](U25_LOAD_WORD_0E0C.md), and the
[party-index report](U25_PARTY_INDEX_0756.md). The word-helper transcription
uses the same original span and arithmetic helpers as the merged
[operand loader](U25_LOADER_0000.md). All body hashes and calls were rechecked.
DS4 acknowledged this ownership in inbox i17185; the helper is disjoint from
its reserved U1C handlers. This composition supplies their slot-value seam.

## Behavior and machine contract

Enter with IP=0136 and return IP/CS plus an index word at SS:SP. The low index
byte is sign-extended before **each** table read. Thus index FF accesses the
byte before each table base, and 80 means minus 128, not plus 128. The first
two reads from DS:7085 and DS:7045 are saved into live stack locals; the third
read from DS:7005 chooses the path. Aliases can affect later reads.

| Tag byte | Behavior |
| --- | --- |
| 00 | Zero-extend the saved low byte into the local result word. |
| 01,03,80 | Assemble an address, execute complete +0E0C, save returned AX. |
| 02,81 | Push the two byte-bearing words, execute +06AF, save returned AX. |
| Others | Read the preexisting local result word without initializing it. |

The full-chain audit exposed a bug in the existing caller epilogue at entry
SP=FFFE under the load-word/Unicorn profile: it wrapped the CS read to
SS:0000, while the original-byte oracle reads all four frame bytes
contiguously before wrapping SP. The epilogue now reads
CS at physical(frame)+2, modulo 1 MiB. The local +06AF transcription uses
the same rule. A separate 48-case boundary audit compares both outer and
nested RETF frames under normal and A20-wrapped stacks; all match, including
eight altered returns. These cases are retained in the complete corpus.
This is an explicit contiguous-frame profile, also used by the merged
[U33 grid driver](U33_GRID_DRIVER_11B4.md). Some other recovered APIs use
individually wrapped stack-word profiles. No universal CPU-boundary claim
or native +0136 FFFE/FFFF observation follows from this emulator comparison;
that native boundary gate remains open.

PUSH CS and CALL each wrap SP independently. Therefore a nested call frame
can straddle the offset boundary differently from RETF's contiguous read:
entry SP=0010/0011 on the load-word path or 0012/0013 on the pair path may
return to a different CS. The API exposes that actual state as TRANSFER;
it does not force the intended continuation. Synthetic regressions verify
these transfers and outer returns at FFFE/FFFF.

The real near calls receive far frames from explicit PUSH CS. Their original
return cleanup is executed, and the enclosing caller verifies return CS/IP
before continuing. The entry restores BP and executes RETF 2. Original
stack writes, flags, argument mutations and all callee effects remain visible.

RAM is exactly 1 MiB, synchronous and distinct from CPU. Loaded code must
remain immutable; data writes may not overlap code. Every initial 16-bit SP
is supported. Effective offsets and SP wrap at 16 bits; bytes within a word
operand remain physically consecutive modulo 1 MiB. No artificial minimum
stack offset is imposed by this API.

The complete load-word dependency may traverse the party list. A cyclic
list remains nonterminating, just as in the original; there is no fabricated
termination or loop budget in this API. TRANSFER exposes an altered nested
return. CALLEE exposes a rejected dependency. Invalid initial pointers,
RAM size or IP reject atomically. Undefined flag bits use the dependencies'
published conventions; successful C execution does not establish a native
whole-entry route or independent strict certification.

## Differential and synthetic validation

The [audit](../../tools/overlay/u25_slot_value_0136_audit.py) executes all six
original bodies and compares all fourteen CPU words and all 1 MiB RAM to the
C composition. It maps the same backing RAM at both physical megabytes so
original accesses above FFFFF wrap with A20 off. RETF instructions are stepped
to compare actual nested returns before permitting the caller continuation.
No original callee is replaced with a callback result.

Coverage includes all 256 index bytes across immediate/load/pair/unmatched
paths, all 256 tag bytes, every class-1 address, the complete existing
load-word fixture corpus, pseudo-field and record-byte sweeps, finite party
lists with count wrap, stack boundaries, physical A20 aliases, and added
stack/table aliases. The coverage gate requires every reachable instruction;
only +0EA4 is excluded, because the fixed classifier returns 0..4 and cannot
reach its caller's outside-class fallback jump.

All **5,581 cases pass**, comparing all 14 CPU words and every RAM byte:
5,567 outer returns and 14 unexpected nested transfers. Coverage is
57/57 caller instructions, 16/16 word assembly, 28/28 classifier, 35/35
party index, 233/233 load hook and 124/125 load word: **493/494**, with only
the explicitly unreachable +0EA4 excluded. The four focused CTests, strict
warning builds, ASan/UBSan and the full recovered library pass after the
far-return correction.

Focused synthetic tests check all index bytes and seven tag groups, actual
pseudo-field and active-record results, untouched locals, pair assembly,
wrapped/A20 stacks, return cleanup and atomic rejection. Existing caller,
load-word and party-index tests are retained alongside the new API tests.
Owner media and generated disassembly remain under ignored resources.

~~~sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u25_slot_value_0136_audit.py --ovr resources/DOS/POOLRAD/GAME.OVR
cmake -S . -B /tmp/poolrad-slot-build
cmake --build /tmp/poolrad-slot-build --target poolrad_overlay_u25_slot_value_0136_tests
ctest --test-dir /tmp/poolrad-slot-build -R poolrad_overlay_u25_slot_value_0136_tests --output-on-failure
~~~

## Adversarial parity review

- **target_function:** OVL_0025_0136, complete entry 1 with both dependencies.
- **source_candidates:** pr_u25_slot_value_0136, existing entry-1 body,
  fixed +06AF transcription and complete pr_u25_load_word_0e0c chain.
- **signals:** pinned extents; both exact PUSH CS/CALL frames; signed index
  and independent table reads; actual unwritten locals and alias effects;
  independent original execution versus every CPU word and RAM byte.
- **score:** 5/5 listed evidence categories agree; this is not an exhaustive
  state-space proof.
- **verdict:** match within the published contiguous-frame machine profile.
- **review_notes:** a zero default for unknown tags, unsigned table indexing,
  cached table reads, result-only callbacks or dropping callee state on a
  transfer would all change behavior. All CPU fields cross the private type
  adapter explicitly. Cyclic lists, immutable code and undefined flags are
  stated contract limits. Native caller-entry/route parity remains open.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0025_0136 --require-current-main
```

## Pinned flag split after #1433 (2026-10-23, no claim moves)

After #1433 the composed `+0E0C` load-word path sets AF after a shift per the pinned rule; raw Unicorn leaves it clear, so the audit failed on main at `composed-address-c04d` (`flags: 0213/0203`, AF), the same stop as the `+0E0C` audit.

The audit now declares the split through the shared `tools/overlay/pinned_shift_flags.py` (#1446): it tracks the last FLAGS-writing instruction the original executed and requires C to equal Unicorn **exactly**, except that after a word `DIV`/`IDIV` the six arithmetic bits must equal the pinned `instructions.h` values, and after a shift with a non-zero count AF must be **set**. A model that reverted to the Unicorn value fails. Nothing is masked.

Run (`--ovr`): **5,581** slot-value cases (14 internal transfers), 13 CPU words and all 1 MiB of RAM equal; the split was used at **272** stops, all after a shift.

**Per-credit sentence (OVL_0025_0136): evidence did not depend on AF after a shift**; those stops hold C to AF=1 and every other stop is exact. Credit stands.

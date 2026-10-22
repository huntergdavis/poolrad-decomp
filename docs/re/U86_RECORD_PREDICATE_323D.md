# Unit 86 +323D record-table predicate

## Selection, ownership and provenance

The private search expanded the 898-start ledger beyond the historical
game-evidenced shortlist, checking complete reachable call-free CFGs and
joining incoming callers. U86 +323D was selected at base
a55000acc09f780621ca500346e7557f30b7143d: it is 77 bytes with ten aligned
game-path callers and a specific record/table operation. The canonical
classifier currently labels it helper_candidate because it records no fixed
DS global: its DS table access is indexed through DI. That label is retained
here; caller/record evidence supports game review, not an automatic promotion.

The disjoint reserved follow-on is OVL_0038_31A2, +31A2..38A8 inclusive,
1799 bytes/556 instructions, SHA-256
4a60eda9584cb62276f52b14e2ae7af2077857c521f3d5a835a344f2f50b2519.
Its effect-table installer role is already documented in
OVERLAY_UNIT38_EFFECT_ENTRIES.md and FUNCTION_LEDGER.md, and its caller is
the game-verified U36 +002D allocator at +060C. Neither selected original
had an existing C/header/test implementation, strict manifest credit or open
PR collision. U7F +05E0, U6A +1601, the U1C menu lane and current assigned
PRs were excluded. No canonical classifier/identity/count output is edited.

Owner-supplied START.EXE is 65231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR is 206105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The exact function is file [172629,172706), unit 86:+323D..3289 inclusive:
77 bytes/29 contiguous instructions, SHA-256
2ec1f95931eeb474593d0733ece3c8d582ab3a2a4d1c23b9de59d8f0bb5e87cd.
There are no calls, interrupts, external branches or loader-fixup operands.
RETF 4 at +3287 consumes four argument bytes after the far return frame.

## Caller and game-role evidence

This is U86 public entry 6, stub 0096:003E. The source audit independently
checks every far call's segment-word fixup and decodes from the containing
function start to establish instruction alignment:

| Caller function | Aligned call site |
| --- | --- |
| U31 +03E4 | +04A8 |
| U6A +0000 | +008A |
| U6A +019C | +01C3 |
| U6A +0B5B | +0C8D |
| U77 +0D70 | +0DA1 |
| U77 +0EF8 | +12E4 |
| U77 +1A8A | +1AA4, +1C22 |
| U7D +09E7 | +0A22 |

The local caller U86 +051C uses PUSH CS; CALL +323D at +0571/+0572.
Most incoming functions are already game_evidenced; the raw-call scan alone
does not assign ownership. Independent record/table evidence also matters:
the leaf reads record byte +2E as an unsigned index into a 16-byte DS table
at 5582. The reviewed resident character-link routines use the same linked
record byte and 16-byte table family at 558E/5590; see
RESIDENT_CHARACTER_LINK_CHECK.md, RESIDENT_CHARACTER_LINK_FLAGS.md and
RESIDENT_CHARACTER_TARGET_SELECT.md. The meanings of the record type and
the indexed category byte are not named here. No class/race/UI meaning is
inferred merely from the numeric values.

## Complete raw behavior

pr_u86_record_predicate_323d takes 1 MiB RAM and the 14-word CPU layout
AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS after the far CALL.
The record offset/segment are at SS:[entry SP+4]/[entry SP+6]. It returns
AL=1 iff the pointer is non-null and the indexed table byte is 11, 12 or 13.
A null pointer returns AL=0 without changing ES/DI/CL. AH returns zero.

The non-null path performs LES, reads record +2E, zero-extends via XOR AH,AH,
sets CL=4, shifts DI left four, and compares DS:[DI+5582] to 10. A value
above 10 causes the same LES/record/table sequence again and compares to 14.
The C retains those separate reads. ES and shifted DI remain from the last
sequence; CH is preserved and CL becomes four. CMP supplies all returned
arithmetic flags on non-null paths, including defined AF. The null path's
final flag writer is OR AX,AX; only its AF is undefined. Intermediate XOR
and SHL flags can be folded because CMP overwrites them before observation.

The boolean is written to live SS:[BP-1], copied to AL, then the actual BP
and CS:IP are restored from RAM. RETF 4 advances entry SP by eight in total.
PUSH BP can alias record/table/argument memory, and those effects are retained.
Only host null/short RAM/CPU arguments are rejected; arbitrary record pointers
are accessed as DOS would access them.

Offsets and SP wrap at 16 bits. Word/LES/RETF operands read contiguous
physical bytes with A20 disabled (1 MiB wrap). Thus the initial pointer
test's two separate word reads and LES's contiguous four-byte read can differ
across offset FFFF; the C preserves both. Source code must stay immutable.
Undefined null-path AF is normalized to zero; defined non-null AF is checked.

## Reproducible verification

With owner media and Capstone 5.0.7/Unicorn 2.1.4:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u86_record_predicate_323d_audit.py --output resources/u86-323d-parity/differential.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u86_record_predicate_323d_native.py

The original-byte oracle passes 2251 cases, comparing all 1 MiB RAM bytes and
14 CPU words with 29/29 instruction sites covered. It checks all 256 table
values, all 256 unsigned record indices at seven boundary/category values,
null and single-word-zero pointers, record-offset/A20 wrapping, 54 frame/
table alias boundaries, and 128 seeded RAM/flag states. RETF is single-stepped
separately so an aliased return does not accidentally execute more source.

The fresh DOSBox-X normal-core 386 oracle passes 1025 synthetic actual
far-CALL frames: every table byte at indices 0, 127, 128 and 255, plus null.
All 14 CPU words, 48 frame bytes and record/table input words match C.
Interrupts are disabled from setup through capture (FLAGS=0002), then enabled
for DOS I/O. AF is checked on all 1024 non-null cases and masked only on null.
Result SHA-256:
245d3d2dfe75505d6ef498af1de43bdc3765f3efe61a0c0ba590f4de7e66d1ee.
DOS allocation segments may change this hash in another environment.
Copied source bytes, disassembly, assembly, COM images, output and logs remain
under ignored resources/. These are synthetic native frames; no reached game
caller or natural saved-game route is claimed.

Focused strict C tests verify guards, threshold precedence, unsigned index
255, CL/CH and null-path preservation, defined CMP AF, offset wrapping and
a saved-BP write changing the live table byte before comparison.

## Review and reuse

Target_function: OVL_0086_323D. Source_candidates: the new
pr_u86_record_predicate_323d implementation. Signals: pinned complete extent;
nine aligned/fixup-confirmed far callers plus the near caller; independently
reviewed record/table family; 2251 full-state cases with complete coverage;
1025 isolated native frames. Score: 5/5 listed behavioral checks pass, a
checklist rather than a probability or identity score. Verdict: match under
the immutable-code, A20-off, far-frame contract. Review_notes: category meaning
and reached-game state remain unmeasured; PM exact-head review decides game
identity and strict credit. No count promotion is included.

Bounded deja searches for the new leaf/caller lane and U86 +323D table 5582
yielded no completed prior-session finding. Reused repository findings are
the cited resident record/table notes, the unit-38 installer role, the existing
898-start inventory/ledger/callgraph and the U86 +3413 full-state/native oracle
method. The search results are private metadata, not a whole-game denominator.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0086_323D --require-current-main
```

# U3C6 +0CB9: original-byte and native parity evidence

The four-step neighbour scan already had a complete live CPU/RAM model, but
its published evidence was synthetic C only. This change supplies executable
original-byte and native checks for that model. It does not change the C
implementation, choose a return policy for callers, remove the LES guard,
or award function credit.

## Target and provenance

- target_function: OVL_03C6_0CB9, public entry 16, loaded offset +0CB9.
- source_candidates: src/recovered/overlay_u3c6_0cb9_cpu.c and its public
  header; the existing synthetic tests remain registered in CMake.
- Caller: GAME.OVR file offset 201715 (0x313F3), 344 bytes, 139 instructions.
- Complete folded chain: caller plus +0C5F (90 bytes/32 instructions),
  +0000 (104/41), and +04BE (109/42): 647 bytes/254 instruction starts.
- Near call sites +0CD8, +0D2A and +0D85 target those three bodies in that
  order. No relocation fixup lies inside a modelled body.
- The audit pins both input files, unit 0x3C6 at file offset 198458,
  runtime stub segment 0x3E0, public entry 16, every body hash, complete
  decoding, and the closed call set.

| Input | SHA-256 |
| --- | --- |
| START.EXE, 65231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR, 206105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Caller [0CB9,0E11) | 63f0434e2d8cadf426464601eed98ae42fa5d13b708303f45ac5440faf95d43b |
| Lookup [0C5F,0CB9) | c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41 |
| Pair [0000,0068) | 05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207 |
| Map [04BE,052B) | bf1db9df7666fe7c5615d826c14442fd10e08f768289f65ebfd3ac10daf2027e |

Original media, extracted bodies, generated programs and captures remain
under ignored resources/. Public tools contain synthetic initialization,
addresses, hashes and counts.

## Original-byte audit

tools/overlay/u3c6_scan_0cb9_audit.py executes the complete original chain in
Unicorn 2.1.4 before invoking the unbounded C implementation. It compares the
status, all 1,048,576 RAM bytes and all 14 CPU words. The original instruction
hook records visited sites and both outcomes of every conditional jump.
A fixture that exceeds 20,000 original instructions aborts before C; a
nonterminating fixture is never counted as a passing C comparison.

The full corpus has 1102 cases, covering 254/254 sites and 40/40 conditional
outcomes. It includes signed map bounds, negative direction pairs, class zero,
slot search failure and byte-counter wrap, zero/30/rank selection, equal and
descending ranks, occupant suppression, live output aliases, both explicit
return policies, low and wrapped stack frames, unsupported LES stops,
code-write stops, UNMEASURED stops and 800 seeded cases.

| C outcome | Cases |
| --- | ---: |
| RETURNED | 1082 |
| UNSCREENED_ALIAS | 2 |
| RETURN_CHANGED | 4 |
| RETURN_PENDING | 4 |
| UNSUPPORTED_LES_SEAM | 10 |

Two oracle differences are explicit and separately reported:

1. Six WRAPPED_SS cases need 20 return-CS substitutions after Unicorn's
   single-stepped RETF. The IP word, stack adjustment, preceding original
   instructions and every RAM byte remain compared. These are hybrid
   comparisons, not unmodified Unicorn runs. The native captures below
   independently exercise the wrapped returns. CONTIGUOUS_PHYSICAL receives
   no substitution; changed internal return addresses remain status 3.
2. Ten unsupported-LES stops expose the undefined AF after SHL: C uses 1,
   Unicorn uses 0. Only bit 0x10 of FLAGS is masked, and only for status 5.
   Two of these ten cases also belong to the six hybrid cases above; the
   groups overlap. Native captures at this stop compare the full FLAGS word
   without masking and measure AF=1. No other FLAGS bit is masked.

The original oracle stops before +0D29 when the declared unsupported LES
condition is reached, before a selected code-alias write, or before the first
RETF for UNMEASURED. These comparisons validate the retained partial state;
they do not imply that the original executable implements model status codes.

## DOSBox-X captures

tools/overlay/u3c6_scan_0cb9_native.py builds synthetic DOS programs which
execute the pinned original bodies under DOSBox-X normal core, 386 CPU,
A20-off real mode. The installed executable SHA-256 is
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.

All 27 captures match the WRAPPED_SS C profile, including all 14 CPU words
and 28,896 RAM bytes: 224 stack bytes plus DS:[0000,7000). The stack windows
are EFC0..F01F, FFC0..FFFF and 0000..003F. The DS window includes map data,
slot/rank/direction tables, output bytes, and the physical continuation above
the SS segment. Native FLAGS comparisons have zero masks.

- 25 captures return through the full original caller and all reached callees.
- Six have caller entry SP=FFFE or FFFF, exercising wrapped final return CS.
- Two full returns have entry SP=001C or 001D, exercising wrapped internal
  +0000 and +04BE returns.
- Two partial captures have entry SP=0014 or 0015, exercising the wrapped
  +0C5F return before reaching the unsupported-LES guard at +0D29. For these
  two only, the harness replaces +0D29 with a far jump to the observer.
  It records logical pre-instruction CS:IP at that boundary. The replaced
  instruction and subsequent original instructions are not executed, and
  these cases are not counted as complete returns.

The observer saves CPU state before copying RAM. Its scratch PUSHF uses
SS:7FFE, outside the captured stack windows. This is a measured RAM subset,
not a native comparison of the entire 1 MiB.

## Reproduce and existing tests

Make the owner-provided START.EXE and GAME.OVR available at
resources/DOS/POOLRAD. With Capstone and Unicorn on PYTHONPATH:

    python3 tools/overlay/u3c6_scan_0cb9_audit.py --output resources/u3c6-0cb9/audit.json
    python3 tools/overlay/u3c6_scan_0cb9_native.py

The existing C test passes 6367 checks under strict C11 warnings, ASan and
UBSan. A complete poolrad_recovered library build and the registered
poolrad_overlay_u3c6_0cb9_cpu_tests CTest also pass. No implementation change
was needed by these comparisons.

The ignored evidence receipt records the exact author commit, source and
tool hashes, capture hashes and check logs. Its path and SHA-256 accompany
the PR/reviewer handoff.

## Review judgment and readiness caveats

- signals: pinned full bodies and call topology; 254/254 executed sites;
  40/40 branch outcomes; whole-RAM original-byte comparisons; independently
  executed native returns and partial captures; existing negative controls.
- score: 5/5 listed evidence categories present for this declared profile;
  this is an author checklist, not a probability or acceptance decision.
- verdict: likely_match, pending independent review and credit.
- review_notes: no code mismatch was found in the measured corpus. The
  return policy is still explicit. The +0000 LES seam remains fail-closed;
  no actual LES operand at FFFD..FFFF was executed for parity. Code writes
  stop before modifying modelled instructions. Live aliases can make the
  original and C nonterminate; the public model has no budget. Native
  captures use synthetic inputs, not a natural game route or overlay-loader
  integration. Neither complete branch coverage nor these captures proves
  equivalence for every possible input/profile.

Queue readiness must preserve these contract limits when composing an
upstream caller. Main's existing model designation alone did not provide
original-byte/native evidence. Callee ownership and all acceptance/credit
ledgers remain unchanged; the reviewer decides whether this evidence upgrades
the caller's classification.

Prior work reused: the CPU model and ABI in U3C6_0CB9_SCAN.md; the original
oracle hook/return sequencing and native capture approach from
u3c6_step_071a_audit.py and u3c6_step_071a_native.py (PR #923); shared CPU/RAM
helpers from u86_procvars_3413_audit.py. Local deja recall for
"U3C6 0CB9 U3B0 0B03 next composable" returned no matches; no recalled session
finding was reused.

# U3C6 +04BE: standalone original-byte and native parity

The standalone range-checked cell lookup already had a complete CPU/RAM
implementation, but its published status was source-only and its synthetic
test had not been run at that head. This change adds standalone original-byte
and native capture tools. No C behavior or API is changed. Return policies,
code-write guards and the four unsupported LES boundaries remain explicit.
Independent verification and any credit belong to the reviewer.

## Target and provenance

- target_function: OVL_03C6_04BE, public entry 6, stub 0x3E0:003E.
- source_candidates: src/recovered/overlay_u3c6_04be_cpu.c and its public
  header, with tests/recovered/test_overlay_u3c6_04be_cpu.c.
- Body [04BE,052B), 109 bytes, 42 instruction starts, no calls or loops.
- GAME.OVR unit 0x3C6 begins at file offset 198458; this body begins at
  199672 (0x30BF8).
- The audit pins both media files, unit/stub/entry identity, body hash,
  complete decoding, absence of fixups/calls, and RETF 0xC at +0528.

| Input | SHA-256 |
| --- | --- |
| START.EXE, 65231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR, 206105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Body [04BE,052B) | bf1db9df7666fe7c5615d826c14442fd10e08f768289f65ebfd3ac10daf2027e |

Owner media, extracted bytes, generated DOS programs and captures stay under
ignored resources/. The public tools contain synthetic fixtures, addresses,
counts and hashes.

## Original-byte audit

tools/overlay/u3c6_cell_04be_audit.py runs the original leaf in Unicorn 2.1.4,
then compares the standalone C entry against all 1,048,576 RAM bytes and all
14 CPU words. Every FLAGS bit is compared: there are no masks.

The 1869 cases cover all 1250 valid grid cells, signed bounds, both successful
return policies, wrapped stack frames, live output aliases, A20-wrapped map
and output addresses, A20-wrapped code, all four LES stop sites, each
code-write stop, UNMEASURED returns, and 400 seeded cases. All 42 instruction
sites and all eight conditional outcomes are covered.

| Outcome | Cases |
| --- | ---: |
| RETURNED | 1823 |
| UNSCREENED_ALIAS | 14 |
| RETURN_PENDING | 8 |
| UNSUPPORTED_LES_SEAM | 24 |

The 14 code-alias cases stop at the initial BP push (six), or one of the
four byte stores (two each). The 24 LES stops cover each of +04F4, +0511,
+0519 and +0520 six times: all three unsupported operand offsets under both
return policies.

Four WRAPPED_SS return cases use one explicit return-CS substitution each
after Unicorn single-steps RETF. These four are hybrid comparisons, not
unmodified Unicorn returns. The other 1865 cases have no emulator-state
substitution. All original instructions before the RETF, its popped IP and
stack adjustment, and all RAM remain compared in the hybrid cases.
CONTIGUOUS_PHYSICAL uses Unicorn's unmodified return behavior.

The observer independently detects code writes and unsupported LES operands
from the current original CPU state and stops before the declined
instruction. UNMEASURED stops after POP BP and before RETF. These are
comparisons of the retained partial CPU/RAM state, not claims that the
original program implements model status codes. A 128-instruction original
budget catches unexpected execution; it does not change the public C API.

## Native captures

tools/overlay/u3c6_cell_04be_native.py runs synthetic DOS fixtures through the
original body under DOSBox-X normal core, 386 CPU, A20-off real mode.
The installed executable SHA-256 is
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.

All 39 captures compare all 14 CPU words and 28,896 RAM bytes: 224 stack
bytes plus DS:[0000,7000). Stack windows are EFC0..F01F, FFC0..FFFF and
0000..003F. The DS window includes map bytes, secondary cell bytes, output
bytes and the physical continuation above SS. No native FLAGS bit is masked.

| Capture type | Cases |
| --- | ---: |
| Complete original returns | 21 |
| Stop before unsupported LES | 12 |
| Stop before code-alias store | 4 |
| UNMEASURED stop before RETF | 2 |

The 21 complete returns include:

- four return frames with entry SP=FFFE/FFFF, on both valid and rejected
  coordinates, discriminating WRAPPED_SS from CONTIGUOUS_PHYSICAL;
- two entry-SP=1 frames, placing the pushed/restored BP word at SS:FFFF
  with its high byte at the next physical address;
- five live-alias fixtures: A overwrites the row, column, B offset word,
  saved BP or B destination. The row/column cases force the second
  coordinate calculation to read changed data;
- ordinary corners and signed rejection paths.

For the 18 partial captures only, a far jump replaces the first instruction
the model declines. The observer reports logical CS:IP at that boundary,
saves CPU state before copying RAM, and does not execute the replaced
instruction. Each of the four LES sites is captured at operand offsets
FFFD, FFFE and FFFF. These captures preserve earlier stores where applicable;
they do not execute or resolve LES at the seam. The four alias captures
similarly observe before the code write, and the two UNMEASURED captures
observe before RETF.

Frame setup writes an AX-valued word at SS:FFFF as two byte stores at
consecutive physical addresses. DOSBox-X's ordinary AX-to-moffs word store
raises a segment-limit exception there; that setup instruction is not part
of +04BE. The original BP push/pop and RETF are still executed unchanged
in the complete-return cases. The measurements do not imply that every
word-access opcode has the same segment-limit behavior.

The observer's scratch PUSHF uses SS:7FFE, outside the captured stack windows.
Native RAM comparison is a declared subset, not whole-1-MiB equivalence.
All these inputs are synthetic; no natural game route or overlay dispatch
is claimed measured.

## Reproduce and validation

Place the owner-provided media at resources/DOS/POOLRAD. With Capstone and
Unicorn available on PYTHONPATH:

    python3 tools/overlay/u3c6_cell_04be_audit.py --output resources/u3c6-04be/audit.json
    python3 tools/overlay/u3c6_cell_04be_native.py

The existing C test passes all 257 checks under strict C11 warnings, ASan
and UBSan. The complete poolrad_recovered library builds, and the registered
poolrad_overlay_u3c6_04be_cpu_tests CTest passes. No implementation defect
was demonstrated by these comparisons.

An ignored evidence.json receipt records the exact author commit, original
and model hashes, tool versions, capture hashes and validation log hashes.
Its path and checksum accompany the PR and reviewer handoff.

## Review judgment and readiness

- signals: exact body identity and complete decoding; complete instruction
  and branch coverage; whole-RAM original-byte comparison; independent
  native execution including live aliases and stack/return seams; explicit
  negative controls and partial-state observations.
- score: 5/5 listed evidence categories present for the declared contract;
  this is an author checklist, not a probability or an acceptance decision.
- verdict: likely_match, pending independent review.
- review_notes: native observations support selected WRAPPED_SS returns
  and physically consecutive stack words. Return policies remain caller
  selected. The four LES seam stops remain fail-closed; no seam instruction
  was executed for parity. Writes to modelled code remain unsupported.
  A20-wrapped data/code cases were compared in Unicorn, not native DOSBox
  captures. Coverage and finite fixtures do not prove every input/profile.

Queue readiness must carry these limits into upstream composition. The
folded +04BE implementation exercised by +0CB9 (PR #933) is separate from
this standalone API and was not treated as its verification. No acceptance
or credit ledger is changed here.

Prior work reused: the model/ABI in U3C6_04BE_CELL.md; the source pinning and
native observer approach in u3c6_scan_0cb9_audit.py and
u3c6_scan_0cb9_native.py (PR #933); shared CPU/RAM helpers from
u86_procvars_3413_audit.py. Local deja recall timed out; deja-health --repair
started an index rebuild. No recalled session finding was reused.

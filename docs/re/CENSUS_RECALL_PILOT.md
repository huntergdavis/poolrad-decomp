# Census recall pilot (t248)

**No new game function is established by this bounded pilot.** It found two
callable runtime-role helpers outside the 898-row queue plus seven extras,
45 false raw-call witnesses, and one unresolved sampled address. The original
947-row census catalog is unavailable, so neither helper is established as
absent from that catalog. The existing **815 CEILING** is neither validated
nor revised here. Independent static recount is pending.

## Measured scope and results

The successful static run used worktree base
2d13272135543bb6a749f10d87c102ad1dd3391c, GNU objdump 2.46, and these pins:

| Input or artifact | SHA-256 |
| --- | --- |
| START.EXE | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| 898-row comparison queue | ce7a5462da1f8f8b85c187c1d476a3b8fc8c4ddd0a6877ab5b5c638520ca3049 |
| Sampler source | 7c486730be79926bffa1e83887efcec7aeaa8cd8145c10503075bf17b07d3c90 |
| Final private pilot.json | 356686c7c5fdb49dc8f1d8678d84be782c29292fd0ec469e93391bf1955df01e |

| Measurement | Result |
| --- | ---: |
| Overlay code containers / bytes | 35 / 197,641 |
| Reconstructed resident image bytes (includes data) | 68,592 |
| Unique raw direct-call target candidates | 995 |
| Targets overlapping the 905-start comparison set | 730 |
| Targets outside that set | 265: 82 overlay, 183 resident |
| Comparison starts not captured by this method | 175 |
| Chosen outside-target sample | 48: 32 overlay, 16 resident |
| Recorded ingress witness rejected / helper / unresolved | 45 / 2 / 1 |

These are counts in the selected frame, not frequencies in play. The overlap
730/905 is **not census recall**: the scanner itself misses known starts and
also proposes operand bytes. Thirteen outside targets have fixup-backed far
sources; none landed in the chosen sample. They remain unreviewed.

The [48-row assessment table](census_recall_pilot_samples.csv) records every
selected address, source sites, containing-instruction ranges, and limits.
All 45 rejected witnesses place the E8 byte inside another decoded instruction,
including ModRM, displacement and immediate fields. This rejects the recorded
call witness under the stated alignment; it does not prove the target has no
other ingress. Source alignment uses a catalog anchor after independent
selection, so it is not an independent control-flow proof.

The two retained near-helper entries have three aligned source calls each:

| Image entry | Bounded interpretation | What remains open |
| --- | --- | --- |
| 0ABA8 | Converts a length-prefixed path and invokes DOS file open, using its caller's frame | Exact vendor origin and membership in the unavailable 947-row catalog |
| 0AE8B | Restores overlay trap stubs using an overlay descriptor's entry count | Same; no new game-ownership claim |

RES_IMAGE_094C3 is unresolved: its source exceeds the 2,048-byte context
bound. It lies in the prior report's driver-data interval, but that no-ingress
and data-typing proof was **not independently reproduced** here.

## Seven rejection spot-checks and controls

The [rejection table](census_recall_pilot_rejects.csv) records seven of the 90
excluded rows: four SYSTEM, two CRT, and the one overlay-manager policy row.
The 384-byte window hashes are **not function-body hashes**; windows can include
adjacent routines and can truncate a larger routine.

- SYSTEM 0AF80: runtime memory setup and exception-vector installation.
- SYSTEM 0B411: allocation-size normalizer. Its 17-byte body hash is
  440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d, matching
  the existing [GetMem evidence](SYSTEM_GETMEM_023F.md).
- SYSTEM 0B4E7 and 0B4EC: alternate file/text mode entries into a shared
  record-validation and dispatch core. They are not independent ownership trials.
- CRT 0A42E and 0A6BD: console setup, BIOS-tick calibration and its delay
  helper, with interrupt-1B installation in the initializer.
- 0AA20: a short error stub. Its SYSTEM callee and segment policy do **not**
  independently establish library ownership. This exclusion remains conditional.

Thus six runtime roles are corroborated, and one policy exclusion is not
independently established. Exact stock/vendor provenance remains unverified
for all seven; the other 83 exclusions were not spot-checked.

The known-game OVL_001C_0126 control reads documented ECL operand tags while
also calling SYSTEM string services. It defeats an exclude-on-SYSTEM-call rule,
which would wrongly exclude this game caller. This is a known-label sanity
check, not a blind classifier-accuracy test; the same shortcut cannot settle
0AA20.

Synthetic controls accept a literal descriptor marker and reject a wrong
marker or incomplete stub array. A raw E8 embedded in a MOV immediate creates
a candidate, while objdump shows its source and target inside instructions.
Withholding one known start per domain recovers OVL_005E_2574 and
RES_IMAGE_0C482 as outside addresses. That checks membership plumbing,
not discovery recall or ownership. These controls passed.

## Method, bias and ranges

[The sampler](../../tools/overlay/census_recall_pilot.py) generates candidates
**before reading census addresses**. It scans every byte for E8 rel16 targets
and scans relocation/fixup-backed far calls, resolving recognized overlay
entry stubs. Its container parser uses no prologue/function seeds. The existing
loader unpacker and format interpretation are shared dependencies, explicitly
not independent reconstruction evidence.

Only afterward does it subtract the queue and seven extras. It selects the
lowest SHA-256 ranks of t248-pilot-v1-2026-10-27, a NUL separator and canonical
ID: at most 32 overlay and 16 resident targets. Reject selection uses a
reject: prefix and the fixed 4/2/1 strata. Target windows are 192 bytes;
reject windows are 384 bytes. No larger sample or runtime test was performed.

The three retained proposals permit **0–3 previously uncataloged callable
starts**, conditional on this witness-based triage. That is not a bound on
hidden entries at the other addresses or elsewhere. No additional game-owned
body is proved. If the checkpoint's 335 repository-verified identities are
retained as a floor, the deliberately overinclusive 1,170-address union
(905 comparison starts plus 265 outside candidates) gives a conditional
ownership range **[335, 1,170] within that union only**. Neither endpoint is a
new credit measurement. It is not a whole-game count range.

For the actual whole-game question the honest statement is **[335, U], with U
not established by this pilot**, conditional on that recorded identity floor.
There is no justified statistical extrapolation or completion percentage.
The direct-call frame is biased; repeated instruction operands and shared
helper bodies are correlated. Computed/indirect transfers, near-only pointers,
fallthrough entries, CS:IP wrapping, unrecognized code regions and dynamically
modified code are not completely recovered. Plausible decoding alone is not
ingress, a function boundary, or authorship.

Dispatch i30058 reports that the original census2_starts.csv was not located
in the searched workspace/temporary roots. Its documented 947-row hash is
9efb9c49daec38ecc78d0431e5575db8813a0a44ee08e296efb79efeb7beea65.
No regenerated file is relabeled as that original. In particular, the two
runtime-role helpers may already belong to its additional library starts.

## Reproduce and audit trail

Run the sampler through the shared poolrad-heavy.sh, one job at a time:

    python tools/overlay/census_recall_pilot.py --exe resources/DOS/POOLRAD/START.EXE --ovr resources/DOS/POOLRAD/GAME.OVR --queue resources/t248/queue.json --output resources/t248/pilot-final

Output must stay under ignored resources/: it includes extracted windows and
disassembly. The JSON receipt pins the sampler/unpacker, inputs and decoder.
The public tables contain addresses, hashes and assessments, not original bytes.

Two failed attempts are retained privately. The new parser first accepted
header-like patterns without complete stub-array validation, then failed its
container-count guard. A bounded diagnostic found the literal 3F marker byte
had been treated as regex '?'. Escaping the marker plus structural controls
fixed the tool; v3 and the final run produced identical samples. The final run
also uses explicit exceptions for controls, so Python optimization cannot
silently remove their checks. Optimized-mode execution was not separately run.
These failures were not evidence of missing functions.

Memory sessions a3ad1536-abb2-4fb8-af2c-1362b8656678 and
4d9851e1-f261-48b9-9cfa-5876d1500eb7 supplied historical census/layout leads;
current media, format code and documents were then read directly. The
[CEILING report](FUNCTION_CEILING_CENSUS.md),
[queue description](FUNCTION_CENSUS_QUEUE.md), and t144 Step0/status artifacts
remain prior evidence, not independent sampling frames.

NOT VERIFIED: independent recount; exact vendor provenance; the remaining 83
rejects or 217 unsampled outside targets; full historical-census membership;
complete control-flow/alias recovery; new game bodies; whole-game recall or a
finite whole-game upper count; runtime/native parity; identity/credit/readiness.
No protected accounting file is changed.

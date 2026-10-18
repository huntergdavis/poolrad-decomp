# U77 +27DF native return profile

The complete U77 +27DF active-selection body has two explicit return profiles.
The existing pr_u77_active_select_27df API retains its contiguous physical
return read. The new pr_u77_active_select_27df_run API accepts either
CONTIGUOUS_PHYSICAL or WRAPPED_SS. This isolates a measured difference at
entry SP FFFE/FFFF without changing the existing API's CPU/RAM behavior.

With return IP at SS:SP, DOSBox-X normal/386 reads return CS at
SS:(uint16_t)(SP+2). The historical C and Unicorn read it at physical(IP)+2.
At SP FFFE/FFFF these locations differ by 64 KiB. Individual words still
use consecutive physical bytes, including a word starting at offset FFFF.
RETF 2 then advances SP by six. At FFFD both profiles read the same CS word.

The function's selection logic, live pointer reads, memory writes, flags,
frame restore, and retained cyclic-list nontermination are unchanged.
The policy is validated before any state mutation. CPU storage must not
overlap RAM; code is immutable and execution is synchronous. Flat A20-off
RAM does not model video devices, interrupts, or hardware faults. LES operand
semantics remain the existing contiguous profile; this work measures RETF,
not native LES seams or a naturally reached game frame.

## Source and reuse

The pinned 223-byte body is GAME.OVR [145433,145656), unit
+27DF..+28BD, 71 instruction starts, ending in RETF 2 at +28BB.
SHA-256: 461b9a087f9daa616a7bb52320c801cb4e7608f279f0b1105dca3c08746a9b1c.
The owner GAME.OVR SHA-256 is
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

The implementation and 975-case fixture set are reused from
[the existing U77 report](U77_ACTIVE_SELECT_27DF.md).
The native harness adapts the landing-marker, byte-seeding and capture
method from U6F repair 45314d7179e53ca5ccd1a882623bfce5319c6d6d.
Target-specific deja recall failed with an index append EOF; no recalled
session result is reused. No original bytes or captures are committed.

## Directed native measurement

tools/overlay/u77_27df_retf_wrap_native.py executes 39 synthetic programs:

- Three RETF-only probes copy the original final instruction unchanged.
- Thirty-six full-body frames cross null/head/middle/tail active selection
  with G/O/other key bytes and SP FFFD/FFFE/FFFF.
- Each frame compares CPU14, a landing marker, and 68 selected RAM bytes:
  48 stack/extension bytes, eight active/head global bytes, and twelve
  next-link bytes. The native output is 106 bytes per frame.
- The full-body checks also compare C's entire 1 MiB RAM with the original
  executed in Unicorn; this is separate from native selected-memory evidence.

Return-word setup uses byte stores for crossing words, avoiding a setup-time
segment-limit fault. The exact body is copied to COM offset 1100; relative
branches are unchanged. Two landing sites sixteen bytes apart distinguish
wrapped CS from a contiguous-CS decoy. Registers and RAM are captured before
PUSHF or DOS I/O, with interrupts disabled through capture.

The pre-change run shows 24 C/native CS mismatches in full-body cases:
all twelve selection/key scenarios at FFFE and FFFF. The repaired wrapped
profile has zero mismatches. The final harness still checks the legacy API
against all frames and requires those same 24 CS mismatches as a negative
control. Unicorn differs in CS in 26 frames, including the two RETF-only
boundary probes; the other thirteen frames agree on defined state.

AF is architecturally undefined after OR. Twenty-one full-body frames end
with OR flags and mask only AF bit 4. The remaining fifteen full-body frames
and all three RETF-only probes compare all FLAGS bits. The mask does not
cover CS or any defined flag; no native frame is excluded.

DOSBox-X executable SHA-256:
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
The harness records each capture hash and both divergence lists in JSON.
Native comparison is bounded to 90 seconds and 2 MiB of logs, and cleans up
its owned process group on failure. Products remain under ignored
resources/oracle-runs/u77-27df-retf-wrap-native/.

## Reproduction

Use PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay:

    python3 tools/overlay/u77_27df_retf_wrap_native.py       --ovr /path/GAME.OVR --report resources/u77-native.json
    python3 tests/recovered/check_overlay_u77_active_select_27df.py       --ovr /path/GAME.OVR --output resources/u77-dual-policy.json

The whole-RAM audit runs each fixture through both profiles. Legacy output
is checked against original Unicorn execution through RETF. Wrapped output
uses the native-established CS word rule; every other CPU word and all RAM
remain checked against original execution. JSON names every hybrid-return
and undefined-AF case separately, along with instruction and branch coverage.

The completed sweep passes 975 fixtures under both profiles: 1,950
CPU/RAM comparisons, all 71/71 instruction starts and 20/20 conditional
outcomes. Seven wrapped-profile cases substitute the native RETF CS rule;
the other 968 execute the same effective return as Unicorn. The 975
contiguous-profile comparisons execute the original RETF in Unicorn.
In each profile, 117 cases mask only the final OR's undefined AF bit;
858 compare all FLAGS bits. No fixtures are excluded or drawn from the
optional unrelated-route capture.

The focused C regression retains the historical contiguous-return assertion,
adds all three native boundary offsets, and verifies invalid-policy rejection
leaves CPU and RAM unchanged. Strict C11 warnings-as-errors and ASan/UBSan
passed, as did the registered U77 CTest.

| Review field | Assessment |
| --- | --- |
| target_function | Complete OVL_0077_27DF, return-profile boundary |
| source_candidates | Existing API and new explicit-policy API |
| signals | Exact source identity, native landing markers, old-profile negative control, whole-RAM dual-profile differential |
| score | Strong boundary evidence under the declared emulator/flat-RAM profile |
| verdict | likely_match, pending independent review |
| review_notes | Undefined OR AF remains declared; no natural replay, ownership promotion, or self-credit |

# U3C6 slot getters: reusable raw CPU interface

This change exports the already accepted U3C6 +0BE7/+0C0F/+0C37/+0C5F
execution as one reusable CPU14 API. U5E +32E6 now calls that shared API.
The four identities already have strict acceptance from PR #150; this is
an interface upgrade with **no new identity credit**.

## Identity and reuse

| Entry | Original stub | Span | Bytes / instructions | SHA-256 |
| --- | --- | --- | --- | --- |
| 12 | 03E0:005C | 0BE7..0C0F | 40 / 17 | eb48355d64c585ee95a31739074ede53929cc991ba519157009c7c8e1c9b7ed5 |
| 13 | 03E0:0061 | 0C0F..0C37 | 40 / 17 | a1000b2f6a29a0d74f37a9b631b26844044f2c1a163b2de793672cbc0664395d |
| 14 | 03E0:0066 | 0C37..0C5F | 40 / 17 | 71c5655d5b32a4688be0ab98761c4b5d2cfb4cc117056648ab92a552ffc06e48 |
| 15 | 03E0:006B | 0C5F..0CB9 | 90 / 32 | c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41 |

All four end in RETF 4. GAME.OVR file starts are 201505, 201545,
201585, and 201625; the last body ends at 201715.

- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

The implementation is extracted from the private raw slot engine in the
accepted [U5E +32E6 composition](U5E_CALLER_32E6.md), including its memory
and arithmetic primitives. The changes are public argument guards,
standalone +0C5F completion, and the caller's status/CPU adapter. The older
scalar slot API remains available. The raw public interface is not inferred
from a queue label.

Before implementation, bounded deja queries for the slot getter ABI timed
out; deja-health/repair was used, and the later health probe passed.
No recalled implementation was imported. The accepted +32E6 source and
evidence note are the reused implementation/provenance. VP messages
i18059/i18074 approve exporting these interfaces before U9E +0160.

## Contract

pr_u3c6_slot_getter_cpu accepts a CPU14 state, exactly 1 MiB of A20-off
RAM, and a slot-visit budget. Initial IP selects one of the four bodies;
SP must be in [0020,FFF8]. Arguments after the far return pair are record
offset and segment. CPU storage must be separate from RAM.

+0C5F increments its byte index before looking up the far pointer at
DS:65B9+4*i. Search order is 1 through the live count at DS:5F2A;
count zero permits all 256 indices, ending at zero. Equality compares
the two pointer words, not physical-address equivalence. AL is the
matching index or zero. The getters call this body and load AL from
DS:5F27/5F28/5F2A+4*index, respectively, with AH zero. Other CPU effects,
stack locals, and argument/table/return reads remain live.

The budget counts visits to +0C71 before INC. Zero is valid: setup runs
and LIMIT exposes the exact pre-INC frame. Limits preserve partial state
and do not manufacture a return. Changed nested return targets yield
TRANSFER; an outer RETF uses the actual return pair. TRANSFER is a
defensive status and has no direct differential case in this corpus.

Original code/bindings are immutable; writes may alias data but must avoid
code. Reached return frames must not wrap, words use contiguous physical
bytes, and faults/interrupts are outside this API. The undefined-flag
profile is DOSBox-X normal/386: logical AF=0, one-bit SHL AF=1.
This convention matters at a getter's final return.

## Measurements

- **352 direct original/C cases**, 88 per entry: all CPU14 words and all
  1,048,576 RAM bytes compared. Each entry has 86 successful returns and
  two exact budget stops. All 49 reached instruction starts for each
  getter+index chain, all 32 for direct index, and all 10 conditional
  outcomes per chain are covered.
- Cases vary count/index through 0,1,2,5,127,128,254,255; missing offset
  and segment; zero/one visit budget; SS=DS; A20 stack wrap; odd and
  endpoint SP; randomized background/registers; and count/table/frame
  aliases. The audit validates body hashes, bounds, public entries,
  RETF cleanup, and each near-call site.
- **258 explicit Unicorn SHL-AF exceptions**: successful calls of the
  three getters require C FLAGS XOR Unicorn FLAGS to equal exactly 0010,
  with C AF=1. Every other flag/CPU word and every RAM byte must match.
  The direct index and budget-stop cases compare FLAGS exactly.
  No emulator state is patched and no other difference is permitted.
- **192 native original calls**, 48 per entry, under DOSBox-X normal/386:
  CPU14, 64 bytes of SS covering the complete target stack footprint,
  and all 65,536 DS bytes compare without any flag masks. Six incoming
  flag profiles, eight counts, and matches/misses confirm the AF profile.
  These are synthetic direct calls, not saved-game reachability evidence.
- Strict C11 -Wall/-Wextra/-Wpedantic/-Werror with ASan+UBSan:
  **1,551 semantic checks passed**. Integrated CTest: **2/2 passed**
  (direct getter and existing +32E6 caller).
- Existing +32E6 native regression: **27/27 complete returns**, all CPU14
  words, 1,888 SS bytes and all 65,536 DS bytes per case, without masks.
- Existing +32E6 original-code regression: **440 cases passed**, CPU14
  and all 1,048,576 RAM bytes, with no flag masks or state substitutions.
  Caller coverage remains 62/62 starts
  and 4/4 conditional outcomes;
  transitive coverage is 1387/1428 starts and
  177/206 outcomes.
  Uncovered transitive sites/outcomes remain listed in the receipt.

Native result SHA-256:
1668a7bd0d181493910c5f940062912e898d75920538b972037b317ca7e0d2fb.

Private evidence receipt: resources/u3c6-getters-evidence.json, SHA-256
**4b8a858272b29ad445f2bef7af0e3071614660d6cd56e6c72eedb0daed824871**. It pins source/dependency hashes, both direct
reports, the caller original/native regression reports and all native
capture hashes, DOSBox-X executable hash, and the rebased 2/2 CTest result.
Integration base: 649205982fac09a0f40949acfbd101ea4ef45748.

## Reproduction

Keep owner-supplied media at ignored resources/DOS/POOLRAD. Capstone,
Unicorn, DOSBox-X, GNU as/ld, and a C compiler are required.

    python3 tools/overlay/u3c6_slot_getters_cpu_audit.py --unicorn --native --output resources/u3c6-getters.json
    python3 tools/overlay/u5e_caller_32e6_audit.py --output resources/u5e-32e6-regression.json
    python3 tools/overlay/u5e_caller_32e6_native.py
    cmake -S . -B build
    cmake --build build --target poolrad_overlay_u3c6_slot_getters_cpu_tests poolrad_overlay_u5e_caller_32e6_tests
    ctest --test-dir build -R 'poolrad_overlay_(u3c6_slot_getters_cpu|u5e_caller_32e6)_tests' --output-on-failure

Original bytes, generated native programs, fixtures, and captures remain
under ignored resources/. Public source contains synthetic fixtures and
identity/hash evidence only.

## Readiness and author review

This removes public raw-interface caveats for three getters and exposes
the index alongside them. U9E +0160 still reaches **U9E +0000 dynamic
dispatch**. Its unrestricted live type/table boundary is not closed by
these getters; neither +0160 nor its +02FA caller becomes strictly
composable merely because these interfaces exist.

- target_function: U3C6 entries 12,13,14,15 at the complete spans above.
- source_candidates: overlay_u3c6_slot_getters_cpu.c, extracted from
  overlay_u5e_caller_32e6.c; existing scalar APIs are unchanged.
- signals: pinned original identities, exact full-RAM/CPU comparisons,
  every direct instruction/conditional outcome, independently executed
  native bytes with exact flags, and integration through an existing caller.
- score: high within-contract identity/parity evidence; undefined flags
  are execution-profile-specific and native data alias coverage is narrower.
- verdict: likely_match, pending independent review of the interface change.
- review_notes: no new function credit, no acceptance/accounting edits,
  no dynamic-dispatch closure, no natural gameplay claim. Independent
  verification and acceptance remain with the reviewer/VP.

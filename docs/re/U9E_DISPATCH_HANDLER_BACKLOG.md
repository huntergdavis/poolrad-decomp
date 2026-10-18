# U9E dispatcher handler backlog

Tracked assignments and prerequisites: [issue #1019](https://github.com/huntergdavis/poolrad-decomp/issues/1019).

Interface snapshot: main 3b62678a plus the U38 +1363 raw chain. This is a work inventory, not a strict-credit update.
The reproducible [JSON inventory](u9e_dispatch_handlers.json) maps139 primary
slots to118 distinct handlers, canonical strict-map membership, and a
source-backed interface class. Its script reads the original installer stores:
U38 installs138 slots; U86 supplies the skipped type88 slot.

48 handlers have canonical strict-map rows after independent acceptance #1026
of U38+133F. This inventory adds no credit. Interface review finds22 raw CPU
APIs,37 scalar/callback models, and59 with no located model.
The70 identities without canonical rows comprise59 missing models, nine
callback models, and two raw models (U38+12B2 and +1363). A raw interface does
not itself prove complete dispatcher composition. Reviewers decide credit.

## Prerequisites separate from handler work

- [ ] Prove or enforce an evidence-backed type1..139 precondition on the reached
  game route; the original dispatcher accepts the entire byte range.
- [ ] Validate live table pointers and relocated stub/body bindings; installing
  the table once does not prove later writes cannot change it.
- [ ] Resolve slots0 and140..214 if the type invariant cannot be established.
  They are uninitialized by these installers; type0 overlaps DS:6826/6828 scratch.
- [ ] Resolve high-type aliases215..255 if the type invariant cannot be established:
  they reach41 initialized pointers in U86's adjacent procedure table. The JSON
  records these addresses separately from the118 effect handlers.
- [ ] Upgrade37 scalar/callback interfaces and verify22 existing raw interfaces
  through the exact U9E frame. Canonical credit does not prove raw CPU parity.
- [ ] Complete U9E+0000's indirect call/return, then compose +0026 and replace
  U7D+0000's #1015 PENDING boundary with fresh chain evidence.

## Suggested first assignments

1. U38+319B/+0647/+1355/+135C now have a shared raw CPU implementation and
   separate per-identity evidence in [U38_DISPATCH_NOP_CPU.md](U38_DISPATCH_NOP_CPU.md).
   Their four identities were independently accepted in #1023; do not duplicate them.
2. U38+133F now composes a raw resident00BA:2637 API; see
   [U38_TARGET_RESET_133F_CPU.md](U38_TARGET_RESET_133F_CPU.md). It was independently
   accepted in #1026; the old scalar API remains available unchanged.
3. U38+12B2: existing raw API with no canonical row; route existing evidence
   to independent review rather than author a duplicate or self-credit it.
4. U38+2EDA: eight slots,113 bytes/47 instructions; calls U9E+0026,+0E52,+0000.
   The [explicit boundary audit](U38_DISPATCH_BOUNDARY_2EDA.md) confirms both
   the eight incoming slots and an unrestricted outgoing record-byte dispatch.
   It remains a dynamic closure dependency; recursion support alone is insufficient.

5. U38+1363 now has a raw U9E+0E52 append chain; see
   [U38_APPEND_1363_CPU.md](U38_APPEND_1363_CPU.md). Public entry46 serves type56.
   It awaits independent review; no credit is inferred from the raw interface.

## 70 handlers without canonical strict-map rows

Each checkbox is a separately assignable identity. This list is derived from
the current canonical CSV, including the reviewer's acceptance #1026.
No other completion is inferred. Missing means no located standalone/grouped
model at the reviewed snapshot, not proof of absence from all branches.
Coordinate ownership before work. All remaining boxes are unchecked;
issue #1019 completion is left to the reviewer.

- [ ] OVL_0038_017F — slots 4; missing
- [ ] OVL_0038_0233 — slots 7; missing
- [ ] OVL_0038_0400 — slots 11; missing
- [ ] OVL_0038_05EB — slots 15; missing
- [ ] OVL_0038_0679 — slots 18; missing
- [ ] OVL_0038_070F — slots 21; missing
- [ ] OVL_0038_0775 — slots 22; missing
- [ ] OVL_0038_07E0 — slots 23; missing
- [ ] OVL_0038_0947 — slots 26; missing
- [ ] OVL_0038_09C0 — slots 28; missing
- [ ] OVL_0038_0A65 — slots 30; missing
- [ ] OVL_0038_0B1B — slots 32; missing
- [ ] OVL_0038_0BCF — slots 34; missing
- [ ] OVL_0038_0C4B — slots 39; missing
- [ ] OVL_0038_0CC3 — slots 40; missing
- [ ] OVL_0038_1086 — slots 41; missing
- [ ] OVL_0038_10DE — slots 43; missing
- [ ] OVL_0038_1168 — slots 44; missing
- [ ] OVL_0038_11F9 — slots 47; missing
- [ ] OVL_0038_1276 — slots 48; missing
- [ ] OVL_0038_12B2 — slots 49; raw_cpu; [overlay_u38_effect_match_12b2.h](../../include/poolrad/recovered/overlay_u38_effect_match_12b2.h)
- [ ] OVL_0038_12FB — slots 50; missing
- [ ] OVL_0038_1363 — slots 56; raw_cpu; [overlay_u38_append_1363_cpu.h](../../include/poolrad/recovered/overlay_u38_append_1363_cpu.h)
- [ ] OVL_0038_1399 — slots 57; missing
- [ ] OVL_0038_14EB — slots 62; missing
- [ ] OVL_0038_165D — slots 64; missing
- [ ] OVL_0038_1671 — slots 65; missing
- [ ] OVL_0038_1685 — slots 66; missing
- [ ] OVL_0038_1699 — slots 67; scalar_or_callback; [overlay_u38_roll_dispatch_1699.h](../../include/poolrad/recovered/overlay_u38_roll_dispatch_1699.h)
- [ ] OVL_0038_16BB — slots 68; missing
- [ ] OVL_0038_16E6 — slots 69; missing
- [ ] OVL_0038_170B — slots 70; missing
- [ ] OVL_0038_176F — slots 75; missing
- [ ] OVL_0038_18B5 — slots 76; missing
- [ ] OVL_0038_1935 — slots 77; missing
- [ ] OVL_0038_19D9 — slots 78; missing
- [ ] OVL_0038_1A19 — slots 79; missing
- [ ] OVL_0038_1A4C — slots 80; missing
- [ ] OVL_0038_1AAA — slots 81; missing
- [ ] OVL_0038_1BA8 — slots 82; missing
- [ ] OVL_0038_1CAD — slots 83; missing
- [ ] OVL_0038_1E77 — slots 84; missing
- [ ] OVL_0038_23A4 — slots 85; missing
- [ ] OVL_0038_23C2 — slots 86; missing
- [ ] OVL_0038_23EC — slots 87; missing
- [ ] OVL_0038_2556 — slots 91; missing
- [ ] OVL_0038_25BC — slots 94; scalar_or_callback; [overlay_u38_target_mask81_25bc.h](../../include/poolrad/recovered/overlay_u38_target_mask81_25bc.h)
- [ ] OVL_0038_2610 — slots 95; missing
- [ ] OVL_0038_264B — slots 96; scalar_or_callback; [overlay_u38_effect_clear_264b.h](../../include/poolrad/recovered/overlay_u38_effect_clear_264b.h)
- [ ] OVL_0038_2730 — slots 99; missing
- [ ] OVL_0038_27D9 — slots 100; missing
- [ ] OVL_0038_2866 — slots 102; missing
- [ ] OVL_0038_28BF — slots 103; missing
- [ ] OVL_0038_2901 — slots 104; missing
- [ ] OVL_0038_2A87 — slots 115; scalar_or_callback; [overlay_u38_target_index_gate_2a87.h](../../include/poolrad/recovered/overlay_u38_target_index_gate_2a87.h)
- [ ] OVL_0038_2AE6 — slots 116; scalar_or_callback; [overlay_u38_target_gate_2ae6.h](../../include/poolrad/recovered/overlay_u38_target_gate_2ae6.h)
- [ ] OVL_0038_2B20 — slots 117; scalar_or_callback; [overlay_u38_link_roll_2b20.h](../../include/poolrad/recovered/overlay_u38_link_roll_2b20.h)
- [ ] OVL_0038_2B7D — slots 119; scalar_or_callback; [overlay_u38_target_reset_2b7d.h](../../include/poolrad/recovered/overlay_u38_target_reset_2b7d.h)
- [ ] OVL_0038_2BC8 — slots 120; scalar_or_callback; [overlay_u38_link_message_2bc8.h](../../include/poolrad/recovered/overlay_u38_link_message_2bc8.h)
- [ ] OVL_0038_2C1C — slots 121; missing
- [ ] OVL_0038_2D88 — slots 123; scalar_or_callback; [overlay_u38_effect_scale_clear_2d88.h](../../include/poolrad/recovered/overlay_u38_effect_scale_clear_2d88.h)
- [ ] OVL_0038_2E08 — slots 125; missing
- [ ] OVL_0038_2EDA — slots 128,129,130,133,134,136,138,139; missing
- [ ] OVL_0038_2F57 — slots 131; missing
- [ ] OVL_0038_30A5 — slots 132; missing
- [ ] OVL_0038_3134 — slots 135; missing
- [ ] OVL_0038_317A — slots 137; missing
- [ ] OVL_005E_39D2 — slots 73; missing
- [ ] OVL_005E_3A86 — slots 74; missing
- [ ] OVL_0086_30D9 — slots 88; missing

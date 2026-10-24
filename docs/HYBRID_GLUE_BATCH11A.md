# Hybrid glue, batch 11a: par-d2's RES_00BA raw entries 2E46, 23DB and 1509 (PM i22809, i22841)

| Row | C entry | Return | Source |
| --- | --- | --- | --- |
| `RES_00BA_2E46` | `pr_res_00ba_link_flags_2e46_cpu` | RETF 4; composes 2E03 | #1520 |
| `RES_00BA_23DB` | `pr_res_00ba_party_count_23db_cpu` | RETF; budget `2^20` | #1522 |
| `RES_00BA_1509` | `pr_res_00ba_sidecar_remove_1509_cpu` | RETF 8; bus | #1524 |

**Build:** `76d7ff55`, on main, with 136 model entries and 147 dispatch rows.

**Both runs are pinned** (`12345678`, route 3) and compared with the all-original `hybrid-pin2-r3-a`. **They are
test-harness runs.**

## The 1509 guard (PM i22841)

1509's two failure paths call the resident prompt `0709:0BF5`:
- a null record;
- a record not in the character's `+0C8h` list.

That prompt reads a key (ReadKey, INT 16h), and this dispatch cannot replay a VGA bus and an INT 16h service in the
same body.

So before anything runs, the dispatch:
1. reads the frame: the record at `SS:SP+4`, the character at `SS:SP+8`;
2. walks the list from character `+0C8h` through the `+2Ah` links, bounded at 4,096 nodes.

It **declines** (the original runs, nothing changed) unless the record is found. A combined bus + INT 16h replay is
on the PM's standby list.

**Measured split, in both runs: found 1 (run as C), declined null 0, declined not found 0.** The guard never fired.

## V1, `hybrid-b11a-v1-pin-r3`

driver_rc 0, non-equal 0. All 1,447 post-pin draws (`ad42d362`) and the final frame (`1a371fdb`) equal the reference.

| Row | V1 equal / compared |
| --- | --- |
| `RES_00BA_2E46` | 9 / 9 |
| `RES_00BA_23DB` | 9 / 9 |
| `RES_00BA_1509` | 1 / 1 |

The census counted more arrivals (181, 11 and 2) in observer-only runs. There, an arrival inside a glued parent's call
is counted too. In dispatch runs those arrivals run composed inside the parent's C and are not counted.

## Route mode, `hybrid-b11a-route-pin-r3`

driver_rc 0. 27,445 routed returns. Routed: 2E46 9, 23DB 9, 1509 1.

- The final frame is **0 of 786,432 px** from the reference (raw-RGB `eada8752`).
- The 60 visible draws equal the reference prefix.
- One hand-back: the known `OVL_006F_1446` cold stub.

#1536 adds the three to [re/replaced_bodies.json](re/replaced_bodies.json) (98 → 101).

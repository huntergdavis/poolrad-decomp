# Hybrid executor, stage (a): one cold start with every reached credited body running our C (VP i20687)

**The two numbers (VP i20687).**

1. **Bodies running as our C: 12 of the 12 credited bodies the cold start reaches** (stage 0's measured set,
   [HYBRID_STAGE0_OBSERVER.md](HYBRID_STAGE0_OBSERVER.md)).
   - **10 run C-only.** The original bytes are never executed: 119 routed returns, 0 stops.
   - **2 are DOS- or BIOS-service-bound and run in shadow form:** `RES_0802_02EB` (FileExists, 35 arrivals) and
     `RES_0899_0059` (ReadKey, 1 arrival). The original runs first, then our C runs with the original's service results
     replayed. All 36 are equal, and the game continues from the C's state.
2. **How far the original's own entry counts fell: from 976 to 155 credited arrivals, which is 821 fewer (−84%).**
   - The whole drop is `RES_0709_01DF`, the glyph routine: **824 → 3**. Its draws are now made inside the composed C of
     `0626`, `05EB` and `01DA`, so the original glyph entry is never reached for them.
   - Every other ID's count is unchanged.

**The screen.** The 55 s main menu has **0 differing pixels** against the unsubstituted run.

Run `hybrid-stage-a-route-b`, with the binary built from the cumulative patch (sha256 prefix `4813e8a3570db7a5`), in one
cold start of the audited v1.3 game to the main menu.

| Body | Mode in this run | Arrivals | Our C returned | Stops |
|---|---|---|---|---|
| `RES_0899_0104` | C-only | 51 | 51 | 0 |
| `RES_0464_2595` | C-only | 2 | 2 | 0 |
| `RES_0709_0626` | C-only, real VGA | 51 | 51 | 0 |
| `RES_0709_05EB` | C-only, real VGA | 4 | 4 | 0 |
| `RES_03F1_0137` | C-only, real VGA | 4 | 4 | 0 |
| `RES_0709_01DF` (direct calls) | C-only, real VGA | 3 | 3 | 0 |
| `OVL_03B0_01DA` | C-only, real VGA | 1 | 1 | 0 |
| `OVL_0086_3413` | C-only | 1 | 1 | 0 |
| `OVL_0038_31A2` | C-only | 1 | 1 | 0 |
| `OVL_03B0_00CB` | C-only | 1 | 1 | 0 |
| `RES_0802_02EB` | shadow, DOS replay | 35 | 35 (equal) | 0 |
| `RES_0899_0059` | shadow, INT 16h replay | 1 | 1 (equal) | 0 |

## What the C-only arrivals rest on

A C-only arrival has no per-arrival comparison in the run itself. Its evidence is the shadow-form check of the same body, on
the same path, in earlier runs:

| Run | Bodies | Result |
|---|---|---|
| `hybrid-v1-twelve-a` | all 12 | every uninterrupted arrival equal (full state; device streams for the bus bodies; replayed services for 02EB/0059) |
| `hybrid-v1-all5-a`, `-b` | the 5 bus bodies | equal |
| `hybrid-dispatch-2595-a` | 2595 | equal, including the mid-body hand-back |

The mutants that show each comparison can fail:

| Mutant | Result | Where |
|---|---|---|
| 0104 parity flag | 15 `C_DIFFERS` | #1372 |
| 2595 `[BP-2]` | `MIDBODY_HANDBACK_DIFF` | #1377 |
| glyph OUT | 814 `V1_STREAM_DIFF`, 7,896 pixels | #1383 |
| 02EB record size | 35 service mismatches | #1390 |

The run-level checks are the counts and the pixels above.

## Rules this work established

- **The side-effect rule.** When a body's C stops partway, the hybrid normally hands back: the C's partial state goes to
  the guest, and the original bytes continue from it (tested mid-body in #1377). **A body whose service has an external
  effect must never be handed back** after the original already ran in shadow form. A DOS open, close or read, or anything
  that writes a disk, would execute **twice**. For such bodies, on any stop or service mismatch, the original's result is
  kept. Every future service with an external effect (file writes, saves) needs the same treatment, or the hybrid will
  duplicate writes to the disk. This is a correctness rule, not a preference (VP i20687).
- **Service replay records three things per call:**
  - the registers at the `INT`;
  - the registers at its return;
  - the **RAM delta** across the call.

  A real `INT 21h` or `INT 16h` writes RAM (the interrupt frame below SP, and for files the PSP handle table), even though
  the model contracts state that the service "must not write RAM". A registers-only replay would differ on every arrival
  and would look like a model bug.
- **"This address executed our C" is measured by the original's entry counts, not assumed.** In particular, the composed
  glyph routine's original entry fell from 824 to 3.

## What this does not show (loud, per VP i20687)

- **Service replay is SHADOW-ONLY.** No C-authoritative route through the real DOS or BIOS exists: no
  `CALLBACK_RunRealInt` path is built. **The hybrid cannot yet drive DOS itself.** `02EB` and `0059` still need the
  original to run first, in every run.
- **One path:** the cold start to the main menu. **One screen of pixels.**
- **Coverage of the original's own code.** The entry-count collapse measures only composed callees. For a top-level body
  run C-only, the evidence that its original bytes did not run is the dispatch log, which writes the C's result and moves
  CS:IP to the return address before the entry instruction executes. Its observer arrival is still counted, because the
  observer counts at the entry.
- **Not covered:** V2 (full EGA state), timing, and every credited body not reached on this path (258 of 270, from stage 0).

## Build and run

As [HYBRID_STAGE_A_COLD_START.md](HYBRID_STAGE_A_COLD_START.md). The models object adds ReadKey's sources, for 41 in total:

- `resident_0899_quit_0000_cpu`
- the `resident_08ae_*` driver set: `driver`, `dispatch_00dc`, `entry_017b`, `vector_137c`, `vector_13b1`, `table_1742`,
  `table_17ae`, `byte_181a`, `delay_006c`, `time_0000`
- `dos_gettime_00a2_cpu`, `system_longmul_0a5b_cpu`, `dos_intr_000b_cpu`
- `resident_0899_readkey_0059_cpu`, `crt_keypressed_02fd_cpu`, `crt_readkey_030f_cpu`
- `system_paramstr_16bc_cpu`, `system_string_move_leaves_cpu`

`memory_bus.c` and `ega_hw.c` are not linked; the patch defines `pr_bus_*`. To run, set:

```
PR_HYBRID_DISPATCH=RES_0899_0104,RES_0464_2595,RES_0709_01DF,RES_0709_05EB,RES_0709_0626,RES_03F1_0137,OVL_03B0_01DA,OVL_0086_3413,OVL_0038_31A2,OVL_03B0_00CB,RES_0802_02EB,RES_0899_0059
PR_HYBRID_BUS_MODE=route
```

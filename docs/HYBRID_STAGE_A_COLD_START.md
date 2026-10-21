# Hybrid executor: 11 of the 12 reached credited bodies run as our C on the cold-start path (VP i20660, i20667)

**Coverage, stated honestly (VP i20667).** Stage 0 measured 12 credited bodies on the cold-start path to the main menu.

- On 2026-10-21 the first update covered 7 of them (the table below).
- A second update (the section "11 of 12") adds the three remaining RAM-only bodies and the file-service body `02EB`.
- **The one body left, `RES_0899_0059` (ReadKey), is a known piece of work, not an unknown.** It needs keyboard, driver and
  quit services declared, which is the same class of work as `02EB`'s DOS replay.

**Result.** On the cold-start path to the main menu, the credited bodies that stage 0 found reached were these 12. Seven of
them now run as our C inside the live game: the two RAM-only bodies plus all five device-touching ones.

| Body | Kind | V1 (shadow): checked equal | Interrupted (excluded) | Route: C ran | Route: stops |
|---|---|---|---|---|---|
| `RES_0899_0104` | RAM-only | 46 + 5 hand-back reruns | 0 | (always C) | 0 |
| `RES_0464_2595` | RAM-only, composes FreeMem | 1 full + 1 mid-body hand-back | 0 | (always C) | — |
| `RES_0709_0626` (text printer) | `pr_bus`, composes StrStore + glyph | 44 (run a) + 43 (run b) | 7 + 8 | 51 | 0 |
| `RES_0709_05EB` (space) | `pr_bus`, composes glyph | 3 + 4 | 1 + 0 | 4 | 0 |
| `RES_03F1_0137` (window clear) | `pr_bus`, composes rect fill, page select, EGA reset, FillChar | 4 + 3 | 0 + 1 | 4 | 0 |
| `RES_0709_01DF` (glyph, direct calls) | `pr_bus` | 3 + 3 | 0 + 0 | 3 | 0 |
| `OVL_03B0_01DA` (overlay, menu entry) | `pr_bus`, composes glyph | 0 + **1** (2,156 device events) | 1 + 0 | 1 | 0 |

- **V1** runs are `hybrid-v1-all5-a` and `-b`. Every checked arrival has the C's ordered device stream equal to the
  original's element by element (VRAM reads with values, writes, OUTs), and its registers and 1 MiB equal too. There are
  **0 differences** in either run.
- **Route mode** is run `hybrid-route-all5-a`. The C drives the real VGA for all five device bodies: 63 routed returns,
  0 stops. The 55 s main-menu screen has **0 differing pixels** against the unsubstituted run.

**The original's glyph bytes stopped running, measured.** In route mode the observer counts `RES_0709_01DF` arriving
**3** times, against **824** in the unsubstituted run, and total arrivals fall from 976 to 155 (976 − 821).

- That is because 0626, 05EB and 01DA now run as C, and their C composes the glyph model.
- So the original glyph routine was never entered for those 821 draws.
- This is the scope's "this address executed our C" made a measured fact: the original's own entry point stopped being
  reached.

All other 269 IDs have identical counts.

**Which 12 these are.** Stage 0 found 12 credited IDs on this path. Seven run as C here. The other five are:

- `RES_0802_02EB` and `RES_0899_0059`: RAM-only but service-bound (DOS file and keyboard services), not yet glued;
- `OVL_0086_3413`, `OVL_0038_31A2` and `OVL_03B0_00CB`: RAM-only, not yet glued.

So this is not yet "every reached credited body runs as C". It is every reached device-touching body, plus the two
RAM-only bodies already done.

## 11 of 12: the remaining RAM-only bodies and the first service-bound body

Run `hybrid-v1-ten-a` (10 bodies) and run `hybrid-v1-eleven-a` (11 bodies). Each checked arrival is compared with the
original from the same snapshot (32-bit registers, segments, flags, all 1 MiB):

| Body | Kind | Arrivals | Equal | Other |
|---|---|---|---|---|
| `OVL_0086_3413` | RAM-only, `RETF` | 1 | 1 | — |
| `OVL_0038_31A2` | RAM-only, `RETF` | 1 | 1 | — |
| `OVL_03B0_00CB` | RAM-only, `RETF 6`, composes SYSTEM leaves | 1 | 1 | — |
| `RES_0802_02EB` (FileExists) | DOS-service-bound, `RETF 4` | 35 | **35** | 2 DOS calls each (open, close) |

The other seven bodies held unchanged in the same runs, and all 270 observer counts equal the unsubstituted run.

**How a DOS-service body is checked (service replay).** The model's contract has DOS services as callbacks, and it states
that "the service must not write RAM". A real `INT 21h` does write RAM: DOS's interrupt frame below SP, and the PSP's handle
table. So the dispatch records each `INT 21h` the original issues inside its window as three things:

- the registers at the `INT`;
- the registers at its return;
- the RAM delta across it.

The C's service callbacks must ask for the recorded calls in order, with all 14 registers equal. Each callback applies the
recorded delta and returns the recorded registers. The C therefore gets exactly the original's DOS effects, and the 1 MiB
comparison stays meaningful.

On a service mismatch, or any stop, the original's result is kept. A hand-back would make DOS execute the call twice.

**The service replay can fail.** In the mutant, `02EB` pushes record size 129 instead of 128 for Reset (run
`hybrid-02eb-mutant`).

- All 35 arrivals stop with a service mismatch at the second DOS call: `service 1: C AX=3E81 …, original AX=3E80 …`. The
  mutated value reaches AL by the time of the Close.
- The good object was restored, and the binary's sha256 prefix equals the pre-mutation one (`bb10e5d2d876a7e7`).

## How the arrivals are checked

- **Interrupt exclusion is measured, not inferred.** A counter in `pic.cpp` counts every hardware interrupt the CPU takes
  (both `CPU_HW_Interrupt` call sites). An arrival is excluded only when that counter moved inside the original's window.
  This replaces the earlier "allowed code segments" heuristic, which could not cover bodies whose callees run in other
  segments (0626, 01DA).
- **Composed callees are checked inside their caller.**
  - When 0626 is dispatched, the 01DF calls it makes run inside its window, as original bytes on the original run and as
    the composed glyph model in the C.
  - The direct `01DF` rows above count only calls made from outside the five dispatched bodies.
- **The two V1 runs are independent cold starts.** Interrupts land differently in each. `OVL_03B0_01DA`'s single arrival
  was interrupted in run a and clean in run b, which is why the table shows both runs.

## What this does not show

- **One path:** the cold start to the main menu only.
- **One screen of pixels:** earlier screens (loading, publisher, credits, code-word) are covered only by V1's per-arrival
  stream equality.
- **No mutant was run for these four new bodies.** The ability of both comparisons to fail was shown on the glyph routine
  (#1383: 814 `V1_STREAM_DIFF`, 7,896 pixels). The same compare code is used here, but no per-body mutant was run.
- **V2 (full EGA state)** is not built, and **timing** is not covered.
- **One of the 12 reached bodies** still runs as original bytes: `RES_0899_0059`, ReadKey (keyboard, driver and quit services).
- **Service replay is shadow-only.** In route mode, `02EB` is still checked against the original. A C-authoritative service
  route through the real DOS (for example `CALLBACK_RunRealInt`) is not built.

## Build and run

As [HYBRID_EGA_ADAPTER_V1_ROUTE.md](HYBRID_EGA_ADAPTER_V1_ROUTE.md), with the patch refreshed. It now also adds the
interrupt counter in `src/hardware/pic.cpp`.

The models object is 14 sources:

- `resident_0899_idle_0104_cpu`
- `resident_0464_bitmap_release_2595_cpu`
- `system_freemem_0254_cpu`
- `resident_0709_space_05eb_cpu`
- `resident_0709_glyph_01df_cpu`
- `system_runtime_leaves_cpu`
- `resident_0709_printer_0626_cpu`
- `system_strstore_fullstate_0b25`
- `resident_03f1_clear_0137_cpu`
- `resident_0709_rect_fill_00b7_cpu`
- `resident_0709_page_select_00a0_cpu`
- `resident_0709_ega_reset_0000_cpu`
- `system_fillchar_bus_16a8_cpu`
- `overlay_u3b0_entry_01da_cpu`

For 11 of 12, add `overlay_u86_procvars_3413`, `overlay_u38_effects_install_31a2`, `overlay_u3b0_item_split_00cb_cpu`,
`resident_0802_02eb_cpu`, `system_ioresult_assign_cpu`, `system_reset_1410_cpu`, `system_close_1491_cpu` and
`system_filecheck_14b6_cpu`, which makes 22 sources. Add `OVL_0086_3413,OVL_0038_31A2,OVL_03B0_00CB,RES_0802_02EB` to the
dispatch list.

Neither `memory_bus.c` nor `ega_hw.c` is linked. Set
`PR_HYBRID_DISPATCH=RES_0899_0104,RES_0464_2595,RES_0709_01DF,RES_0709_05EB,RES_0709_0626,RES_03F1_0137,OVL_03B0_01DA`, and
add `PR_HYBRID_BUS_MODE=route` for route mode.

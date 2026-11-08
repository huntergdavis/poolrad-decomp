# 16EC frame-preservation and loaded-target guards (t24, NC2 and NC4 evidence)

**Proposed enforced guards for two open closure reasons; nothing is discharged
here.** This change makes two of the port's load-bearing preconditions
*enforced* instead of *assumed*: the composed C models now refuse, with a
distinct status, exactly the situations in which the
[conditional boundary proof](RES_00BA_16EC_PORT_BOUNDARY.md) said the original
would continue on corrupted control state (NC2) or follow an overlay target the
port has not pinned (NC4). The [closure record](RES_00BA_16EC_CLOSURE_STATUS.md)
keeps NC2 and NC4 open until an independent reviewer accepts an exact head and
records the discharge there. No accounting product, identity, readiness,
membership, credit or replacement row changes.

## What is enforced, where

| Guard | Model | Protected state | New stop |
| --- | --- | --- | --- |
| Tile frame | `OVL_040C_015D` (emitted by `emit_ovl040c_015d.py`) | `[BP-6,BP+10h)`: locals, saved BP, return address, the five arguments (1951 still reads `[BP+0Ch]`/`[BP+0Eh]` after Move) | `PR_OVL040C_015D_FRAME` at the 08D4, Move or 1951 call site |
| Box frame | `RES_03F1_0000` (emitted by `emit_res03f1_box_0000.py`) | `[BP-4,BP+10h)`: loop counters, the base-tile local `[BP-3]`, saved BP, return address, five arguments | `PR_R03F1_BOX_FRAME` after the tile call that changed it; 015D's FRAME propagates |
| 16EC frame | `RES_00BA_16EC` | `[BP-102h,BP+12h)`: colour byte, message copy, saved BP, return address, six arguments, except StrStore's documented write `[BP-100h,BP)` | `PR_RES_16EC_FRAME` after the callee that changed it (callee 1..8); the box's FRAME propagates |
| Loaded target | `RES_03F1_0000` with `env.verify_target` (always set by 16EC) | the 336 bytes at the stub's target `:015D`, FNV-1a 64 with the eight loader fixup words zeroed, pinned as `PR_R03F1_BOX_TILE_BODY_FNV1A64` (emitted from the body with SHA-256 `2a8a7826…`) | `PR_R03F1_BOX_TARGET` at the stub, before the target is followed; 16EC reports it as `CALLEE` (callee 6, status 8) |

Each guard saves the protected bytes before the callee and compares them
after a returned callee. The bytes are read at 16-bit `SS` offsets, which is
exact only while no frame word straddles `SS:FFFF` (word pushes and pops are
physically contiguous beyond it). The tile and box models therefore refuse,
with `ARGUMENT` and before any write, an entry SP outside their non-wrapping
profiles (`[0008h,FFF1h]` for the tile, `[0006h,FFF1h]` for the box; 16EC's
`[0800h,FFF0h]` guard already excludes wrapping), and the guard claim is made
inside those profiles only (t3697 R1: a standalone tile entry at SP `0001h`
let a Move into `8000:0000` change the saved BP's high byte unseen). Nothing is written by the guards, no budget is
consumed, and on every original-path case the comparison is equal, so the
models' existing full-state receipts are unchanged in meaning: the box audit
(56 Unicorn cases, 1,124 tile calls) and the tile audit (105 cases) were
rerun on the regenerated models with `verify_target` set and still compare
CPU14 and the whole 1 MiB with zero flag masks.

A guard is a refusal, not a model of the corrupted continuation. The
original CPU keeps running in those worlds; the guard's purpose is that the
port never silently diverges there. `OVERLAY` (a cold `INT 3Fh` stub) is
unchanged and remains a boundary, not a success.

## Executed controls (each guard level fired by a different alias)

The [port-boundary checker](../../tools/overlay/res16ec_port_boundary.py)
builds matched initial-memory worlds that differ only in the Move object's
destination pointer `DS:5E4E/5E50`, as the t131 control did, and now names
three destinations:

| Control | Move destination + 17h | Original (Unicorn, observed) | Candidate (C) |
| --- | --- | --- | --- |
| `frame-alias-011f-two-tiles-exit` | the box's `[BP-3]` | continues; second tile is `011Fh`, error branch, EXIT after two tile calls | `FRAME` from the box guard after the first tile call (callee 6, status 9, box site `0038`, 1 call) |
| `frame-alias-16ec-message-continues` | 16EC's message length `[BP-100h]` (graphics mode 3, where 1951 returns at once) | continues through all 92 tiles and RETURNS with message length `1Eh`, the last (vertical-edge) tile's pinned source byte (mode-3 baseline: 5) | `FRAME` from the 16EC guard after the box returned (callee 6, status -1, site `177B`, 92 calls) |
| `frame-alias-tile-row-continues` | the first tile's `[BP+0Ch]` (mode 3) | continues through all 92 tiles and RETURNS with changed 1951 coordinates | `FRAME` from the tile guard at Move (callee 6, box status 6, tile site `028B`, 1 call) |
| `target-unpinned` | stub retargeted one paragraph into the unit | not run (the original would execute shifted bytes) | `CALLEE` with `PR_R03F1_BOX_TARGET` at the stub (callee 6, status 8, 1 call) |
| `target-relocated` | the unit copied to another segment, stub retargeted | not run (the Unicorn runner confines guest code to the pinned segments) | RETURNED after 92 tile calls: the pin is on content, not the segment (candidate-only) |

None of the alias rows is a parity comparison: the original and the candidate
stop differently by design, and the checker records them separately. The two
baselines (mode 0 for the pre-registered t131 control, mode 3 for the two new
aliases) are full-state comparisons; the two target controls are candidate-only. The first
attempt placed the colour-byte alias in mode 0; 1951 then read a nonzero
"height" from the stack and the original ran past the watchdog, which is why
the new aliases use mode 3 and target the message length instead. The checker
refuses if any guard stops at a different level, site or call count than
the table says.

The synthetic C tests also require: `TARGET` at a verified stub whose target
holds no body, `FRAME` after a tile call whose Move lands on `[BP-3]` (box
test, 15 checks), and `FRAME` at Move when the destination lands on the tile's
own `[BP+0Ch]` (tile test, 17 checks).

## Relation to the removal tests

- **NC2** asks for "a checked write-range/frame-preservation argument or
  enforced guard" excluding "the demonstrated Move alias and all other
  control-frame writes" for every admitted box-taking entry. The three frame
  guards compare the protected bytes themselves, so any write into them by any
  callee, through any alias, stops the composition; the demonstrated alias is
  the executed control. This is the enforced-guard branch of the test, not a
  proof that live gameplay never aliases.
- **NC4** asks for "a checked loaded-stub/target invariant or enforced guard
  excluding cold calls". A cold stub still stops with `OVERLAY`; a loaded stub
  is now followed only when its target is the pinned body. Cold loading
  itself is still not modelled: the overlay manager is outside the port.

## Reproduce

```sh
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/emit_ovl040c_015d.py --check
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/emit_res03f1_box_0000.py --check
cmake -S . -B resources/t24-guards/build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON && cmake --build resources/t24-guards/build --target \
  poolrad_resident_03f1_box_0000_cpu_tests poolrad_overlay_u40c_tile_015d_cpu_tests poolrad_resident_00ba_window_16ec_cpu_tests
ctest --test-dir resources/t24-guards/build -R 'poolrad_(resident_03f1_box_0000_cpu|overlay_u40c_tile_015d_cpu|resident_00ba_window_16ec_cpu)_tests'
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res03f1_box_0000_cpu_audit.py --unicorn --output resources/t24-guards/box-audit.json
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/ovl040c_015d_cpu_audit.py --unicorn --output resources/t24-guards/tile-audit.json
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res16ec_port_boundary.py --output resources/t24-guards/boundary-guards.json
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh python3 -m unittest -v test_res16ec_port_boundary
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res00ba_window_16ec_cpu_audit.py --unicorn --keep-going --output resources/t24-guards/16ec-complete.json
```

## Receipts (this head; media-dependent outputs stay under ignored `resources/t24-guards/`)

| Check | Result |
| --- | --- |
| `emit_ovl040c_015d.py --check`, `emit_res03f1_box_0000.py --check` | both `EMIT_CHECK OK` (the tracked C is byte-identical to the regenerated output) |
| Debug CMake C tests: box, tile, 16EC, printer | 4/4 passed; box 17 checks, tile 83 checks (the SP `0001h` refusal compares 64 bytes around `SS:FFFF`) |
| Box differential `--unicorn` (`box-audit.json`) | 56 cases, 1,124 tile calls, 145/145 starts, 8/8 outcomes, zero flag masks, with `verify_target` set |
| Tile differential `--unicorn` (`tile-audit.json`) | 105 cases, 127/127 starts, 29/30 outcomes (1 declared unreachable), zero flag masks |
| Port-boundary checker (`boundary-guards.json`) | 9 boundary + 2 baseline full-state comparisons; the three alias controls stop at the box, 16EC and tile guards; unpinned target refused, relocated pinned copy followed; 80 s |
| Port-boundary unit tests | 8/8 passed, zero skips (112 s) |
| 16EC full differential `--unicorn --keep-going` (`16ec-complete.json`) | **388/388** comparisons passed, zero failures: 247 RETURNED, 140 LIMIT, 1 OVERLAY; 116/116 starts, 6/6 outcomes, 243 independent statements, zero flag masks and zero declared splits (rerun on the profile-enforcing models: 2,278 s under load) |

Earlier attempts are recorded for honesty: the first colour-byte alias in
mode 0 ran the original past the six-million-instruction watchdog (1951 read
a nonzero height from the stack), and the relocated-target world cannot be
run under the Unicorn runner's segment confinement; both controls were
redesigned as described above rather than relaxed.

## Limits

- The guards protect the three bodies' own control frames at the named
  callee boundaries. They do not protect the callees' internal frames, the
  display object, DS cells or VRAM, and they say nothing about what the
  original does after an alias: that remains an observed divergence.
- `verify_target` pins content at the moment the stub is followed; a unit
  reloaded over the same segment between tile calls is re-checked at every
  call, but a cold stub is still refused, not loaded.
- A 64-bit FNV-1a pin is a strong but not cryptographic identity; the
  Python audits recompute it from the SHA-256-pinned body.
- No natural arrival, live frame, identity, readiness, membership or
  closure claim; NC1/NC5 ([entry contract](RES_00BA_16EC_ENTRY_CONTRACT.md),
  PR #1642) and NC3 are separate.

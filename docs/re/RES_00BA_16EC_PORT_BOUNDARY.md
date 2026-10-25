# 16EC's conditional display boundary (t131)

**A narrower tile path can be established under an explicit frame-preservation
condition. Sufficiency for the live port is not established.** This is an
evidence step from #1558, not another recovered body and not a readiness,
identity, credit, replacement, or downstream-unlock claim. The reviewer decides
those separately. No accounting product is changed.

Base: `ceb665018d911d61891e7f39df778a5e6de1a020`.
The runnable evidence is `tools/overlay/res16ec_port_boundary.py`; its tests
are automatically discovered by `tools/run_media_checks.sh`.

## What the fixed caller actually supplies

The comparisons at `00BA:1707/170C` and `1757/175C` select the box precisely
when the mode read at 1707 is not 5 and the byte read at 1757 is zero. These
are **values at the reads after StrStore**, not an unconditional claim about
entry values: an aliasing copy could have changed them. If either test bypasses
the box, none of its overlay closure is needed by that invocation.

On the box branch, the pushes at `176A..177A` give the following low bytes
in the callee frame. The argument order matters; the push sequence must not
be read as the callee's argument order.

| Box slot | Value |
| --- | ---: |
| `[BP+6]` style | 0 |
| `[BP+8]` bottom | 22 |
| `[BP+0A]` right | 38 |
| `[BP+0C]` top | 17 |
| `[BP+0E]` left | 1 |

With the pinned box body and its control frame preserved by its callees,
the adjusted edges are `(left, top, right, bottom) = (0,16,39,23)`.
Its base tile is `style*3+114h = 114h`. The original loop bounds then give:

| Calls, in execution order | Number | Tile |
| --- | ---: | --- |
| Four corners | 4 | `114h` |
| Top/bottom pairs, columns 1 through 38 | 76 | `116h` |
| Left/right pairs, rows 17 through 22 | 12 | `115h` |

That is **92 calls by construction**, not a frequency of gameplay behavior.
Each tile call's `[BP+8]` flag low byte is zero. Its full word can be `0100h`:
the preceding tile number leaves AH=1, and MOV AL,0 does not clear AH. A
full-word-zero assumption would be wrong.

## The exclusion proof, and its exact scope

For those three tile numbers, `OVL_040C_015D` selects band 4 (`100h..11Eh`).
The error range (`0` or `11Fh..7FFFh`) and stale-band range (`8000h..FFFFh`)
are excluded. The zero flag byte selects Move/1951; it excludes the direct
08D4 call. A null band-4 pointer returns without either placement call.

| Tile-body site | Conditional disposition |
| --- | --- |
| `01C5`, `01D7`, `01DC`, `01E1`, `01E9` | Error video, text output, ReadKey and QUIT calls excluded |
| `0249` | Direct 08D4 call excluded |
| `028B`, `02A2` | Move and 1951 still required when band 4 is non-null |

This is a **site-local proof about the tile body**. It does not say that
ReadKey, timing, sound, interrupts, or any other service is absent from the
rest of 16EC: its printer/prompt and wait/clear paths remain in scope.
It does not remove 1951's own display effects or its dependencies.

**Author-added, load-bearing conditions:**

1. Code is the pinned immutable original; segment relocations refer to the
   intended bodies. A loaded stub must target the correct unit and `015Dh`.
2. Called routines preserve the box's saved frame, arguments, base-tile local,
   and loop-control locals except for the box's own documented updates.
   They also preserve each tile's live arguments until their last use.
   In particular, Move destinations and display/object/table writes must not
   corrupt that control state. Merely excluding the EGA aperture does **not**
   prove this. No universal no-alias proof is supplied here.
3. #1558's stack, aperture, display-state and bounded-execution conditions
   continue to apply. This proof does not turn finite budgets into unbounded
   termination equivalence.

These are not inferred live-game invariants. Under them, the arithmetic and
branches above exclude the named sites for the complete preserved-frame box.
The synthetic runs corroborate that conditional argument; they do not prove
that every port arrival meets the conditions.

## Measurement and controls

The checker hashes all three bodies against the hashes in
[16EC](RES_00BA_16EC_CPU.md), [the box](RES_03F1_BOX_0000_CPU.md), and
[the tile body](OVL_040C_015D_CPU.md). It uses the loader-expanded START.EXE
and fixup-backed overlay code, never a raw START.EXE file offset.

A read-only observer records the original selector reads, box arguments,
tile frames, box base-tile local, and tile call-site counts. It leaves all
guest instructions and calls running. The existing #1558 comparator still
checks CPU14, whole 1 MiB RAM, EGA state, ordered port/write/environment
events, stop and budget, with its declared callee flag rules unchanged.

The case matrix covers mode 5, two nonzero colour selectors, loaded boxes in
graphics modes 0 through 3, a null band, and a cold stub. The cold case stops
`OVERLAY` before the manager executes. It is a **remaining boundary**, not a
successful box return or evidence that loading is off the port path.

Controls deliberately change guest code in synthetic worlds: style 0 to 3
tests the excluded error branch; the first tile's flag byte 0 to 1 tests
the excluded 08D4 branch. These are labelled mutations, not original-byte
parity cases. Separate controls reject changed body pins and treating a cold
stub as a loaded success.

The focused owner-media tests passed **5/5**, zero skips (68.379 seconds),
including the nine-case original-path matrix and both executed mutations.
The receipt is retained under ignored `resources/t131/focused.log`.

Reproduce, one heavy command at a time, with Capstone 5.0.7 / Unicorn 2.1.4
available to the selected Python:

```sh
../poolrad-heavy.sh python3 tools/overlay/res16ec_port_boundary.py --output resources/t131/boundary.json
../poolrad-heavy.sh env PYTHONPATH=tools/overlay python3 -m unittest -v test_res16ec_port_boundary
tools/run_media_checks.sh
```

## Still not verified

- Live 16EC selector values, frame/object aliasing, message/title lengths,
  actual loaded-stub state, and a correct live overlay target.
- Native 16EC CPU/RAM/EGA parity, live C dispatch, and live frame parity.
- Overlay-manager execution or a proof that cold stubs cannot occur.
- Universal termination or exclusion of #1558's 255-byte printer boundary.
- Identity, helper admission, strict closure, or any downstream unlock.

The historical observer table supplied by lane %17 (VP inbox coordination,
`i24635`) records 109 16EC arrivals in a pinned route-3 run. It has no selector,
stack, or stub snapshot, and its old completion/head/binary provenance was
not revalidated here. Arrival counts cannot discharge these obligations.

Recall before analysis: Dunking Sheep memory returned session
`281c8700-5acc-438b-b93d-c061e28437eb` for the historical closure study. This
step reuses the published [harness plan](RES_00BA_16EC_PORT_HARNESS.md),
[one-hop list](RES_00BA_16EC_ONE_HOP_TARGETS.md), #1558's world/comparator,
and the box/tile evidence above. Their old closure counts are not remeasured.

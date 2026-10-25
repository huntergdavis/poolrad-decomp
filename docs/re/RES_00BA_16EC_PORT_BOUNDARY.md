# 16EC's conditional display boundary (t131)

**A narrower tile path can be established under an explicit frame-preservation
condition, and one native selector checkpoint satisfies the box-bypass
predicate. The whole port boundary remains NOT-CLOSABLE on this evidence.** This is an
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

On `333f29624436f323e3fc5125d5450d46299c94ab`, the nine original/C full-state
comparisons also passed: eight RETURNED and one cold-stub OVERLAY. This
receipt precedes the preservation control below; it does not validate that
new control. It is retained under `resources/t131/boundary.json`.

### Preservation-control prediction, recorded before execution

Reviewer request `i24647`, reinforced by VP ruling `i24650` and reviewer
`i24664`: make the frame-preservation condition itself falsifiable.

The named synthetic initial-memory violation is
`frame-alias-011f-two-tiles-exit`. Both worlds use mode 0 and a band-4 object
at `3400:0000` with two-byte tile items. The first item's source bytes encode
`011Fh`. The worlds differ only in `DS:5E4E/5E50`, the Move-object destination
pointer. In the violation world, destination+17h is the box's `[BP-3]` local;
in the baseline it is the normal synthetic object. No guest code is patched
and the observer writes nothing.

**Prediction:** the baseline returns after 92 tile calls. In the violation,
the original first Move changes `[box BP-3]` from `0114h` to `011Fh`.
The following mode-0 1951 returns without rows: the aliased object's height
overlaps the tile frame's saved source-offset local, which is zero. The
second box tile is therefore `011Fh`, takes the real tile error branch, and
exits after **two tile calls**, not 92. The tile-base and count conclusions
break; no nonzero flag byte is predicted.

The checker must observe that actual Move destination, its completed write,
the next tile number, the error call and EXIT. It must also explicitly reject
applying the conditional exclusion to that trace. A premature stop is an
unfalsified control, not a passing rejection: the assertions fail and the
command exits nonzero. This is a constructed violation, not a natural-game
reachability claim. Execution results are not yet recorded in this paragraph.

**Observed result (prediction unchanged):** the original-only matched-world
test passed at `24d3c1a7aa0fc3acd36c0a04b9bcb9d0bec3a415` (12.695 seconds).
The baseline returned with 92 calls. In the violation, the real Move copied
two bytes from `3400:003F` to `7000:EEE9`, the box local. The observed local
values at tile entries were `0114h`, then `011Fh`; two tile calls occurred.
Each error site `01C5/01D7/01DC/01E1/01E9` ran once and the original reached
EXIT. The flag words remained `0100h`; the predicted tile-number and count
conclusions failed, and the validator refused the exclusion.

**A separate comparison failed and is retained:** the subsequent attempt to
include this violated world as a full original/C parity case stopped with
`C CALLEE (callee 6/6, site 177B)` versus original EXIT. This is not a passing
parity result. The original observation and failed expanded comparison are
both preserved in `resources/t131/alias-validation.log`.

The existing `flat_only()` guard in `overlay_u40c_tile_015d_cpu.c` refuses
**every** error-branch callee when EGA is attached; it does not first test an
actual aperture alias for those calls. The first such call is `01C5`, the
video-mode restore. Thus the preserved-frame proof matters to the display
contract: violating it reaches an existing model boundary. The follow-up
checker records the original-only violation and the C guard separately;
it must not turn their different stops into parity. The fixture and prediction
are unchanged, and no recovered C body is repaired or broadened.

The boundary covered here is the `00BA:177B` call, its fixed box frame,
`0428:002A` loaded target, and the resulting tile/Move/1951 composition.
A new caller, changed frame layout, changed stub/target, or changed object/
display write contract invalidates reuse and requires a boundary recheck.
VP rulings `i24650/i24656` do not grant any membership: this PR does not
define or cite a new admission class, remove a readiness dependency, or
change the t100 NO result.

Reproduce, one heavy command at a time, with Capstone 5.0.7 / Unicorn 2.1.4
available to the selected Python:

```sh
../poolrad-heavy.sh python3 tools/overlay/res16ec_port_boundary.py --output resources/t131/boundary.json
../poolrad-heavy.sh env PYTHONPATH=tools/overlay python3 -m unittest -v test_res16ec_port_boundary
tools/run_media_checks.sh
```

## One native selector observation, not native parity

At author head `5a724d4842cef4af799340f2e862145535ab63c2`, a private original-only
probe captured the first watched arrival immediately before `00BA:1707`.
It used the existing t116 configurable checkpoint binary, with a unique
32-byte masked signature across the expanded resident image and overlay
code. It cleared inherited dispatch/checkpoint settings, enabled no C
dispatch and requested no seed pin. The RAM dump's entire 247-byte 16EC
body, after normalizing loader relocation words, matches the pinned body.

| Observed metadata | Value |
| --- | --- |
| Live CS:IP / load base | `08DE:1707` / `0824h` |
| Watched arrival / instruction counter | 1 / 423317518 |
| `DS:49F3` at the mode read | **5** |
| `DS:6DDA` snapshot (not read on this branch) | 0 |
| Inferred entry SP / current-frame relation | `3E98h` / `SP = BP-102h` |
| Local message length after StrStore | 1 |
| Return frame | `08DE:0A1E` |
| Routed C returns / stops | 0 / 0 |

The return frame matches the original `PUSH CS; near CALL 16EC` sequence
at `00BA:0A1A`, including its four live instruction bytes. An initial
direct-far-call-only check returned false; the near-call check explains
that result without changing the snapshot or claiming caller identity.

**Inference from the pinned branch:** mode 5 selects the path that bypasses
the `177B` box call. This is one measured selector point, not an observation
of branch completion, a frequency, or an invariant over other arrivals.
The selected mode-5 path still calls `03F1:0137`, `00BA:1816`, and
`0709:0768`; the common tail can also delay and clear. Their live effects
have not been compared here. In particular this does not reopen t100's NO.
The snapshot has opcode `CDh` at the tile overlay stub, rather than the
loaded jump used by the synthetic box cases. No loader execution or correct
loaded tile target is established by observing a bypass predicate.

The route driver had reached its encounter gate after loading the save.
The probe deliberately interrupted the route at the checkpoint and shut
down only its own disposable emulator (SIGTERM). It did not complete the
route. The checkpoint contains RAM, VGA memory, and registers, not a full
machine restore state; a saved frame is not a presentation fence. No C
candidate was executed from this state.

Private receipts are retained under
`resources/oracle-runs/t131-first-selector-v1/` (`checkpoint.*`,
`t131-provenance.json`, `t131-attempt.json`) and
`resources/t131/native-selector.json`. The private probe/analyzer are
`resources/t131/native_capture.py` and `resources/t131/analyze_native.py`;
these are local experiment artifacts, not a shipped native parity harness.

| Provenance item | SHA-256 |
| --- | --- |
| Checkpoint binary | `fc66e91e8e8d48cc0b176bc350febab753bff297f2d93fc6c1e9d1f21cb6efae` |
| Checkpoint header | `becb1b8712d3d1bae8d0fb654e92299b1133930beb863b6b721ba26bc5957d78` |
| Checkpoint JSON | `ba2ac14dc4dfb340cbcee99415d930ab541331d00f5397bb66a7e8bbf478dad2` |
| Checkpoint RAM | `034e0dd9f3b876e20cf923a88fdd7797e07cfe2e7c138ab16ab10e1762e1f72e` |
| START.EXE | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## NOT-CLOSABLE: the remaining obligation

The display boundary has moved from an untested assumption to a conditional
site-local proof with an executed original-code violation, plus one native
box-bypass predicate. It is still insufficient to establish that the rest
of the dependency closure is off the port path. For box-taking arrivals,
live preservation of the caller's control frame remains unproved, and its
violation demonstrably reaches a callee that the EGA-attached model refuses.
The one observed mode-5 point neither proves all arrivals bypass the box nor
validates the mode-5 clear/title/printer and common-tail effects. Cold loading
and the bounded printer/termination contract also remain unresolved.

The next evidence would have to establish a defined port entry contract and
validate its live selector/frame/object/loading conditions, including the
remaining selected callees. Additional synthetic coverage alone cannot
discharge that obligation. This document is a conditional evidence step;
it supplies no readiness, membership, identity, credit, or unlock verdict.

## Still not verified

- Selectors across other live 16EC arrivals; live frame/object preservation;
  title lengths or general message-length bounds; a correct live loaded
  overlay target. The one post-StrStore mode/length and stub-opcode snapshot
  above does not establish those invariants.
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

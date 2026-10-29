# OVL_0038_0B1B: the 009E dispatcher's slot-32 handler (raw CPU, composing SYSTEM StrLoad, stopping at unit 38h +0058)

`pr_u38_0b1b_cpu_run` is a raw-CPU model of GAME.OVR unit `38h` `+0B1B` (full CPU14 + RAM state inside the composition
profile below): public entry 27 (stub `0041:00A7`), the handler the `009E` dispatcher reaches through slot 32
(`DS:68A8`), listed as a named gap in [OVL_009E_0000_HANDLERS.md](OVL_009E_0000_HANDLERS.md) and as `missing` in
[U9E_DISPATCH_HANDLER_BACKLOG.md](U9E_DISPATCH_HANDLER_BACKLOG.md). It runs on flat RAM, composes the accepted SYSTEM
StrLoad leaf (`pr_system_strload_0b0b`), and **stops** at the near call to the unit-local routine `+0058` with saved
state. This is a **bounded body with one exact-call stop boundary**, not a strict full-C translation; it claims no
identity, credit, accounting row or dispatcher wiring.

Task t515 (owner 2370/2371 authorization relayed by the VP; retargeted from `OVL_0038_0947`, which is already merged as
`pr_u38_string_match_0947_cpu`; private branch `claude/t515-u38-0b1b-cpu` off accepted main `24e2c752`; no shared
edits). Prior ABI frame: the five-word frame of [OVERLAY_UNIT38_EFFECT_ENTRIES.md](OVERLAY_UNIT38_EFFECT_ENTRIES.md) is
reused as attributed prior evidence for the argument layout only. Every entry, extent, return, callee and fixup fact
below was re-measured from the bytes with Capstone 5.0.7 and the relocation-backed inventory.

## Endpoint (established before translation)

| Item | Value |
| --- | --- |
| Span | unit `38h` `+[0B1B,0BAC)`, **145 bytes, 47 instructions**, one `RETF 0Ah` at `0BA9`; file offset `0x0F776` |
| SHA-256 of the 145 bytes | `267792d56c52855703d8e35c99ca6ffb5a90f3a97fb3f3e0d14c5cbbc2e9195d` |
| Ledger row | `overlay_function_ledger.csv` `0x38:0x0b1b`: grade A, entry slot 27, **bound 145, decoded 145, 47 insns** (exact: the next entry `+0BAC` begins with `PUSH BP; MOV BP,SP`); `far_runtime` 1, `near` 1, `ds_globals` 1, `es_refs` 11, `retf 0xa` |
| Classification row | `overlay_classification.csv`: `game_evidenced`, `provisional`, `calls` 2 |
| Dispatcher | `overlay_u9e_dispatch_0000_cpu.c` slot table: `{K_GAP_UNCREDITED, 0x00A7, 0x0041, 0x0B1B, …}` (slot 32); `u9e_dispatch_handlers.json`: `interface: missing` |
| Source constant | `CS:0B11..0B1A`: `09h` + `"collapses"` (10 bytes), the preceding entry's trailing constant, loaded by StrLoad into the 10-byte local |
| Far call | `LCALL 0AF8:0B0B` at `0B43` (SYSTEM StrLoad); segment word `CS:0B46` **is a loader fixup** |
| Near call | `PUSH CS` at `0B48`; `CALL 0058` at `0B49` (unit-local `+0058`, 181 bytes, `RETF 0Ah`; a far frame for a `RETF` callee) |
| Backward branches | none (`JNE 0B4C` at `0B2E` and `JNE 0B9D` at `0B92` are forward) |
| Undefined flags | one producer: `SHR AX,CL` at `0B57` with CL = 4 (OF for a count other than 1, and AF). The emitter's path report: **never observed**, overwritten by `CMP B+84h,0B3h` at `0B8C` before any read. `XOR AH,AH` (`0B53`) clears CF/OF/AF by the documented convention and is overwritten the same way |
| DS globals | `DS:[681Fh]` (the gate byte, read only; the same gate `OVL_0038_05EB` reads). ES references: `A+4`, `A+3`, `B+10Eh`, `B+10Fh`, `B+76h`, `B+98h`, `B+6Bh`, `B+72h`, `B+84h`, `B+9Fh` |

Inputs are the pinned GAME.OVR (SHA-256 `872964a3…`) and START.EXE (`d825df2b…`), hash-asserted by the emitter.
`tools/overlay/emit_ovl038_0b1b.py` generates the C from the bytes with `emit_raw_cpu_c.py` (flat-RAM accessors) plus
a near-`CALL` case the base emitter lacks; `--check` regenerates it byte for byte. The emitter asserts the single
`RETF`, no backward branch and exactly one unobserved undefined-flag producer.

## Behaviour

Entered after a far call: `[SP]` IP, `[SP+2]` CS, then five argument words: `[BP+6]` (never read), the far pointer A at
`[BP+8]/[BP+0Ah]` and the far pointer B at `[BP+0Ch]/[BP+0Eh]`. `PUSH BP; MOV BP,SP; SUB SP,0Ah` makes a 10-byte local L
at `SS:BP-0Ah`. A and B are reread with `LES` at every use.

1. **`A+4` := 0.**
2. **`CMP byte DS:[681Fh], 0`; not equal: step 4.**
3. Push B (segment, offset); `AL` := 6, push `AX` (the entry AH rides in the high byte); push `SS:L`; push `CS:0B11`;
   **`LCALL 0AF8:0B0B`**: StrLoad copies the 10-byte constant to L. Its accepted contract: `RETF 4` pops the source pair
   and leaves `SS:L`; `AL` := length `09h`, `CX` := 0, `DX` := DS, `BX` := its entry SP, `SI`/`DI` advanced, DF cleared,
   flags `XOR CH,CH`'s; AH is kept. Then **`PUSH CS; CALL 0058`**: the unit-local routine receives (B, the AX word,
   `SS:L`) under a far return frame and would balance the five words with its `RETF 0Ah`. **The model stops here:
   `STOP_0058`.**
4. (Directly when the gate is nonzero; after `+0058` otherwise, unreachable in this model.) `AL` := byte `A+3`; `AH` :=
   0; `CL` := 4; `SHR AX,CL`; `B+10Eh` := `AL` (the high nibble of `A+3`); `B+10Fh` := 1; `B+76h` := 0; `B+6Bh` := byte
   `B+98h`; `B+72h` := `0Ch`; `CMP byte B+84h, 0B3h`; equal: `B+84h` := 0; `B+9Fh` := 0. `MOV SP,BP; POP BP; RETF 0Ah`.

**Stop convention (saved partial state, no synthetic callee success).** `STOP_0058` leaves the CPU exactly as the
`CALL` left it: CS (from `PUSH CS`) and the return offset `0B4C` pushed under the five argument words, CS unchanged, IP
`0058h`, BP this body's frame, L filled, `A+4` already cleared, no step-4 store done. Nothing is rolled back.
`diagnostic.callee` = 2 and `diagnostic.site` = `0B49`.

**Why `+0058` is a stop and not a composition.** `OVL_0038_0058` has no accepted model. Its 181 bytes reach
`00BA:16EC` (the open keystone, t24), `00B0:0061`, `00B0:002F`, `03E0:0075`, `0709:0DBE`, `00BA:17E3` and `00BA:0767`;
[STRICT_CANDIDATE_QUEUE.md](STRICT_CANDIDATE_QUEUE.md) measures its one-hop closure at 4,571 bytes with five unready
callees of its own. The saved stop state is exactly the entry state a future `+0058` model takes (its frame is
`[BP+6]/[BP+8]` = `SS:L`, `[BP+0Ah]` = the AX word, `[BP+0Ch]/[BP+0Eh]` = B), so composing it is a separate bounded
step for the reviewer to authorize, not an improvisation here.

**Statuses.** `OK` (the gate-nonzero path); `ARGUMENT` (null pointers, RAM not 1 MiB, IP not `0B1Bh`; nothing changed,
diagnostic initialised -1/-1/0); `CALLEE` (StrLoad did not return: its status in `diagnostic.callee_status`; only its
own ARGUMENT exists, unreachable from here); `TRANSFER` (StrLoad returned elsewhere than `CS:0B48`, or the stop handler
somehow continued); `STOP_0058`. No limits parameter: StrLoad has no budget and this body has no backward branch.

## Composition profile

The state claim is full CPU14 + RAM **inside this profile**:

- The CPU and diagnostic objects are distinct from RAM.
- Loaded code and bindings are read live and immutable during the run: the constant `CS:0B11..0B1A` and the segment
  word `CS:0B46`. The test shows the constant is read live (a shorter string copies fewer bytes). The segment word is
  loaded into CS for the StrLoad call and restored by its `RETF`; the leaf model runs on IP alone, so a wrong word at
  `CS:0B46` is **not** detected by this model (the same holds for the accepted `OVL_0038_070F` composition).
- StrLoad's accepted contract applies to the copy: `REP MOVSB` byte by byte, `SI`/`DI` wrap at 16 bits, L and the
  source may alias; a longer string at `CS:0B11` overruns L into the frame exactly as on the CPU (not exercised).
- `+0058`'s own entry preconditions are **not** checked, because it is not entered.
- Reached dependency return frames are nonwrapping; stack and data aliases stay live; no rollback on any stop.
- Interrupts, faults and self-modifying code are outside this synchronous profile.
- Flags: `SUB SP,0Ah` (`0B1E`) is overwritten by `CMP DS:[681Fh],0` (`0B29`), which the `JNE` reads. At `STOP_0058`
  they are StrLoad's (`XOR CH,CH`: ZF, PF, DF clear). On the `OK` path `XOR AH,AH` and `SHR AX,CL` (AF := 1 pinned,
  OF by the emitter's rule for a count of 4; both architecturally undefined) are overwritten by `CMP B+84h,0B3h`
  before any read, and the return flags are that CMP's. Deterministic conventions of the accepted models; no native
  equality implied.

## What is and is not claimed

- The 47 instructions, operand order, both call sites, the fixup, the constant and the single `RETF 0Ah` are measured
  from the bytes. The StrLoad composition reuses the merged leaf model unchanged.
- `OVL_0038_0058` is an **unresolved dependency** for this body: exact-call stop with saved state. The post-call step 4
  is emitted and reached only on the gate-nonzero path.
- No original-byte, Unicorn or DOSBox-X execution was run (not authorized at this stage). No row is added to
  `strict_full_c_addresses.csv`, `reviewed_function_identities.csv` or `partial_callee_models.csv`; slot 32 of
  `overlay_u9e_dispatch_0000_cpu.c` is untouched. The census row in `aperture_observability.csv`
  (`overlay_u38_0b1b_cpu.c,flat,no,no`) and the `emit_ovl038_0b1b` entry in `test_emitters_reproduce_sources.py` are the
  generated-census and emitter-registry gates for this source, nothing more. The pre-existing `check_record_shas.py`
  failure (identities CSV commit ids absent from this clone) is unrelated.

## Validation

**Focused C test.** `tests/recovered/test_overlay_u38_0b1b_cpu.c` passes **14,169** hand-predicted checks, built with
asserts on (no `CMAKE_BUILD_TYPE`, no `NDEBUG`), on synthetic RAM with no game bytes: only the 10-byte constant, the
segment word, the gate byte, records A and B and the frame. It covers:

- gate 1, `FFh`, `80h` for all 256 values of `A+3`: `OK` with `A+4` cleared, `B+10Eh` = `A+3 >> 4`, `B+10Fh` 1,
  `B+76h` 0, `B+6Bh` = `B+98h`, `B+72h` `0Ch`, `B+84h` cleared from `0B3h`, `B+9Fh` 0, the neighbouring bytes of A and
  B still fill, `AX` = `B+98h` (AH 0), `CX` = CH|04, ES:DI = B, L never written, `[BP+6]` unread, return flags `0646h`;
- `B+84h` = `0B3h`, 0, `0B4h`, `0B2h` with `B+98h` = `FFh`/0: the clear happens only for `0B3h` and the return flags
  are `0646h`, `0617h`, `0602h`, `0697h` (hand-derived from the CMP: ZF PF; CF PF AF; none; CF PF AF SF);
- gate 0 for all 256 entry AH values: `STOP_0058` with the saved state checked word by word (BP = entry − 2, L = the
  constant, B's words, the AX word AH|06, `SS:L`, CS and `0B4C`, the StrLoad far frame left below at entry − 28/−30, SP
  = entry − 26, CS unchanged, IP `0058h`; `AX` AH|09, `CX` 0, `DX` DS, `BX` entry − 30, `SI` `0B1Bh`, `DI` BP, ES SS,
  flags `0246h`; `A+4` cleared and every step-4 target still fill);
- the constant is read live (a length-3 string copies 4 bytes, `SI`/`DI` advance by 4, AL = 3); a different word at
  `CS:0B46` passes through the leaf unchanged (documented above as not detected);
- the argument checks (null RAM, wrong size, null CPU, wrong IP): CPU unchanged, diagnostic initialised, RAM untouched.

```sh
ctest --test-dir <build> -R poolrad_overlay_u38_0b1b_cpu --output-on-failure
python3 tools/overlay/emit_ovl038_0b1b.py --check
python3 tools/overlay/aperture_census.py            # census row present
python3 -m unittest discover -s tools/overlay -p test_emitters_reproduce_sources.py
```

`emit --check`, the generated-census check, the emitter-registry test and the `0400`, `string_match` and
`system_string_leaves` sibling tests were rerun on this head.

**Not run (needs separate bounded authorization):** a Unicorn or DOSBox-X differential against the original bytes, a
mutation suite, and the `+0058` composition. A comparator for this body stays private until the reviewer accepts the
source.

## Review routing

Author pane `%91` (Claude, t515). Independent non-Codex review routed by the CoS/VP; no author self-acceptance, no
self-merge.

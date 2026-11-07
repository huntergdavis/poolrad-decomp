# OVL_0038_05EB: the 009E dispatcher's slot-15 handler (raw CPU, composing 001F, stopping at 009E_1344 and 00BA_0767)

`pr_u38_05eb_cpu_run` is a raw-CPU model of GAME.OVR unit `38h` `+05EB` (full CPU14 + RAM state inside the composition profile below): public entry 14 (stub `0041:0066`),
the handler the `009E` dispatcher reaches through slot 15 (`DS:6864`), which the credited installer `OVL_0038_31A2` stores
([OVL_009E_0000_HANDLERS.md](OVL_009E_0000_HANDLERS.md) lists the slot as a named gap). It runs on flat RAM, as the
dispatcher's handlers do, composes the merged raw model of this unit's `+001F` (`pr_u38_gated_append_001f_cpu`), and
**stops** at the two far calls whose callees have no raw model. This is a **bounded body with two exact-call stop
boundaries**, not a strict full-C translation; it claims no identity, credit, accounting row or dispatcher wiring.

Task t489 (owner 2370/2371 authorization relayed by the VP; private branch, no shared edits). Prior ABI frame: the
five-word frame of [OVERLAY_UNIT38_EFFECT_ENTRIES.md](OVERLAY_UNIT38_EFFECT_ENTRIES.md) is reused as attributed prior
evidence for the argument layout only; every entry, extent, return, callee and fixup fact below was re-measured from the
bytes with Capstone 5.0.7 and the relocation-backed overlay inventory (`tools/overlay/overlay_inventory.py`).

## Exact original

| Item | Value |
| --- | --- |
| Span | unit `38h` `+[05EB,0647)`, 92 bytes, 35 instructions, `RETF 0Ah`; file offset `0x0F246` |
| SHA-256 of the 92 bytes | `bb3ec9bf7f2ef1f80d82faafc0273eabeb727253daf7de13a36337386856421c` |
| Ledger row | `overlay_function_ledger.csv` `0x38:0x05eb`: grade A, entry slot 14, bound 92 = decoded 92, 35 insns, `far_overlay` 1, `far_resident` 1, `near` 1, `ds_globals` 4, `es_refs` 2, `retf 0xa` |
| Near call | `PUSH CS` at `0603`, `CALL 001F` at `0604` (this unit's `+001F`, the gated append, raw model merged) |
| Far call 1 | `LCALL 00B0:007F` at `062A`; segment word `CS:062D` **is a loader fixup**; `00B0h` is unit `9Eh`'s stub paragraph and stub offset `007F` is entry 19, body `OVL_009E_1344` (no model) |
| Far call 2 | `LCALL 00BA:0767` at `063E`; segment word `CS:0641` **is a loader fixup**; `RES_00BA_0767` (load pieces; only a bounded semantic model, [RESIDENT_LOAD_PIECES_0767.md](RESIDENT_LOAD_PIECES_0767.md)) |
| Backward branches | none (`JE 0643` at `0609`, `JBE 0643` at `0614`, `JE 0643` at `0634` are all forward) |
| Undefined flags | no `SHL`/`SHR`/`MUL` producer (the emitter's `undefined_report` enumerates only those and is empty). One logical producer: `OR AL,AL` at `0607` leaves **AF architecturally undefined**; the tree's `logic()` convention models AF := 0 (with CF/OF := 0). That AF is **visible at the `RETF`** on the `AL` = 0 path (the `OR` is the last flags writer there) and is overwritten by `CMP` at `060E` on the other paths. Deterministic modelling convention; no native AF equality is implied |
| DS globals | `6819` (written 0), `49F3` (read), `5D92`/`5D94` (read, pushed) |
| ES references | `A+3` (read), `B+11Bh` (read) |

Inputs are the pinned GAME.OVR (SHA-256 `872964a3…`) and START.EXE (`d825df2b…`), both hash-asserted by the emitter.
`tools/overlay/emit_ovl038_05eb.py` generates the C from the bytes with `emit_raw_cpu_c.py` (flat-RAM accessors), adding
a `CALL rel16` case the shared emitter lacks; `--check` regenerates it byte for byte.

## Behaviour

Entered after a far call: `[SP]` IP, `[SP+2]` CS, then five argument words. `[BP+6]` is a word **this body never
reads**; `[BP+8]` far pointer A; `[BP+0Ch]` far pointer B. Far pointers are reread with `LES` at each use.

1. **`+001F` with (a = `0Ah`, c = byte `A+3`, b = `0Fh`, record B).** The pushes are B segment, B offset, `AX` with
   `AL = 0Fh`, `AX` with `AL = A+3`, `AX = 000Ah`, then `PUSH CS` and the near call. The two byte arguments carry the
   **entry AH** in their high bytes (`MOV AX,000Ah` then clears AH before the call). `001F` sees exactly a far-call frame
   (`PUSH CS` supplies the segment; its `RETF 0Ah` pops both and the five words). `001F` returns `AL` 1 when `DS:[681F]`
   was zero and it appended (b, a, c, 1) to the record through the live stub `0052` / U9E `+0E52` / GetMem, else `AL` 0.
2. **`OR AL,AL`; `AL` 0: return.** Else `LES` B; **`B+11Bh` ≤ 1 (unsigned): return.** The last flags writer on these
   two return paths is the `OR` or the `CMP` respectively.
3. Else **`DS:[6819]` := 0**, push B segment, B offset, then `AX` with `AL` = 1, 0, 0 (high bytes: AH as `001F` left it),
   and **`LCALL 00B0:007F`**. The model stops here: `STOP_1344`.
4. (Not reachable while step 3 stops.) `DS:[49F3]` = 5: return; else push `DS:[5D94]`, `DS:[5D92]` and
   **`LCALL 00BA:0767`**: `STOP_0767`.
5. `POP BP; RETF 0Ah`.

**Stop convention (saved partial state, never a synthetic callee success).** A stop leaves the CPU exactly as the
`LCALL` left it: the return frame (CS, `062F` or `0643`) pushed, CS loaded from the live segment word at `CS:062D` /
`CS:0641`, IP `007F` / `0767`, BP still this body's frame, every earlier write (the `001F` append, `DS:[6819]`) in RAM.
Nothing is rolled back. `diagnostic.callee` names the boundary (2 = `00B0:007F`, 3 = `00BA:0767`) and `diagnostic.site`
the `LCALL` IP. For `STOP_1344` the saved CS:IP is the **stub** entry (the `EA` far jump to `+1344` is loader data a
successor resolves, as `001F`'s model resolves stub `0052`).

**Statuses.** `OK`; `ARGUMENT` (null pointers, RAM not 1 MiB, IP not `05EBh`; nothing changed, diagnostic initialised
to -1/-1/0); `CALLEE` (`001F` stopped: its status in `diagnostic.callee_status`, its own `append_status`/`getmem_status`
in `diagnostic.d001f`, state as it left it); `TRANSFER` (`001F` returned elsewhere than `CS:0607`); `STOP_1344`;
`STOP_0767`. `limits` carries `001F`'s two caps (`max_heap_steps`, `max_links`); `NULL` means both zero with `001F`'s
dependency meanings. There is no instruction budget of this body's own: it has no backward branch.

## Composition profile (t498 correction)

The state claim is **full CPU14 + RAM inside this profile**, not unqualified. The profile is `001F`'s merged contract,
restated here for the composed entry:

- **SP range.** `001F` is entered at SP′ = (entry SP − 16) mod 65536 (BP save, five argument words, `PUSH CS`, return
  IP) and requires SP′ ∈ `[0040,FFF2]`. Outside it `001F` returns `ARGUMENT`; this model then reports `CALLEE` with
  `callee_status` `ARGUMENT` and the **partial prefix in place** (BP frame and the seven pushed words written, IP `001F`).
  This body's own `ARGUMENT` (null pointers, RAM size, IP) changes nothing.
- **Objects.** The CPU, limits and diagnostic objects are distinct from RAM.
- **Immutable code and bindings.** The segment words `CS:062D`/`CS:0641` and `001F`'s binding chain (`CS:004A`, stub
  `0052`, U9E `+0E52`, SYSTEM) are read live and must not be rewritten by the modelled code during the run.
- **Frames.** Reached dependency return frames are nonwrapping. Data, heap and stack aliases stay live (`001F`'s append
  may write under this frame). No rollback on any stop.
- **Exclusions.** Interrupts, faults and self-modifying code are outside this synchronous profile.
- **Flags convention.** Logical `AF := 0` at `0607` as in the table above; the test's return-flag values (`0646h` on the
  `AL` = 0 path) encode that convention.

## What is and is not claimed

- The 35 instructions, their operand order, the three call sites, both fixups and the single `RETF 0Ah` are measured
  from the bytes. The `001F` composition reuses the merged raw model and its merged dependency contracts unchanged.
- `OVL_009E_1344` (a 38-body closure per the dispatcher inventory) and `RES_00BA_0767` are **unresolved
  dependencies**: the body stops at the exact call with saved state. The `0767` boundary is emitted for exactness but
  is unreachable in this model because the `1344` stop precedes it on the only path that reaches it.
- No original-byte, Unicorn or DOSBox-X execution was run for this change (not authorized at this stage). No row is
  added to `strict_full_c_addresses.csv`, `reviewed_function_identities.csv` or `partial_callee_models.csv`, and slot 15
  of `overlay_u9e_dispatch_0000_cpu.c` is not touched. Those are reviewer decisions.
- The pre-existing `check_record_shas.py` failure (two commit ids in `reviewed_function_identities.csv` absent from
  this clone) is unrelated to and untouched by this change.

## Validation

**Focused C test.** `tests/recovered/test_overlay_u38_05eb_cpu.c` passes **1,597** hand-predicted checks, built with
asserts on (no `CMAKE_BUILD_TYPE`, no `NDEBUG`), on synthetic RAM with no game bytes: only the two relocated segment
words, the binding operands `001F`'s own test uses for its stub chain, the heap words its append walks, records A and
B, and the frame. It covers:

- the gate-set path (`DS:[681F]` ≠ 0, both `80h` and `1`): `AL` 0, return, no append, no `DS:[6819]` write, and the
  frame `001F` saw left below SP (B, `AX` words carrying the **entry AH** with `0Fh` and `A+3`, `000Ah`, CS, `0607`,
  `001F`'s BP save and result byte);
- the append path for all 256 entry AH values: the node (`0Fh`, `000Ah`, `A+3`, 1, link 0:0) at the heap, linked from
  `B+81h`; `AL` 1; flags from `CMP B+11Bh,1` (ZF, PF for 1; borrow CF/AF/SF/PF for 0); ES:DI = B; `[BP+6]` unread;
- `B+11Bh` = 2, `FFh`, `80h` (unsigned compare): `DS:[6819]` := 0 and `STOP_1344` with the saved state checked word by
  word (frame, the three `AX` words carrying `001F`'s AH, B, BP, CS from `CS:062D`, IP `007F`, SP, ES:DI, DS, SS,
  records untouched, append retained);
- the frozen prefix: `DS:[49F3]` = 5 and `DS:[5D92]/[5D94]` are not read at the stop;
- `NULL` limits and diagnostic;
- `001F`'s `CODE` stop (changed `CS:004A` byte) and its `CALLEE` stop (GetMem RunError 203 from an aliased free list)
  propagated as `CALLEE` with the nested statuses and the state as `001F` left it;
- the argument checks (null RAM, wrong size, null CPU, wrong IP): CPU unchanged, diagnostic initialised.

```sh
ctest --test-dir <build> -R poolrad_overlay_u38_05eb_cpu --output-on-failure
python3 tools/overlay/emit_ovl038_05eb.py --check
```

`emit --check` and the `001F`, `0400` and `0E52` sibling tests were rerun on this head.

**Not run (needs separate bounded authorization):** a Unicorn or DOSBox-X differential against the original bytes,
and a mutation suite. A comparator for this body stays private until the reviewer accepts the source.

## Review routing

Author pane `%91` (Claude, t489). Independent review reserved for `%30` through the CoS canonical routing; no author
self-acceptance, no self-merge.

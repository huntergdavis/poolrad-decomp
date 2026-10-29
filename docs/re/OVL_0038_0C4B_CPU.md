# OVL_0038_0C4B: the 009E dispatcher's slot-39 handler (raw CPU, a partial chain stopped at the 16EC window message)

`pr_u38_handler_0c4b_cpu` is a raw-CPU, full-state model of GAME.OVR unit `38h` `+0C4B`, the handler the `009E` dispatcher
reaches through slot 39 (`DS:68C4`, stub `0041:00C5`, public entry 33), stored by the credited installer `OVL_0038_31A2`
([OVL_009E_0000_HANDLERS.md](OVL_009E_0000_HANDLERS.md) lists it as a named gap). It runs on flat RAM, as the dispatcher's
handlers do, and composes the SYSTEM string load `0AF8:0B0B`.

**Claim, stated narrowly.** This change supplies a body (`src/recovered/overlay_u38_0c4b_cpu.c`, header
`include/poolrad/recovered/overlay_u38_0c4b_cpu.h`), its test, a static checker and this document. It is a **partial
chain**: the resident window message `RES_00BA_16EC` is recorded as **not closable on the currently recorded evidence**
([RES_00BA_16EC_CLOSURE_STATUS.md](RES_00BA_16EC_CLOSURE_STATUS.md); keystone t24; `+0C4B` is on its caller list), so the
body stops before its call with the exact state saved. Its bus-based model (`resident_00ba_window_16ec_cpu.h`, EGA display
closure) is not pulled into this flat handler. **It is not a strict full-C port, proposes no credit, and flips no dispatcher
slot** (slot 39 stays a named gap in `overlay_u9e_dispatch_0000_cpu.c`). No original or native execution was run: the
checker reads the media as bytes only. Task t516 (the queued successor of t492; every fact below was re-derived from the
bytes); reviewer reserved through the CoS.

## Exact original (the endpoint, established before translation)

| Item | Value |
| --- | --- |
| Span | unit `38h` `+[0C4B,0CA8)`, 93 bytes, 37 instructions, `RETF 0Ah` at `+0CA5`; the ledger's 120-byte bound (`overlay_function_ledger.csv`, `0x38:0x0c4b`, entry 33, `retf 0xa`) is this body plus entry 34's 26-character Pascal string in `+[0CA8,0CC3)` (27 bytes, printable, SHA-256 `17fa53bb…`, not quoted) |
| Body SHA-256 | `0a0640175f1273dac34d180a437f9eae7d0957b21264332f6f3bc80e19f07575` |
| Unit identity | file offset `0EC5Bh`, code SHA-256 `1dd5e9d5…`, run-time stub segment `0041`, file stub of entry 33 `INT 3Fh` to `+0C4B`, loaded stub `0041:0020+5·33 = 0041:00C5` |
| 009E slot | 39 (`DS:6828+4·39 = DS:68C4`), per the accepted dispatcher table (`OVL_009E_0000_HANDLERS.md`, installer `OVL_0038_31A2`); `U9E_DISPATCH_HANDLER_BACKLOG.md` lists it as missing |
| Far calls | `+0C81 LCALL 0AF8:0B0B`, `+0C8C LCALL 00BA:16EC`; their segment words `CS:0C84` and `CS:0C8F` are the only loader fixups in `+[0C46,0CC3)`, read live |
| Near calls | none |
| Branches | one forward jump `+0C5C JNE +0C98`; no backward jump, no INT, no port access, no multiply |
| Undefined flag | `SHL AX,1` at `+0C9D` (AF): nothing writes FLAGS after it, so it is **visible at the RETF** (the emitter's report); the model applies the pinned DOSBox-X rule AF := 1 (Unicorn would clear it) |
| DS access | `DS:681A` read at `+0C98` and written at `+0C9F`; no other DS access |
| Record accesses | `A+3` read twice and written (`MOV byte ES:[DI+3],AL`), `B+30h` incremented (`INC word ES:[DI+30h]`, after the stop) |
| String | `CS:0C46`, a 4-character printable Pascal string (length byte `04h`) in `+[0C46,0C4B)`, right after the `+0C26` body's `RETF 0Ah`; SHA-256 `83ef3176…`; not quoted here |

Inputs are the pinned GAME.OVR (SHA-256 `872964a3…`) and START.EXE (`d825df2b…`), read only. `emit_ovl038_0c4b.py` generates
the C from the bytes with `emit_raw_cpu_c.py` (flat-RAM accessors) and `--check` regenerates it byte for byte; its one
hand-written case is the `+0C8C` stop. The classification row (`overlay_classification.csv`) is `game_evidenced`,
`provisional`.

## Behaviour

Frame after the far call: `[SP]` IP, `[SP+2]` CS; `[BP+6]` a word, **unused**; `[BP+8]` a far pointer A, a record whose
byte `+3` holds flag bits; `[BP+0Ch]` a far pointer B, a record whose word `+30h` is a counter. `SUB SP,6` makes one local;
the body uses `[BP-5]`, a 5-byte string buffer (`[BP-6]` is never touched).

1. **Bit test.** `AL := A+3; AND AL,10h; OR AL,AL; JNE +0C98`: with bit 4 already set the body goes straight to step 3.
2. **Message.** `XOR AH,AH; ADD AX,10h; A+3 := AL` (bit 4 was clear, so the add never carries out of the byte; `A+3 = EFh`
   becomes `FFh`). Push B and `SS:BP-5`; push `CS:0C46`; `0AF8:0B0B` (StrLoad, [SYSTEM_STRING_LEAVES.md](SYSTEM_STRING_LEAVES.md))
   copies the 4-character string into the buffer and pops only its source (`RETF 4`); push `AH:0Ah` and `AH:01h` (AH is
   whatever StrLoad left in it); then `+0C8C LCALL 00BA:16EC`. **The model stops here (`PENDING`)**: IP `0C8C`, the six
   argument words (B, the buffer pointer, `0Ah`, 1) pushed and nothing pushed for the call, the buffer holding the copied
   string, `A+3` updated, `DS:681A` untouched.
3. **Doubling.** `AL := DS:681A; XOR AH,AH; SHL AX,1; DS:681A := AL` (the byte doubles; its top bit falls into AH and is
   lost); `MOV SP,BP; POP BP; RETF 0Ah`.
4. **Not completed paths** (after the stop): 16EC's effects (`RETF 0Ch`) and `+0C91 LES DI,[BP+0Ch]; INC word ES:[DI+30h]`
   before step 3. Their generated cases are exact but **no test world reaches them**; OK is reachable only through the
   step-1 jump.

**Statuses.** `OK`, `ARGUMENT` (null RAM or CPU, RAM not 1 MiB, IP not `0C4Bh`; nothing changed), `CALLEE` (StrLoad's own
`ARGUMENT`, unreachable from this site: the model enters it at its entry with a 1 MiB RAM), `TRANSFER` (unreachable: the
leaf returns to the pushed frame) and `PENDING` (`+0C8C`). There are no limits: the body has no loop and no capped callee.
The diagnostic names the last callee (1 = StrLoad, 2 = the 16EC stop), its status and the site.

## Dependencies, resolved before composing

| Callee | Site | Resolution | Model | Status |
| --- | --- | --- | --- | --- |
| SYSTEM StrLoad `0AF8:0B0B` | `+0C81` | segment word `CS:0C84` is a fixup | `pr_system_strload_0b0b` (merged raw leaf) | runtime leaf |
| `RES_00BA_16EC` (the window message) | `+0C8C` | segment word `CS:0C8F` is a fixup | **none composed; exact stop boundary** (a bus-based model exists; the closure is recorded as not closable) | not closable (t24) |

`docs/re/partial_callee_models.csv` gains `OVL_0038_0C4B,partial`.

## Validation

**Focused C test.** `test_overlay_u38_0c4b_cpu.c` passes **89** hand-predicted checks, built with asserts on, on synthetic
RAM with no game bytes (the two relocated segment words, a 4-character synthetic source string at `CS:0C46`, a synthetic
record A and the DS byte). It covers: the step-1 jump for six `DS:681A` values (`00h`, `05h`, `07h`, `5Ah`, `80h`, `FFh`)
with the stored byte, AX and the **exact FLAGS word** after `SHL AX,1` (CF = OF = SF = 0, AF = 1, ZF for 0 only, PF of the
result byte), plus a whole-RAM comparison that only the saved BP and `DS:681A` changed; an A offset that wraps at 16 bits;
a run without a diagnostic; the full path to the `+0C8C` stop for `A+3 = EFh`, `00h` and `0Fh` with the stack frame, the
copied string, the untouched `[BP-6]`, StrLoad's exit registers and its popped return frame below SP, the updated
`A+3`, the untouched `DS:681A`, and a whole-RAM comparison that only the 24 bytes below the frame and `A+3` changed; every `ARGUMENT` case with the CPU
and RAM unchanged; and a sweep that the world writes nothing outside its declared bytes.

**Static checker.** `tools/overlay/ovl038_0c4b_cpu_audit.py` (`CHECKER_ROWS = ('OVL_0038_0C4B',)`) proves every row of the
"Exact original" table from the pinned media read as bytes, including the loaded stub address, the trailing string that
fills the ledger bound, the two calls in order, the two fixups and no other, the one jump, the one SHL, the A+3 test and
store, the B+30h increment, the single DS access, the string and the `+0C26` body's `RETF` before it, and that the emitter
regenerates the committed C byte for byte. `--compile-only` builds the model with its composed source and binds the symbol
with no media (`CHECKER_BUILD_OK`). The report says `executed: false`.

**Not done, by scope.** No Unicorn differential, no DOSBox-X run, no mutation run: original and native execution of this
body are a separately authorised review, and the comparator stays private until the body is accepted. The post-16EC
increment of `B+30h`, `CALLEE` and `TRANSFER` are not reached by any test world.

```sh
ctest --test-dir <build> -R poolrad_overlay_u38_0c4b_cpu --output-on-failure
python3 tools/overlay/emit_ovl038_0c4b.py --check
PYTHONPATH=tools/overlay python3 tools/overlay/ovl038_0c4b_cpu_audit.py --compile-only
PYTHONPATH=tools/overlay python3 tools/overlay/ovl038_0c4b_cpu_audit.py --output resources/u38-0c4b/report.json
```

# OVL_0038_10DE: the 009E dispatcher's slot-43 handler (raw CPU, a partial chain stopped at the 16EC window message)

`pr_u38_handler_10de_cpu` is a raw-CPU, full-state model of GAME.OVR unit `38h` `+10DE`, the handler the `009E` dispatcher
reaches through slot 43 (`DS:68D4`, stub `0041:00D9`, public entry 37), stored by the credited installer `OVL_0038_31A2`
([OVL_009E_0000_HANDLERS.md](OVL_009E_0000_HANDLERS.md) and `u9e_dispatch_handlers.json` list it as a named gap, interface
`missing`; `U9E_DISPATCH_HANDLER_BACKLOG.md` as missing). It runs on flat RAM, as the dispatcher's handlers do, and composes
the credited unit `38h` gated append `+001F`, the SYSTEM string load `0AF8:0B0B`, the credited resident sidecar lookup
`00BA:2199` and the credited unit `9E` list append `+0E52` (through its loaded stub).

**Claim, stated narrowly.** This change supplies a body (`src/recovered/overlay_u38_10de_cpu.c`, header
`include/poolrad/recovered/overlay_u38_10de_cpu.h`), its test, a static checker and this document. It is a **partial
chain**: the resident window message `RES_00BA_16EC` is recorded as **not closable on the currently recorded evidence**
([RES_00BA_16EC_CLOSURE_STATUS.md](RES_00BA_16EC_CLOSURE_STATUS.md); keystone t24; `+10DE` is on its caller list), so the
message path stops before its call with the exact state saved. Its bus-based model (`resident_00ba_window_16ec_cpu.h`, EGA
display closure) is not pulled into this flat handler. **It is not a strict full-C port, proposes no credit, and flips no
dispatcher slot** (slot 43 stays a named gap in `overlay_u9e_dispatch_0000_cpu.c`). No original or native execution was
run: the checker reads the media as bytes only. Task t522 (the queued successor of t516; every fact below was re-derived
from the bytes; the free-model draft under `docs/re/free_runs/OVL_0038_10DE/` was not used: its own verdict file records
fabricated citations); reviewer reserved through the CoS.

## Exact original (the endpoint, established before translation)

| Item | Value |
| --- | --- |
| Span | unit `38h` `+[10DE,1168)`, 138 bytes, 60 instructions, `RETF 0Ah` at `+1165`; **the whole 138-byte ledger bound is code** (`overlay_function_ledger.csv` `0x38:0x10de`, entry 37, `retf 0xa`; entry 38 starts at `+1168`) |
| Body SHA-256 | `814e17d09ed4b1022515a5c2e1f9933048504b1b1ff44c6e6cddba49caa58b95` |
| Unit identity | file offset `0EC5Bh`, code SHA-256 `1dd5e9d5…`, run-time stub segment `0041`, file stub of entry 37 `INT 3Fh` to `+10DE`, loaded stub `0041:0020+5·37 = 0041:00D9` |
| 009E slot | 43 (`DS:6828+4·43 = DS:68D4`), per the accepted dispatcher table and `u9e_dispatch_handlers.json` (`slot 43`, `table_address 68D4`, `original_stub 0041:00D9`) |
| Near call | `+10F9 PUSH CS; +10FA CALL +001F` (the credited gated append, `RETF 0Ah`; its body SHA-256 `342e00b6…` is re-pinned by this checker) |
| Far calls | `+111B LCALL 0AF8:0B0B`, `+1126 LCALL 00BA:16EC`, `+1142 LCALL 00BA:2199`, `+115D LCALL 00B0:0052`; their segment words `CS:111E`, `CS:1129`, `CS:1145`, `CS:1160` are the only loader fixups in `+[10D2,1168)`, read live |
| Branches | `+10FF JE +1162`, `+1109 JBE +1134`, `+1132 JMP +1162`, `+1149 JNE +1162`, all forward; no backward jump, no INT, no port access, no shift or multiply (no undefined flag) |
| DS access | none in the body (`+001F` reads the gate `DS:681F`; GetMem uses the SYSTEM heap words) |
| Record accesses | `A+3` read once (`MOV AL,ES:[DI+3]`); `B+10h` compared with 3, then decremented after the stop (`DEC byte ES:[DI+10h]`); B's list head `+7Fh/+81h` and nodes are touched only by the callees |
| String | `CS:10D2`, an 11-character printable Pascal string (length byte `0Bh`) in `+[10D2,10DE)`, right after the `+10C1` body's `RETF 0Ah`; SHA-256 `39a03f21…`; not quoted here |
| Stub binding | unit `9E` descriptor: run-time stub segment `00B0`; entry 10 (`00B0:0052`) is `+0E52`; file stub `INT 3Fh` to `+0E52`; the `+0E52` body SHA-256 `cfde5096…` is re-pinned |

Inputs are the pinned GAME.OVR (SHA-256 `872964a3…`) and START.EXE (`d825df2b…`), read only. `emit_ovl038_10de.py` generates
the C from the bytes with `emit_raw_cpu_c.py` (flat-RAM accessors) and `--check` regenerates it byte for byte; its three
hand-written cases are the `+10FA` near call, the `+1126` stop and the `+115D` checked stub hop. The classification row
(`overlay_classification.csv`) is `game_evidenced`, `provisional`.

## Behaviour

Frame after the far call: `[SP]` IP, `[SP+2]` CS; `[BP+6]` a word, **unused**; `[BP+8]` a far pointer A, a record whose
byte `+3` is read once; `[BP+0Ch]` a far pointer B, a record whose byte `+10h` is a counter and whose far list head is at
`+7Fh/+81h` (links at node `+5/+7`, nine-byte nodes). `SUB SP,10h` makes the locals: `[BP-10h]`, a 16-byte string buffer, and
`[BP-4]`, the 4-byte cursor cell used on the other path (the same bytes).

1. **Append.** Push B, `AH:2Bh`, `AH:(A+3)` and `3Ch` (`MOV AX,3Ch` clears AH); `PUSH CS; CALL +001F`
   ([U38_GATED_APPEND_001F_CPU.md](U38_GATED_APPEND_001F_CPU.md)): with `DS:681F = 0` it appends a node (byte 0 `2Bh`, word 1
   `3Ch`, byte 3 `A+3`, byte 4 `1`, link nil) to B's list through the live stub `00B0:0052`, unit `9E` `+0E52` and GetMem
   and returns `AL = 1`; with `DS:681F <> 0` it returns `AL = 0`. `OR AL,AL; JE`: zero returns.
2. **Branch.** `CMP byte ES:[DI+10h],3; JBE` goes to step 4.
3. **Message** (`B+10h > 3`). Push B and `SS:BP-10h`; push `CS:10D2`; `0AF8:0B0B` (StrLoad,
   [SYSTEM_STRING_LEAVES.md](SYSTEM_STRING_LEAVES.md)) copies the string into the buffer and pops only its source (`RETF 4`);
   push `AH:0Ah` and `AH:01h` (AH is whatever StrLoad left); then `+1126 LCALL 00BA:16EC`. **The model stops here
   (`PENDING`)**: IP `1126`, the six argument words (B, the buffer pointer, `0Ah`, 1) pushed and nothing pushed for the call,
   the node appended, the buffer holding the copied string, `B+10h` unchanged. 16EC's effects (`RETF 0Ch`),
   `DEC byte ES:[DI+10h]` and the `JMP +1162` to the epilogue are **not completed paths**: their generated cases are exact
   but no test world reaches them.
4. **Lookup** (`B+10h <= 3`). Push B, `AH:1Fh` (AH is 0 here: `+001F` preserves it) and `SS:BP-4`; `LCALL 00BA:2199`
   ([RESIDENT_SIDECAR_LOOKUP_2199_CPU.md](RESIDENT_SIDECAR_LOOKUP_2199_CPU.md)): the sidecar lookup walks B's list for a node whose byte 0 is `1Fh`,
   leaves the cursor (the node, or nil) in `[BP-4]` and `AL = found` (`RETF 0Ah`). `OR AL,AL; JNE`: found returns.
5. **Second append** (not found). Push B, `AH:1Fh` (2199's AH), `0` (`XOR AX,AX`), `00FFh` and `0000h`; `LCALL 00B0:0052`:
   the model reads the five stub bytes at the live segment word `CS:1160`, offset `0052`, and requires `EAh`, offset `0E52`
   **before anything is pushed** (`CODE` otherwise, IP `115D`), as the dispatcher model does for its own stubs; the credited
   `pr_u9e_list_append_0e52_cpu` ([U9E_LIST_APPEND_0E52_CPU.md](U9E_LIST_APPEND_0E52_CPU.md)) runs at the stub's segment and
   appends a node (byte 0 `1Fh`, word 1 `0`, byte 3 `FFh`, byte 4 `0`) through GetMem (`RETF 0Ch`). `MOV SP,BP; POP BP;
   RETF 0Ah`.

**Statuses.** `OK` (the gate return, the found return, or after the second append), `ARGUMENT` (null RAM, CPU or limits,
RAM not 1 MiB, IP not `10DEh`; nothing changed), `CODE` (the `+115D` stub), `CALLEE` (any non-returned status of `+001F`,
StrLoad, 2199 or `+0E52`, with the callee's status and the state as it left it), `TRANSFER` and `PENDING` (`+1126`).
`limits` carries GetMem's heap-walk cap and `+0E52`'s link cap (both appends) and 2199's loop-back cap; the diagnostic names
the last callee (1 = `+001F`, 2 = StrLoad, 3 = the 16EC stop, 4 = 2199, 5 = `+0E52`), its status, the site, `+001F`'s own
diagnostic and the second append's GetMem status.

## Dependencies, resolved before composing

| Callee | Site | Resolution | Model | Status |
| --- | --- | --- | --- | --- |
| `OVL_0038_001F` (the gated append) | `+10FA` | near call inside the unit after `PUSH CS`; its own binding `CS:004A LCALL 00B0:0052` and GetMem's binding are read live by it | `pr_u38_gated_append_001f_cpu` (credited, PR #1009) | strict-credited |
| SYSTEM StrLoad `0AF8:0B0B` | `+111B` | segment word `CS:111E` is a fixup | `pr_system_strload_0b0b` (merged raw leaf) | runtime leaf |
| `RES_00BA_16EC` (the window message) | `+1126` | segment word `CS:1129` is a fixup | **none composed; exact stop boundary** (a bus-based model exists; the closure is recorded as not closable) | not closable (t24) |
| `RES_00BA_2199` (the sidecar lookup) | `+1142` | segment word `CS:1145` is a fixup | `pr_res_00ba_2199_cpu` (credited) | strict-credited |
| `OVL_009E_0E52` (entry 10, the list append) | `+115D` | stub `00B0:0052`: unit `9E` descriptor entry 10 = `+0E52`; file stub `INT 3Fh` to `+0E52`; segment word `CS:1160` is a fixup | `pr_u9e_list_append_0e52_cpu` (credited) with `pr_system_getmem_023f` | strict-credited |

`docs/re/partial_callee_models.csv` gains `OVL_0038_10DE,partial`; `docs/re/aperture_observability.csv` gains the derived
row for the new flat model (`aperture_census.py --write`).

## Validation

**Focused C test.** `test_overlay_u38_10de_cpu.c` passes **228** hand-predicted checks, built with asserts on, on synthetic
RAM with no game bytes (the four relocated segment words, `+001F`'s binding word, the loaded-stub form of unit `9E` entry
10, unit `9E`'s GetMem binding, the SYSTEM heap words, an 11-character synthetic source string at `CS:10D2`, synthetic
records A and B, a synthetic old node and the gate byte). It covers: the gate return for 15 gate values with the three
argument words below SP (the entry AH on the first two, `003Ch` on the third) and AX zero; the full message path to the
`+1126` stop for six `B+10h` values with node 1's contents and head link, the frame, the copied string, StrLoad's exit
registers and popped frame, the untouched `B+10h`; the lookup path for `B+10h` 0..3 with node 2 linked after node 1, the nil
cursor, `+0E52`'s exit AX, ES:DI and flags, and the five pushed words; an old node with the key found (cursor = it, one
allocation) and with another key (three-node list); the `+115D` stub refused as `CODE` on a wrong opcode and a wrong offset
through a fixup word distinct from `+001F`'s binding, with nothing pushed for the call and no allocation; `CALLEE` from an
exhausted heap (`+001F`'s GetMem at RunError, AX = 203), from a zero loop-back cap (2199 at its loop head) and from a zero
link cap (the second append at `+0EA8`); an A offset that wraps at 16 bits; a run without a diagnostic; every `ARGUMENT`
case with the CPU and RAM unchanged; and whole-RAM comparisons on every completed path (only the stack below SP, the heap
words `DS:4404..4418`, GetMem's saved size `DS:8754`, the heap area, B's head and the old node's link change) plus a sweep
that the world writes nothing outside its declared bytes.

**Static checker.** `tools/overlay/ovl038_10de_cpu_audit.py` (`CHECKER_ROWS = ('OVL_0038_10DE',)`) proves every row of the
"Exact original" table from the pinned media read as bytes, including that the whole bound is code, the five calls in
order, the four fixups and no other, the four jumps, the argument loads in order, the `B+10h` test and decrement, the two
locals, the absence of any DS access, the string and the `+10C1` body's `RETF` before it, `+001F`'s and `+0E52`'s pinned
bodies, the unit `9E` binding, and that the emitter regenerates the committed C byte for byte. `--compile-only` builds the
model with its five composed sources and binds the symbol with no media (`CHECKER_BUILD_OK`). The report says
`executed: false`.

**Not done, by scope.** No Unicorn differential, no DOSBox-X run, no mutation run: original and native execution of this
body are a separately authorised review, and the comparator stays private until the body is accepted. The post-16EC
decrement, `TRANSFER`, StrLoad's own `ARGUMENT` and `+001F`'s `CODE`/`TRANSFER` are not reached by any test world.

```sh
ctest --test-dir <build> -R poolrad_overlay_u38_10de_cpu --output-on-failure
python3 tools/overlay/emit_ovl038_10de.py --check
PYTHONPATH=tools/overlay python3 tools/overlay/ovl038_10de_cpu_audit.py --compile-only
PYTHONPATH=tools/overlay python3 tools/overlay/ovl038_10de_cpu_audit.py --output resources/u38-10de/report.json
```

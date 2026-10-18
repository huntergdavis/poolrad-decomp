# U1C +00F4: raw-CPU ECL GOTO composing U25 +0000 and a public raw-CPU U25 +06AF

`pr_u1c_goto_00f4_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x1C` routine `+00F4`, the ECL opcode `0x01` GOTO handler
([ECL_CONTRACTS.md](ECL_CONTRACTS.md)). It is the raw-CPU counterpart of
the provisional wrapper [`pr_u1c_goto_00f4`](U1C_GOTO_FULLSTATE_00F4.md),
which takes callee effects from an external callback. This model runs the
merged callee models instead and leaves that file unchanged.

The VP assigned it to this lane on 2026-10-17 (i17678), after Astra
cleared it, together with a small enabling step:
`pr_u25_word_06af` is a public raw-CPU entry for U25 `+06AF` (public
entry 9, stub `002B:004D`). U25 `+06AF`'s identity is already strict, but
only through the semantic `ecl_helpers.h` API. The merged U25 loader
already transcribes `+06AF` as a raw-CPU body for its tag-81h path, so the
new entry simply exposes that body with an IP and argument check. It adds
no credit. The classifier lists `+00F4` as `game_verified`, grade B, so
this is a strict-credit upgrade with no new identity row. Strict credit is
the verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; expanded image at load segment 0 places the overlay stub table at `002B` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U1C `+00F4` | GAME file `0x0E3A..0x0E56`; **29 bytes, 13 instructions, no branches**; plain `RETF`; SHA-256 `da02d8926de5945de103e7c90ee1ba9856c645e82f531193ba40eb0f28a2779b` |
| Far calls | `+00FA` `LCALL 002B:002A` (stub `EA 0000:U25`, U25 `+0000`); `+0107` `LCALL 002B:004D` (stub `EA 06AF:U25`, U25 `+06AF`); both segment words are fixup-backed |
| U25 `+06AF` | `[06AF,06D3)`, 36 bytes, 16 instructions; SHA-256 `53565df24104754f01dff68b10cf0dda3fe72ad394a5253c611ceb594d93e862` ([U25_LOADER_0000.md](U25_LOADER_0000.md)) |

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP and
`[SP+2]` the return CS. It returns with plain `RETF`.

1. `PUSH BP; MOV BP,SP`.
2. `MOV AL,1; PUSH AX` and the load stub run U25 `+0000`, which loads one
   ECL operand: the tag at `DS:7006`, the value byte at `DS:7086` and, for
   tags 1–3 and 81h, the third byte at `DS:7046`.
3. `MOV AL,[7086h]; PUSH AX; MOV AL,[7046h]; PUSH AX` and the word stub run
   U25 `+06AF`, which returns AX := (`DS:[7046h]` << 8) + `DS:[7086h]`.
   A tag-0 operand leaves `DS:7046` as it was.
4. `MOV [49EDh],AX` sets the script PC to that word. U25 `+0000`'s own
   PC increment is overwritten. Then `POP BP; RETF`.

The pushed words keep AH from the previous AX: the entry AH for the count,
and U25 `+0000`'s returned AH for the value bytes. CX ends with CL = 8 and
DX as the low byte, both from `+06AF`. Every operand is reread from RAM in
source order.

**Stops.**

- `CODE`: a reached `LCALL` whose opcode or offset word differs, or whose
  stub is not `EA` to the expected entry, stops before it with IP at the
  site. The sites move with the entry IP.
- `CALLEE`: a callee model stopped. `*callee` names it and
  `*callee_status` gives its status, for example U25 `+0000`'s rejection of
  an entry SP outside its `[022A,FFFA]` contract.
- `TRANSFER`: a callee's `RETF` popped a CS:IP other than the call's next
  instruction. The audit produces one with DS = SS, where U25 `+0000`'s
  operand write lands on its own return IP.

**Flags.** The routine writes no flags of its own. The final flags are
`+06AF`'s `ADD AX,DX`: CF, AF and OF are clear, and PF/ZF/SF come from the
word. At a `CODE` stop the flags are the entry flags at the first site, and
U25 `+0000`'s at the second. Offsets wrap at 16 bits and physical addresses
at 1 MiB. `POP` and this routine's `RETF` frame are read physically
contiguous. `+06AF` keeps the loader's per-word stack profile, and each
callee keeps its own contract. Null RAM, CPU or limits, or RAM that is not
exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u1c_goto_00f4_cpu.c` runs under CTest with
ASan/UBSan, and this file compiles with `-Wconversion -Wsign-conversion
-Werror`. It uses no game bytes: only the two `LCALL` sites, the two
loaded-stub `EA` jumps and one synthetic operand. An independent reference
predicts the new PC, AX, DX, CL, the operand cells, the pushed bytes,
`+06AF`'s frame and the final flags. It passes 4,838 checks:

- 602 jumps over tags 0–3 and 10h, including target 0 (ZF) and a set high
  bit (SF);
- `CODE` stops at both sites and both stubs;
- U25 `+0000`'s SP-contract `CALLEE`;
- a relocated entry;
- `pr_u25_word_06af` on its own, including its IP and argument guards;
- the argument guards.

**Unicorn differential.** `tools/overlay/u1c_goto_00f4_cpu_audit.py` places
the expanded START.EXE image at load segment 0. It patches the U25 stub
entries `002B:002A` and `002B:004D` to their loaded `EA` jumps and loads
units `0x25` and `0x1C`, so U25 runs live from original bytes. It compares
the entire 1 MiB RAM and all 14 CPU words with no mask. Site and stub bytes
are read live at each site fetch.

| Stops | Cases |
| --- | --- |
| Returns | 174 |
| `CODE` | 4 |
| `TRANSFER` | 1 |

Coverage is 13/13 instruction starts; the routine has no branches. The
cases cover:

- 160 random operands over tags 0, 1, 2, 3, 81h, 10h and 55h with stale
  operand cells;
- each tag once more;
- stacks near both segment ends inside U25 `+0000`'s SP contract;
- SS and DS across A20 and a random background;
- the operand write on U25 `+0000`'s own return IP, plus a control
  writing the same byte, which returns normally;
- four rewritten sites or stubs.

A one-operand GOTO cannot exceed U25 `+0000`'s operand budget. The
audit's only `CALLEE` source would be the budget, so the C test covers
`CALLEE` instead.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **64
cases** with no mask.

- The COM copies unit `0x1C` to paragraph `8800` and unit `0x25` to
  `7800`. It writes the loaded stub jumps at `7F00:002A/004D` and relocates
  both site segment words.
- The cases use tags 0–3, 10h and 55h, which make no SYSTEM string calls.
- Each case compares all 14 CPU words, a 256-byte stack window and five
  data bytes: the new PC and the operand cells.
- The result SHA-256 is
  `62bd638ddb91d89c175dc1d9ef2cef0f59490e6d17dfee064fbd081a938256d3`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u1c_goto_00f4_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u1c_goto_00f4_cpu_audit.py --unicorn --native --output resources/u1c-00f4-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U1C `+00F4..+0110` (ECL GOTO), composing U25 `+0000` and `+06AF` through the loaded stub |
| `source_candidates` | `src/recovered/overlay_u1c_goto_00f4_cpu.c` with `src/recovered/overlay_u25_loader_0000.c` (now also exposing `pr_u25_word_06af`) and `src/recovered/overlay_u25_string_cores.c` |
| `signals` | Exact hash, whole-RAM/CPU differential with U25 live and full instruction coverage, every reachable stop kind, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `game_verified` (ECL opcode 01h): a strict-credit upgrade only. The new U25 `+06AF` entry adds no credit. The provisional wrapper stays as it is. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.

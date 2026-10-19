# U9E +0E52 (entry 10): raw-CPU list append composing GetMem

`pr_u9e_list_append_0e52_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x9E` routine `+0E52`, public entry 10 (stub `00B0:0052`). It
appends a 9-byte node, allocated with SYSTEM GetMem, to a singly linked list
whose far head sits at `+7Fh` of a caller-supplied record. The VP approved it
on 2026-10-18 (i17863).

## Why this target

On current main, GetMem `0AF8:023F` is in `RUNTIME_COVERED_ENDPOINTS`, so
GetMem is no longer an unready callee and `+0E52` is composable.

The PR #991 triage table lists `OVL_009E_0E52` as the only untranslated
callee of 9 rows (5 game_evidenced) and as a callee in 25 rows. In the
2026-10-18 03:06 PDT queue snapshot it has:

- one aligned `PUSH CS` caller, U9E `+1656` (itself a callee of U86 `+08C3`);
- 38 fixup-backed call sites in U38 (18 bodies), U6C `+056C` and U86 (5 bodies).

## Exact original

| Item | Value |
| --- | --- |
| Span | unit `0x9E` `+0E52..+0F06` (exclusive upper bound), GAME.OVR file offset `0x2BD3C` |
| Size | 180 bytes, 57 instructions |
| SHA-256 | `cfde5096f098f1f0c4aa15f805eb6a02c3540f56a2f0df8632b99501a6366b20` |
| Calls | one `LCALL 0AF8:023F` at `+0E5C` (fixup at `+0E5F`) |
| Exit | `RETF 0Ch` at `+0F03` |

GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; START.EXE
is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

## ABI and effects

The routine is entered after a far call:

- `SS:[SP]` and `[SP+2]` hold the return IP and CS.
- The caller then pushed P (segment, then offset) and the words b, a, c, d.
  At the entry, d is at `[SP+4]`, c at `[SP+6]`, a at `[SP+8]`, b at
  `[SP+0Ah]` and P at `[SP+0Ch]`/`[SP+0Eh]`.

After `PUSH BP; MOV BP,SP; SUB SP,8`:

1. **Allocate.** `node := GetMem(9)`, kept at `[BP-4]`/`[BP-2]`.
2. **Link.**
   - If the head `P+7Fh`/`P+81h` is nil, head := node.
   - Otherwise a cursor at `[BP-8]`/`[BP-6]` starts at the head and follows
     each node's link (`+5`/`+7`) while it is not nil. The last node's link
     := node.
3. **Fill.** The node's link := nil. Then, in this order: byte `+0` := b,
   word `+1` := a, byte `+4` := d, byte `+3` := c. The bytes are the low
   bytes of the pushed words.
4. **Return.** `MOV SP,BP; POP BP; RETF 0Ch`.

The node layout is byte, word, byte, byte, far link. Which game list this is
(for example, a per-character effect list) is not established here.

**GetMem.** GetMem is the merged `pr_system_getmem_023f`. Its segment is
read from the live `LCALL` operand, and `max_heap_steps` caps its free-list
walk. A failed allocation stops at RunError `0AF8:00E2` with AX = 203, which
this routine reports as `CALLEE`, with GetMem's status in `*getmem_status`.

**Rereads.** Every operand is reread from RAM in source order; each `LES`
reloads its pointer. The record, the list, the node and the frame may
therefore alias.

**Registers at return:**

- AX = `(a & FF00h) | (c & FFh)` as last loaded.
- DX and ES:DI hold the node, as last loaded from `[BP-2]` and `[BP-4]`.
- BX, CX and SI are GetMem's.
- The arithmetic flags are `XOR AX,AX`'s (ZF = PF = 1, the rest 0); the other
  flags are GetMem's.

**Statuses:**

| Status | When | IP |
| --- | --- | --- |
| `OK` | the routine's own `RETF 0Ch` executed | the return address |
| `ARGUMENT` | null RAM or CPU, RAM not exactly 1 MiB, or IP not `0E52h`; nothing changes | unchanged |
| `CODE` | the `LCALL` bytes at `+0E5C` are not `9A 3F 02`; the size word is already pushed and the flags are `SUB SP,8`'s | `+0E5C` |
| `CALLEE` | GetMem stopped: RunError, a custom HeapError, or its walk cap | GetMem's stop |
| `TRANSFER` | GetMem's `RETF 2` popped another CS:IP; an unexercised guard | the popped CS:IP |
| `LIMIT` | the tail walk would follow link `max_links + 1`, a host cap on the original's unbounded loop | `+0EA8`, with `OR AX,ES:[DI+7]`'s flags |

Offsets wrap at 16 bits and physical addresses at 1 MiB. Word operands,
`LES` pairs, `POP` and the `RETF` frame are read physically contiguous.

## Validation

**Focused C test.** `test_overlay_u9e_list_append_0e52_cpu.c` runs under
CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. It passes **47** hand-predicted checks:

- an empty list, with the record head, locals, GetMem frame, every register
  and the flags;
- a three-node append;
- a two-node cycle stopping at `LIMIT` after three links;
- a full heap ending in RunError 203;
- two `CODE` stops, one with the locals wrapped below 0 so that `SUB SP,8`
  borrows (CF, PF, AF, SF);
- a record whose head offset wraps;
- the guards.

**Unicorn differential.** `tools/overlay/u9e_list_append_0e52_cpu_audit.py`
runs the original unit bytes, and GetMem from the expanded START.EXE image,
under Unicorn. After each stop it compares the entire 1 MiB RAM and all
14 CPU words with **no mask**.

- **Cases: 185.** 171 return, 3 `CODE`, 8 `CALLEE` (RunError and a custom
  HeapError), 3 `LIMIT`.
- **Coverage.** All 57 instruction starts and all 4 conditional outcomes.
- **Inputs.**
  - Lists of 0–40 nodes, including cyclic lists and an exact link cap.
  - Records and nodes whose offsets wrap past `FFFF`.
  - Random free lists, full heaps and a custom HeapError.
  - Patched `LCALL` bytes.
  - Frames at SP `0100h`, `FFE0h` and `FFF4h` (arguments wrapping), and
    SP `0006h` (locals wrapping).
  - SS across A20, DF/IF set, and a random background.
- **Separation.** Every case keeps the stack frame apart from the heap
  arena, the record and the list.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **64 cases**
with no mask.

- **Loading.** The COM copies the SYSTEM span `0000..04FF` (GetMem, its
  default HeapError and the RunError entry) to paragraph `7000`. It copies
  unit `0x9E` to `8800`, relocating the `LCALL`, and patches a far `JMP` trap
  over RunError.
- **Each case.** It rebuilds a heap and a list of up to 7 nodes in the data
  segment, then calls the entry with IRQs masked.
- **Comparison.** Each case compares all 14 CPU words, the 256-byte stack
  window, the whole 64 KiB data segment and the 16 KiB heap arena.
- **Results.** 46 cases return and 18 end in RunError; 48 had a non-empty
  list. The result SHA-256 is
  `710447bdde00f0d0589c8498442dade009b872868e83d28c7f5e9fe8e1769067`, with COM
  segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u9e_list_append_0e52_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u9e_list_append_0e52_cpu_audit.py --unicorn --native --output resources/u9e-0e52/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | GAME.OVR unit `0x9E` `+0E52` (entry 10), list append via GetMem |
| `source_candidates` | `src/recovered/overlay_u9e_list_append_0e52_cpu.c` |
| `signals` | Exact span hash; whole-RAM/CPU differential with no mask and every instruction and outcome; composed merged GetMem; an independent C test; native synthetic execution |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `TRANSFER` is an unexercised guard; `LIMIT` is a host cap on an unbounded original loop |

No classifier, queue or count ledger is changed.

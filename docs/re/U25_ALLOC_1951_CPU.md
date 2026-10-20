# U25 +1951: raw-CPU list push composing GetMem and U25 +06AF

`pr_u25_alloc_1951_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x25` routine `+1951`. It returns the whole CPU14 and 1 MiB
RAM through the routine's own `RETF 2`, and it composes two callees:

- SYSTEM GetMem (`0AF8:023F`), the raw-CPU [GetMem model](SYSTEM_GETMEM_023F.md)
  from PR #983;
- U25 `+06AF`, the merged entry `pr_u25_word_06af` from PR #963.

The routine pushes a 6-byte node onto a list and updates a word. The VP
assigned it on 2026-10-18 (i17796), stacked on #983. GetMem was its only
unready callee. The meaning of the list and of the word is not asserted.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U25 `+1951` | GAME file `0x8E24..0x8EAD`; **138 bytes, 47 instructions, one conditional branch**; `RETF 2` at `+19D8`; SHA-256 `f7f3ac99c3229d1d1e86c7ee4668164f201b40c9853431e442d6ad4fd4593008` |
| Calls | `+1961` and `+198F LCALL 0AF8:023F` (fixup-backed); `+19D0 PUSH CS; CALL +06AF` (its `RETF 4` at the end of `[06AF,06D3)`) |
| GetMem | the four SYSTEM spans pinned in the [GetMem document](SYSTEM_GETMEM_023F.md) |

## ABI and ordered effects

The routine is entered after a far call, or after a near `CALL` preceded by
`PUSH CS`. `SS:[SP]` holds the return IP, `[SP+2]` the return CS and the
low byte of `[SP+4]` the index. After `PUSH BP; MOV BP,SP`:

1. If the far head `DS:6F70h`/`6F72h` is nil:
   - head := GetMem(6);
   - the node's link (`+2`/`+4`) := nil.

   Otherwise:
   - the old head is kept at `DS:6F74h`/`6F76h`;
   - head := GetMem(6);
   - the node's link := the old head.
2. The node's word `+0` := `DS:[49EDh]`.
3. `DS:[49EDh]` := `+06AF` of the bytes `DS:[7085h + index]` and
   `DS:[7045h + index]`. That value is (`7045h` byte << 8) + `7085h` byte.
4. `POP BP; RETF 2`.

**Callee contracts.**

- GetMem's segment is read from each site, and it runs with
  `max_heap_steps` for its free-list walk. It keeps its own contract: its
  default HeapError is `xxxx:00A9`, and a failed allocation stops at
  RunError (`00E2`) with AX = 203.
- `+06AF` must sit at its original offset, so the entry must be `+1951`.
- Every operand is reread from RAM in source order.

**Stops.**

- `CODE`: a reached GetMem `LCALL` is not `9A 3F 02`. The model stops at the
  site with only the size pushed.
- `CALLEE`: GetMem stopped, for example at RunError 203, at a custom
  HeapError, at its walk cap or at a return guard. `+06AF` rejecting its
  entry is also `CALLEE`. `*callee_status` holds the callee's status.
- `TRANSFER`: GetMem's `RETF 2` or `+06AF`'s `RETF 4` popped another
  return. This is a guard that no audit case exercises.
- `ARGUMENT`: null RAM or CPU, RAM that is not exactly 1 MiB, or IP not
  `1951h`. Nothing changes.

**Flags.** The final flags are `+06AF`'s, from its `ADD`. At a GetMem site
they are those of `OR AX,[6F72h]`. After a callee stop they are the
callee's.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs, `POP`
and this routine's `RETF` frame are read physically contiguous.

**Stated limit (reviewer-added 2026-10-20, VP i19691): entry SP 0006/0007 and FFFE/FFFF.** The model has no entry-SP guard: ARGUMENT checks only the RAM, CPU, size and IP, so these SPs lie inside its stated contract. (1) It calls SYSTEM GetMem with GetMem's entry SP = E−8, so an entry SP of 0006/0007 enters GetMem at FFFE/FFFF, where GetMem's `RETF 2` frame is read contiguous. (2) Its own `POP BP; RETF 2` at `19D7` reads its frame contiguous, which matters at entry SP FFFE/FFFF because `PUSH BP`/`POP BP` are balanced. In both cases DOSBox-X normal/386 (binary `99d74601…`) wraps the return CS within SS, while the model and Unicorn read `SS*16+10000h`. Measured with a three-oracle probe ([`tools/overlay/retf_stack_edge_probe.py`](../../tools/overlay/retf_stack_edge_probe.py)) that gives the wrapped and contiguous frame copies different return CS values. At SP FFFD, where the word straddles FFFF, all three oracles read it contiguous, and at SP ≤ FFFC they agree. **Unicorn 2.1.4 shares the model's contiguous reading, which is why the original-byte differential could not have caught this.** **Never exercised by this row's evidence**: the audited SPs are 0100, FFE0, 0400 and 8000. The credit stands within what was measured; behaviour at these four entry SPs is unverified.

**Reachability (reviewer-added 2026-10-20, VP i19720): the band is unreachable under the game's stack.** The EXEPACK header in START.EXE (signature `RB`) restores the game's stack to SS:SP = `14F2:4000`: a 16 KiB Turbo Pascal stack that starts even and grows down. (The MZ header's `111A:0080` belongs to the packer stub.) A static sweep of the unpacked START image and of GAME.OVR finds no `MOV SS` or `LSS`. Every `POP SS` hit is a 0x17 data byte in a word table or a length-prefixed string. Entry SP 0006/0007 needs 16,378 of the 16,384 stack bytes already in use, and FFFE/FFFF needs SP already wrapped below 0000. Both conditions already corrupt memory above the stack. In this band the composer's own LCALL to GetMem already writes its return frame across the wrap: return CS at SS:0000 and return IP at SS:FFFE (0001/FFFF at an odd entry SP). Every later push or interrupt runs at SP FFFx, beginning with GetMem's PUSH BP at SS:FFFC. That is about 48 KiB above the stack top at SS:4000, where Turbo Pascal places the heap. So in this band the original program is no longer executing correctly, and a wrapped and a contiguous model differ only in how a crashing program crashes. The "never exercised" sentence above stays: it records what was audited, and this records that nothing reachable remains to audit. **This claim relies on** (1) the `14F2:4000` stack: 16 KiB, even, growing down; (2) no in-game stack switch, which is supported by a static sweep but not proven at runtime; and (3) no ISR or driver running this body on a private stack. It would be wrong if any of these fails. Odd SPs additionally need an odd stack adjustment, which was not audited. Heap occupancy at SS:FFFx was not inspected.


## Validation

**Focused C test.** `test_overlay_u25_alloc_1951_cpu.c` runs under CTest
with ASan/UBSan. The source compiles with `-Wconversion -Wsign-conversion
-Wshadow -Werror`. The test places only the two GetMem `LCALL` encodings,
plus a synthetic heap, list head and tables. It passes 28 checks predicted
by hand:

- the first node and a push onto an existing head: the node's word and
  link, the head, the old-head scratch, HeapPtr's growth, `49EDh` and the
  return;
- GetMem's RunError (state at `00E2`) and a custom HeapError, both as
  `CALLEE`;
- a rewritten site as `CODE`;
- the guards.

**Unicorn differential.** `tools/overlay/u25_alloc_1951_cpu_audit.py` works
as follows:

- It runs GetMem from its original bytes in the expanded START.EXE image,
  and loads unit `0x25`.
- It pre-decodes `+1951`, `+06AF` and the GetMem spans. Any other fetch is
  an error.
- It compares the entire 1 MiB RAM and all 14 CPU words with **no mask**.

| Stops | Cases |
| --- | --- |
| Returns | 166 |
| `CODE` | 2 |
| `CALLEE` (GetMem RunError, custom HeapError, walk cap) | 6 |

Coverage is 47/47 instruction starts and 2/2 conditional outcomes. The
cases cover:

- 160 random heaps, heads, indexes, stacks and flags;
- an empty list, with and without a fitting block, and a push onto a head;
- a full heap in both branches;
- a custom HeapError and a cyclic free list;
- both sites rewritten;
- both stack bounds, SS across A20, a random background, and preserved
  DF/IF.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **64
cases** with no mask: 55 returns and 9 GetMem RunError 203.

- **Loading.** The COM copies unit `0x25` to `8800`, with both GetMem sites
  relocated to `7000`. It copies the SYSTEM span `0000..04FF` to `7000`,
  with RunError's entry replaced by a recorder.
- **Each case.** The COM rebuilds DS and a 16 KiB heap arena, then calls
  `+1951` with IRQs masked.
- **Comparison.** Each case compares all 14 CPU words, a 256-byte stack
  window, the whole 64 KiB DS segment and the arena.
- **Result.** The result SHA-256 is
  `10715d2a1782d28413301bc24773971db438d62ce591fb1b638161b99a413467`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u25_alloc_1951_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u25_alloc_1951_cpu_audit.py --unicorn --native --output resources/u25-1951-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U25 `+1951..+19DA`, composing SYSTEM GetMem `023F` and U25 `+06AF` |
| `source_candidates` | `src/recovered/overlay_u25_alloc_1951_cpu.c` with `system_getmem_023f_cpu.c` (#983) and the merged U25 loader and string-core sources |
| `signals` | Exact hash and CFG; a whole-RAM/CPU differential with GetMem and `+06AF` live and no mask; full instruction and branch-outcome coverage; every reachable stop kind; an independent C test; native synthetic execution including GetMem's RunError |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Identity and strict credit are the verifying lane's call. This PR stacks on #983 (GetMem). Field meanings and natural saved-route reach are not asserted |

No classifier, queue or count ledger is changed.

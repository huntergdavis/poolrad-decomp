# SYSTEM GetMem 0AF8:023F: raw-CPU Turbo Pascal 6 heap allocation

`pr_system_getmem_023f` is a raw-CPU, full-state model of Turbo Pascal 6
GetMem in the START.EXE SYSTEM segment (`0AF8` at load segment 0). The queue
marks it `runtime:575`, and it is a runtime enabler for game routines such
as U25 `+1951`, U6A `+0E37` and U86 `+028D`. The VP approved it on
2026-10-18 (i17787), including the error-203 path.

The model covers the entry `023F..0253`, its near core `0308..03D2`, the
size rounding `0491..04A1` and the default HeapError `00A9..00AD`. It stops
at RunError's entry `00E2`, which belongs to a separate routine.

## Exact original

| Span (SYSTEM offsets) | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
| `023F..0253` entry and error-203 tail | 21 / 9 | `d4ceab56ec05612e8ba6c90dab11d9da62f1f38235503781f80d636daa63d346` |
| `0308..03D2` core | 203 / 88 | `03e08d678db922d962aafbd79b301ca90556775acef11b832fecbeb29fe0dff0` |
| `0491..04A1` rounding | 17 / 8 | `440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d` |
| `00A9..00AD` default HeapError | 5 / 2 | `702a44410b4b3c0c9df438b32a641b3203e9046ef66052fef66163bebdfa12b7` |

START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
SYSTEM initialization at `0037`/`003D` installs HeapError := `CS:00A9`.
Those are the only writes to `DS:4414h` in START.EXE. A raw scan of the
GAME.OVR unit code finds no other installer.

## ABI and ordered effects

GetMem is entered after a far call. `SS:[SP]` holds the return IP, `[SP+2]`
the return CS and `[SP+4]` the size. The heap variables are DS words:

| Variable | DS offsets |
| --- | --- |
| HeapPtr | `4404h`/`4406h` |
| HeapEnd | `4408h`/`440Ah` |
| FreeList | `440Ch`/`440Eh` |
| HeapError | `4414h`/`4416h` |
| saved size | `8754h` |

The routine runs as follows:

1. A zero size returns nil: `XOR AX,AX; CWD`.
2. Otherwise the size is saved and rounded up to whole 8-byte granules. DX
   gets the paragraphs, (size + 7) >> 4, with the 17th bit carried through
   `RCR`. AX gets (size + 7) & 8.
3. The walk starts from the link FreeList. Each normalized free block holds
   its next pointer at `+0`, its size paragraphs at `+6` and its granule at
   `+4`. The walk stops at the first block whose segment equals HeapPtr's,
   which is the end of the list. The first block at least as large is
   taken:
   - on an exact fit, its link is unlinked;
   - otherwise the normalized remainder becomes a free block in its place.

   DX:AX := the block.
4. At the list's end the block is HeapPtr itself, provided HeapPtr + size
   (normalized) does not pass HeapEnd. HeapPtr and the last link become the
   new top, and HeapError is called with 0.
5. Otherwise HeapError is called with the size. AL = 0 returns with CF
   set, and GetMem's `JB` goes to `MOV AX,0CBh; JMP 00E2`, which is
   RunError 203.

The default HeapError (`00A9`: `XOR AX,AX; RETF 2`) is folded when
`DS:4414h`/`4416h` equal `00A9h` and GetMem's CS. It returns AL = 0, so the
failure path always ends in RunError. The `AL = 1` (nil) and `AL ≥ 2`
(retry) branches need another HeapError, and the model stops at its call.

All near calls and returns use the live stack, and every operand is reread
in source order. The code must sit at its original offsets, so IP = `023Fh`.

**Stops.**

| Status | Meaning |
| --- | --- |
| `RETURNED` | `RETF 2` executed; DX:AX is the block, or nil |
| `RUNERROR` | AX = 203 at `CS:00E2`, RunError's entry, with the caller's far return frame on top of the stack, as RunError pops it |
| `HEAP_ERROR` | the HeapError pointer is not the default; stopped at the indirect `LCALL` (`+03B7` or `+03C3`) with its argument pushed |
| `TRANSFER` | a near `RET` or the default HeapError's `RETF 2` popped another return address. This is a guard; no audit case exercises it |
| `LIMIT` | the host cap on the free-list walk's taken `JA +0319`. A cyclic free list walks forever, so the cap stops at `+0319` |
| `ARGUMENT` | null RAM or CPU, RAM that is not exactly 1 MiB, or IP not `023Fh`. Nothing changes |

**Flags.** On return, the flags come from the path's last writer, with CF
clear:

- `XOR AX,AX` for nil;
- the equal `CMP` for an exact fit;
- `AND AL,0Fh` for a split;
- the default HeapError's `XOR` at the heap top.

At RunError they are those of `CMP AL,1` with AL = 0. At a stop they are
those the original leaves there. `OR`, `XOR` and `AND` leave AF clear, as
both oracles do.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs, `POP`
and every `RET`/`RETF` frame are read physically contiguous.

## Validation

**Focused C test.** `test_system_getmem_023f_cpu.c` runs under CTest with
ASan/UBSan. The source compiles with `-Wconversion -Wsign-conversion
-Wshadow -Werror`. It passes 48 checks over synthetic heaps, predicted by
hand:

- nil;
- exact fits on the first and the second block;
- a split, with the normalized remainder's fields;
- heap-top growth, with HeapPtr and the last link advanced;
- the rounding of 1 and 65,535 bytes;
- the RunError state: AX, IP, SP, BP and flags;
- both HeapError stops;
- the walk cap on a self-linked block;
- the guards.

**Unicorn differential.** `tools/overlay/system_getmem_023f_audit.py` runs
GetMem from its original bytes at `0AF8` in the expanded START.EXE image. It
compares the entire 1 MiB RAM and all 14 CPU words with **no mask**.

| Stops | Cases |
| --- | --- |
| Returns | 214 |
| `RUNERROR` | 45 |
| `HEAP_ERROR` | 2 |
| `LIMIT` | 2 |

By path, the returns and stops split as 18 nil, 37 exact, 132 split, 28
heap top, 46 failed and 2 cyclic.

Coverage is 104/107 instruction starts and 21/24 conditional outcomes. The
audit asserts that the only gaps are `+03CB`, `+03CD` and `+03D0` with their
outcomes, which are the `AL = 1` and `AL ≥ 2` HeapError results that the
default never returns. The cases cover:

- 240 random free lists with sizes from 0 to 65,535, including block-exact
  and near-exact sizes;
- an empty list, an exact HeapEnd and a granule HeapEnd;
- a full heap, the maximum size, and custom HeapError pointers;
- a cyclic list, and an exact cap with one short;
- stacks at both segment ends, SS across A20, and a random background;
- preserved DF/IF.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **64
cases** with no mask: 53 returns and 11 RunError 203.

- **Loading.** The COM copies the SYSTEM span `0000..04FF` to paragraph
  `7000`. RunError's entry `00E2` is replaced by a far jump to a recorder,
  so the error path is captured exactly at `7000:00E2`.
- **Each case.** The COM rebuilds the DS segment and a 16 KiB heap arena at
  `6000`, then calls GetMem with IRQs masked.
- **Comparison.** Each case compares all 14 CPU words, a 256-byte stack
  window, the whole 64 KiB DS segment and the arena.
- **Result.** The result SHA-256 is
  `326eb772e800c7e507323b8d5b7d245954230e4e6c2c8f98c848871a4499367b`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_getmem_023f_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_getmem_023f_audit.py --unicorn --native --output resources/system-getmem/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | SYSTEM `0AF8:023F` GetMem with `0308`, `0491` and the default HeapError `00A9` |
| `source_candidates` | `src/recovered/system_getmem_023f_cpu.c` |
| `signals` | Exact span hashes; a whole-RAM/CPU differential with no mask; coverage of everything except the two branches the default HeapError cannot take; every reachable stop kind; an independent C test; native synthetic execution including RunError 203 |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | This is a runtime (library) model; how it counts toward game credit is the verifying lane's call. A custom HeapError stops at its call. Natural saved-route reach is unmeasured |

No classifier, queue or count ledger is changed.

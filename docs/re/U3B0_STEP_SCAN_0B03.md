# U3B0 +0B03: raw-CPU step scan composing the strict U3B0 +0000

`pr_u3b0_step_scan_0b03_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x3B0` routine `+0B03`. The VP assigned it to this lane on
2026-10-17, after Astra confirmed they do not own it and have no in-flight
work on it. In the census-2115 queue its only callee is U3B0 `+0000`, which
is strict, with no inline coverage, model, branch or main file for `+0B03`.
Both near calls run the merged raw-CPU `pr_u3b0_entry1_0000`. The classifier
lists the routine as `game_or_other_library_unresolved`, grade B. Identity
and strict credit are the verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U3B0 `+0B03` | GAME file `0x2D3D2..0x2D501`; **304 bytes, 103 instructions, nine conditional branches**; `RETF 4`; SHA-256 `6f797c1ec66c90e89812e7154faa6417e1d895ed1311754ed8b9c0895b4120f2` |
| Calls | `+0B39` and `+0BC4`, each `PUSH CS; CALL +0000` |
| Callers | U3B0 `+0C33` (site `+0CB1`) and `+0CC5` (site `+0D7F`), through `PUSH CS; CALL` |

## ABI and ordered effects

The routine is a nested Pascal procedure. It is entered after a far call
with `SS:[SP]` the return IP, `[SP+2]` the return CS, `[SP+4]` the static
link L (the parent's BP) and `[SP+6]` a byte d. It returns with `RETF 4`.
It reads the parent's frame through `SS:[L+disp]`:

| Parent slot | Meaning in this code |
| --- | --- |
| `L-59h` | a limit word |
| `L-56h` | a second bound word |
| `L+0Ah/L+0Ch` | a far pointer X to an index word |
| `L+14h/L+16h` | a list head far pointer |

After `PUSH BP; MOV BP,SP; SUB SP,2`, the count c := 0 in `[BP-2]`. Then
while c < limit (signed):

1. node := `+0000`(head, X^), the node X^ steps along the list or nil,
   returned in DX:AX.
2. Stop when node^[29h] = 0. A nil node reads its flag from `0000:0029`.
3. c := c + 1.
4. When d <> 0: X^ += 1. If `DS:[6B29]` + limit − 1 < X^, X^ :=
   `DS:[6B29]`. If L[−56h] − 1 < X^, X^ := `DS:[6B29]`.
5. When d = 0: X^ −= 1. If X^ < `DS:[6B29]`, X^ := `DS:[6B29]` + limit − 1.
   If L[−56h] − 1 < X^, X^ := L[−56h] − 1.

AX ends as c when the limit ends the walk, or as the node's offset when its
flag does. Every operand is reread from RAM in source order. An index store
that aliases the counter, the limit or the parent's frame is seen by later
reads.

**Stops.** The model returns one of three stop statuses:

- `LIMIT`: aliasing can make the original loop forever, so
  `max_loopbacks` caps the taken `JMP +0B17` and `JMP +0B9F`. The next one
  stops at that loop head with every earlier effect kept.
- `CALLEE`: `+0000`'s model rejects an entry IP other than 0. The near call
  reaches entry IP − 0B03h, so this happens only when the routine is entered
  away from its own offset.
- `TRANSFER`: a guard for `+0000` returning elsewhere. It cannot trigger,
  because `+0000` writes only its locals below its return frame.

**Flags.** The final flags are the last `CMP`'s. The `INC`, `DEC`, `ADD`
and `XOR` writes are overwritten before any stop and are folded.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs, `POP`
and this routine's `RETF` frame are read physically contiguous. `+0000`
keeps its own contract, whose `RETF` pops wrap per word. Null RAM or CPU, or
RAM that is not exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u3b0_step_scan_0b03_cpu.c` runs under
CTest with ASan/UBSan and compiles with `-Wconversion -Werror`. It uses no
game bytes. An independent reference of both walks, including `+0000`'s
nil result, predicts the index, count, AX and every byte outside the stack
window. It passes 7,510 checks:

- 1,500 seeded worlds over list lengths, directions, limits, indices,
  bounds and flags;
- a `LIMIT` stop at the third loop-back;
- a `CALLEE` stop from a relocated entry;
- the argument guards.

**Unicorn differential.** `tools/overlay/u3b0_step_scan_0b03_audit.py`
loads unit `0x3B0`, so `+0000` runs from its original bytes. It compares
the entire 1 MiB RAM and all 14 CPU words with no mask. The original's
behaviour decides each stop. For `CALLEE`, the original is compared at the
callee's first fetch.

| Stops | Cases |
| --- | --- |
| Returns | 185 |
| `CALLEE` | 1 |
| `LIMIT` | 4 |

Coverage is 103/103 instruction starts, 18/18 conditional outcomes and
`+0000` 38/38. The cases cover:

- 160 random worlds;
- walks clamped at both bounds in both directions;
- a nil head and a cyclic list;
- X^ on the counter and on the parent's limit;
- the parent's words under `+0000`'s frame, where `+0000` clobbers them;
- small caps;
- stacks at both segment ends, SS and nodes across A20, a random
  background and a relocated entry.

Segment-end stacks keep `+0000`'s `RETF` frame off the boundary, because
the two oracles' `RETF` profiles differ there.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **80
cases** with no mask. In 26 of them the index starts off the list, so
`+0000` returns nil and the flag comes from `0000:0029`. The harness records
the low 64 bytes of memory once for the replay. Each case compares all 14
CPU words, a 128-byte stack window that includes the parent frame, the index
word and `DS:6B29`. The COM copies unit `0x3B0` to paragraph `8800`; there
are no far calls to relocate. The result SHA-256 is
`26f8926b8bf452ffd024d1a5e65536b50fd25d6138991d33a0c2acdc866668b1` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u3b0_step_scan_0b03_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u3b0_step_scan_0b03_audit.py --unicorn --native --output resources/u3b0-0b03-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U3B0 `+0B03..+0C32`, composing the strict U3B0 `+0000` twice |
| `source_candidates` | `src/recovered/overlay_u3b0_step_scan_0b03_cpu.c` with `src/recovered/overlay_u3b0_entry1_0000.c` |
| `signals` | Exact hash and CFG, whole-RAM/CPU differential with `+0000` live and full instruction and branch-outcome coverage, every reachable stop kind, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `game_or_other_library_unresolved`; identity is the verifying lane's call. The parent slots' game meaning is not asserted. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.

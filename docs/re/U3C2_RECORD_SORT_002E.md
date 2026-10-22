# U3C2 +002E: raw-CPU record sort composing SYSTEM Move

`pr_u3c2_record_sort_002e_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x3C2` routine `+002E`. The VP assigned it to this lane on
2026-10-17, after Astra confirmed they do not own it and have no in-flight
work on it. In the census-2115 queue its only callee is SYSTEM Move
(`runtime:2623`), with no inline coverage, model, branch or main file. Each
swap calls Move as a real far call into the raw-CPU
[`pr_system_move_0a3f`](SYSTEM_RUNTIME_LEAVES.md) from #936. The timeboxed
`deja` recall for the runtime leaves reached its cap, so nothing is reused.
The classifier lists the routine as `game_or_other_library_unresolved`,
grade B. Identity and strict credit are the verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U3C2 `+002E` | GAME file `0x2FB80..0x2FCF1`; **370 bytes, 158 instructions, nine conditional branches**; plain `RETF`; SHA-256 `81e890a18c92bd83b78b50a33c1295975183a2706ec5b03b52e7cf550544ca3f` |
| Calls | `+0131`, `+0160` and `+0181 LCALL 0AF8:0A3F` (Move); every segment word is fixup-backed |
| Caller | U3C2 `+0909`, site `+0BD0`, through `PUSH CS; CALL` |

## ABI and ordered effects

The routine takes no arguments and returns with a plain `RETF`. It sorts the
3-byte records R[k] = `DS:(6718h + 3k)`, k = 1..n, where n = `DS:[671Ah]`.
That count byte is also R[0]'s third byte. After `PUSH BP; MOV BP,SP;
SUB SP,0Ah`:

1. If n <= 1, return. `[BP-8]` := n − 1, and i in `[BP-1]` runs from 1 up to
   `[BP-8]`.
2. Each outer pass rereads `[BP-9]` := n. j in `[BP-2]` runs from the byte
   i + 1 up to `[BP-9]`. The pass is skipped when i + 1 exceeds `[BP-9]`.
3. For each pair, `[BP-4]` := R[i][2] and `[BP-3]` := R[j][2]. R[i] and
   R[j] swap when R[j][1] < R[i][1]. They also swap on a tie when
   R[j][2] < R[i][2] and R[j][2] mod 2 <= R[i][2] mod 2. The remainders
   come from `IDIV CX` with CX = 2, compared by a signed word `JG`.
4. A swap is three Moves of 3 bytes: R[i] to the temp at `SS:BP-7`, R[j] to
   R[i], and the temp to R[j].

Every operand is reread from RAM in source order, so an earlier record
store is seen by later reads. The loops compare byte counters for equality.
They terminate unless a store aliases those counters, which can happen only
when SS and DS overlap.

**Loop cap and stops.** `max_loopbacks` caps the taken `JMP +0072` and
`JMP +0056` loop-backs. The next one stops at that `INC`, entry IP + 44h or
+ 28h, with `LIMIT` and every earlier effect kept. A call site holding other
bytes returns `CODE`, with IP at the site and its arguments pushed. A Move
that returns anywhere but the next instruction returns `TRANSFER`. That
happens when a swapped record overwrites Move's own frame.

**Unreachable edge.** The `JBE` at `+004C` is always taken: `[BP-8]` was
just written as n − 1 with n > 1. So `JMP +019C` at `+004E` never runs.

**Flags.** The final flags are the last `CMP`'s, with DF clear once a Move
has run. The `INC`, `DEC`, `XOR`, `SHL`, `ADD` and `IDIV` writes are
overwritten before any read and are folded. `SUB SP,0Ah` and the
`ADD DI,6718h` before each call are computed, because a `CODE` stop exposes
them.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `POP` and the
`RETF` frames are read physically contiguous. Null RAM or CPU, or RAM that
is not exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u3c2_record_sort_002e_cpu.c` runs under
CTest with ASan/UBSan and compiles with `-Wconversion -Werror`. Its only
game bytes are the three five-byte far-call sites. An independent
byte-level sort predicts the final records, and all RAM outside the stack
frame must match it. It passes 1,092 checks:

- counts 0 to 12 with random, reversed, tied, many-tied and sorted keys;
- 60 counts up to 255;
- another entry offset;
- a `LIMIT` stop at the third loop-back;
- `CODE` stops at the first and second sites;
- the argument guards.

**Unicorn differential.** `tools/overlay/u3c2_record_sort_002e_audit.py`
places the expanded START.EXE image at load segment 0, so the original
Move runs from all three unrelocated far calls. It loads unit `0x3C2` and
compares the entire 1 MiB RAM and all 14 CPU words with no mask. The
original's behaviour decides each stop, and the C status must match.

| Stops | Cases |
| --- | --- |
| Returns | 120 |
| `CODE` | 3 |
| `TRANSFER` | 7 |
| `LIMIT` | 6 |

Coverage is 157/157 reachable instruction starts, 17/17 reachable
conditional outcomes and Move 9/9. The unreachable edge never occurs. The
cases cover:

- counts 0 to 12 in five key patterns, 40 random counts up to 89, and
  counts 254 and 255;
- SS = DS worlds where the temp rewrites the count byte and records reach
  the stack counters;
- records over Move's own frame;
- stacks at both segment ends, SS and DS across A20 and a random
  background;
- small caps and three rewritten sites.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **40
cases**, 31 of them with swaps, with no mask. Each compares all 14 CPU
words, a 64-byte stack window and the `0x300`-byte record table. The COM
copies unit `0x3C2` to paragraph `8800` and Move to `9000:0A3F`. It
relocates the three fixup-backed segment words to `9000`, loader-style. The
result SHA-256 is
`433ef06ab70fd6b4ca3b210920f5cffc9e52e7019cdeaeaf451d37abd11d606a` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u3c2_record_sort_002e_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u3c2_record_sort_002e_audit.py --unicorn --native --output resources/u3c2-002e-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U3C2 `+002E..+019F`, composing SYSTEM Move `0AF8:0A3F` three times |
| `source_candidates` | `src/recovered/overlay_u3c2_record_sort_002e_cpu.c` with `src/recovered/system_runtime_leaves_cpu.c` |
| `signals` | Exact hash and CFG, fixup-backed far calls, whole-RAM/CPU differential with Move live, full reachable instruction and branch-outcome coverage, every stop kind, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `game_or_other_library_unresolved`; identity is the verifying lane's call. The records' game meaning is not asserted. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_03C2_002E --require-current-main
```

# U3B0 +0CC5: raw-CPU cursor step composing the strict U3B0 +0B03

`pr_u3b0_cursor_step_0cc5_cpu` is a raw-CPU, full-state model of the
complete GAME.OVR unit `0x3B0` routine `+0CC5`. The VP assigned it to this
lane on 2026-10-17 (i17629), after Astra cleared it with no in-flight work.
In the census regenerated on main `11c22a0` its only callee is U3B0
`+0B03`, which is strict (PR #943), and it has no inline coverage, model,
branch or main file. The near call runs the merged raw-CPU
`pr_u3b0_step_scan_0b03_cpu`, which in turn runs the strict
`pr_u3b0_entry1_0000`. The classifier lists the routine as
`game_evidenced`, grade B. Identity and strict credit are the verifying
lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U3B0 `+0CC5` | GAME file `0x2D594..0x2D656`; **195 bytes, 68 instructions, five conditional branches**; `RETF 4`; SHA-256 `3c0d86662dcc84e68f99fc4b4a056606cf0ac652f4918d523b7ceb650168538a` |
| Trailing bytes | `+0D88..+0D99` (18 bytes) are string data before the next routine `+0D9A`; they are not executed and are not part of the model |
| Call | `+0D7F`, `PUSH CS; CALL +0B03` |
| Callers | U3B0 `+0D9A` at sites `+0ED2`, `+1000` and `+100E`, each through `PUSH CS; CALL` |

## ABI and ordered effects

The routine is a nested Pascal procedure. It is entered after a far call
with `SS:[SP]` the return IP, `[SP+2]` the return CS, `[SP+4]` the static
link L (the parent's BP) and `[SP+6]` a byte d. It returns with `RETF 4`.
It uses the same parent slots as `+0B03`:

| Parent slot | Meaning in this code |
| --- | --- |
| `L-59h` | a limit word lim |
| `L-56h` | a second bound word B |
| `L+0Ah/L+0Ch` | a far pointer X to an index word |
| `L+14h/L+16h` | a list head far pointer, read only by `+0B03` |

After `PUSH BP; MOV BP,SP; SUB SP,8` (the 8 bytes are never touched), it
moves X^ one step with wraparound. G is `DS:[6B29h]`, and every comparison
is signed:

1. When d <> 0: X^ += 1. If G + lim − 1 < X^, X^ := G. Then AX := B − 1,
   and if B − 1 < X^, X^ := G and AX := G.
2. When d = 0: X^ −= 1. If X^ < G, X^ := G + lim − 1. Then AX := B − 1,
   and if B − 1 < X^, X^ := B − 1 (AX is reloaded as B − 1).

It then executes `MOV AL,[BP+8]; PUSH AX; PUSH WORD [BP+6]; PUSH CS; CALL
+0B03`. The pushed word keeps AH from the last AX load, so the stack holds
(AH, d) exactly as above. `+0B03` walks from the stepped index in the same
direction and returns with `RETF 4`. Finally `MOV SP,BP; POP BP; RETF 4`.

Every operand is reread from RAM in source order. X is reloaded by each
`LES`, and L by each `MOV DI,[BP+6]`. An X^ that aliases the parent's words,
the static link, d, a return slot or `DS:6B29` is therefore seen by every
later read.

**Stops.** The model returns one of two stop statuses:

- `CALLEE`: the `+0B03` model stopped. `*callee_status` receives its
  status and `*leaf_status` the `+0000` status that `+0B03` reports. The
  three inner causes are as follows.
  - `+0B03`'s loop-back cap (`max_loopbacks` is passed through) gives
    status 4.
  - A relocated entry gives status 2 with leaf status 1. The near calls are
    relative, so `+0000`'s call reaches entry IP − 0CC5h, and `+0000`'s
    model rejects any IP other than 0.
  - `+0000` returning elsewhere would give status 3, but it cannot occur:
    `+0000` writes only its locals and saved BP, below its return frame.
- `TRANSFER`: `+0B03`'s `RETF 4` popped a CS:IP other than `+0D82`. This
  happens when X^ is `+0B03`'s return slot and the walk rewrites it.

**Flags.** `+0B03` always executes `SUB`, `XOR` and `CMP` before it can
stop. Every flag written by this routine is therefore overwritten, and all
are folded. The final flags are `+0B03`'s.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs, `POP`
and this routine's `RETF` frame are read physically contiguous. `+0B03` and
`+0000` keep their own contracts. Null RAM or CPU, or RAM that is not
exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u3b0_cursor_step_0cc5_cpu.c` runs under
CTest with ASan/UBSan and compiles with `-Wconversion -Wsign-conversion
-Werror`. It uses no game bytes. An independent reference of the step and
of `+0B03`'s walk predicts the index, the pushed (AH, d) word, the static
link and return slots, `+0B03`'s count, AX and every byte outside the stack
window. It passes 10,540 checks:

- 1,500 seeded worlds over list lengths, directions, limits, indices,
  bounds and flags;
- seven wraparound and clamp edges, including signed 7FFFh/8000h bounds,
  each checking the stepped index and the pushed AH;
- a `CALLEE` stop at `+0B03`'s third loop-back, a `CALLEE` stop from a
  relocated entry, and a `TRANSFER` with X^ on `+0B03`'s return IP;
- the argument guards.

**Unicorn differential.** `tools/overlay/u3b0_cursor_step_0cc5_audit.py`
loads unit `0x3B0`, so `+0B03` and `+0000` run from their original bytes.
It compares the entire 1 MiB RAM and all 14 CPU words with no mask. The
original's behaviour decides each stop, and the C callee and leaf statuses
must name the same inner stop. For a relocated entry, the original is
compared at `+0000`'s first fetch.

| Stops | Cases |
| --- | --- |
| Returns | 220 |
| `TRANSFER` | 2 |
| `CALLEE`, `+0B03` cap | 3 |
| `CALLEE`, relocated `+0000` | 1 |

Coverage is 68/68 instruction starts and 10/10 conditional outcomes, with
`+0B03` at 103/103 and `+0000` at 38/38 starts. The cases cover:

- 160 random worlds;
- 30 wraparound and clamp edges across d = 1, 0 and FFh;
- walks that run to the limit, a nil head and a cyclic list;
- X^ on the parent's limit and B words, the static link, d, this routine's
  return IP, the pushed AX, `+0B03`'s return IP and `DS:6B29`;
- B on `+0B03`'s count, and the parent words under `+0B03`'s pushes;
- small caps;
- stacks at both segment ends, and SS, the nodes and X across A20;
- a random background, and relocated entries with and without a walk.

Segment-end stacks keep `+0000`'s `RETF` frame off the boundary, because
the two oracles' `RETF` profiles differ there.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **96
cases** with no mask. In 76 of them lim > 0, so `+0B03` walks. The harness
records the low 64 bytes of memory once for the replay. Each case compares:

- all 14 CPU words;
- a 256-byte stack window holding this frame, `+0B03`'s and `+0000`'s
  frames, and the parent words;
- the index word and `DS:6B29`.

The pushed word carries a random high byte for d. The COM copies unit
`0x3B0` to paragraph `8800`; there are no far calls to relocate. The result
SHA-256 is
`489f413b6f526c3916f10670a2abae62a6255f3a7e5c1bc83440135a625fc936` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u3b0_cursor_step_0cc5_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u3b0_cursor_step_0cc5_audit.py --unicorn --native --output resources/u3b0-0cc5-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U3B0 `+0CC5..+0D87`, composing the strict U3B0 `+0B03` (and through it `+0000`) |
| `source_candidates` | `src/recovered/overlay_u3b0_cursor_step_0cc5_cpu.c` with `src/recovered/overlay_u3b0_step_scan_0b03_cpu.c` and `src/recovered/overlay_u3b0_entry1_0000.c` |
| `signals` | Exact hash and CFG, whole-RAM/CPU differential with `+0B03` and `+0000` live and full instruction and branch-outcome coverage, every reachable stop kind with its inner status, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `game_evidenced`; identity is the verifying lane's call. The parent slots' game meaning is not asserted. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_03B0_0CC5 --require-current-main
```

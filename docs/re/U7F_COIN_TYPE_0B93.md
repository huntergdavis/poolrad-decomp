# U7F +0B93: raw-CPU coin-type parser composing the string store and UpCase

`pr_u7f_coin_type_0b93_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x7F` routine `+0B93`, public entry 5. The VP assigned it to
this lane on 2026-10-17, after Astra confirmed they do not own it and have no
in-flight work on it. In the census-2115 queue its callees are SYSTEM's
string store (`runtime:2853`) and UpCase (`runtime:6186`), with no inline
coverage, model, branch or main file. Every call is a real far call into the
raw-CPU [`pr_system_strstore_0b25`](SYSTEM_STRSTORE_0B25.md) or
[`pr_system_upcase_182a`](SYSTEM_RUNTIME_LEAVES.md). The classifier lists the
routine as `game_evidenced`, grade A. The function name describes the code
path only; identity and strict credit are the verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U7F `+0B93` | GAME file `0x2581C..0x25942`; **295 bytes, 125 instructions, eight conditional branches**; `RETF 8`; SHA-256 `cd1bbb38be9d177c33de492a357365a8729eee8d5c3fac3cf9ebe980db77030b` |
| Bytes after the code | `+0CBA..+0CEA` are string data for the next routine; no branch reaches them |
| Literals | Pascal strings in the code segment at `+0B5A`, `+0B60`, `+0B66`, `+0B70`, `+0B7A`, `+0B82` and `+0B8A`, read by the string store |
| Calls | eight `LCALL 0AF8:0B25` and one `LCALL 0AF8:182A` at `+0BE6`; every segment word is fixup-backed |
| Callers | U7F `+0CEB` (near, site `+0EBC`); U77 `+2156` and `+24A7` through the U7F stub (sites `+239F`, `+269D`) |

The inventory lists the routine as `+0B93..+0CEB`. Its `RETF 8` ends at
`+0CBA`, and the audit checks that no branch target lies beyond it.

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP,
`[SP+2]` the return CS, `[SP+4]/[SP+6]` a far pointer D to an output string
and `[SP+8]/[SP+0Ah]` a far pointer S to an input string. It returns with
`RETF 8`. After `PUSH BP; MOV BP,SP; SUB SP,102h`:

1. The string store copies S into the local `string[255]` L at
   `SS:BP-101h`, with cap FFh.
2. i := 1 in `[BP-102h]`. While L[i] is a space, i := i + 1. The byte i
   has no bound, so an all-space buffer loops forever in DOS.
3. c := L[i] selects the output:

| c | Action | Result r |
| --- | --- | --- |
| `'G'` | if UpCase(L[i+1]) = `'E'`, D := the literal at `+0B5A`; otherwise D := `+0B60` | 5, or 3 |
| `'P'` | D := `+0B66` | 4 |
| `'E'` | D := `+0B70` | 2 |
| `'S'` | D := `+0B7A` | 1 |
| `'C'` | D := `+0B82` | 0 |
| `'J'` | D := `+0B8A` | 6 |
| other | nothing | the stale `[BP-1]` |

4. Each D := literal is the string store with cap FFh. The literal offsets
   are immediates in the code, so they do not move with the entry IP.
   `[BP-1]` := r, before the store for result 5 and after it otherwise.
5. AL := `[BP-1]`. Then `MOV SP,BP; POP BP; RETF 8`.

Every operand is reread from RAM in source order. A store whose D aliases
L, `[BP-1]` or the routine's frame changes later reads and the return.

**Flags.** On a matched path the final flags are the last store's
`XOR CH,CH` flags. On the unmatched path they are `CMP AL,4Ah`'s. DF is
clear from the first store. Three flag writes are computed because a stop
exposes them:

- `SUB SP,102h`, for a `CODE` stop at the first site;
- `XOR AH,AH` then `INC AX`, for a `CODE` stop at the UpCase site;
- the loop's `INC BYTE [BP-102h]`, for a `LIMIT` stop.

Other dead writes are folded.

**Stops.** The model returns one of three stop statuses:

- `CODE`: a call site holds other bytes. IP is at the site.
- `TRANSFER`: a callee returns anywhere but the next instruction. This
  happens when D aliases a store's own frame.
- `LIMIT`: `max_loopbacks` caps the taken `JMP +0BB3`. The next one stops
  at the loop head, entry IP + 20h, with every earlier effect kept.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs, `POP`
and the `RETF` frames are read physically contiguous. Null RAM or CPU, or
RAM that is not exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u7f_coin_type_0b93_cpu.c` runs under
CTest with ASan/UBSan and compiles with `-Wconversion -Werror`. Its only
game bytes are the nine five-byte far-call sites. The literals are
synthetic, not the game's strings. All RAM outside the stack frame must
match an expected copy in each case. It passes 78 checks:

- every letter path, including UpCase on a lower-case `'e'` and stale
  bytes past a short copy;
- the unmatched path returning the stale `[BP-1]`;
- a `LIMIT` stop with exact `INC` flags;
- `CODE` stops at the first and UpCase sites;
- a `TRANSFER` through D over the store's own frame;
- the argument guards.

**Unicorn differential.** `tools/overlay/u7f_coin_type_0b93_audit.py`
places the expanded START.EXE image at load segment 0, so the original
store and UpCase run from the nine unrelocated far calls. It loads unit
`0x7F`, including its literals, and compares the entire 1 MiB RAM and all
14 CPU words with no mask. The original's behaviour decides each stop.

| Stops | Cases |
| --- | --- |
| Returns, wherever `RETF 8` goes | 111 |
| `CODE` | 4 |
| `TRANSFER` | 2 |
| `LIMIT` | 2 |

| Code | Instruction starts | Conditional outcomes |
| --- | --- | --- |
| `+0B93` body | 125/125 | 16/16 |
| String store | 15/15 reachable | 1/1 reachable |
| UpCase | 8/8 | 4/4 |

The store's `MOV AL,CL` cannot run here, because every call passes cap FFh.
The cases cover:

- every letter and four other first bytes, with 0, 1 and 3 leading spaces;
- `'G'` followed by `'E'`, `'e'` and other bytes;
- 60 random worlds;
- all-space locals, and spaces up to i = 255;
- D over L, `[BP-1]`, the store's frame and the routine's frame;
- stacks at both segment ends, S, D and SS across A20, and a random
  background;
- four rewritten sites.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **60
cases** with no mask. They reach every result 0 to 6 and the stale-byte
path. Each case compares all 14 CPU words, a 320-byte stack window and a
256-byte output window. The COM copies unit `0x7F` to paragraph `8800`, and
the store and UpCase to `9000`. It relocates all nine fixup-backed segment
words to `9000`, loader-style. The result SHA-256 is
`e4282865dce15710f5ad930a1d897bd177821818c26a7d8b08a99c9f3b3a0935` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u7f_coin_type_0b93_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u7f_coin_type_0b93_audit.py --unicorn --native --output resources/u7f-0b93-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U7F `+0B93..+0CB9` (public entry 5), composing SYSTEM `0AF8:0B25` and `0AF8:182A` |
| `source_candidates` | `src/recovered/overlay_u7f_coin_type_0b93_cpu.c` with `src/recovered/system_strstore_fullstate_0b25.c` and `src/recovered/system_runtime_leaves_cpu.c` |
| `signals` | Exact hash and CFG, fixup-backed far calls, whole-RAM/CPU differential with both callees live, full body coverage, every stop kind, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `game_evidenced`, grade A; identity is the verifying lane's call. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.

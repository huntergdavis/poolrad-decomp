# SYSTEM leaves 0AF8:0B57, 0C10, 0C3B and 1685: raw-CPU Copy, string compare, char to string and Move

`system_string_move_leaves_cpu.c` holds raw-CPU, full-state models of four
Turbo Pascal SYSTEM leaves in the START.EXE SYSTEM segment (`0AF8` at load
segment 0). They reuse the types of `system_runtime_leaves_cpu.h`. The VP
approved them as one batch on 2026-10-18 (i17873); they are runtime
enablers, not game credit.

In the strict-queue snapshot taken at 03:06 PDT on 2026-10-18, all four are
`runtime_unmapped`. Counting the game_evidenced rows that call each one:

- the compare `0C10` is called by 11 rows. With GetMem and the string load
  and concatenation now covered, it is the last unready callee of U38
  `+11F9` and `+1276`.
- Move `1685` is called by 9 rows. It is one of U9E `+0160`'s blockers, on the
  `+02FA` → `+08C3` chain.
- char-to-string `0C3B` is called by 8 rows.
- Copy `0B57` is called by 6 rows.

## Exact original

| Leaf | Span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| Copy | `0B57..0B97` | 65 / 29 | `18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa` |
| String compare | `0C10..0C3A` | 43 / 20 | `11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58` |
| Char to string | `0C3B..0C4C` | 18 / 8 | `b2f0ef7b50632ff33ad1b2b251db935542c81b596360ac8b7cd6b391f3fca484` |
| Move | `1685..16A7` | 35 / 16 | `1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972` |

START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

## ABI and effects

Each leaf is entered after a far call: `SS:[SP]` holds the return IP and
`[SP+2]` the return CS. The frame is read through `SS:[BX+disp]` with
BX := SP, and BX keeps that value.

Common rules:

- Pointer pairs (`LES`/`LDS`) and the `RETF` frame are read physically
  contiguous.
- `REP MOVSB` and `REPE CMPSB` step one byte at a time and reread each
  source byte. Overlapping blocks therefore behave as on the CPU.
- SI and DI wrap at 16 bits.

### Copy (`0B57`)

| Frame slot | Contents |
| --- | --- |
| `[SP+4]` | Count |
| `[SP+6]` | Index |
| `[SP+8]` | the source string S |
| `[SP+0Ch]` | the result D |

`RETF 8` leaves D on the stack, as a Pascal string-function result.

1. An Index of 0 or below (signed) becomes 1, and SI := S + Index.
2. If length(S) < Index, the result is empty (`XOR AX,AX`'s flags).
   Otherwise AX := length(S) − Index + 1, and a negative Count becomes 0.
   AX is then the unsigned minimum of the two (`CMP AX,CX`'s flags).
3. D's length byte := AL, and AX bytes are copied.

### String compare (`0C10`)

`[SP+4]` holds the right string R and `[SP+8]` the left string L;
`RETF 8`. The result is in the flags.

- AL := length(L), AH := length(R), and CL := the smaller length.
- If CL is 0, CH keeps its entry value and no text is compared.
- Otherwise CH := 0 and `REPE CMPSB` compares the text. A differing byte
  leaves that `CMPSB`'s flags.
- If no byte differs, the flags are `CMP AL,AH`'s.

### Char to string (`0C3B`)

`[SP+4]` holds the char word and `[SP+6]` the result D; `RETF 2` leaves D
on the stack.

- D becomes the one-character string (length 1, then the char).
- AL := the char.
- No arithmetic flag changes; only DF is cleared.

### Move (`1685`)

| Frame slot | Contents |
| --- | --- |
| `[SP+4]` | Count |
| `[SP+6]` | the destination |
| `[SP+0Ah]` | the source |

`RETF 0Ah`. `CMP SI,DI` compares the offsets only, not the segments.

- **Forward.** When the source offset is at least the destination's, the
  copy runs forward and the flags are the CMP's.
- **Backward.** Otherwise SI and DI move to their last byte (`ADD`, then
  `DEC`), `STD` runs, and the copy runs backward:
  - DF stays set on return.
  - CF comes from `ADD DI,CX`.
  - The other arithmetic flags come from `DEC DI`.
- AX is unchanged.

**DS.** Copy, the compare and Move return DS through DX.

**Arguments.** Null RAM or CPU, or RAM that is not exactly 1 MiB, returns
`ARGUMENT` before any change.

**Stated limit (reviewer-added 2026-10-20, VP i19691): entry SP FFFE and FFFF.** The shared `retf()` reads the far-return frame physically contiguous. At a return-time SP of FFFE or FFFF it therefore takes the return CS from `SS*16+10000h`/`+10001h`, while DOSBox-X normal/386 (binary `99d74601…`) wraps it within SS (`SS:0000`/`SS:0001`). These leaves push nothing (`MOV BX,SP`), so the return-time SP is the entry SP. This was measured on Move `[1685,16A8)` itself; Copy, compare and char-to-string use the same contiguous read but were not separately probed. Measured with a three-oracle probe ([`tools/overlay/retf_stack_edge_probe.py`](../../tools/overlay/retf_stack_edge_probe.py)) that gives the wrapped and contiguous frame copies different return CS values. At SP FFFD, where the word straddles FFFF, all three oracles read it contiguous, and at SP ≤ FFFC they agree. **Unicorn 2.1.4 shares the model's contiguous reading, which is why the original-byte differential could not have caught this.** **Never exercised by this row's evidence**: the audited SP is 0x4000. These are runtime-coverage endpoints, not strict credits.

## Validation

**Focused C test.** `test_system_string_move_leaves_cpu.c` runs under
CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. It passes **48** hand-predicted
checks:

- Copy("Hello, Phlan", 8, 5) = "Phlan", Index 0, an Index past the end and
  a negative Count;
- equal strings, a shorter left string, a first-byte difference, and an
  empty left string, which keeps CH;
- char to string, including a wrap at `FFFF`;
- Move forward and backward over an overlap, which gives the `memmove`
  result with DF left set;
- a backward Move whose `ADD DI,CX` carries (CF);
- one whose `DEC DI` reaches `7FFFh` (OF, AF);
- the guards.

**Unicorn differential.** `tools/overlay/system_string_move_leaves_audit.py`
runs each leaf from its original bytes in the expanded START.EXE image. After
the leaf's `RETF` it compares the entire 1 MiB RAM and all 14 CPU words, with
**no mask**.

- **Cases: 511.** Copy 130, compare 129, char 124, Move 128.
- **Coverage.** Every instruction and both outcomes of every conditional
  jump run in all four leaves: Copy 8/8, compare 6/6, Move 2/2.
- **Placements.** Strings and blocks sit apart, overlapping in both
  directions, at the same address, in different segments and across segment
  ends.
- **Copy inputs.** Index and Count include signed and unsigned edges.
- **Compare inputs.** Equal, prefix, differing, random and empty strings.
- **Move inputs.** Forward and backward wraps, a zero Count both ways, and
  the CF, OF and ZF edges of the backward setup.
- **State.** SS across A20, DF set on entry, and random backgrounds.
- **Separation.** Every stack frame stays apart from the data.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **128
cases** (32 per leaf) with no mask.

- **Loading.** The COM copies the SYSTEM span `0B00..16FF` to paragraph
  `7000`.
- **Each case.** It rebuilds one 64 KiB data segment from blob chunks
  (strings, a differing byte, Move sources, wrapping placements). It then
  pushes the leaf's frame and calls it from a single call site with IRQs
  masked.
- **Comparison.** Each case compares all 14 CPU words, the stack window and
  the whole data segment.
- **Result.** The result SHA-256 is
  `adadc6a7906b05ea8f41d6e40d5490ad4118bcf223d00d9eb96254370bc60d98`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_string_move_leaves_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_string_move_leaves_audit.py --unicorn --native --output resources/system-string-move/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | SYSTEM `0AF8:0B57` Copy, `0C10` string compare, `0C3B` char to string, `1685` Move |
| `source_candidates` | `src/recovered/system_string_move_leaves_cpu.c` |
| `signals` | Exact span hashes; a whole-RAM/CPU differential with no mask over every instruction and conditional outcome; an independent C test; native synthetic execution |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Runtime (library) models; adding them to `RUNTIME_COVERED_ENDPOINTS` is the verifying lane's call |

No classifier, queue or count ledger is changed.

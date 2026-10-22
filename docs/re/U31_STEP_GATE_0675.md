# Unit 0x31 +0675 step gate

`pr_u31_step_gate_0675` translates the complete DOS v1.3 routine `GAME.OVR`
unit `0x31:+0675..+07D5`. It composes, on the same RAM and CPU words:

- the merged [U3C6 `+0CB9` CPU model](U3C6_0CB9_SCAN.md), through the
  overlay stub `03E0:0070`;
- the merged [raw-CPU `RES_00BA_2199` model](RESIDENT_SIDECAR_LOOKUP_2199_CPU.md),
  at four far calls.

It returns a byte gate in AL and may set a caller byte. No gameplay meaning
is inferred, and "step gate" names the effect. The recall attempt for this
lane found nothing to reuse.

The VP review of this target found that the credited callback model of
`RES_00BA_2199` cannot back a full-state composition. This port therefore
waited for the raw-CPU model, merged as PR #902.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit | memory segment `0x38`, code at GAME file `0xABC0`; public entries `+080F`, `+1922` |
| Function | GAME file `0xB235..0xB395`; **353 bytes, 137 instructions, ten conditional branches, five far calls** |
| Function SHA-256 | `f9168e9bb1ad4b8bad993f506ece8ad99d629d15f148e8dd4b622ef56a5069a1` |
| Far calls | `+06DA LCALL 03E0:0070` (U3C6 entry 16, `+0CB9`, `RETF 0Eh`), and `+0758`, `+076F`, `+0786`, `+079D LCALL 00BA:2199` (`RETF 0Ah`); all five segment words are fixup-backed |
| Exit | `RETF 0Ch` at `+07D3`; the code ends at `+07D6` |

The candidate queue lists this routine as 381 bytes. That span runs to the
next prologue at `+07F2`, so it includes a 25-character Pascal string after
the `RETF`; the code body is 353 bytes. Every branch target lies inside the
body. Decoding every unit-0x31 routine from its own prologue finds exactly
one aligned `PUSH CS; CALL +0675`: `+094B` inside `+07F2`.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP and `[SP+2]`
return CS. The Pascal arguments are:

- `[BP+6]/[BP+8]` result-byte pointer R;
- `[BP+0Ah]` byte b;
- `[BP+0Ch]` byte c;
- `[BP+0Eh]/[BP+10h]` object pointer X.

`RETF 0Ch` leaves SP sixteen bytes above entry.

1. `PUSH BP; MOV BP,SP; SUB SP,0Ah`. Then `*R` := 0 and the result
   `[BP-1]` := 0.
2. Y := X→`+108` (far pointer).
   t := `DS:[02AE + 5·Y[15h] + c]`, stored in `[BP-6]`.
   r := (b + t) mod 8 (`CWD; MOV CX,8; IDIV CX; XCHG DX,AX`), stored in
   `[BP-7]`. The dividend is a sign-extended word, so `IDIV` cannot fault.
3. Call `+0CB9` with P6 = `SS:BP-9`, P10 = `SS:BP-8`, k = r and Q = X.
4. If `[BP-9]` is 0, `*R` := 1 and the result stays 0.
   If `[BP-8]` is nonzero, the result stays 0.
5. w := `DS:[2744 + 4·[BP-9]]`. Unless w < Y[6], the result stays 0.
6. The threshold `[BP-0Ah]` := w × 2, or w × 3 when r is odd (`SHR AL,1`
   carry). This keeps the low byte of `MUL DX`, and DX takes the product's
   high word.
7. When `[BP-9]` is 1Eh, the routine looks up keys 20h, 1Eh, 6Fh and 7Dh in
   X's sidecar list, in that order. Each lookup is a `RES_00BA_2199` call
   with the cursor cell `SS:BP-5`, and the first hit stops the series. With
   no hit, the threshold := Y[6] + 1. Y[6] = FFh wraps it to 0, so the gate
   then opens.
8. The result := 1 unless Y[6] < threshold. Then AL := the result, and
   `MOV SP,BP; POP BP; RETF 0Ch`.

The composed calls leave their registers in place: AH, DX, ES:DI, BX, CX
and SI carry the last callee's values or this body's later loads. The final
flags are the last `CMP`'s, at `+06DF`, `+06EF`, `+070F` or `+07C4`.

These flag writes are folded: `XOR AH,AH`, `SHL`, `ADD`, `IDIV`, `SHR`,
`MUL`, `INC` and `OR AL,AL`. Each is overwritten before any flag is read
or the routine returns, either by a later `CMP` or by a callee's prologue
`SUB` (`+0CB9` opens `SUB SP,0Ch` and `RES_00BA_2199` opens `SUB SP,2`).
`SHR`'s carry selects the factor directly.

Every operand is reread from RAM in source order, including R, X, Y, the
DS tables and every frame byte. Offsets wrap at 16 bits and physical
addresses at 1 MiB. Words, `LES` pairs and the `RETF` frame are read
physically contiguous.

## Composition contract

Each far call is read from RAM at its site: opcode `9A`, offset, segment.

- **`+0CB9`.** The call needs offset `0070h` and a stub at that segment
  whose bytes are `EA B9 0C` plus a segment. It then runs
  `pr_u3c6_0cb9_cpu_run` there with the `CONTIGUOUS_PHYSICAL` `RETF`
  policy, as merged `+1434` does.
- **Lookups.** Each needs offset `2199h`. It runs `pr_res_00ba_2199_cpu` at
  the call's segment with the caller's `lookup_max_loopbacks`.

Each call pushes CS and the next IP relative to the entry IP, and each model
must return there. Otherwise the result is one of these statuses:

- **`TRANSFER`.** A model returned somewhere else.
- **`CALLEE`.** A model stopped, for example a cyclic sidecar list hitting
  the lookup cap. `callee` then names the site and that model's status.
- **`CODE`.** A reached site or stub is not in its loaded form. The routine
  stops before that call with every earlier effect kept. Its arithmetic
  flags are then the last modeled `CMP`'s or the entry flags, not the
  folded instructions'.

Null RAM/CPU, or RAM that is not exactly 1 MiB, is rejected before
mutation.

## Validation and parity review

The focused C11 test runs under CTest with ASan/UBSan, alongside the
composed models' tests, and compiles with `-Wconversion -Werror` on the new
files. It passes 59 checks using the shared synthetic `+0CB9` fixture
`tests/recovered/u33_place_fixture.h`, which holds no game bytes:

- the even and odd factors, with exact frame bytes and flags;
- the weight exit, the occupant exit, and the class-0 `*R` := 1 exit;
- class-1Eh hits on the third and the fourth key, with the cursor cell
  holding the hit node;
- no hit, plus the Y[6] = FFh wrap;
- a cyclic sidecar list stopping as `CALLEE` with the lookup model's
  `LIMIT` at `21B9`;
- `CODE` at the `+0CB9` site and at a lookup site;
- the argument guards.

The Unicorn original-byte differential places the hash-checked expanded
START.EXE image at load segment 0. The unrelocated `00BA:2199` calls
therefore reach the original resident bytes. The `03E0:0070` stub is
patched to its loaded `EA` form. The differential loads unit 0x31 and the
three unit 0x3C6 spans that `+0CB9` runs. The DS world is the `+1577`
audit's `+0CB9` world plus the `+0675` tables, Y, sidecar lists and R.

It passes **407 returning cases**, each comparing the entire 1 MiB RAM and
all 14 CPU words with no mask:

- 300 random worlds;
- 92 directed class-1Eh worlds, most of them steered so that all four
  `+0CB9` probes land on class 1Eh, with every key subset and key 7Dh
  alone;
- class-forced worlds;
- four stack offsets;
- R inside the stack;
- a random background and preserved DF/IF.

66 cases reach the lookups: 15 end at `+0758`, 10 at `+076F`, 4 at `+0786`
and 37 at `+079D`. All 137 instruction starts and all 20 conditional
outcomes are reached.

The DOSBox-X normal-core/386 synthetic oracle passes **50 cases**, 22 of
which reach `RES_00BA_2199`. Each case compares all 14 CPU words, 256 stack
bytes and 39,936 data bytes, with no mask. The setup works as follows:

- The COM copies the original unit 0x31 code, the three unit 0x3C6 spans
  and the 111 resident bytes to absolute paragraphs.
- The five fixup-backed far-call segment words are relocated loader-style:
  the stub paragraph `8C00` holds `EA` to `+0CB9`, and the resident
  paragraph is `8C10`.
- For each case, the data regions are streamed from `WORLDS.BIN`, built by
  the same fixture.

The result SHA-256 is
`aed897615da537a779844f95c57fa158c16d9789177866efa67d19cae9f5c9c1`, with
COM segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
cmake -S . -B resources/build-0675 -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all'
cmake --build resources/build-0675 --target poolrad_overlay_u31_step_gate_0675_tests
ctest --test-dir resources/build-0675 -R poolrad_overlay_u31_step_gate_0675 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u31_step_gate_0675_audit.py --unicorn --native \
  --output resources/u31-0675-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x31` `+0675..+07D5`, with U3C6 `+0CB9` and `RES_00BA_2199` (raw CPU, four sites) composed |
| `source_candidates` | `src/recovered/overlay_u31_step_gate_0675.c` |
| `signals` | Exact hash/CFG, one aligned caller, fixup-backed far calls, whole-RAM/CPU differential with full instruction and branch-outcome coverage through both callees, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `+0CB9` is a merged model whose own strict credit is pending, the same caveat as `+1434`. `RES_00BA_2199` composes through the raw-CPU model of PR #902. Natural saved-game reach is unmeasured. |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0031_0675 --require-current-main
```

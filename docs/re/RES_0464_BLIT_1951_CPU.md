# Resident 0464:1951: raw-CPU bitmap blit

**Readiness work, not credit.** This body is a graphics primitive. No
identity is claimed here, and identity may be refused on the RES_0464_2A0D
line. It is translated so that the chain under `OVL_0025_0511` can become
composable. The only credit that chain can end in is 0511's 51-byte body
(VP i19624).

`pr_r0464_blit_1951_cpu` models START.EXE resident `0464:[1951,1B52)`. It
composes the accepted raw SYSTEM Move (`pr_system_move_1685`,
[SYSTEM_STRING_MOVE_LEAVES.md](SYSTEM_STRING_MOVE_LEAVES.md)).

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| RES 0464 blit | `0464:[1951,1B52)`, loader-expanded image `[05F91,06192)` | 513 / 201 | `d64bb18e4503a5679481894c5ddaad44b3c0f703b00d1810afbbba8e9f358de3` |
| SYSTEM Move | `0AF8:[1685,16A8)` | 35 / 16 | `1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972` |

The image is unpacked with `resident_loader_map.unpack`; a raw file offset is
never used. The audit asserts that the first and only return in the span is
`RETF 8` at +1B4F, so the span ends at +1B52, where `RES_0464_1B52` begins.
The two far calls (+19DC, +1B39) go to SYSTEM `0AF8:1685`, and their
segment words are at +19DF and +1B3C.

## Behaviour

Entry follows a far call: `[SP]` IP, `[SP+2]` CS, `[SP+4]` a far bitmap
pointer B, `[SP+8]` a row argument X, `[SP+0Ah]` a column argument Y. A null
B (offset and segment both zero) returns at once. Otherwise the video-mode
byte DS:4A1E selects one of three paths. Rows run from 8·X to
8·X + B[0] − 1, compared signed.

| DS:4A1E | Path |
| --- | --- |
| 0 | Per row r, Move `2·B[2]` bytes from B+17h+running offset to `DS:[4EC0+2r]` : `2·Y` |
| 1 | `OUT 3C4h,2`. Per row r, column k = Y .. Y+B[2]−1 and plane p = 0..3: `OUT 3C5h, DS:[25F6+p]`, then store the next bitmap byte at `A000 : DS:[5050+2r] + k`. Finally `OUT 3C5h,0Fh` |
| 2 | Per row r, Move `4·B[2]` bytes to `DS:[51E0+2r]` : `4·Y` |
| other | return |

Every operand is reread from the bus in source order. The running source
offset, the loop counters and the row/column bounds live in the frame
(`[BP-2]..[BP-19h]`), and table words and bitmap bytes are reread on every
use. A Move whose destination lands on the frame therefore changes later
behaviour, and the model follows that (see *Aliases*). The function ends
`MOV SP,BP; POP BP; RETF 8`.

## Contract and stops

- Exactly 1 MiB of A20-off RAM through `pr_bus`; with an EGA attached, A000
  accesses go through `pr_ega`. IP must be 1951h. Code (body, Move and the
  two segment words) is immutable, and the model does not refetch it, so
  there is no CODE stop.
- Stack and frame words are addressed at `SS:(offset mod 64K)`; other word
  bytes are linear-contiguous with 1 MiB wrap.
- **Contract limit (CONTRACT stop):** SYSTEM Move runs on the flat RAM. With
  an EGA attached, a Move whose source or destination range meets
  A0000..AFFFF is refused at Move entry (CS:IP 0AF8-image:1685, frame pushed)
  instead of bypassing the EGA. The original would copy through VRAM. This
  is feasible only with a mode-0/2 row-table segment, or a bitmap pointer,
  in A000..AFFF. No oracle case uses it; the C test exercises it.
- Stops: RETURN (RETF 8); ARGUMENT (null bus or CPU, size ≠ 1 MiB, IP ≠
  1951h; nothing changed); PORT (the bus refused an OUT; IP at the OUT);
  LIMIT (`max_loopbacks` taken backward jumps; stopped before the next);
  TRANSFER (Move returned anywhere but the call's next IP); CALLEE (Move's
  leaf returned a non-RETURNED status, which it does only for invalid host
  arguments, so it is unreachable by contract); CONTRACT (above).
- Flags. ADD/ADC/SUB/SBB/CMP/INC/DEC/AND/OR/XOR are exact. For SHL, AF is
  architecturally undefined, as is OF for counts > 1. The model uses
  Unicorn's rule (AF := 0, OF := MSB(v<<(n−1)) ^ MSB(result)). A per-producer
  path analysis shows these flags are overwritten on every path before any
  branch, return, OUT or LIMIT stop. The exception is SHL DI,1 at +19CC and
  +1B29, whose AF is live into the Move call, and Move's first flag write
  (`CMP SI,DI`) overwrites it before any Move stop. So the undefined flags
  are visible only at the model-only CONTRACT stop, which no oracle compares.

## Validation

Receipt: 2026-10-20 13:48:21–13:51:20 PDT on this branch, with the `dosbox_x`
block (resources/1951/final.json, ignored, SHA-256 `039494a0…3676`). It
reproduces the 13:13 receipt exactly; RESULT.BIN is unchanged.

**Unicorn: 94 original-byte cases** compare unmasked CPU14, all 983,040
ordinary RAM bytes, the ordered OUT stream (473 OUTs) and the ordered A000
write-operand stream (411 operands). The A000 aperture is checked by replay:
the original's flat bytes equal the initial bytes with the writes replayed,
and the model's RAM aperture is unchanged. All **1,048,576 bytes** are
checked, with **flag masks 0**.

| Coverage (successor-based) | Starts | Conditional outcomes |
| --- | ---: | ---: |
| Body `[1951,1B52)` | 201/201 | — |
| Body + SYSTEM Move | 217/217 | 28/28 |

Nothing is excluded; the audit fails closed on any uncovered outcome.

| Stop | Cases | How |
| --- | ---: | --- |
| RETURN | 76 | RETF 8 |
| PORT | 10 | the n-th OUT refused, n = 0..9, in a mode-1 1×2 blit (10 OUTs) |
| LIMIT | 6 | m0-limit0, m2-limit0, m1-limit0/1/3 (budgets 1 and 3 return in modes 0/2), and `alias-counter` |
| TRANSFER | 2 | `alias-move-frame` (mode 0) and `alias-move-frame-m2` (mode 2) |

Cases include all mode bytes (0, 1, 2, 3, FFh); null bitmap pointers; zero
rows or columns; signed-negative and wrapping row/column arguments; six flag
profiles; odd, minimum and maximum SP; SS = DS; an A20 stack; and a bitmap
crossing offset FFFF.

**Aliases inside the contract.** These cases make the row segment equal SS,
so Move's destination lands on the stack. Each one asserts its precondition
(the destination meets the stated range) and its outcome:

- `alias-locals`: overwrites the running source offset and the
  count/end locals, and returns;
- `alias-own-return`: overwrites only the saved BP and return IP; the outer
  RETF goes elsewhere (asserted);
- `alias-counter`: overwrites the row counter `[BP-4]`, so the loop misses
  its end and stops at the budget (LIMIT);
- `alias-move-frame` and `-m2`: overwrite Move's own return pair, giving
  TRANSFER.

**Native DOSBox-X: 40 cases equal**, machine=ega, mode 0Dh, normal/386,
binary `99d74601…`. The cases are modes 0/1/2/3 × six flag profiles, plus
zero, small, wide and negative-row shapes and a null pointer. Each compares
unmasked CPU14, a 1 KiB stack window holding the frame, DS, the bitmap
segment, the row-target segment (3 × 64 KiB) and all four 64 KiB EGA planes.
RESULT.BIN SHA-256 `f4edcc21…de9f`.

Synthetic setup and declared deviations: a COM program places the original
body at 8800:1951 and the original SYSTEM Move at 6000:1685. It patches the
two far-call segment words to 6000h, which is the relocation, as in the
Unicorn fixture. It loads the three segments per case, fills the planes,
pushes the four argument words and calls far. The mode-0/2 row tables point
into ordinary RAM at 3000h rather than B800h, which is video memory on an
EGA machine. Nothing in the body or Move is stubbed. OUT order is not traced
natively; its effect is compared through the planes. EGA sequencer registers
are write-only, so the final `OUT 3C5h,0Fh` is not natively observable. It is
covered by the Unicorn OUT stream and the C test. The native sample is not
1 MiB evidence and repeats none of the alias, limit or port cases.

### Declared substitution: the DOSBox-X binary

The native oracle is **not** the pinned DOSBox-X source build (git
`5a9c7c53481c2e933948039c33256776653f4faf`). It is the packaged binary
`/usr/bin/dosbox-x`, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, reporting
`DOSBox-X version 2026.01.02 SDL2` (Debian package `2026.01.02+dfsg-2`). It runs
with `machine=ega`, `core=normal`, `cputype=386` and `cycles=max`. The receipt's
`dosbox_x` block records path, hash and version. The pinned build was neither
rebuilt nor re-run here.

What could differ from the pinned build:

- lazy-flag conventions: AF after OR/XOR/AND/TEST, MUL/DIV and non-zero
  shifts; OF after SHL with counts above 1; ZF/PF after MUL;
- LES wraparound above offset FFFC;
- stack-offset wrap at FFFF;
- EGA write-path details.

**SHL insulation.** This body's undefined-flag producers are SHL (AF, and OF
for the `SHL AX,CL` counts). The audited 386 behaviour sets AF = 1 after a
non-zero shift where the model uses AF = 0. The per-producer path analysis
above shows every such AF/OF dead before every compared stop: each is
overwritten by a later flag writer before any branch, OUT, LIMIT stop or
RETURN, or, for SHL DI,1 at +19CC/+1B29, by Move's `CMP SI,DI` before any Move
stop. The only stop that could expose them is the model-only CONTRACT stop,
which no oracle compares. The 40/40 native equality, FLAGS included, is
consistent with that. It does not prove agreement with the pinned build on
states the replay does not reach. The native frames sit at SS:F000 and
below, so they do not exercise LES or stack wrap at FFFF, and the native
sample has no alias, LIMIT or PORT cases. A reviewer who needs the pinned result
must re-run the native harness with that binary.

Four native mismatches during development were **harness** defects, each
diagnosed and fixed before this receipt; the model was not changed for any
of them:

1. a map-mask read-back (write-only on EGA);
2. a post-return `PUSH CS` over the argument word;
3. a window copy without `CLD` when the body returned with DF set;
4. a timer IRQ during the plane fill whose BIOS-handler pushes landed below
   the frame. Interrupts are now off from the window fill until the record
   is taken.

**C test:** `cc -std=c11 -Wall -Wextra -Wpedantic -Werror
-fsanitize=address,undefined -fno-sanitize-recover=all` over the test and
the five sources builds without warnings, and **30 hand-derived checks
pass**. They cover:

- null and mode-3 returns;
- a mode-1 1×1 blit: 6 OUTs, 4 A000 writes, per-plane bytes and the final
  map mask;
- zero rows;
- mode-0 and mode-2 row copies through Move;
- the CONTRACT stop state at Move entry;
- LIMIT at +19ED after one Move;
- PORT at +1A2E;
- ARGUMENT rejections that leave the state unchanged.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0464_1951',)`. `--compile-only` builds the five sources
with `-std=c11 -Wall -Wextra -Wpedantic -Werror` before any media access and
prints `CHECKER_BUILD_OK`.

    python3 tools/overlay/res0464_blit_1951_cpu_audit.py --compile-only
    python3 tools/overlay/res0464_blit_1951_cpu_audit.py --unicorn --native --output resources/1951/final.json
    python3 tools/overlay/check_checker_builds.py RES_0464_1951 --require-current-main
    ctest --test-dir <build> -R '^poolrad_resident_0464_blit_1951_cpu_tests$' --output-on-failure

The C source was produced by the per-instruction emitter
`tools/overlay/emit_raw_cpu_c.py`, driven by `tools/overlay/emit_res0464_blit_1951.py`.
Each case carries its original instruction as a comment, and the
hand-written helpers carry the flag rules above. The source is reviewable
without the emitter. `python3 tools/overlay/emit_res0464_blit_1951.py --check`
regenerates the header and source byte-for-byte (`EMIT_CHECK OK`) and prints
the undefined-flag report.

## Author review

- target_function: RES_0464_1951, complete `[1951,1B52)`, composing SYSTEM
  Move.
- verdict: likely_match within the stated contract. **No identity or credit
  claimed**; manifest `full` declaration proposed for readiness only.

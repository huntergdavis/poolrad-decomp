# RES_00BA:1887 suffix-copy return gate

This bounded gate starts at the nonzero `RES_00BA:1816` display path's
SYSTEM `0AF8:0B0B` call entry and runs the complete Pascal-string copy
through `RETF 4` to `00BA:1887`. It stops before the second printer call.
The same-run input comes from the two flag-one overlay caller prefixes,
through the original first printer and the separately checked `1858`
continuation. It is synthetic route evidence, not a saved-DOS natural-flag
observation.

## Pinned original bytes and ABI

| Span | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Expanded display body `[0x23B6,0x2432)` (`00BA:1816`) | 124 | `645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175` |
| Private Pascal source `[0x23B3,0x23B6)` (`00BA:1813`) | 3 | `4bd4cdfadef19a0803e203a5c05b37a6a3e79e76d25454fa559743b912409844` |
| SYSTEM copy `START.EXE[0xB781,0xB79B)` (`0AF8:0B0B`) | 26 | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |

At helper entry, `SS:SP` holds return `1887:00BA`, source
`1813:00BA`, and destination `[BP-4]:SS`, followed by the retained
second-printer arguments. `CLD`, `MOV BX,SP`, and `MOV DX,DS` precede
`LES DI,SS:[BX+8]` and `LDS SI,SS:[BX+4]`. `LODSB` copies the Pascal
length with `STOSB`; `MOV CL,AL; XOR CH,CH; REP MOVSB` copies the payload.
`MOV DS,DX; RETF 4` restores DS, removes the return and source pointer,
and leaves the destination pointer at the top of the stack for the second
printer. The original literal has length two. No literal content is
published here.

The new C bridge validates this display-specific frame and reuses
[`pr_slot_a_saved_copy_1965_step`](../../src/replay/slot_a_saved_copy_1965.c)
for those 12 helper instructions. That adapter names the same pinned
26-byte helper at its saved-game relocated segment `131C`; its instruction
model never reads CS. The bridge maps only the helper entry label to
`131C:0B0B`, preserves the real `00BA:1887` return on the stack, and
checks the resulting CPU/RAM against execution at original `0AF8:0B0B`.
It accepts the reached frame and source/destination spans within a 1 MiB
flat image. It does not execute the second printer.

## Differential evidence

The focused test runs original bytes from first-printer return `1858` to
SYSTEM entry and compares the prior C continuation there. It then runs
the original helper to `1887` and compares the C bridge. The two flag-one
overlay sites × three selected colors × upper/lowercase glyph inputs give
**12/12** original-literal returns. Six additional same-run variants set
only the source length to zero at helper entry and give **6/6** returns.
Every case compares all 1 MiB of RAM and 14 CPU fields at `1887`; AF is
masked after the original `XOR CH,CH` because x86 leaves it undefined.
The test also checks the ordered destination stores, retained stack
pointer, and next PC. The C sources compile with strict C11 warnings.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_00ba_1887_copy_return.py
```

This proves the copy-return gate for the measured synthetic frames. The
second printer, saved-DOS natural nonzero-flag reach, complete display
effects, hardware EGA planes, and strict full-C/game/playable credit remain
open. Original media and native captures remain under ignored `resources/`.
Before implementation, bounded `deja "Poolrad RES_00BA 1887 after SYSTEM
0AF8 0B0B copy second printer return"` timed out without a result; no
prior-session assertion was reused. Existing repository evidence reused
and checked: the 1858 gate, the pinned SYSTEM helper and native copy ABI
in [SLOT_A_SAVED_COPY_1965_JOIN.md](SLOT_A_SAVED_COPY_1965_JOIN.md).

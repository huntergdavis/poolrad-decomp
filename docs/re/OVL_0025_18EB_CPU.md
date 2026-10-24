# OVL_0025 +18EB: the ECL word compare as a raw full-state model (no new credit)

`OVL_0025_18EB` is **already strict**. Its credit rests on the semantic `pr_rec_ecl_compare_words`
([ECL_HELPERS_ORIGINAL_AUDIT.md](ECL_HELPERS_ORIGINAL_AUDIT.md)). This change adds the body's first **raw** translation,
`pr_u25_compare_words_18eb_cpu`, so that the hybrid dispatch can run the body from guest state. **No credit or ledger
change** is proposed. The audit is added to the row's evidence in `credit_evidence_kind.csv`.

**Pre-claim grep (widened per PM i22089).** `(\+|0x|:)18EB\b` over `src/recovered`, `tools/overlay` and `include` hits
only:
- the semantic contract (`ecl_helpers.c` / `.h`);
- its audit;
- the overlay classifier tables.

There is no full-state or inline translation to carve out. Contrast `+06D3` and `+06FB`, which do have inline raw copies.

## Executable extent

- **Body:** GAME.OVR unit 0x25 `[18EB,1951)`, file offset `0x8DBE`. 102 bytes, 36 instructions, SHA-256 `582a3a5a…a8e9`.
- **Return:** one `RETF 4`, at `194E`.
- **Call:** exactly one, `LCALL 0AF8:16A8` at `+18FA`. Its segment word at `+18FD` is the span's only fixup.
- **Callee:** the SYSTEM FillChar `0AF8:[16A8,16BC)`, 7 instructions ending `RETF 8`, SHA-256 `f1c5fade…17f7`. It comes from
  the loader-expanded START.EXE, and the audit hash-checks it.

## What the body does

1. `PUSH BP; MOV BP,SP`.
2. `FillChar(DS:6F78, 6, 0)` (`MOV DI,6F78; PUSH DS; PUSH DI; MOV AX,6; PUSH AX; MOV AL,0; PUSH AX; LCALL`) clears the six
   compare-flag bytes.
3. Six times: `MOV AX,[BP+8]; CMP AX,[BP+6]; Jcc; MOV BYTE [6F78+k],1`. In order, the bytes are equal, not equal, below,
   above, below or equal, and above or equal. All six comparisons are unsigned (`JNE`, `JE`, `JAE`, `JBE`, `JA` and `JB`
   skip the store).
4. `POP BP; RETF 4`.

**Operands are reread** from RAM for every compare, in instruction order. When DS = SS and the flag bytes overlap the
frame, the clear and each store change what the next compare reads. The audit's `alias-*` sweep and the C test both
exercise this.

**FillChar** is composed with the existing raw C leaf `pr_system_fillchar_16a8`. The model checks that the bytes at
`+18FA` are `9A A8 16` and reads the FillChar segment from `+18FD`, as the loaded code holds it.

**Flags at return** are the last CMP's six arithmetic flags, computed with the pinned `t_CMPw` rule. DF is clear, from
FillChar's `CLD`. The census row is classified "agree / agree".

## Raw contract and stops

- **Entry:** IP `18EB` after a far call. The frame holds the return IP, the return CS, b and a.
- **Memory:** exactly 1 MiB, A20 off.
- **Entry SP:** in `[000Eh,FFF8h]`, so neither the 14 bytes pushed below nor the 8-byte frame wrap.
- **Code:** immutable.

| Stop | Pinned by |
| --- | --- |
| RETURNED | Unicorn 357; native 40/40 |
| TRANSFER (FillChar's or the body's RETF popped another address, because the clear or a store overwrote the frame) | Unicorn 20 (`alias-*`); C test (`0100:0100`) |
| CODE (the LCALL bytes differ) | C test: stops at `+18FA` |
| ARGUMENT | C test (asserts on): CPU and RAM untouched |

## Checks

`tools/overlay/u25_compare_words_18eb_cpu_audit.py` (CHECKER_ROWS `OVL_0025_18EB`) runs the original body **and** the
original FillChar in Unicorn. It compares the 14 CPU words, all 1,048,576 RAM bytes and the stop, **with no flag
masks**.

**Unicorn: 377 cases, every one equal.**

| | |
| --- | --- |
| End states | RETURNED 357, TRANSFER 20 |
| Instruction starts reached | 43 / 43 (36 body + 7 FillChar) |
| Conditional outcomes | 12 / 12 |
| Independent statements (without the C model) | 346 |

The independent statements check, in each case where the flag bytes don't overlap the stack: the six bytes against the
stated relations; AX = a, CX = 0, DI = 6F7E and ES = DS; DF clear; SP + 8, with BP and the return CS:IP restored.

**What the cases cover:**
- 14 edge pairs, covering every relation and each CMP's flags at the sign and nibble edges;
- seven entry FLAGS values, including DF set;
- the SP guard's edges;
- a DS = SS sweep of entry SP `6F68..6F8B` across the flag bytes, the frame and FillChar's own return address;
- A20 wrap of DS and SS;
- 300 random states. Random states that would write into the code are excluded, because code is immutable.

**Mutants: 10. Nine are caught by the differential.**

| Mutant | Result |
| --- | --- |
| JBE taken on CF only | caught |
| JAE read as signed | caught |
| the stores start at 6F79 | caught |
| FillChar clears 5 bytes | caught |
| FillChar not called | caught |
| the operands read once, before the loop | caught (the alias cases) |
| CMP leaves AF clear | caught |
| CMP OF from the result sign only | caught |
| RETF pops 2 | caught |
| **MOV AL,0 clears AH too** | **survives, and is equivalent**: `MOV AX,6` just before leaves AH = 0 |

**C test:** `tests/recovered/test_overlay_u25_compare_words_18eb_cpu.c`, built with ASan/UBSan and asserts on. It covers:
- below, equal and unsigned-above with the exact flags;
- FillChar's register effects;
- the reread chain at entry SP `6F74`;
- the frame-overwrite TRANSFER at `6F78`;
- the CODE stop;
- the SP edges;
- atomic argument rejection.

**Native:** `tools/overlay/u25_compare_words_18eb_native.py`, DOSBox-X normal/386 (the host's packaged build, sha256
`99d74601…0df3`), with no masks and IF cleared. **40 of 40 cases are equal**, comparing 14 CPU words and 5 × 64 KiB per
case. It covers the edge pairs, the FLAGS values, the SP edges and the 18 `alias-*` cases that the C model completes.
The other 18 alias cases end in TRANSFER, which never reaches the oracle's return label, so they are not run natively.

## What this does not show

- That this is the routed path in the hybrid. It is added to the dispatch in a later glue run.
- Pinned-DOSBox-X `LES` wrap behaviour inside FillChar. The SP guard keeps FillChar's frame reads below `FFF5h`, where
  the wrapped and contiguous reads coincide.

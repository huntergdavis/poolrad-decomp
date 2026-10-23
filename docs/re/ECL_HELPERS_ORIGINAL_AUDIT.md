# ECL helpers: original-byte checker for eleven credited models

`tools/overlay/ecl_helpers_original_audit.py` is a registered checker. It replaces a cited shared tool as the
re-runnable evidence for eleven credited bodies modelled in `src/recovered/ecl_helpers.c`. Each body's evidence document
is [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md), which records the translations themselves.

**Before this checker,** `credit_evidence_kind.csv` labelled these rows `cites-tool`. Their evidence cited only
`ecl_walk.py`, a shared script decoder that runs no body-specific check (VP i21878, PM i21879).

## What it checks

**Original side.** Each body runs from the ORIGINAL bytes under Unicorn:

- GAME.OVR units `1Ch`, `25h` and `38h` load at their own segments, with every segment fixup applied;
- the hash-checked, loader-expanded START.EXE loads with its relocation table applied. That supplies the SYSTEM leaves
  the compares call: StrStore `0B25`, FillChar `16A8` and StrCmp `0C10`.
- The body's extent, instruction count, `RETF` immediate and far calls (SYSTEM only) are asserted from the bytes.

**C side.** The C model runs through `tools/overlay/ecl_helpers_original_shim.c`, which passes plain arguments into
the large loader-state struct and back out.

**Comparison.** For each call, the checker compares:

- the model's result (AX or AL), or the data bytes it writes;
- the **whole 64 KiB data segment**, which must differ from its state before the call in exactly the model's bytes.

There are no masks.

| Row | Original | Model | Compared | Cases |
| --- | --- | --- | --- | ---: |
| `OVL_001C_2AEB` | unit `1Ch` `+[2AEB,2B14)`, 13 insns, `RETF` | `pr_rec_ecl_op_print_return` | `[49ED]` (PC), `[84DF]`, `[5E24]`, `[5E25]` | 48 |
| `OVL_0025_06AF` | unit `25h` `+[06AF,06D3)`, 16 insns, `RETF 4` | `pr_rec_ecl_operand_word` | AX | 4,611 |
| `OVL_0025_06D3` | unit `25h` `+[06D3,06FB)`, 16 insns, `RETF 2` | `pr_rec_ecl_char_remap` | AL, all 256 codes | 256 |
| `OVL_0025_06FB` | unit `25h` `+[06FB,0756)`, 28 insns, `RETF 2` | `pr_rec_ecl_address_class` | AL; every range edge ±1 plus random | 2,962 |
| `OVL_0025_1815` | unit `25h` `+[1815,18EB)`, 86 insns, `RETF 8` | `pr_rec_ecl_compare_strings` | `[6F78..6F7D]`, with random prior flag bytes | 212 |
| `OVL_0025_18EB` | unit `25h` `+[18EB,1951)`, 36 insns, `RETF 4` | `pr_rec_ecl_compare_words` | `[6F78..6F7D]`, with random prior flag bytes | 428 |
| `OVL_0038_0377` | unit `38h` `+[0377,03AC)`, 15 insns, `RETF 0Ah` | `pr_rec_u38_entry8` | `[6816]`, `[6822]`, `[6818]`, `[6819]`; record `+A0h` over 0–9, 7Fh, 80h, FFh; the record must be unchanged | 221 |
| `OVL_0038_03AC` | unit `38h` `+[03AC,03E1)`, 15 insns, `RETF 0Ah` | `pr_rec_u38_entry9` | as above | 221 |
| `OVL_0038_03E1` | unit `38h` `+[03E1,0400)`, 13 insns, `RETF 0Ah` | `pr_rec_u38_entry10` | the four modifier bytes | 17 |
| `OVL_0038_0C06` | unit `38h` `+[0C06,0C15)`, 6 insns, `RETF 0Ah` | `pr_rec_u38_entry30` | the four modifier bytes | 17 |
| `OVL_0038_0C15` | unit `38h` `+[0C15,0C26)`, 6 insns, `RETF 0Ah` | `pr_rec_u38_entry31` | the four modifier bytes | 17 |

All 8,810 cases pass. These are semantic checks: each model is a contract-level translation and does not return a
CPU, so the comparison covers what the model states plus the whole data segment. It is not a CPU14 parity.

## Reproduce

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/ecl_helpers_original_audit.py --output resources/ecl-helpers-audit/report.json
python3 tools/overlay/ecl_helpers_original_audit.py --compile-only     # CHECKER_BUILD_OK, no media
```

`CHECKER_ROWS` lists all eleven rows, so `credit_evidence_kind.py` labels them `checker`. Media stays under the ignored
`resources/`; the report holds only counts and hashes.

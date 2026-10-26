# Candidate scout batch 3 - C-verified ECL handlers (OVL_001C_0292, OVL_001C_0EBC)

Status: **proposals, no credit, no accounting change.** Both C drafts are compiled and
checked against the original with the Unicorn differential, as the VP required. Neither
counts until a reviewer verdict accepts it.

## Candidates

| body | ECL | unit:offset | file | bytes | span SHA-256 |
| --- | --- | --- | ---: | ---: | --- |
| `OVL_001C_0292` | 0x09 SAVE | `1C:0292` | 4056 | 78 | `7f66f8be811fe835d3ed3ab0fc0772f57e10ab39b5b5beda8aa2542262f92ebb` |
| `OVL_001C_0EBC` | 0x35 SAVE TABLE | `1C:0EBC` | 7170 | 76 | `faa0e052825907531da7a6782f1cd0e6f8f4fc5a2cdc51e8629ed8fb19d0536b` |

Both spans end on `RETF`; both are far Pascal with no arguments. Each span is identified
by its digest above; the generators read the bytes from `GAME.OVR` sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` and no original bytes
are quoted in this document.

Models are raw-CPU C generated from the original bytes, one case per instruction:

- `include/poolrad/recovered/candidate_ecl_cpu.h` - shared CPU layout and helper-double API
- `src/recovered/candidate_ecl_doubles.c` - the five unit-0x25 helper doubles
- `src/recovered/overlay_u1c_save_0292_cpu.c` + header
- `src/recovered/overlay_u1c_save_table_0ebc_cpu.c` + header
- generators: `tools/overlay/emit_u1c_save_0292.py`,
  `tools/overlay/emit_u1c_save_table_0ebc.py` (shared template
  `tools/overlay/candidate_ecl_template.py`), both registered in
  `tools/overlay/test_emitters_reproduce_sources.py`

## The check that counts

```
$ python3 tools/overlay/u1c_save_cpu_audit.py 0292
OVL_001C_0292 file 4056 bytes 78 sha256 7f66f8be811fe835... calls 5
  AGREE save-word
  AGREE save-zero
  AGREE save-high-hi
  AGREE save-string
  AGREE save-string-empty
  AGREE save-string-long
mutant killed on 6/6 cases
PASS: 6 cases, 21 dispatches, full 1 MiB RAM + 14 CPU words

$ python3 tools/overlay/u1c_save_cpu_audit.py 0ebc
OVL_001C_0EBC file 7170 bytes 76 sha256 faa0e05282590753... calls 4
  AGREE table-basic
  AGREE table-zero-index
  AGREE table-wrap
  AGREE table-high-value
  AGREE table-index-ff
mutant killed on 4/5 cases
PASS: 5 cases, 25 dispatches, full 1 MiB RAM + 14 CPU words
```

What each audit does:

1. Locates the span from the hash-pinned `GAME.OVR`, asserts the extent and the `RETF`
   ending, and checks the far-call count and targets.
2. Runs the **original bytes** under Unicorn 16-bit on 1 MiB RAM with a far-caller frame.
3. Runs the **compiled C** model from the same initial RAM/CPU.
4. Routes each unit-0x25 far call to the **same helper double** on both sides. On the
   original side a Unicorn code hook lets the double perform the callee's `RETF` and Pascal
   argument cleanup; the C side calls the same compiled function.
5. Compares **all 1,048,576 RAM bytes, all 14 CPU words unmasked, the dispatch count, and
   the final `CS:IP:SP`**.
6. Compiles a mutated body (0292: the tag compare `0x80` -> `0x81`; 0EBC: the `add` ->
   `sub`) and requires it to differ, so the differential is shown able to fail.

Cases cover the word and string SAVE paths (including an empty and a long string), a
zero/high destination, and for SAVE TABLE the zero index, a 16-bit destination wrap, a high
value, and index `0xFF`.

## Verification ceiling

- The five helper doubles (`load_operands`, `operand_word`, `slot_value`, `store_word`,
  `store_string`) stand in for the real unit-0x25 routines. This verifies each handler
  body's control flow and argument marshalling **up to that boundary**, not the helpers.
- The dispatcher/caller that reaches these bodies is not exercised; the harness synthesises
  the far-caller frame and the operand block.
- No credit, strict-map, or accounting claim. A reviewer verdict is what can make either
  body count.

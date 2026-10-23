# RES_00BA pr_rec_* models: original-byte checker for eight credited bodies

`tools/overlay/res00ba_rec_models_audit.py` is a registered checker. It replaces a cited shared tool,
`resident_loader_map.py` (which proves byte provenance only), as the re-runnable evidence for eight credited resident
models (VP i21878, PM i21879). Each body keeps its own evidence document, which now links here from a "Reproduce"
section.

## What it checks

**Original side.** Each body runs from the ORIGINAL bytes of the loader-expanded START.EXE under Unicorn. The image is
hash-checked and loaded at base `1000h` with its relocation table applied. Each body's extent, instruction count and
`RETF` immediate are asserted from the bytes.

**C side.** The C model runs through the reusable io adapter, `tools/overlay/res00ba_io_adapter.py` with
`res00ba_io_adapter.c` (PM i21978: par-b's hybrid glue imports the same module). Its callbacks read and write the same
1 MiB RAM image at the exact segment:offset each model passes. DS globals come from the image's data segment.

**Compared, per call:**

- **The result:** AL for `23B7`, `2E03` and `2E46`; AX for `2637` and `2E8B`. Those two take AX in and out, keeping the
  caller's AH as the original does, so both sides start from the same random AX. The status must be OK.
- **Every RAM byte outside the call's own stack window.** These models are contract-level and keep no stack, while the
  original keeps its locals there, including `1509`'s StrLoad copy.
  - **The window, per body:** `[return address - 200h, return address + 4 + RETF bytes)`. The `RETF` size is asserted
    from that body's bytes, and the pushed argument bytes are checked equal to it.
  - **Why it stops there:** the arguments are pushed downward from `SS:F000h`, so the window ends exactly at the top of
    the arguments. The caller's frame above them is outside the window. It holds a non-zero sentinel, and that sentinel
    is compared, so an out-parameter store aimed at the caller's frame would be caught.
  - **The rule:** an exclusion window must be provably disjoint from the region the check is testing, not merely likely
    to be.

  | Body | RETF bytes | Excluded window (SS:) |
  | --- | ---: | --- |
  | `RES_00BA_1509` | 8 | `[EDF4, F000)` |
  | `RES_00BA_15F7` | 8 | `[EDF4, F000)` |
  | `RES_00BA_23B7` | 4 | `[EDF8, F000)` |
  | `RES_00BA_23DB` | 0 | `[EDFC, F000)` |
  | `RES_00BA_2637` | 4 | `[EDF8, F000)` |
  | `RES_00BA_2E03` | 4 | `[EDF8, F000)` |
  | `RES_00BA_2E46` | 4 | `[EDF8, F000)` |
  | `RES_00BA_2E8B` | 8 | `[EDF4, F000)` |

  The first version of this checker used a fixed `[SS:SP-200h, SS:SP+10h)`, which reached 10h bytes into the caller's
  frame. The PM caught it (i22001/i22004), and the per-body bound replaces it.
- **For `1509` and `15F7`, the declared boundaries in call order.**
  - On the original side they are intercepted at their entries. GetMem `0AF8:023F` returns the same caller-chosen block
    (`RETF 2`). FreeMem `0AF8:0254` and the message `0709:0BF5` are recorded and skipped (`RETF 6`). StrLoad `0AF8:0B0B`
    and Move `0AF8:1685` run from the original bytes and are recorded.
  - These are exactly the io boundaries the models declare. The literal's segment is compared relative to the load
    base.

| Row | Original | Model | Fixtures | Cases |
| --- | --- | --- | --- | ---: |
| `RES_00BA_1509` | `00BA:[1509,15F7)`, 238 bytes, 83 insns | `pr_rec_res_00ba_1509` | item list of 0–5 nodes; target null, absent, first, middle or last (both not-found messages) | 300 |
| `RES_00BA_15F7` | `00BA:[15F7,1693)`, 156 bytes, 53 insns | `pr_rec_res_00ba_15f7` | item list of 0–5 nodes; random 63-byte source; caller-chosen heap block | 300 |
| `RES_00BA_23B7` | `00BA:[23B7,23DB)`, 36 bytes, 13 insns | `pr_rec_res_00ba_23b7` | six random character records with cross-linked far-pointer fields | 400 |
| `RES_00BA_23DB` | `00BA:[23DB,2433)`, 88 bytes, 29 insns | `pr_rec_res_00ba_23db` | party list of 0–6 records from DS:5D96, random +10Dh/+10Eh, random DS:6780..687F | 400 |
| `RES_00BA_2637` | `00BA:[2637,267D)`, 70 bytes, 20 insns | `pr_rec_res_00ba_2637` | as 23B7, random entry AX | 400 |
| `RES_00BA_2E03` | `00BA:[2E03,2E46)`, 67 bytes, 23 insns | `pr_rec_res_00ba_2e03` | as 23B7, random DS globals | 400 |
| `RES_00BA_2E46` | `00BA:[2E46,2E8B)`, 69 bytes, 27 insns | `pr_rec_res_00ba_2e46` | as 2E03 | 400 |
| `RES_00BA_2E8B` | `00BA:[2E8B,2F57)`, 204 bytes, 69 insns | `pr_rec_res_00ba_2e8b` | as 23B7, four-byte output slot, random entry AX | 400 |

All 3,000 cases pass. These are semantic checks against contract-level models: no CPU state is returned, so they are not CPU14 parity.

## Reproduce

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/res00ba_rec_models_audit.py --output resources/res00ba-rec-audit/report.json
python3 tools/overlay/res00ba_rec_models_audit.py --quick          # ten cases per body
python3 tools/overlay/res00ba_rec_models_audit.py --compile-only   # CHECKER_BUILD_OK, no media
```

`CHECKER_ROWS` lists all eight rows, so `credit_evidence_kind.py` labels them `checker`.

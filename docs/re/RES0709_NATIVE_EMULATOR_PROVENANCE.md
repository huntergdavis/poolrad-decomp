# RES0709 native executable provenance

Task t108, batch 5. Candidate based on `f116c915a5dc07c909eac1a02da9170fd5fa1457`. This is a provenance-only change: strict C, fixtures, comparison windows, masks and existing oracle exclusions are unchanged. No recovery or replacement credit is requested.

The six native companions now resolve DOSBox-X once through `native_emulator_provenance.describe_dosbox`, launch the returned path and record executable path/version/SHA-256/pin status in `dosbox_x`.

| Provider | Scope |
| --- | --- |
| `res0709_clear_0dd3_cpu_native.py` | Existing native audit contract unchanged |
| `res0709_glyph_01df_cpu_native.py` | Existing native audit contract unchanged |
| `res0709_printer_0626_cpu_native.py` | Existing native audit contract unchanged |
| `res0709_rect_fill_00b7_cpu_native.py` | Existing native audit contract unchanged |
| `res0709_screen_0cab_cpu_native.py` | Existing native audit contract unchanged |
| `res0709_space_05eb_cpu_native.py` | Existing native audit contract unchanged |

## Verification protocol

Fresh before/after native comparisons are **not yet completed** on this candidate. The baseline providers are copied from the stated main revision, while both phases use the candidate's identical recovered C and audit fixtures. Each complete report must match after removing only `dosbox_x`; per-case capture hashes, existing exclusions and refusal outcomes remain part of that comparison. Partial runs are not passing receipts.

Each phase records provider and transitive C/header hashes plus executable identity. The candidate head and recorded input hashes must remain unchanged during the comparison. The later evidence document may add measured results without changing those inputs; its relationship to the native measurement head must be explicit.

The inventory delta removes exactly these six providers: main `190` command lists in `175` files becomes `184` lists in `169` files. These are structural CSV counts; the tracked-source AST census has not yet been rerun on this candidate.

`res0709_text_06a7_cpu_native.py` remains outside this batch and in the inventory. A prior unchanged-baseline attempt on base `c318fd47` stopped at case 51 because both C and installed DOS flags were `0406` where the harness expected its historical oracle split. That archived failure is not a fresh result or passing text comparison; no expectation, mask or text fixture was changed.

Private receipts and generated programs remain under ignored `resources/t108-batch5/`, with resumed measurements in `resume/`. All heavy native, census and media work uses the shared heavy-job wrapper. This document makes no fresh native, full-game, natural-route, full-RAM or pinned-binary claim.

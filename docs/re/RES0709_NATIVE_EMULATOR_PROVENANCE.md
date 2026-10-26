# RES0709 native executable provenance

Task t108 batch 5. Six native companions resolve DOSBox-X once through the shared helper, launch its returned path, and record executable identity in `dosbox_x`. Their `NOT_A_ROW_CHECKER` markers name the existing audit companions. Recovered C, fixtures, windows, masks, tracing, capture traps and oracle expectations are unchanged. No recovery or replacement credit is requested.

Native measurement head: `cb484a8500d9f0aa0b475107894d9cd6386ef6d9`. Baseline providers came from fresh main `f116c915a5dc07c909eac1a02da9170fd5fa1457`. Both phases used the same candidate C/header inputs and isolated output directories. All complete reports are identical after removing only `dosbox_x`; every native capture hash is identical. The frozen head and private input hashes were checked around each phase. This later document-only evidence commit does not constitute another native run.

| Fixture | Cases before / after | OUT events per phase |
| --- | ---: | ---: |
| `res0709_clear_0dd3_cpu` | 65 / 65 | 1857 |
| `res0709_glyph_01df_cpu` | 64 / 64 | 3060 |
| `res0709_printer_0626_cpu` | 59 / 59 | 5236 |
| `res0709_rect_fill_00b7_cpu` | 50 / 50 | 1100 |
| `res0709_screen_0cab_cpu` | 43 / 43 | 889 |
| `res0709_space_05eb_cpu` | 58 / 58 | 4930 |

Every listed case compares CPU14, 65,536 DS bytes and 262,144 EGA plane bytes, with zero flag masks. Additional declared windows:

| Fixture | Stack bytes | Font bytes | String bytes |
| --- | ---: | ---: | ---: |
| `res0709_clear_0dd3_cpu` | 256 | 0 | 0 |
| `res0709_glyph_01df_cpu` | 256 | 65536 | 0 |
| `res0709_printer_0626_cpu` | 1024 | 65536 | 65536 |
| `res0709_rect_fill_00b7_cpu` | 256 | 0 | 0 |
| `res0709_screen_0cab_cpu` | 256 | 0 | 0 |
| `res0709_space_05eb_cpu` | 1024 | 65536 | 0 |

These bounded snapshots do not cover all backing RAM or a natural game route. No full-RAM or full-game native claim is made.

Unchanged native capture SHA-256 values:

- `res0709_clear_0dd3_cpu`: `7111237d005d1a9c25f1129f9cecf05b2c5bc77d820a94c94dd22bee9010243f`
- `res0709_glyph_01df_cpu`: `25009fbb4c17593017cae26fa2f9845e450e20a224b957eb983679756529769a`
- `res0709_printer_0626_cpu`: `06eb18cd00537960fb196b88afaa31e6591293645dfad0fc4704798955d6522c`
- `res0709_rect_fill_00b7_cpu`: `fa0db1c025982f9ceba13ef16d63cc86c75598ca5671c12048461a50cdce1669`
- `res0709_screen_0cab_cpu`: `878124290686d505b773c5cecc36805cb2b7446efa9fe68bff6bf848bc0bfbf7`
- `res0709_space_05eb_cpu`: `502f7a76b2ef46529534c087cd7d136faf5df34eb62c0f9e9c19ab669800e1e9`

All six reports carry `result_sha256`, which hashes the capture, **not the original body**. None carries an original-body `source_sha256`/body-pin field. The separate receipt binds all **6 provider hashes and 27 recursively included C/header hashes**, checked across phases; those bindings are distinct from an original-body pin. The before/after reports include all existing report fields, not a selected subset.

Executable: `DOSBox-X version 2026.01.02 SDL2`, version exit status `1`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, `pinned_build_used: false`. Identity was checked before and after both phases. The reference source pin and release banner do not certify the installed binary.

The tracked-source AST census passed at the measurement head: main **190 command lists / 175 files**, minus exactly these six provider rows, leaves **184 / 169**. Synthetic test command lists are excluded. The final document-only head retains those same tools and CSV bytes; its media receipt is separate.

`res0709_text_06a7_cpu_native.py` remains excluded and in the inventory. The earlier unchanged-baseline attempt on `c318fd47` refused case51 at +06FD: C and installed DOS both had FLAGS0406 where the harness expected a historical oracle split. That failure is preserved under `resources/t108-batch5/failed-text-baseline/`; it was not rerun or repaired in this six-provider batch. No expectation or mask was relaxed.

Private receipts under ignored `resources/t108-batch5/resume/`: `inputs.json`, `scope.json`, `before/` and `after/` reports/executable/provider/model hashes, `source-census.json`, and comparison logs/exit receipts. Heavy native work used shared ticket783. No original game bytes or generated programs are committed. Media and reviewer results must be stated against their actual final head; this document makes no claim that those pending gates have passed.

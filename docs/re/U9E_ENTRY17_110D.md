# U9E public entry 17: three-pointer byte conversion at +110D

`pr_u9e_entry17_110d` translates the complete DOS v1.3 `GAME.OVR`
unit `0x9E` public entry 17. It takes far pointers A, B and C, clears A,
copies `C[3] & 0x7F` into B, and branches on B's resulting byte. At B
`<=0x65`, it writes B minus one to A and `0x12` to B. Above that threshold,
it subtracts `0x64` from B. The code rereads far-pointer operands with
`LES`; targets may alias argument words, so each access follows source order.

## Source and scope

`deja "Poolrad U9E +110D entry17 +1932 entry0 leaf original C parity"`
timed out locally. The entry identity and call counts come from the
[overlay function ledger](overlay_function_ledger.csv). Contiguous `LES`
operand reads across offset `FFFF` follow the already recovered
[U7F party count](U7F_05E0_NATIVE_REACH_FULLC_GATE.md) evidence.

| Evidence | Value |
| --- | --- |
| Original `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Entry 17 body | file `[0x2BFF7,0x2C046)`, U9E `+110D..+115B`; 79 bytes, 29 instructions; SHA-256 `63aaf31a1bb13ac3a652db1b1fef79afa6ddf2aee869fe426bc4bda7e5395830` |
| Identity | public entry 17; ledger grade A, three far and one near call references; no calls inside; `RETF 12` at `+1159` |

The translation executes all 79 bytes, including saved `BP`, every `LES`,
ES byte writes, final AX/flags and `RETF 12`. Invalid API inputs return
before memory mutation; this host guard is outside DOS parity.

`tests/recovered/check_overlay_u9e_entry17_110d.py` checks the original
span and compares all 1 MiB RAM bytes and all 14 CPU words against 16-bit
Unicorn at the far return across 110 synthetic cases. It exercises both arithmetic branches, A/B/C
aliases, an argument pointer altered by a prior write, separate segment:
offset aliases to the same physical byte, and each `LES` operand crossing
offset `FFFF`. `tests/recovered/test_overlay_u9e_entry17_110d.c` pins
ordinary branches and A/B alias behavior under strict C11 ASan/UBSan.

The Unicorn audit does not use incoming SP `FFFE` or `FFFF`, where the
previous U9E entry 16 review found an unreliable `RETF` return oracle.
`tools/overlay/u9e_entry17_110d_native.py` adapts that entry 16 native
harness and runs this exact original 79-byte body under DOSBox-X normal-core
386. The native executable is 12,192,432 bytes, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`.
SP `FFFE` and `FFFF` each pass with source byte `65` and `66`, covering both
arithmetic branches. All four returns match C for all 14 CPU words, 25
relevant stack bytes including the physical byte beyond SS:`FFFF`, and
three A/B/C data bytes. The generated COM is checked against the original
body hash; generated code, media and result files stay under ignored
`resources/`.

All frames are synthetic. Natural saved-game reach is unmeasured, and
function accounting remains with PM review.

## Adversarial parity review

| Field | Assessment |
| --- | --- |
| `target_function` | Original U9E `+110D..+115B`, public entry 17 |
| `source_candidates` | `pr_u9e_entry17_110d` |
| `signals` | Exact byte extent and hash; control flow and `RETF 12`; ordered `LES` alias effects; 110 full-state Unicorn comparisons; four native wrap comparisons |
| `score` | Five corroborating signals; no contradiction in the tested frames |
| `verdict` | `match` for the complete executable body under the tested synthetic frames |
| `review_notes` | The Unicorn oracle excludes SP `FFFE`/`FFFF`; DOSBox-X covers those placements. Native saved-game reach remains unmeasured. The native oracle compares all CPU words and affected stack/data bytes, while the Unicorn oracle compares full RAM. |

# Unit 0x3C2 public entry 3: bounded record step

The C model in `src/recovered/overlay_u3c2_entry3_02cc.c` executes the
complete original entry at `GAME.OVR` unit `0x3C2:+0x02CC..+0x0421`.
It is a byte-backed match for the bounded function, with no call substitute.
Its classification as a game routine remains **provisional**. No natural
saved-game caller frame was reached, and no strict count is claimed.

The bounded `deja "poolrad U3C2 +02CC entry 3 342 bytes full C parity
original bytes"` recall timed out (exit 124), so it supplied no reusable
claim. Reused evidence is the exact U3C2 row of
[overlay_function_ledger.csv](overlay_function_ledger.csv), the original-byte
comparison pattern in `tests/recovered/check_overlay_u9e_entry17_110d.py`
at commit `4b2c882`, and the DOSBox-X native frame pattern in
`tools/overlay/u9e_entry17_110d_native.py` at that same commit. The
original `GAME.OVR` bytes and all generated native files remain under
ignored `resources/`.

## Exact boundary and behavior

| Item | Measured value |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit code base | file `0x2FB52`, runtime stub segment `0x3DB` |
| Entry 3 body | unit `+0x02CC..+0x0422` exclusive; file `0x2FE1E..0x2FF74` exclusive; **342 bytes / 111 instructions** |
| Body SHA-256 | `818132a151e78bff884848a874203a09a50b6d459e0ad9f5e7278de692d28337` |
| Terminal instruction | `RETF 4` at `+0x041F`; next public entry 5 starts at `+0x0422` |
| Calls | none inside entry 3; ledger reports two far and two near incoming references |

The argument is one far record pointer at `[BP+6]`. The routine saves the
pointer in stack locals and repeatedly reloads it with `LES`. It compares
two signed word fields at record `+0x0A` and `+0x0C`; one of two directional
updates may advance word fields `+0x14` or `+0x16`, update word `+0x08`,
and increment byte `+0x18`. The signed bytes at `+0x12` and `+0x13`
form an index into `DS:[0x25C0 + index]`. That lookup byte is stored at
record `+0x19`, and AL returns whether a directional update occurred.
All reads and writes follow original instruction order. The C model preserves
16-bit offset wrap, physical 1 MiB wrap, stack-local and record aliases,
the final arithmetic FLAGS, and the `RETF 4` frame.

The local original disassembly of U3C2 entry 5 `+0x0422..+0x057A`
shows direct near calls to entry 3 at `+0x0541` and `+0x054D`.
These are static callers only. The broader caller chain and natural game
reach have not been measured.

## Parity report

| Field | Verdict |
| --- | --- |
| `target_function` | Original DOS v1.3 U3C2 public entry 3 `+0x02CC..+0x0421` |
| `source_candidates` | `pr_u3c2_entry3_02cc` in `src/recovered/overlay_u3c2_entry3_02cc.c` |
| `signals` | Exact 342-byte hash and 111-instruction bound; no internal calls; both signed branches, repeated far-pointer reads, DS lookup, stack writes, FLAGS and `RETF 4` match original execution |
| `score` | 88/88 Unicorn synthetic frames match all 1 MiB RAM bytes and 14 CPU words; 4/4 DOSBox-X normal-core synthetic frames match 14 CPU words, 65 stack bytes, and 26 record bytes |
| `verdict` | `match` for this bounded C/original-byte behavior; `unclear` for strict game-routine identity |
| `review_notes` | Synthetic frames include signed byte extremes, both update directions and stopped paths, stack-local and DS-table aliases, `SS:FFFF` far-pointer operand, and SP `FFFE`/`FFFF` returns. No gameplay-reached caller frame; no identity or count credit. |

The focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest
`poolrad_overlay_u3c2_entry3_02cc_tests` passes. The original-byte audit
is `tests/recovered/check_overlay_u3c2_entry3_02cc.py`; the native oracle
is `tools/overlay/u3c2_entry3_02cc_native.py`. The latter pins the
DOSBox-X normal 386 executable to 12,192,432 bytes and SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`.
The ignored native report is
`resources/u3c2-02cc-parity/native/report.json`.

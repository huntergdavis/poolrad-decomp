# U3B0 entry4 `+0DAF`: controlled SYSTEM-copy predecessor

The original U3B0 entry4 prefix makes far calls to SYSTEM
`0AF8:0B25` at `+0DAF` and `+0DC2`, then reaches the node-count leaf
call at `+0E06`. The controlled original-byte audit below executes
both copy calls, enters the original `+0067` leaf, observes its return
at `+0E09`, and stops at `+0E0C`. This resolves the immediate
predecessor call **for synthetic frames only**. No natural saved-game
frame, strict full-C identity, or count credit is claimed.

The pre-analysis `deja "poolrad U3B0 +0DAF 0AF8:0B25 entry4 predecessor
copy natural caller"` query timed out (exit 124) and supplied no
reusable claim. The source-address lead and copy semantics came from
the reviewed `docs/re/RESIDENT_0709_0626_POSTCOPY_CONTROL.md`; the
bounded successor approach came from U3B0 +0E06 commit
`f4c9e515f1d7408be2597e496a4f343bb98106cb`. All original bytes
and this entry4 path were checked independently.

| Original region | File span | Identity |
| --- | --- | --- |
| DOS v1.3 `GAME.OVR` | Entire file | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| DOS v1.3 `START.EXE` | Entire file | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| U3B0 entry4 `+0D9A..+0E0B` | `GAME.OVR [0x2D669,0x2D6DB)` | 114 bytes; SHA-256 `489519ea721f349921a6888b4172dc71c7ea692def347afc64dfc1afb0deea62` |
| SYSTEM `0AF8:0B25..0B48` | `START.EXE [0xB79B,0xB7BF)` | 36 bytes; SHA-256 `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` |
| U3B0 leaf `+0067..+00AA` | `GAME.OVR [0x2C936,0x2C97A)` | 68 bytes; SHA-256 `965144eab404415b7ce5d3f6e22d1c6991af4725dac559160302d89305468693` |

Entry4 copies Pascal strings from far pointers at `[BP+0x2A]` and
`[BP+0x26]` into SS locals `[BP-0x2A]` and `[BP-0x53]`, with a
40-byte cap for each. The original SYSTEM routine restores DS and
returns with `RETF 10`. For a non-null node head at `[BP+0x14/+0x16]`,
entry4 next computes a local byte, calls `+0067`, and stores the node
count at `[BP-0x56]`.

| Case | Source lengths | Copied lengths | Node count stored | Executed instructions |
| ---: | --- | --- | ---: | ---: |
| 1 | 0 / 0 | 0 / 0 | 1 | 96 |
| 2 | 5 / 9 | 5 / 9 | 2 | 120 |
| 3 | 44 / 60 | 40 / 40 | 3 | 198 |

All three controlled frames reached `+0DAF → 0AF8:0B25 → +0DB4`,
`+0DC2 → 0AF8:0B25 → +0DC7`, and `+0E06 → +0067 → +0E09 → +0E0C`.
Each case checks copy output bytes, DS restoration, expected SP at
both copy returns and the final stop, the count store, and that RAM
changes stay within the synthetic caller stack or the decoded DS
marker byte. The source strings and finite node chains are synthetic.

| Field | Verdict |
| --- | --- |
| `target_function` | U3B0 entry4 prefix `+0D9A..+0E0B`, through two SYSTEM copy calls and the `+0067` leaf |
| `source_candidates` | No new C candidate; original-byte composition only |
| `signals` | Exact hash-locked overlay, SYSTEM and leaf bodies; both decoded far calls reached and returned; bounded near caller reached |
| `score` | 3/3 controlled original-byte cases; both copies and the node-count store observed |
| `verdict` | `match` for the controlled original-byte route; `unclear` for natural gameplay reach |
| `review_notes` | Stops before `+0E0C` execution. No assertion about the rest of entry4, C parity, or full-C count. |

Reproduce with `PYTHONPATH=/tmp/poolrad-capstone python3
tools/overlay/u3b0_entry4_copy_predecessor_audit.py`. Its JSON report
and the game media stay under ignored `resources/`.

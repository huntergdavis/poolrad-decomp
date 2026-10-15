# Controlled U3B0 +0E06 node-count caller

This is a **controlled original-byte** exercise of U3B0 entry4's
`+0DFF..+0E0B` prefix. It proves that the decoded near call at `+0E06`
enters the original `+0067` node-count leaf, returns to `+0E09`, and
stores the returned AX word at `SS:[BP-0x56]`. It does **not** prove a
natural saved-game caller frame or strict full-C game-routine identity.

The pre-analysis `deja "poolrad U3B0 +0E06 +0067 near caller original
bytes node count immediate successor"` query timed out (exit 124) and
supplied no reusable claim. The original-byte identity and full-RAM
emulation method follows the reviewed U3B0 +0067 leaf at commit
`53c87573f3ead2b91ffa2e1590cedbcd60513146`; this caller path was
checked independently against DOS v1.3 `GAME.OVR`.

| Original region | File span | Evidence |
| --- | --- | --- |
| U3B0 `+0067..+00AA` | `[0x2C936,0x2C97A)` | 68 bytes, SHA-256 `965144eab404415b7ce5d3f6e22d1c6991af4725dac559160302d89305468693`, `RETF 4` |
| Entry4 `+0DFF..+0E0B` | `[0x2D6CE,0x2D6DB)` | 13 bytes, SHA-256 `d23c4f4e4838df0b499375696ce48731a564594277605903bee06c3ee1b1fdf8` |
| First entry4 dependency | `+0DAF` | Far call `0AF8:0B25` (`9A 25 0B F8 0A`) before the controlled start |

The caller pushes the segment and offset from `[BP+0x16/+0x14]`, pushes
CS, and makes a near call (`E8 5E F2`) at `+0E06`. The leaf's `RETF 4`
restores the incoming SP after returning to `+0E09`; that instruction
stores AX at `[BP-0x56]`. The audit stops at `+0E0C` before further
entry4 instructions or dependencies. It uses the exact original caller
and leaf bytes in a 1 MiB Unicorn machine with a synthetic caller frame
and finite node chains.

| Node count | Executed instructions | Returned AX | `[BP-0x56]` | SP restored |
| ---: | ---: | ---: | ---: | --- |
| 0 | 23 | 0 | 0 | Yes |
| 1 | 33 | 1 | 1 | Yes |
| 2 | 43 | 2 | 2 | Yes |
| 3 | 53 | 3 | 3 | Yes |

All four cases reached `+0E06 → +0067 → +0E09 → +0E0C`; all memory
changes stayed in the synthetic stack and caller's destination word.
The first missing dependency for an entry4 natural route is the
`0AF8:0B25` far call at `+0DAF`. The separate U3B0 `+07F9` route to
the `+0711` leaf remains blocked on its preceding `03F1:0137` call and
has no reached saved-game frame.

The reproducible audit is `tools/overlay/u3b0_node_count_call_0e06_audit.py`:

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u3b0_node_count_call_0e06_audit.py
```

The JSON report and original game media stay under ignored `resources/`.

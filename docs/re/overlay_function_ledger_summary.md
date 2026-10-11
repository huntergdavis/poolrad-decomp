# Overlay routine ledger summary (generated)

Inputs: START.EXE `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`, GAME.OVR `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Rows: 673 prologue candidates in 34 units; features via Capstone: yes; near-call evidence from validate_near_calls.py.

| grade | meaning | count |
| --- | --- | ---: |
| A | public entry slot on the prologue, or fixup-confirmed far call to it | 399 |
| B | decoded direct near-call target only | 269 |
| C | procedure-pointer constant target only | 0 |
| D | public entry alias (slot word = offset + 0x800) only | 5 |
| U | no listed evidence | 0 |

Overlapping evidence: entries on prologues 399, far-called 189, near-called 354, procedure-pointer targets 169 (208 constant sites; 1 unresolved), aliases 5.

| public entry aliases | unit | slot | raw word | normalized offset | raw word inside code? |
| --- | --- | ---: | --- | --- | --- |
| | 0x19 | 1 | 0x8fd | 0xfd | no |
| | 0x2f | 1 | 0x87a | 0x7a | yes |
| | 0x31 | 1 | 0x80f | 0xf | yes |
| | 0x36 | 1 | 0x82d | 0x2d | no |
| | 0x3d2 | 1 | 0x804 | 0x4 | no |

| measured class | count |
| --- | ---: |
| glue | 122 |
| glue-runtime-only | 16 |
| leaf | 51 |
| leaf-es | 19 |
| state | 465 |

Functions whose linear decode stops before the next prologue: 171; functions with indirect calls through memory: 3.

Entry stub fifth byte: 394 zero, 10 non-zero (unit, slot, entries): [('0x38', 116, 117), ('0x6c', 3, 4), ('0x77', 12, 13), ('0x9b', 4, 5), ('0x9e', 22, 23), ('0x3b0', 7, 8), ('0x3b9', 7, 8), ('0x3bd', 7, 8), ('0x3c2', 6, 7), ('0x3c6', 19, 20)].

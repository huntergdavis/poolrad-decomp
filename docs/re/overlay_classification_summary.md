# Overlay classification summary (generated)

Inputs: GAME.OVR `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, START.EXE `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`; 673 candidates; grades joined from the function ledger.

| classification | confidence | count |
| --- | --- | ---: |
| game_verified | verified | 62 |
| game_evidenced | provisional | 302 |
| helper_candidate | provisional | 82 |
| game_or_other_library_unresolved | unresolved | 227 |

Signals (not mutually exclusive): string constants 216 routines (664 references), documented engine globals 248, calls into verified routines 55, leaves 70, runtime-only 16.

| class | grade A | grade B | grade D |
| --- | ---: | ---: | ---: |
| game_verified | 6 | 55 | 1 |
| game_evidenced | 183 | 118 | 1 |
| helper_candidate | 60 | 22 | 0 |
| game_or_other_library_unresolved | 153 | 74 | 0 |

Coverage: 196236 of 197451 unit code bytes (99.4%) reached by the bounded decode; 184 uncovered ranges (166 of 8 bytes or less; 712 bytes precede a unit's first prologue); 0 uncovered ranges decode as a self-contained return-terminated routine; 0 jump-table dispatch sites.

Largest uncovered ranges (unit, offset, size): 0x19:0x0000 253, 0x86:0x0000 138, 0x77:0x0000 74, 0x40c:0x0000 59, 0x3b0:0x0385 56, 0x23:0x0000 48, 0x36:0x0000 45, 0x9e:0x0abb 44.

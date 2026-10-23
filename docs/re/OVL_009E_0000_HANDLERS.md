# OVL_009E_0000: the dispatcher's handler set, enumerated and priced (VP ruling i21629; measurement only, no model)

The ruling: model every handler the credited installer  stores, state the set handler by handler, name a gap per handler that cannot be modelled, and **price it before building**. This document is the enumeration and the price. **It builds nothing and proposes no credit.**

START.EXE SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd; GAME.OVR SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614. The queue export was regenerated on origin/main () for the prices.

## The dispatcher

 is 38 bytes, , stub :

    0000 push bp / mov bp,sp
    0003 push [bp+0Eh] / push [bp+0Ch] / push [bp+0Ah] / push [bp+8]   (four argument words, forwarded)
    000F mov al,[bp+6] / push ax                                       (a byte, forwarded)
    0013 mov al,[bp+10h] / xor ah,ah / mov di,ax / shl di,1 / shl di,1 (DI = 4 * the index byte)
    001E lcall [di+6828h]                                              (the only dispatch)
    0022 pop bp / retf 000Ch

The index is **one unsigned byte, so the domain is 256 slots (6828h..6C27h)**. The callee receives the forwarded arguments (RETF 0Ah handlers: five words) and returns to the dispatcher.

## What  stores (all identified)

 makes 138 far-pointer installs at DS:682Ch..6A57h (**slots 1..139, not slot 88**); every one resolves, through the loader's stub table, to an overlay entry. They are **117 distinct functions**, 115 in unit 0x38 and 2 in unit 0x5E. **64 are strict-credited; 53 are not.** Nothing  stores is unidentifiable, so the set is **not open from 's side**; the ruling's stop condition is not triggered by  itself.

## What  does not store (the domain is wider than the ruling's set)

| Slots | Address | Who writes it | Consequence |
| --- | --- | --- | --- |
| 88 | 6988h..698Bh | **unit 0x86 +37A7/+37AA** (the  initializer, which runs before ; recorded in ): the far pointer , **unit 0x86 entry 63 = ** | one more handler (the 118th), **not credited, closure 17,602 B** |
| 0 | 6828h/682Ah | not a table entry:  is a **far-pointer variable** written by  (+014E/+0151) and  (+3AA3/+3AA6) | index 0 would call the variable's (offset, high word), which is not a handler; no stored handler exists |
| 140..255 | 6A58h..6C27h | **other game state** (a linear sweep of every overlay finds 108 distinct writer addresses in 6A58h..6C27h that are 1- and 2-byte variables and far-pointer variables, e.g. unit 0x36 at 6A58h/6A5Ch, 6B80h.. onward) | an index of 140 or more would call garbage |

## The callers' index (what is reached in practice)

Eleven far call sites () and five near ones (inside unit 0x9E). Of the 11 far sites, **8 pass a constant** as the first pushed byte ( x2, , , ,  x2, ) and **3 pass a record byte read from memory** (unit 0x38 : ; unit 0x77  and : ). **The domain is therefore not statically bounded by the callers**; an in-range value is a game invariant, not something the bytes prove.

## Price (per handler; , closure bytes and bodies, floors)

**The 53 not-credited handlers carry closures from 55 B to 20,654 B; 6 are at most 1,000 B, 17 are 1,000 to 3,000 B, 30 are above 3,000 B. 34 of the 53 reach  or , whose own closures dispatch through : the handler closures are CYCLIC** (the dispatcher is part of its own handlers' closures). Closures overlap heavily (13-20 KB shared sub-closures), so the sum over handlers (424,482 B) is **not** a cost; the union was not computed here.

| slot(s) | stub | function | own B | status | closure B (bodies) | dispatch cycle through 009E |
| --- | --- | --- | ---: | --- | ---: | --- |
| 1 | 0041:002A | OVL_0038_010D | 16 | strict-credited | - | - |
| 2 | 0041:002F | OVL_0038_011D | 30 | strict-credited | - | - |
| 3 | 0041:0034 | OVL_0038_013B | 68 | strict-credited | - | - |
| 4 | 0041:0039 | OVL_0038_017F | 68 | not credited | 1335 (5) | no |
| 5,13,19,24,27,60,63,92,127 | 0041:0264 | OVL_0038_319B | 7 | strict-credited | - | - |
| 6 | 0041:003E | OVL_0038_01C3 | 112 | strict-credited | - | - |
| 7 | 0041:0043 | OVL_0038_0233 | 324 | not credited | 1591 (5) | no |
| 8,45 | 0041:0048 | OVL_0038_0377 | 53 | strict-credited | - | - |
| 9,46 | 0041:004D | OVL_0038_03AC | 53 | strict-credited | - | - |
| 10 | 0041:0052 | OVL_0038_03E1 | 31 | strict-credited | - | - |
| 11 | 0041:0057 | OVL_0038_0400 | 186 | not credited | 186 (1) | no |
| 12,38 | 0041:005C | OVL_0038_04BA | 284 | strict-credited | - | - |
| 14 | 0041:0061 | OVL_0038_05D6 | 21 | strict-credited | - | - |
| 15 | 0041:0066 | OVL_0038_05EB | 92 | not credited | 15137 (38) | yes |
| 16 | 0041:006B | OVL_0038_0647 | 7 | strict-credited | - | - |
| 17 | 0041:0070 | OVL_0038_064E | 43 | strict-credited | - | - |
| 18 | 0041:0075 | OVL_0038_0679 | 107 | strict-credited | - | - |
| 20 | 0041:007A | OVL_0038_06E4 | 43 | strict-credited | - | - |
| 21 | 0041:007F | OVL_0038_070F | 102 | not credited | 1369 (5) | no |
| 22 | 0041:0084 | OVL_0038_0775 | 107 | not credited | 13344 (31) | yes |
| 23 | 0041:0089 | OVL_0038_07E0 | 312 | not credited | 1579 (5) | no |
| 25 | 0041:008E | OVL_0038_0918 | 47 | strict-credited | - | - |
| 26 | 0041:0093 | OVL_0038_0947 | 121 | strict-credited | - | - |
| 28 | 0041:0098 | OVL_0038_09C0 | 125 | not credited | 1706 (6) | yes |
| 29 | 0041:009D | OVL_0038_0A3D | 40 | strict-credited | - | - |
| 30 | 0041:00A2 | OVL_0038_0A65 | 182 | not credited | 2929 (10) | no |
| 31,51,52,53 | 0041:00F7 | OVL_0038_133F | 22 | strict-credited | - | - |
| 32 | 0041:00A7 | OVL_0038_0B1B | 145 | not credited | 13382 (31) | yes |
| 33 | 0041:00AC | OVL_0038_0BAC | 35 | strict-credited | - | - |
| 34 | 0041:00B1 | OVL_0038_0BCF | 55 | not credited | 55 (1) | no |
| 35 | 0041:00B6 | OVL_0038_0C06 | 15 | strict-credited | - | - |
| 36 | 0041:00BB | OVL_0038_0C15 | 17 | strict-credited | - | - |
| 37 | 0041:00C0 | OVL_0038_0C26 | 37 | strict-credited | - | - |
| 39 | 0041:00C5 | OVL_0038_0C4B | 120 | not credited | 1387 (5) | no |
| 40 | 0041:00CA | OVL_0038_0CC3 | 745 | not credited | 745 (1) | no |
| 41 | 0041:00CF | OVL_0038_1086 | 59 | not credited | 1659 (7) | no |
| 42 | 0041:00D4 | OVL_0038_10C1 | 29 | strict-credited | - | - |
| 43 | 0041:00D9 | OVL_0038_10DE | 138 | not credited | 1405 (5) | no |
| 44 | 0041:00DE | OVL_0038_1168 | 145 | not credited | 15190 (38) | yes |
| 47 | 0041:00E3 | OVL_0038_11F9 | 125 | strict-credited | - | - |
| 48 | 0041:00E8 | OVL_0038_1276 | 60 | strict-credited | - | - |
| 49 | 0041:00ED | OVL_0038_12B2 | 73 | strict-credited | - | - |
| 50 | 0041:00F2 | OVL_0038_12FB | 68 | not credited | 2320 (10) | yes |
| 54 | 0041:00FC | OVL_0038_1355 | 7 | strict-credited | - | - |
| 55 | 0041:0101 | OVL_0038_135C | 7 | strict-credited | - | - |
| 56 | 0041:0106 | OVL_0038_1363 | 31 | strict-credited | - | - |
| 57 | 0041:010B | OVL_0038_1399 | 185 | not credited | 13422 (31) | yes |
| 58 | 0041:0110 | OVL_0038_1452 | 32 | strict-credited | - | - |
| 59 | 0041:0115 | OVL_0038_1472 | 30 | strict-credited | - | - |
| 61 | 0041:011A | OVL_0038_1490 | 91 | strict-credited | - | - |
| 62 | 0041:011F | OVL_0038_14EB | 90 | not credited | 2342 (10) | yes |
| 64 | 0041:0124 | OVL_0038_165D | 20 | not credited | 13551 (33) | yes |
| 65 | 0041:0129 | OVL_0038_1671 | 20 | not credited | 13551 (33) | yes |
| 66 | 0041:012E | OVL_0038_1685 | 20 | not credited | 13551 (33) | yes |
| 67 | 0041:0133 | OVL_0038_1699 | 34 | not credited | 8945 (21) | yes |
| 68 | 0041:0138 | OVL_0038_16BB | 43 | not credited | 8954 (21) | yes |
| 69 | 0041:013D | OVL_0038_16E6 | 37 | not credited | 8948 (21) | yes |
| 70 | 0041:0142 | OVL_0038_170B | 20 | not credited | 13551 (33) | yes |
| 71 | 0041:0147 | OVL_0038_171F | 17 | strict-credited | - | - |
| 72 | 0041:014C | OVL_0038_1730 | 46 | strict-credited | - | - |
| 73 | 0068:008E | OVL_005E_39D2 | 180 | not credited | 20565 (49) | yes |
| 74 | 0068:0093 | OVL_005E_3A86 | 269 | not credited | 20654 (49) | yes |
| 75 | 0041:0151 | OVL_0038_176F | 326 | not credited | 14763 (35) | yes |
| 76 | 0041:0156 | OVL_0038_18B5 | 128 | not credited | 128 (1) | no |
| 77 | 0041:015B | OVL_0038_1935 | 164 | not credited | 1431 (5) | no |
| 78 | 0041:0160 | OVL_0038_19D9 | 64 | not credited | 9297 (23) | no |
| 79 | 0041:0165 | OVL_0038_1A19 | 51 | not credited | 14488 (35) | yes |
| 80 | 0041:016A | OVL_0038_1A4C | 94 | not credited | 14531 (35) | yes |
| 81 | 0041:016F | OVL_0038_1AAA | 254 | not credited | 9628 (22) | yes |
| 82 | 0041:0174 | OVL_0038_1BA8 | 261 | not credited | 9635 (22) | yes |
| 83 | 0041:0179 | OVL_0038_1CAD | 458 | not credited | 15749 (37) | yes |
| 84 | 0041:017E | OVL_0038_1E77 | 395 | not credited | 11472 (24) | yes |
| 85 | 0041:0183 | OVL_0038_23A4 | 30 | not credited | 13914 (33) | yes |
| 86 | 0041:0188 | OVL_0038_23C2 | 42 | not credited | 13926 (33) | yes |
| 87 | 0041:018D | OVL_0038_23EC | 164 | not credited | 1431 (5) | no |
| 89 | 0041:0192 | OVL_0038_2490 | 73 | strict-credited | - | - |
| 90 | 0041:0197 | OVL_0038_24D9 | 125 | strict-credited | - | - |
| 91 | 0041:019C | OVL_0038_2556 | 76 | not credited | 1343 (5) | no |
| 93 | 0041:01A1 | OVL_0038_25A2 | 26 | strict-credited | - | - |
| 94 | 0041:01A6 | OVL_0038_25BC | 73 | strict-credited | - | - |
| 95 | 0041:01AB | OVL_0038_2610 | 59 | not credited | 13296 (31) | yes |
| 96 | 0041:01B0 | OVL_0038_264B | 63 | strict-credited | - | - |
| 97 | 0041:01B5 | OVL_0038_268A | 118 | strict-credited | - | - |
| 98 | 0041:01BA | OVL_0038_2700 | 48 | strict-credited | - | - |
| 99 | 0041:01BF | OVL_0038_2730 | 169 | not credited | 9716 (24) | yes |
| 100 | 0041:01C4 | OVL_0038_27D9 | 59 | strict-credited | - | - |
| 101 | 0041:01C9 | OVL_0038_2814 | 82 | strict-credited | - | - |
| 102 | 0041:01CE | OVL_0038_2866 | 89 | not credited | 9322 (23) | no |
| 103 | 0041:01D3 | OVL_0038_28BF | 66 | not credited | 9671 (25) | yes |
| 104 | 0041:01D8 | OVL_0038_2901 | 20 | not credited | 1620 (7) | no |
| 105 | 0041:01DD | OVL_0038_2976 | 14 | strict-credited | - | - |
| 106 | 0041:01E2 | OVL_0038_2984 | 14 | strict-credited | - | - |
| 107 | 0041:01E7 | OVL_0038_2992 | 36 | strict-credited | - | - |
| 108 | 0041:01EC | OVL_0038_29B6 | 21 | strict-credited | - | - |
| 109 | 0041:01F1 | OVL_0038_29CB | 14 | strict-credited | - | - |
| 110 | 0041:01F6 | OVL_0038_29D9 | 23 | strict-credited | - | - |
| 111 | 0041:01FB | OVL_0038_29F0 | 33 | strict-credited | - | - |
| 112 | 0041:0200 | OVL_0038_2A11 | 23 | strict-credited | - | - |
| 113 | 0041:0205 | OVL_0038_2A28 | 69 | strict-credited | - | - |
| 114 | 0041:020A | OVL_0038_2A6D | 26 | strict-credited | - | - |
| 115 | 0041:020F | OVL_0038_2A87 | 95 | strict-credited | - | - |
| 116 | 0041:0214 | OVL_0038_2AE6 | 58 | strict-credited | - | - |
| 117 | 0041:0219 | OVL_0038_2B20 | 67 | strict-credited | - | - |
| 118 | 0041:021E | OVL_0038_2B63 | 26 | strict-credited | - | - |
| 119 | 0041:0223 | OVL_0038_2B7D | 75 | strict-credited | - | - |
| 120 | 0041:0228 | OVL_0038_2BC8 | 84 | not credited | 1684 (7) | no |
| 121 | 0041:022D | OVL_0038_2C1C | 284 | not credited | 16543 (38) | yes |
| 122 | 0041:0232 | OVL_0038_2D38 | 80 | strict-credited | - | - |
| 123 | 0041:0237 | OVL_0038_2D88 | 92 | strict-credited | - | - |
| 124 | 0041:023C | OVL_0038_2DE4 | 36 | strict-credited | - | - |
| 125 | 0041:0241 | OVL_0038_2E08 | 47 | strict-credited | - | - |
| 126 | 0041:0246 | OVL_0038_2E37 | 163 | strict-credited | - | - |
| 128,129,130,133,134,136,138,139 | 0041:024B | OVL_0038_2EDA | 113 | not credited | 427 (2) | yes |
| 131 | 0041:0250 | OVL_0038_2F57 | 334 | not credited | 1915 (6) | yes |
| 132 | 0041:0255 | OVL_0038_30A5 | 143 | not credited | 14852 (37) | yes |
| 135 | 0041:025A | OVL_0038_3134 | 70 | strict-credited | - | - |
| 137 | 0041:025F | OVL_0038_317A | 33 | not credited | 347 (2) | yes |
| 88 | 0096:015B | OVL_0086_30D9 | ? | not credited | 17602 (42) | see note |

( is the ledger bound; slot 88's own bytes were not read here. A credited handler needs no new body, only composition.)

## What this means for the ruling

1. **The literal ruling is a program, not a body.** 54 uncredited handlers (the 53 plus slot 88), mostly with 9 to 20 KB closures that overlap and **loop back through this dispatcher**: a fixed-point composition, not a leaf-by-leaf build.
2. **A bounded, honest first body exists:** model  with the **64 credited handlers composed**, and give **each of the 54 uncredited handlers (and slot 0, and the slots 140..255) its own named stop** (a CALLEE-class stop that carries the slot and the handler id, never a silent stop at the ). That satisfies "a named gap per handler" at the cost of one 38-byte body plus a table of 64 compositions; it does **not** make the 54 modelled.
3. **The domain needs a declared contract:** index in 1..139 (and 88 via unit 0x86); outside that the original calls a non-handler. The three data-dependent callers mean this is a precondition, not a proven range.

**Decision wanted (the VP's): (a) the bounded body in point 2, with the 54 named gaps; or (b) fund the 54 handlers first (priced above; a fixed-point build), or (c) hold.** Nothing was built.

## Reproduction

    python3 tools/overlay/strict_candidate_queue.py --inventory resources/function-accounting-current/inventory.json --callgraph resources/function-accounting-current/callgraph.json --near-calls resources/function-accounting-current/near.json --ovr resources/DOS/POOLRAD/GAME.OVR --repo . --json queue.json
    python3 tools/overlay/price_row_closures.py --queue queue.json --exe resources/DOS/POOLRAD/START.EXE <the 53 ids in the table>

The enumeration reads the installer's operands through  and the unit stub table; the writer and caller scans are linear sweeps of every overlay unit (a fixup-free  into 6828h..6C27h and ), so a write through a computed pointer would not appear. The prior-session memory tool () was unavailable (ENOENT); nothing was reused from earlier sessions.

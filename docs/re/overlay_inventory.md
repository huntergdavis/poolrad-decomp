# Overlay inventory

START.EXE sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` (65231 bytes), MZ relocations 0, header 512 bytes, resident image 64719 bytes.

GAME.OVR sha256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` (206105 bytes), FBOV payload 206097 bytes.

Units 34, entry points 404, code 197451 bytes, fixup tables 8442 bytes, prologue functions 673 (prologues anywhere in file: 675). Units tile the file from 212 to 206105 of 206105; gaps: 0.

Memory seg = the paragraph each unit's stub segment occupies at run time, normally read from the next unit's header; the final unit 0x40c is inferred as 0x428 from late descriptor spacing and seven fixup-confirmed calls to its stubs. Entry j is called as memory seg:(0x20 + 5*j). The sequential 32-byte-header formula reproduces it for 29 of 33 checkable units (failures mark resident descriptor gaps).

| stub para (file) | memory seg | ovr offset | code | fixups | entries | functions | fixup kinds (lcall/mov ax/other) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 0x6c | 0x78 | 104831 | 17497 | 263 | 4 | 19 | 263/0/0 |
| 0x5e | 0x68 | 75771 | 15474 | 303 | 25 | 39 | 303/0/0 |
| 0x38 | 0x41 | 60507 | 14510 | 377 | 117 | 127 | 239/0/138 |
| 0x86 | 0x96 | 159768 | 14261 | 349 | 64 | 74 | 280/0/69 |
| 0x1c | 0x1f | 3398 | 13581 | 402 | 2 | 56 | 402/0/0 |
| 0x77 | 0x85 | 135226 | 11026 | 320 | 13 | 21 | 320/0/0 |
| 0x6f | 0x7c | 122854 | 9189 | 361 | 12 | 14 | 361/0/0 |
| 0x7f | 0x8f | 150665 | 8795 | 154 | 15 | 22 | 154/0/0 |
| 0x25 | 0x2b | 29907 | 8730 | 141 | 25 | 33 | 141/0/0 |
| 0x6a | 0x75 | 96305 | 8158 | 184 | 3 | 23 | 184/0/0 |
| 0x33 | 0x3b | 50641 | 8082 | 56 | 2 | 23 | 56/0/0 |
| 0x9e | 0xb0 | 175850 | 6455 | 87 | 23 | 24 | 87/0/0 |
| 0x31 | 0x38 | 43968 | 6439 | 117 | 2 | 15 | 117/0/0 |
| 0x20 | 0x25 | 21886 | 5750 | 112 | 2 | 12 | 112/0/0 |
| 0x3c6 | 0x3e0 | 198458 | 4864 | 25 | 20 | 23 | 25/0/0 |
| 0x2f | 0x35 | 38919 | 4735 | 157 | 2 | 14 | 155/0/2 |
| 0x3b0 | 0x3c5 | 182479 | 4682 | 55 | 8 | 20 | 55/0/0 |
| 0x3bd | 0x3d5 | 190711 | 4619 | 40 | 8 | 13 | 40/0/0 |
| 0x68 | 0x72 | 91851 | 4268 | 93 | 3 | 14 | 93/0/0 |
| 0x1e | 0x22 | 17783 | 3895 | 104 | 2 | 12 | 104/0/0 |
| 0x7d | 0x8c | 146892 | 3685 | 44 | 3 | 15 | 44/0/0 |
| 0x3c2 | 0x3db | 195410 | 3038 | 5 | 7 | 8 | 5/0/0 |
| 0x75 | 0x82 | 132765 | 2299 | 81 | 3 | 4 | 81/0/0 |
| 0x17 | 0x19 | 212 | 2062 | 101 | 2 | 4 | 101/0/0 |
| 0x3b9 | 0x3d0 | 188622 | 1989 | 50 | 8 | 10 | 50/0/0 |
| 0x23 | 0x28 | 27860 | 1949 | 49 | 3 | 5 | 49/0/0 |
| 0x3ce | 0x3e9 | 203372 | 1605 | 55 | 6 | 6 | 55/0/0 |
| 0x36 | 0x3e | 58835 | 1562 | 55 | 2 | 2 | 55/0/0 |
| 0x9b | 0xac | 174727 | 1123 | 0 | 5 | 5 | 0/0/0 |
| 0x19 | 0x1c | 2476 | 862 | 30 | 2 | 2 | 30/0/0 |
| 0x40c | 0x428 | 205365 | 690 | 25 | 3 | 3 | 25/0/0 |
| 0x3b4 | 0x3ca | 187271 | 677 | 6 | 3 | 6 | 6/0/0 |
| 0x3b6 | 0x3cd | 187960 | 648 | 7 | 3 | 3 | 7/0/0 |
| 0x3d2 | 0x3ee | 205087 | 252 | 13 | 2 | 2 | 13/0/0 |

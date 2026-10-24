# OVL_009E +0000: can the INDEX byte be 0 or 140..255? (reachability of the refused domain)

**Verdict: not provable from the code alone, and not claimed.** The dispatcher model is **safe for every index**: 0 and 140..255 are REFUSED with CPU, RAM and budget untouched (proven by the composed differential and the C test, `OVL_009E_0000_CPU.md`), so this argument decides **credit scope** (par-e's ruling: the refused domain must be shown unreachable from the call sites), not correctness. What follows is every route by which an index reaches the dispatcher, what is proved for each, and what is **not established**.

Tool (read-only, no emulation, no heavy slot; needs the ignored game media): `python3 tools/overlay/u9e_index_sites.py {sites | callers STUB K NEAR | writers DISP | tables | items}`. Every number below is reproducible with the command beside it.

## 1. The sixteen direct call sites (`u9e_index_sites.py sites`)

The index is the **first** of the six pushed arguments, so it is the byte loaded into AL before the first `PUSH AX`.

| Site | Index supplied by | Value / reach |
| --- | --- | --- |
| far `0038+0BE5` | `MOV AL,2Bh` | 43, in range |
| far `0038+0BFD` | `MOV AL,2Ch` | 44, in range |
| far `0038+191F`, `+19AC` | `MOV AL,3Ah` | 58, in range |
| far `0038+1FF7` | `MOV AL,0Bh` | 11, in range |
| far `0086+1290` | `MOV AL,0Bh` | 11, in range |
| far `0086+18E3` | `MOV AL,4Eh` | 78, in range |
| far `0086+1A00` | `MOV AL,17h` | 23, in range |
| near `009E+0BF7`, `+0C46` | `MOV AL,1Eh` / `1Fh` | 30, 31, in range |
| near `009E+1838` | `MOV AL,4Eh` | 78, in range |
| far `0038+2F40` | item record byte `+3Dh` (`MOV AL,ES:[DI+3Dh]`) | **data** (section 3a) |
| far `0077+1543`, `+1663` | item record byte `+3Eh`, guarded `> 7Fh` | **data**, 128..255 only (section 3b) |
| near `009E+00A1` | parameter `[BP+0Ah]` of function `+0026` | **forwarded** (section 2) |
| near `009E+02F1` | parameter `[BP+6]` of function `+0160` | **forwarded** (section 2) |

**11 far and 5 near sites; 11 of the 16 are immediates and all 11 are in 1..139.**

## 2. The two forwarding functions

* **Function `+0160`** (near-called; its `[BP+6]` is the index it forwards at `+02F1`): `u9e_index_sites.py callers FFFF 1 160` finds **140 near call sites, all 140 pass an immediate, min 1, max 126, none outside 1..139**. This route is closed for **direct near calls** (the scan reads `E8` calls inside unit 0x9E; `+0160` is not a public entry and the byte scan found no fixup word referring to it, but a computed call is not excluded by a byte scan): the refused domain is unreachable through `+0160`.
* **Function `+0026`** (public entry, stub `00B0:002A`; the list-node removal; it dispatches at `+00A1` with its `[BP+0Ah]` only for a matching node whose byte `+4` is nonzero, see `U9E_LIST_RELEASE_0026_CPU.md`): `callers 2A 3 26` finds **36 callers, 28 immediates (15..121, all in range) and 8 that are not constants**:

| Caller | Supplies | Disposition |
| --- | --- | --- |
| `001E+03B1` | `DS:[DI+113h]`, `DI` = 1..6 | table, section 4: 31, 34, 43, 44, 50, 57 (in range) |
| `009E+1032` | `DS:[DI+0C13h]`, `DI` = 1..16 | table, section 4: all in range (one is 137) |
| `009E+1069` | `DS:[DI+0C23h]`, `DI` = 1..2 | table, section 4: 74, 75 (in range) |
| `0038+2F06` | item record byte `+3Dh` | data, section 3a |
| `007D+01FE`, `0086+2481` | `ES:[DI]`: byte 0 of a list node | closure over node types, section 5 |
| `009E+10D4` | `[BP+6]` of function `+1080` | 9 callers: **8 immediates (12..55, in range)** and 1 forward (`0086+27DF`, `[BP+0Ch]`) |
| `009E+16FD` | `[BP+14h]` of function `+1656` | 9 callers, section 5: includes **immediates 0 and 254** |

## 3. Record bytes `+3Dh` and `+3Eh` (item effect ids)

The records are the 63-byte item structures (`CHRDATA*.ITM`, `MON*ITM.DAX` are arrays of them): `+3Ch`, `+3Dh`, `+3Eh` are item effect bytes (`U77_STACK_189D.md` already compares all three).

* **Direct writers in any unit's code** (`writers 3d`, `writers 3e`): `0038+08BA` stores `+3D = 17h` (23), `006C+1CEF` stores `+3D = 01h`, `0038+08C2` stores `+3E = 89h` (**137, in range**). There is no other direct store; every other value arrives by data (file read or record copy).
* **Shipped data** (`items`): 28 items in the `CHRDATA*.ITM` files and 301 in `MON*ITM.DAX` (329 in all): **`+3E` is 0 (318) or 45 (1) or 129, 131, 133, 135 (10); `+3D` is 0 or 3..89; no effect byte of any shipped item is 140 or above.** (Zero is the "no effect" value of the 303 plain items; whether a caller guards it before dispatching is open for `+3D`, see 3a.)

### 3a. `0038+2F40` / `+2F06` (`+2EDA`, `+3Dh`)

`+2EDA` (installed at types 128..139) reads `ES:[DI+3Dh]` of the item it is given and dispatches it with **no range test and no zero test** (`U38_DISPATCH_BOUNDARY_2EDA.md`). The item arrives as the dispatcher's own forwarded argument, so this is the recursive route: an item whose `+3D` is 0 or 140..255, once its slot-128..139 effect fires, reaches the dispatcher. **Not provable from the code**: it is a property of the item data.

### 3b. `0077+1543` / `+1663` (`+3Eh`)

`+14D0` computes `[BP-1] = (ES:[DI+3Eh] > 7Fh)` and dispatches only when it is set, so **index 0 is excluded by the code** and the reachable indices are 128..255; the in range part is 128..139. Values 140..255 need an item whose `+3E` is 140 or above: none is shipped, and the only direct code store is 137.

## 4. The three DS tables (`tables`)

The program data segment is paragraph `0C7C` of the expanded image. `DS:0113h+i` (i = 1..6) is `31 34 43 44 50 57`; `DS:0C13h+i` (i = 1..16) is `7 11 30 31 32 51 52 53 54 58 59 95 98 137 74 75`; `DS:0C23h+i` (i = 1..2) is `74 75`. **All 24 values are in 1..139.** A scan for stores into those ranges (resident code and every unit) finds 19 candidates, **none DS-relative**: all carry an ES override (character-record fields `+114h..+119h`). The scan is a decode-resynchronised sweep, not a proof of absence: a block copy over the table (`REP MOVS`) is not looked for.

## 5. Node types and the removal chain (the part that does not close)

Function `+1656` (`callers 84 5 1656`) takes a type byte at `[BP+14h]` and forwards it at `+16FD` to `+0026`; the list append `+0E52` (`callers 52 4 E52`) writes the type of every node it creates. **Immediates passed to the removal chain include 254 (`0038+1FC5`) and 0 (`0086+21E7`).** Whether the dispatcher is then reached with 254 or 0 depends on whether a node of that type with a nonzero byte `+4` exists in the list being walked. The appenders' constants are all in 1..124 (31 immediates, `callers 52 4 E52`), but **8 of the 39 append sites are not constants** (`0038+004A` parameter, `0038+2F24` item `+3Dh`, `0038+18ED` and `+197A`, `0086+1354`, `+205A`, `+2F44`, `009E+1716` parameter), and the removal-chain forwarders `+1656` (7 non-immediate callers) and `+1080` (1) have their own parameters. Closing this is a global invariant on the set of node types ever created; this document does not close it.

## Scan inputs (by hash)

`python3 tools/overlay/u9e_index_sites.py inputs` prints these; the scans read nothing else. START.EXE and GAME.OVR are the owner files named in `U38_DISPATCH_BOUNDARY_2EDA.md` (same hashes). The data segment used for the tables is paragraph 0C7C of the expanded START.EXE image (`u9e_dispatch_0000_audit.py` `source()`, the same expansion the composed differential runs on).

```
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd  START.EXE
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614  GAME.OVR
a1927b99e61de81f910cae15428e8bce17fd1857197a666e605980f8ba3e8332  CHRDATA1.ITM
1068419d97cae13fa081304c5fee9c10e487a2d7f073bba8b21c1d41e746a48a  CHRDATA2.ITM
a069154daf5870f11c3580d0dc8df3e19dce47d9805060428f39667c04446b2c  CHRDATA3.ITM
2583a54045d43f64e656b4446b30bac7ea2d6d6223cd020d9ef727a135bd6857  CHRDATA4.ITM
caf8e0db328867a8087b0a7311b777f0183fbec193377a3c7146d2021580d7f1  CHRDATA5.ITM
ab5c48038c4000a690e25737d73b647967c991e778346ca0747e11db828f95e3  CHRDATA6.ITM
beb57c33402d8aee81275b913d822c65593bdb0107f49ee1f0016b8875c95d22  CHRDATJ1.ITM
bd8a75e0c112130bd426b1e98e24d9413983b23411a27da52103874c1b89bdcc  CHRDATJ2.ITM
20d8fadd91a6a8484de9add2bc9fd8b6cc151f9cf83b2f7a322e171f3c20d6a2  CHRDATJ3.ITM
9cf0bc38b8f733230508221d39fede49807b3d60158e93579030a1cdf28e98be  CHRDATJ4.ITM
c44a4cde4a148d2c41a64f23ac27f4d92e720e9996ab7fcfa4e0d5d3910fc7c5  CHRDATJ5.ITM
24f23ab53160c78dfc643a5fb9bd2d48f3970740bae9af69b6a9e21b3f158a4c  CHRDATJ6.ITM
048e3f8e2a434004e2fff5e519da2a4e9b3b6aad87eb310a9c21b6e814e7deaa  MON1ITM.DAX
06a69dd2a63077a464a3a936ed3f14dc4d463175d43de6670e3c30fe41481988  MON2ITM.DAX
8ba4c7f722469c671c5b5cc4021b6d11e750b31a492c73e26917894555ef2ed1  MON3ITM.DAX
e9c8ee5a987a498c4b93950d001f45b855f248df779457678f51364bac7517e9  MON4ITM.DAX
dd3bf78c479909b87b0ddf32bc32380c02fa8d62953e26d5ae05c1bc21874c94  MON5ITM.DAX
b56cf3c70e13f31121cfc57464db5420dbf5a1c6cbfdcf3714818038fcb214cb  MON6ITM.DAX
86ef5f43c05db3c1aeedaa26875e1325bd01e6d78ba4ef270aa1c8e99b7a594f  MON7ITM.DAX
ffeebbd5822542894ec0c8090025e735b58f9aafe09e97d2781321d42a00902f  MON8ITM.DAX
```

## 6. What is established and what is not

**Established** (code-proved or measured, with the commands above): 11 of 16 direct sites are in-range immediates; function `+0160` reaches the dispatcher only with 1..126; the three DS tables supply only 1..139; the direct stores to `+3D`/`+3E` are 23, 1 and 137; index 0 cannot come from the `0077` sites; the shipped item data contain no effect byte of 140 or above.

**Not established** (each is a precondition if the credit statement wants "the refused domain is unreachable"):
1. **Item effect bytes** `+3Dh` (`0038+2F40`, `+2F06`) and `+3Eh` (`0077` sites) are file data with no range test in the code. Shipped `.ITM` and monster items are in range; **item creation from the `ITEMS` definition table, ECL-generated treasure and saved games (`SAVGAM*.DAT`, edited saves) were not scanned**, and edited saves cannot be bounded by any code argument.
2. **Node types** (section 5): the 8 non-constant append sites, the removal-chain parameters, and the **immediates 0 and 254 passed into `+1656`**: dispatch with 0 or 254 needs a node of that type with a nonzero byte `+4`, which this document neither shows nor excludes.
3. The two `ES:[DI]` callers of `+0026` (`007D+01FE`, `0086+2481`) read a node's type byte (closure over item 2).
4. The DS-table scan is a sweep, not a proof of no writer.

## 7. The declared precondition (par-e ruling i22829: acceptable for strict credit; the node-type closure is not required)

Strict credit of `OVL_009E_0000` rests on the shipped-game domain, an **atomic refusal outside it**, and the words below. This is an **environment assumption**, labelled as such; it is not a proof that the refused indices are unreachable.

**Assumption.** Every index byte that reaches the dispatcher comes from shipped game data and lies in 1..139 (and a `+3Eh` dispatched by the `0077` sites lies in 128..139), or is zero where a guard excludes it. For any index outside 1..139 the model **REFUSES atomically**: CPU, RAM and budget untouched, nothing called. (The original has no range test: from index 140 up it jumps through non-handler data past the table, so the refusal is strictly safer than the original.)

**What the assumption names and does not close:**
1. **Three data sources were not scanned**: the `ITEMS` item-definition table, ECL-generated treasure, and saved games (`SAVGAM*.DAT`, `CHRDATA*.SAV` beyond the `.ITM` records). The scanned sources are the 28 shipped `.ITM` items and the 301 `MON*ITM.DAX` items.
2. **The immediate 0 and 254 removal-chain path.** `0086+21E7` passes the immediate 0 and `0038+1FC5` passes the immediate 254 into `+1656`, which forwards its type byte to `+0026`; `+0026` dispatches that byte through `+00A1` only for a matching node whose byte `+4` is nonzero. **Index 254 therefore dispatches only through a type-254 node with a nonzero `+4`; that such a node can exist is neither shown nor excluded here, and the same holds for a type-0 node.** The appenders' constants are all in 1..124 and 8 of the 39 append sites are not constants (section 5).
3. **Edited saves are excluded.** A save file with an item effect byte or node type outside the domain can reach the refusal; no code argument bounds hand-edited data.

**If the 254 path (or any other path to an index outside 1..139) is ever shown reachable from SHIPPED data, the credit is REOPENED, not caveated.**

**Reproducible commands** (read-only; need the ignored game media; `inputs` prints the SHA-256 of every file read):
- `python3 tools/overlay/u9e_index_sites.py inputs`
- `python3 tools/overlay/u9e_index_sites.py sites` (the 16 direct sites and their index source)
- `python3 tools/overlay/u9e_index_sites.py callers FFFF 1 160` (function `+0160`: 140 callers, all immediates 1..126)
- `python3 tools/overlay/u9e_index_sites.py callers 2A 3 26`, `callers 84 5 1656`, `callers 52 4 E52` (the removal chain and the appenders)
- `python3 tools/overlay/u9e_index_sites.py tables` (the three DS tables: 24 values, all in 1..139)
- **`python3 tools/overlay/u9e_index_sites.py items`: the shipped-data scan, 28 `.ITM` + 301 `MON*ITM.DAX` items, no effect byte `+3Ch/+3Dh/+3Eh` is 140 or above.**

**The refusal proof is pinned by the C test.** `tests/recovered/test_overlay_u9e_dispatch_0000_cpu.c` (the loop under "REFUSED") runs **all 117 refused index bytes** (0 and 140..255 plus the high-byte noise of the index word): each returns `PR_U9E_0000_REFUSED` with the CPU struct, all 1 MiB of RAM and the budget byte-identical before and after, and `diagnostic.slot` equal to the index. The composed Unicorn differential (`u9e_dispatch_0000_audit.py`, the `refused-*` cases) checks the same against the original bytes.

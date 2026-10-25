# OVL_009E_0000: the dispatcher's handler set, enumerated and priced (VP ruling i21629; measurement only, no model)

The ruling: model every handler the credited installer `OVL_0038_31A2` stores, state the set handler by handler, name a gap per handler that cannot be modelled, and **price it before building**. This document is the enumeration and the price. **It builds nothing and proposes no credit.**

START.EXE SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd; GAME.OVR SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614. The queue export was regenerated on origin/main (`d7f5eb5e`) for the prices.

## The dispatcher

`OVL_009E_0000` is 38 bytes, `RETF 000Ch`, stub `00B0:0025`:

    0000 push bp / mov bp,sp
    0003 push [bp+0Eh] / push [bp+0Ch] / push [bp+0Ah] / push [bp+8]    (four argument words, forwarded)
    000F mov al,[bp+6] / push ax                                        (a byte, forwarded)
    0013 mov al,[bp+10h] / xor ah,ah / mov di,ax / shl di,1 / shl di,1  (DI = 4 * the index byte)
    001E lcall [di+6828h]                                               (the only dispatch)
    0022 pop bp / retf 000Ch

The index is **one unsigned byte, so the domain is 256 slots (6828h..6C27h)**. The handler receives the forwarded arguments (`RETF 0Ah` handlers: five words) and returns to the dispatcher.

## What `31A2` stores (all identified)

`31A2` makes 138 far-pointer installs at DS:682Ch..6A57h (**slots 1..139, except slot 88**); every one resolves, through the loader's stub table, to an overlay entry. They are **117 distinct functions**, 115 in unit 0x38 and 2 in unit 0x5E. **64 are strict-credited; 53 are not.** Nothing `31A2` stores is unidentifiable, so the set is **not open from `31A2`'s side**; the ruling's stop condition is not triggered by `31A2` itself.

## What `31A2` does not store (the domain is wider than the ruling's set)

| Slots | Address | Who writes it | Consequence |
| --- | --- | --- | --- |
| 88 | 6988h..698Bh | **unit 0x86 +37A7/+37AA** (the `+3413` initializer, which runs before `31A2`; already recorded in `U38_EFFECTS_INSTALL_31A2.md`): the far pointer `0096:015B`, **unit 0x86 entry 63 = `OVL_0086_30D9`** | one more handler (the 118th), **not credited, closure 17,602 B** |
| 0 | 6828h/682Ah | not a table entry: `6826h:6828h` is a **far-pointer variable**, written by `OVL_0038_013B` (+014E/+0151) and `OVL_005E_3A86` (+3AA3/+3AA6) | index 0 would call the variable's (offset, high word), not a handler; no stored handler exists |
| 140..255 | 6A58h..6C27h | **other game state**: a linear sweep of every overlay finds 108 distinct written addresses in 6A58h..6C27h, 1- and 2-byte variables and far-pointer variables (for example unit 0x36 at 6A58h/6A5Ch, and 6B80h onward) | an index of 140 or more would call garbage |

## The callers' index (what is reached in practice)

Eleven far call sites (`lcall 00B0:0025`) and five near ones (inside unit 0x9E). Of the 11 far sites, **8 pass a constant** as the first pushed byte (`0Bh` twice, `17h`, `2Bh`, `2Ch`, `3Ah` twice, `4Eh`) and **3 pass a record byte read from memory** (unit 0x38 `+2F30`: `[rec+3Dh]`; unit 0x77 `+1533` and `+164F`: `[rec+3Eh]`). **The domain is therefore not statically bounded by the callers**; an in-range value is a game invariant, not something the bytes prove. The five near callers were not decoded here.

## Price (per handler; `tools/overlay/price_row_closures.py`, closure bytes and bodies, floors)

**The 53 not-credited handlers carry closures from 55 B to 20,654 B: 6 are at most 1,000 B, 17 are 1,000 to 3,000 B, 30 are above 3,000 B.** The pricer lists an `OVL_009E_*` dispatch (`0026`, `0160` or `175D`) for **34 of the 53**; `0026` and `0160` are documented to dispatch through `OVL_009E_0000` itself (`STRICT_CANDIDATE_QUEUE.md`, `IDENTITY_CLEARED_PRICING.md`), so those handler closures are **cyclic**: the dispatcher is part of its own handlers' closures. Closures overlap heavily (9 to 20 KB shared sub-closures), so the sum over handlers (424,482 B) is **not** a cost; the union was not computed here.

### Slot by slot (the VP's condition 1: every slot of the 256-slot domain has a verdict)

Verdicts (PM phasing, i21762): **modelled with credited handler X (raw-CPU model, composed in phase 1)**; **named gap (phase 2): credited handler, semantic model, not composed** (the handler is strict-credited but its model is a semantic or ledger translation that needs a frame-to-struct adapter and a full-RAM comparison; phase 2 builds those, each as its own small PR); **named gap: uncredited handler Y** (no strict model; the body stops with a stop that names the slot and Y); **non-handler** (not a function pointer; the body refuses). The 139 slots 1..139 split into **42 phase-1 modelled slots** (31 distinct raw-CPU credited handlers; some handlers occupy several slots), **35 phase-2 slots** (32 distinct credited handlers with semantic models), **1 deferred slot** (slot 135, `OVL_0038_3134`: a credited raw-CPU model whose world is the 1693 display closure) and **61 named-gap slots** (54 distinct uncredited handlers: the 53 stored by `31A2` plus `OVL_0086_30D9` at slot 88). Slot 0 and slots 140..255 are non-handlers. The raw-versus-semantic split is read from the handler's evidence-document name (`*_CPU.md` is a raw-CPU model) and is confirmed per handler when it is composed.

| slot | address | stub | handler | verdict | closure B (bodies) |
| ---: | --- | --- | --- | --- | ---: |
| 0 | 6828h | - | - | **non-handler** (the far-pointer variable 6826h:6828h; REFUSED) | - |
| 1 | 682Ch | 0041:002A | OVL_0038_010D | modelled with credited handler OVL_0038_010D (semantic model `pr_u38_entry2` through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 2 | 6830h | 0041:002F | OVL_0038_011D | modelled with credited handler OVL_0038_011D (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 3 | 6834h | 0041:0034 | OVL_0038_013B | modelled with credited handler OVL_0038_013B (semantic model through a phase-2 adapter with wrap-exact far-pointer copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 4 | 6838h | 0041:0039 | OVL_0038_017F | **named gap: uncredited handler OVL_0038_017F** | 1335 (5) |
| 5 | 683Ch | 0041:0264 | OVL_0038_319B | modelled with credited handler OVL_0038_319B (raw-CPU model, composed in phase 1) | - |
| 6 | 6840h | 0041:003E | OVL_0038_01C3 | **named gap (phase 2): credited handler, semantic model, not composed** | - |
| 7 | 6844h | 0041:0043 | OVL_0038_0233 | **named gap: uncredited handler OVL_0038_0233** | 1591 (5) |
| 8 | 6848h | 0041:0048 | OVL_0038_0377 | modelled with credited handler OVL_0038_0377 (semantic model through a phase-2 adapter, global active-character pointer DS:[5D92] with wrap-exact copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 9 | 684Ch | 0041:004D | OVL_0038_03AC | modelled with credited handler OVL_0038_03AC (semantic model through a phase-2 adapter, global active-character pointer DS:[5D92] with wrap-exact copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 10 | 6850h | 0041:0052 | OVL_0038_03E1 | modelled with credited handler OVL_0038_03E1 (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 11 | 6854h | 0041:0057 | OVL_0038_0400 | **named gap: uncredited handler OVL_0038_0400** | 186 (1) |
| 12 | 6858h | 0041:005C | OVL_0038_04BA | **modelled with credited CPU-level handler through adapter; constructed candidate-list evidence (t74)** | - |
| 13 | 685Ch | 0041:0264 | OVL_0038_319B | modelled with credited handler OVL_0038_319B (raw-CPU model, composed in phase 1) | - |
| 14 | 6860h | 0041:0061 | OVL_0038_05D6 | modelled with credited handler OVL_0038_05D6 (semantic model through a phase-2 adapter with wrap-exact far-pointer copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 15 | 6864h | 0041:0066 | OVL_0038_05EB | **named gap: uncredited handler OVL_0038_05EB** | 15137 (38) |
| 16 | 6868h | 0041:006B | OVL_0038_0647 | modelled with credited handler OVL_0038_0647 (raw-CPU model, composed in phase 1) | - |
| 17 | 686Ch | 0041:0070 | OVL_0038_064E | modelled with credited handler OVL_0038_064E (semantic model through a phase-2 adapter with wrap-exact far-pointer copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 18 | 6870h | 0041:0075 | OVL_0038_0679 | modelled with credited handler OVL_0038_0679 (raw-CPU model, composed in phase 1) | - |
| 19 | 6874h | 0041:0264 | OVL_0038_319B | modelled with credited handler OVL_0038_319B (raw-CPU model, composed in phase 1) | - |
| 20 | 6878h | 0041:007A | OVL_0038_06E4 | modelled with credited handler OVL_0038_06E4 (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 21 | 687Ch | 0041:007F | OVL_0038_070F | **named gap: uncredited handler OVL_0038_070F** | 1369 (5) |
| 22 | 6880h | 0041:0084 | OVL_0038_0775 | **named gap: uncredited handler OVL_0038_0775** | 13344 (31) |
| 23 | 6884h | 0041:0089 | OVL_0038_07E0 | **named gap: uncredited handler OVL_0038_07E0** | 1579 (5) |
| 24 | 6888h | 0041:0264 | OVL_0038_319B | modelled with credited handler OVL_0038_319B (raw-CPU model, composed in phase 1) | - |
| 25 | 688Ch | 0041:008E | OVL_0038_0918 | modelled with credited handler OVL_0038_0918 composed with the credited resident lookup BA:2199 (callback pr_res_00ba_2199_cpu; the bar is (a) over 216 constructed-list worlds, the random worlds do not terminate in the original: see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 26 | 6890h | 0041:0093 | OVL_0038_0947 | modelled with credited handler OVL_0038_0947 (raw-CPU model, composed in phase 1) | - |
| 27 | 6894h | 0041:0264 | OVL_0038_319B | modelled with credited handler OVL_0038_319B (raw-CPU model, composed in phase 1) | - |
| 28 | 6898h | 0041:0098 | OVL_0038_09C0 | **named gap: uncredited handler OVL_0038_09C0** | 1706 (6) |
| 29 | 689Ch | 0041:009D | OVL_0038_0A3D | modelled with credited handler OVL_0038_0A3D (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 30 | 68A0h | 0041:00A2 | OVL_0038_0A65 | **named gap: uncredited handler OVL_0038_0A65** | 2929 (10) |
| 31 | 68A4h | 0041:00F7 | OVL_0038_133F | modelled with credited handler OVL_0038_133F (raw-CPU model, composed in phase 1) | - |
| 32 | 68A8h | 0041:00A7 | OVL_0038_0B1B | **named gap: uncredited handler OVL_0038_0B1B** | 13382 (31) |
| 33 | 68ACh | 0041:00AC | OVL_0038_0BAC | modelled with credited handler OVL_0038_0BAC (semantic model through a phase-2 adapter with wrap-exact far-pointer copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 34 | 68B0h | 0041:00B1 | OVL_0038_0BCF | **named gap: uncredited handler OVL_0038_0BCF** | 55 (1) |
| 35 | 68B4h | 0041:00B6 | OVL_0038_0C06 | modelled with credited handler OVL_0038_0C06 (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 36 | 68B8h | 0041:00BB | OVL_0038_0C15 | modelled with credited handler OVL_0038_0C15 (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 37 | 68BCh | 0041:00C0 | OVL_0038_0C26 | modelled with credited handler OVL_0038_0C26 (semantic model through a phase-2 adapter with wrap-exact far-pointer copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 38 | 68C0h | 0041:005C | OVL_0038_04BA | **modelled with credited CPU-level handler through adapter; constructed candidate-list evidence (t74)** | - |
| 39 | 68C4h | 0041:00C5 | OVL_0038_0C4B | **named gap: uncredited handler OVL_0038_0C4B** | 1387 (5) |
| 40 | 68C8h | 0041:00CA | OVL_0038_0CC3 | **named gap: uncredited handler OVL_0038_0CC3** | 745 (1) |
| 41 | 68CCh | 0041:00CF | OVL_0038_1086 | **named gap: uncredited handler OVL_0038_1086** | 1659 (7) |
| 42 | 68D0h | 0041:00D4 | OVL_0038_10C1 | modelled with credited handler OVL_0038_10C1 (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 43 | 68D4h | 0041:00D9 | OVL_0038_10DE | **named gap: uncredited handler OVL_0038_10DE** | 1405 (5) |
| 44 | 68D8h | 0041:00DE | OVL_0038_1168 | **named gap: uncredited handler OVL_0038_1168** | 15190 (38) |
| 45 | 68DCh | 0041:0048 | OVL_0038_0377 | modelled with credited handler OVL_0038_0377 (semantic model through a phase-2 adapter, global active-character pointer DS:[5D92] with wrap-exact copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 46 | 68E0h | 0041:004D | OVL_0038_03AC | modelled with credited handler OVL_0038_03AC (semantic model through a phase-2 adapter, global active-character pointer DS:[5D92] with wrap-exact copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 47 | 68E4h | 0041:00E3 | OVL_0038_11F9 | modelled with credited handler OVL_0038_11F9 (raw-CPU model, composed in phase 1) | - |
| 48 | 68E8h | 0041:00E8 | OVL_0038_1276 | modelled with credited handler OVL_0038_1276 (raw-CPU model, composed in phase 1) | - |
| 49 | 68ECh | 0041:00ED | OVL_0038_12B2 | modelled with credited handler OVL_0038_12B2 (a credited CPU-level model that composes its callees, wired in wiring batch W1; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 50 | 68F0h | 0041:00F2 | OVL_0038_12FB | **named gap: uncredited handler OVL_0038_12FB** | 2320 (10) |
| 51 | 68F4h | 0041:00F7 | OVL_0038_133F | modelled with credited handler OVL_0038_133F (raw-CPU model, composed in phase 1) | - |
| 52 | 68F8h | 0041:00F7 | OVL_0038_133F | modelled with credited handler OVL_0038_133F (raw-CPU model, composed in phase 1) | - |
| 53 | 68FCh | 0041:00F7 | OVL_0038_133F | modelled with credited handler OVL_0038_133F (raw-CPU model, composed in phase 1) | - |
| 54 | 6900h | 0041:00FC | OVL_0038_1355 | modelled with credited handler OVL_0038_1355 (raw-CPU model, composed in phase 1) | - |
| 55 | 6904h | 0041:0101 | OVL_0038_135C | modelled with credited handler OVL_0038_135C (raw-CPU model, composed in phase 1) | - |
| 56 | 6908h | 0041:0106 | OVL_0038_1363 | modelled with credited handler OVL_0038_1363 (raw-CPU model, composed in phase 1) | - |
| 57 | 690Ch | 0041:010B | OVL_0038_1399 | **named gap: uncredited handler OVL_0038_1399** | 13422 (31) |
| 58 | 6910h | 0041:0110 | OVL_0038_1452 | modelled with credited handler OVL_0038_1452 (semantic model through a phase-2 adapter with wrap-exact far-pointer copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 59 | 6914h | 0041:0115 | OVL_0038_1472 | modelled with credited handler OVL_0038_1472 (raw-CPU model, composed in phase 1) | - |
| 60 | 6918h | 0041:0264 | OVL_0038_319B | modelled with credited handler OVL_0038_319B (raw-CPU model, composed in phase 1) | - |
| 61 | 691Ch | 0041:011A | OVL_0038_1490 | modelled with credited handler OVL_0038_1490 (raw-CPU model, composed in phase 1) | - |
| 62 | 6920h | 0041:011F | OVL_0038_14EB | **named gap: uncredited handler OVL_0038_14EB** | 2342 (10) |
| 63 | 6924h | 0041:0264 | OVL_0038_319B | modelled with credited handler OVL_0038_319B (raw-CPU model, composed in phase 1) | - |
| 64 | 6928h | 0041:0124 | OVL_0038_165D | **named gap: uncredited handler OVL_0038_165D** | 13551 (33) |
| 65 | 692Ch | 0041:0129 | OVL_0038_1671 | **named gap: uncredited handler OVL_0038_1671** | 13551 (33) |
| 66 | 6930h | 0041:012E | OVL_0038_1685 | **named gap: uncredited handler OVL_0038_1685** | 13551 (33) |
| 67 | 6934h | 0041:0133 | OVL_0038_1699 | **named gap: uncredited handler OVL_0038_1699** | 8945 (21) |
| 68 | 6938h | 0041:0138 | OVL_0038_16BB | **named gap: uncredited handler OVL_0038_16BB** | 8954 (21) |
| 69 | 693Ch | 0041:013D | OVL_0038_16E6 | **named gap: uncredited handler OVL_0038_16E6** | 8948 (21) |
| 70 | 6940h | 0041:0142 | OVL_0038_170B | **named gap: uncredited handler OVL_0038_170B** | 13551 (33) |
| 71 | 6944h | 0041:0147 | OVL_0038_171F | modelled with credited handler OVL_0038_171F (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 72 | 6948h | 0041:014C | OVL_0038_1730 | modelled with credited handler OVL_0038_1730 (raw-CPU model, composed in phase 1) | - |
| 73 | 694Ch | 0068:008E | OVL_005E_39D2 | **named gap: uncredited handler OVL_005E_39D2** | 20565 (49) |
| 74 | 6950h | 0068:0093 | OVL_005E_3A86 | **named gap: uncredited handler OVL_005E_3A86** | 20654 (49) |
| 75 | 6954h | 0041:0151 | OVL_0038_176F | **named gap: uncredited handler OVL_0038_176F** | 14763 (35) |
| 76 | 6958h | 0041:0156 | OVL_0038_18B5 | **named gap: uncredited handler OVL_0038_18B5** | 128 (1) |
| 77 | 695Ch | 0041:015B | OVL_0038_1935 | **named gap: uncredited handler OVL_0038_1935** | 1431 (5) |
| 78 | 6960h | 0041:0160 | OVL_0038_19D9 | **named gap: uncredited handler OVL_0038_19D9** | 9297 (23) |
| 79 | 6964h | 0041:0165 | OVL_0038_1A19 | **named gap: uncredited handler OVL_0038_1A19** | 14488 (35) |
| 80 | 6968h | 0041:016A | OVL_0038_1A4C | **named gap: uncredited handler OVL_0038_1A4C** | 14531 (35) |
| 81 | 696Ch | 0041:016F | OVL_0038_1AAA | **named gap: uncredited handler OVL_0038_1AAA** | 9628 (22) |
| 82 | 6970h | 0041:0174 | OVL_0038_1BA8 | **named gap: uncredited handler OVL_0038_1BA8** | 9635 (22) |
| 83 | 6974h | 0041:0179 | OVL_0038_1CAD | **named gap: uncredited handler OVL_0038_1CAD** | 15749 (37) |
| 84 | 6978h | 0041:017E | OVL_0038_1E77 | **named gap: uncredited handler OVL_0038_1E77** | 11472 (24) |
| 85 | 697Ch | 0041:0183 | OVL_0038_23A4 | **named gap: uncredited handler OVL_0038_23A4** | 13914 (33) |
| 86 | 6980h | 0041:0188 | OVL_0038_23C2 | **named gap: uncredited handler OVL_0038_23C2** | 13926 (33) |
| 87 | 6984h | 0041:018D | OVL_0038_23EC | **named gap: uncredited handler OVL_0038_23EC** | 1431 (5) |
| 88 | 6988h | 0096:015B | OVL_0086_30D9 (unit 0x86 entry 63, stored by U86 +37A7) | **named gap: uncredited handler OVL_0086_30D9** | 17602 (42) |
| 89 | 698Ch | 0041:0192 | OVL_0038_2490 | modelled with credited handler OVL_0038_2490 (semantic model through a phase-2 adapter with wrap-exact far-pointer copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 90 | 6990h | 0041:0197 | OVL_0038_24D9 | **named gap (phase 2): credited handler, semantic model, not composed** | - |
| 91 | 6994h | 0041:019C | OVL_0038_2556 | **named gap: uncredited handler OVL_0038_2556** | 1343 (5) |
| 92 | 6998h | 0041:0264 | OVL_0038_319B | modelled with credited handler OVL_0038_319B (raw-CPU model, composed in phase 1) | - |
| 93 | 699Ch | 0041:01A1 | OVL_0038_25A2 | modelled with credited handler OVL_0038_25A2 (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 94 | 69A0h | 0041:01A6 | OVL_0038_25BC | modelled with credited handler OVL_0038_25BC (a credited CPU-level model that composes its callees, wired in wiring batch W1; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 95 | 69A4h | 0041:01AB | OVL_0038_2610 | **named gap: uncredited handler OVL_0038_2610** | 13296 (31) |
| 96 | 69A8h | 0041:01B0 | OVL_0038_264B | modelled with credited handler OVL_0038_264B (raw-CPU model, composed in phase 1) | - |
| 97 | 69ACh | 0041:01B5 | OVL_0038_268A | **named gap (phase 2): credited handler, semantic model, not composed** | - |
| 98 | 69B0h | 0041:01BA | OVL_0038_2700 | modelled with credited handler OVL_0038_2700 (semantic model through a phase-2 adapter with wrap-exact far-pointer copy-in/out; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 99 | 69B4h | 0041:01BF | OVL_0038_2730 | **named gap: uncredited handler OVL_0038_2730** | 9716 (24) |
| 100 | 69B8h | 0041:01C4 | OVL_0038_27D9 | modelled with credited handler OVL_0038_27D9 (raw-CPU model, composed in phase 1) | - |
| 101 | 69BCh | 0041:01C9 | OVL_0038_2814 | modelled with credited handler OVL_0038_2814 (raw-CPU model, composed in phase 1) | - |
| 102 | 69C0h | 0041:01CE | OVL_0038_2866 | **named gap: uncredited handler OVL_0038_2866** | 9322 (23) |
| 103 | 69C4h | 0041:01D3 | OVL_0038_28BF | **named gap: uncredited handler OVL_0038_28BF** | 9671 (25) |
| 104 | 69C8h | 0041:01D8 | OVL_0038_2901 | **named gap: uncredited handler OVL_0038_2901** | 1620 (7) |
| 105 | 69CCh | 0041:01DD | OVL_0038_2976 | modelled with credited handler OVL_0038_2976 (raw-CPU model, composed in phase 1) | - |
| 106 | 69D0h | 0041:01E2 | OVL_0038_2984 | modelled with credited handler OVL_0038_2984 (raw-CPU model, composed in phase 1) | - |
| 107 | 69D4h | 0041:01E7 | OVL_0038_2992 | modelled with credited handler OVL_0038_2992 (raw-CPU model, composed in phase 1) | - |
| 108 | 69D8h | 0041:01EC | OVL_0038_29B6 | modelled with credited handler OVL_0038_29B6 (raw-CPU model, composed in phase 1) | - |
| 109 | 69DCh | 0041:01F1 | OVL_0038_29CB | modelled with credited handler OVL_0038_29CB (raw-CPU model, composed in phase 1) | - |
| 110 | 69E0h | 0041:01F6 | OVL_0038_29D9 | modelled with credited handler OVL_0038_29D9 (raw-CPU model, composed in phase 1) | - |
| 111 | 69E4h | 0041:01FB | OVL_0038_29F0 | modelled with credited handler OVL_0038_29F0 (raw-CPU model, composed in phase 1) | - |
| 112 | 69E8h | 0041:0200 | OVL_0038_2A11 | modelled with credited handler OVL_0038_2A11 (raw-CPU model, composed in phase 1) | - |
| 113 | 69ECh | 0041:0205 | OVL_0038_2A28 | modelled with credited handler OVL_0038_2A28 (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 114 | 69F0h | 0041:020A | OVL_0038_2A6D | modelled with credited handler OVL_0038_2A6D (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 115 | 69F4h | 0041:020F | OVL_0038_2A87 | modelled with credited handler OVL_0038_2A87 (raw-CPU model, composed in phase 1) | - |
| 116 | 69F8h | 0041:0214 | OVL_0038_2AE6 | modelled with credited handler OVL_0038_2AE6 (raw-CPU model, composed in phase 1) | - |
| 117 | 69FCh | 0041:0219 | OVL_0038_2B20 | modelled with credited handler OVL_0038_2B20 (raw-CPU model, composed in phase 1) | - |
| 118 | 6A00h | 0041:021E | OVL_0038_2B63 | modelled with credited handler OVL_0038_2B63 (semantic model through a phase-2 adapter; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 119 | 6A04h | 0041:0223 | OVL_0038_2B7D | modelled with credited handler OVL_0038_2B7D (raw-CPU model, composed in phase 1) | - |
| 120 | 6A08h | 0041:0228 | OVL_0038_2BC8 | **named gap: uncredited handler OVL_0038_2BC8** | 1684 (7) |
| 121 | 6A0Ch | 0041:022D | OVL_0038_2C1C | **named gap: uncredited handler OVL_0038_2C1C** | 16543 (38) |
| 122 | 6A10h | 0041:0232 | OVL_0038_2D38 | modelled with credited handler OVL_0038_2D38 (a credited CPU-level model that composes its callees, wired in wiring batch W1; the bar is (a): full CPU14, FLAGS and RAM against the original bytes, see `OVL_009E_0000_CPU.md`, phase 2) | - |
| 123 | 6A14h | 0041:0237 | OVL_0038_2D88 | modelled with credited handler OVL_0038_2D88 (raw-CPU model, composed in phase 1) | - |
| 124 | 6A18h | 0041:023C | OVL_0038_2DE4 | modelled with credited handler OVL_0038_2DE4 (raw-CPU model, composed in phase 1) | - |
| 125 | 6A1Ch | 0041:0241 | OVL_0038_2E08 | modelled with credited handler OVL_0038_2E08 (raw-CPU model, composed in phase 1) | - |
| 126 | 6A20h | 0041:0246 | OVL_0038_2E37 | **named gap (phase 2): credited handler, semantic model, not composed** | - |
| 127 | 6A24h | 0041:0264 | OVL_0038_319B | modelled with credited handler OVL_0038_319B (raw-CPU model, composed in phase 1) | - |
| 128 | 6A28h | 0041:024B | OVL_0038_2EDA | **named gap: uncredited handler OVL_0038_2EDA** | 427 (2) |
| 129 | 6A2Ch | 0041:024B | OVL_0038_2EDA | **named gap: uncredited handler OVL_0038_2EDA** | 427 (2) |
| 130 | 6A30h | 0041:024B | OVL_0038_2EDA | **named gap: uncredited handler OVL_0038_2EDA** | 427 (2) |
| 131 | 6A34h | 0041:0250 | OVL_0038_2F57 | **named gap: uncredited handler OVL_0038_2F57** | 1915 (6) |
| 132 | 6A38h | 0041:0255 | OVL_0038_30A5 | **named gap: uncredited handler OVL_0038_30A5** | 14852 (37) |
| 133 | 6A3Ch | 0041:024B | OVL_0038_2EDA | **named gap: uncredited handler OVL_0038_2EDA** | 427 (2) |
| 134 | 6A40h | 0041:024B | OVL_0038_2EDA | **named gap: uncredited handler OVL_0038_2EDA** | 427 (2) |
| 135 | 6A44h | 0041:025A | OVL_0038_3134 | **named gap (deferred): credited handler with a raw-CPU model, not composed in phase 1** (the model is bus-aware and calls the resident 1693 display closure; composing it needs the 1693 world and an EGA oracle) | - |
| 136 | 6A48h | 0041:024B | OVL_0038_2EDA | **named gap: uncredited handler OVL_0038_2EDA** | 427 (2) |
| 137 | 6A4Ch | 0041:025F | OVL_0038_317A | **named gap: uncredited handler OVL_0038_317A** | 347 (2) |
| 138 | 6A50h | 0041:024B | OVL_0038_2EDA | **named gap: uncredited handler OVL_0038_2EDA** | 427 (2) |
| 139 | 6A54h | 0041:024B | OVL_0038_2EDA | **named gap: uncredited handler OVL_0038_2EDA** | 427 (2) |
| 140..255 | 6A58h..6C27h | - | - | **non-handler** (other game state; every index REFUSED) | - |

The closure column is `tools/overlay/price_row_closures.py` (bytes and bodies, floors; `own B` per handler is in `docs/re/overlay_function_ledger.csv`). **34 of the 53 stored uncredited handlers list a 009E dispatch** (`0026`, `0160` or `175D`; `0026` and `0160` dispatch through `OVL_009E_0000` itself), so their closures are cyclic and the sum over handlers (424,482 B) is not a cost.

## VP ruling i21758: option (A), with four conditions that are part of the deliverable

The ruling is the bounded body: `OVL_009E_0000` with the **raw-CPU credited handlers composed** (phase 1, PM i21762; the semantic ones are further named gaps until phase 2) and **a named stop for every uncredited handler and every non-handler slot**. Its four conditions, and where each lives:

1. **Named per slot:** the table above (condition 1).
2. **The credit is scoped to the modelled slots.** In phase 1 any credit for `OVL_009E_0000` would cover **only the 42 raw-composed slots** (the table); phase 2 would add the 35 semantic slots one adapter at a time. **An index outside the composed set is NOT covered**: the 35 phase-2 slots, a named-gap slot (61 slots, 54 handlers), slot 0 and slots 140..255 are outside the claim, and **no sentence in this document or in the body's doc may say or imply that the dispatcher is fully modelled.** The body can only be called "the dispatcher for the raw-composed credited-handler slots".
3. **The domain precondition is declared and will be asserted:** the index byte must be **1..139, or 88 via unit 0x86**; **anything else (0 and 140..255) is REFUSED, never guessed, with CPU and RAM untouched** (a refusal stop in the model's contract, checked in its C test and by a fixture, never a call through the slot). A named-gap slot inside 1..139 stops with a CALLEE-class stop that carries the slot and the handler id and also leaves CPU and RAM as they were at the `LCALL`.
4. **A fixture for each of the three data-dependent far callers** (the sites that pass a record byte: unit 0x38 `+2F30` reading `[rec+3Dh]`, unit 0x77 `+1533` and `+164F` reading `[rec+3Eh]`): the fixture runs the **original caller bytes** up to its `LCALL 00B0:0025` with the record byte set out of range (0 and 140, 255), enters the dispatcher model with the live frame, and proves the refusal fires with CPU and RAM unchanged. These are part of the body's deliverable, not built here.

What the bounded body is **not**: it does not model the 54 uncredited handlers, it does not make the 34 cyclic closures finite, and it does not claim the three data-dependent callers stay in range (that is a game invariant, not something the bytes prove).

## Price of the composed build (009E_0000 + the 64 credited handlers), before starting

Measured counts: the dispatcher is 38 bytes; the 64 credited handlers are **3,506 own bytes** in 30 evidence documents; **32 of them have raw-CPU-state models** (`*_CPU.md`, the same API family the dispatcher composes with directly) and **32 have semantic or ledger models** (`pr_u38_*` state-struct APIs and the 16 `OVERLAY_UNIT38_EFFECT_ENTRIES` translations), which need a stack-frame-to-struct adapter per handler and a **full-RAM comparison they were not built against**; a semantic model that does not reproduce every RAM byte the original touches is a defect the composition would expose.

Estimate (judgement, not a measurement), **phased by the PM (i21762)**: **phase 1, about 1.5 days:** the dispatcher model and its Unicorn world, the 32 raw-CPU compositions, the three caller fixtures, the doc, mutants and the C test. **Phase 2, the variance:** the 32 semantic frame-to-struct adapters, each its own small PR with the full-RAM comparison; any model defect an adapter exposes is a FINDING reported as a STOP to the PM, never patched silently. **Phase 1 starts when this document merges.**

## Reproduction

    python3 tools/overlay/strict_candidate_queue.py --inventory resources/function-accounting-current/inventory.json --callgraph resources/function-accounting-current/callgraph.json --near-calls resources/function-accounting-current/near.json --ovr resources/DOS/POOLRAD/GAME.OVR --repo . --json queue.json
    python3 tools/overlay/price_row_closures.py --queue queue.json --exe resources/DOS/POOLRAD/START.EXE <the not-credited ids in the table>

The enumeration reads the installer's operands through `u38_effects_install_31a2_audit.source()` and the unit stub table; the writer and caller scans are linear sweeps of every overlay unit (a `mov` into 6828h..6C27h with an absolute or `[reg+disp]` operand, and `9A 25 00` with segment B0h), so a write through a computed pointer would not appear. The prior-session memory tool (`deja`) was unavailable (ENOENT); nothing was reused from earlier sessions.

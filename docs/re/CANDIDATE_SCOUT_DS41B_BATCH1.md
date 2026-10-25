# Candidate scout batch 1 (opencode-ds41-b / t115)

Status: **proposals only. No credit, no accounting, no strict-map change.** Bodies are
drawn from the open strict-candidate queue exported at 2026-10-25 (430 candidates). Each
claim below is reproducible against the owner `GAME.OVR` (sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`) and `START.EXE`
(sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`).

The byte compare is:

```
python3 tools/overlay/check_candidate_proposal.py docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md
```

It resolves each `BODY:` block through the repo inventory, adds the unit's `ovr_offset` to
the body-relative offset, and compares the quoted bytes to the file. Exit 0 means every
candidate's bytes match. It does **not** prove the semantic C; that is the independent
reviewer's job.

Each candidate lists what the C models, which callees it composes, and what this packet
does **not** verify.

---

## Candidate 1: OVL_001C_0292 - ECL opcode 0x09 SAVE

BODY: OVL_001C_0292
UNIT: 0x1c
REL_OFFSET: 0x0292
LENGTH: 78
FILE_OFFSET: 4056
SHA256: 7f66f8be811fe835d3ed3ab0fc0772f57e10ab39b5b5beda8aa2542262f92ebb
BYTES:
```bytes
55 89 e5 83 ec 04 b0 02 50 9a 2a 00 2b 00 a0 87
70 50 a0 47 70 50 9a 4d 00 2b 00 89 46 fc 80 3e
06 70 80 73 18 b0 01 50 9a 25 00 2b 00 89 46 fe
ff 76 fc ff 76 fe 9a 52 00 2b 00 eb 0d ff 76 fc
bf c8 70 1e 57 9a 5c 00 2b 00 89 ec 5d cb
```

Disassembly (Capstone, 16-bit):

```
0292 55                 push     bp
0293 89e5               mov      bp, sp
0295 83ec04             sub      sp, 4
0298 b002               mov      al, 2
029A 50                 push     ax
029B 9a2a002b00         lcall    0x2b, 0x2a        ; load_operands(2)
02A0 a08770             mov      al, [0x7087]      ; value_hi[2]
02A3 50                 push     ax
02A4 a04770             mov      al, [0x7047]      ; value_lo[2]
02A7 50                 push     ax
02A8 9a4d002b00         lcall    0x2b, 0x4d        ; operand_word(slot 2)
02AD 8946fc             mov      [bp-4], ax        ; dest
02B0 803e067080         cmp      byte [0x7006], 0x80 ; tag[1]
02B5 7318               jae      0x2cf
02B7 b001               mov      al, 1
02B9 50                 push     ax
02BA 9a25002b00         lcall    0x2b, 0x25        ; slot_value(1)
02BF 8946fe             mov      [bp-2], ax        ; value
02C2 ff76fc             push     [bp-4]            ; dest
02C5 ff76fe             push     [bp-2]            ; value
02C8 9a52002b00         lcall    0x2b, 0x52        ; store_word(dest, value)
02CD eb0d               jmp      0x2dc
02CF ff76fc             push     [bp-4]            ; dest
02D2 bfc870             mov      di, 0x70c8        ; string buffer 1 (DS:70C8h)
02D5 1e                 push     ds
02D6 57                 push     di
02D7 9a5c002b00         lcall    0x2b, 0x5c        ; store_string(buf1, dest)
02DC 89ec               mov      sp, bp
02DE 5d                 pop      bp
02DF cb                 retf
```

Semantic C (proposed decode; composes the existing `pr_rec_ecl_*` helpers):

```c
/* ECL opcode 0x09 SAVE. Far Pascal, no arguments, RETF.
 * Operands were fetched by the caller; slot 1 is the value, slot 2 its destination. */
void pr_rec_ecl_op_save(struct pr_ecl *e)
{
    pr_rec_ecl_load_operands(e, 2);                       /* 0298 */
    uint16_t dest = pr_rec_ecl_operand_word(e, 2);        /* 02A0..02AD, hi[2]<<8|lo[2] */
    if (pr_rec_ecl_tag(e, 1) >= 0x80) {                   /* 02B0 */
        pr_rec_ecl_store_string(e, &e->string[1], dest);  /* 02CF..02D7, DS:70C8h */
    } else {
        uint16_t value = pr_rec_ecl_slot_value(e, 1);     /* 02B7..02BF */
        pr_rec_ecl_store_word(e, dest, value);            /* 02C2..02C8 */
    }
    /* 02DC epilogue: SP = BP, POP BP, RETF */
}
```

Prior art: `docs/re/FUNCTION_LEDGER.md` section 1.17 already documents this body
(`pr_rec_ecl_op_save`) as a path-complete model, and `docs/re/ECL_CONTRACTS.md` names
opcode 0x09 SAVE. This proposal re-derives it from the bytes; it does not inherit credit.

---

## Candidate 2: OVL_001C_0EBC - ECL opcode 0x35 SAVE TABLE

BODY: OVL_001C_0EBC
UNIT: 0x1c
REL_OFFSET: 0x0ebc
LENGTH: 76
FILE_OFFSET: 7170
SHA256: faa0e052825907531da7a6782f1cd0e6f8f4fc5a2cdc51e8629ed8fb19d0536b
BYTES:
```bytes
55 89 e5 83 ec 08 b0 03 50 9a 2a 00 2b 00 b0 01
50 9a 25 00 2b 00 89 46 fa a0 87 70 50 a0 47 70
50 9a 4d 00 2b 00 89 46 fc b0 03 50 9a 25 00 2b
00 89 46 fe 8b 46 fc 03 46 fe 89 46 f8 ff 76 f8
ff 76 fa 9a 52 00 2b 00 89 ec 5d cb
```

Disassembly (Capstone, 16-bit):

```
0EBC 55                 push     bp
0EBD 89e5               mov      bp, sp
0EBF 83ec08             sub      sp, 8
0EC2 b003               mov      al, 3
0EC4 50                 push     ax
0EC5 9a2a002b00         lcall    0x2b, 0x2a        ; load_operands(3)
0ECA b001               mov      al, 1
0ECC 50                 push     ax
0ECD 9a25002b00         lcall    0x2b, 0x25        ; slot_value(1)
0ED2 8946fa             mov      [bp-6], ax        ; value
0ED5 a08770             mov      al, [0x7087]      ; value_hi[2]
0ED8 50                 push     ax
0ED9 a04770             mov      al, [0x7047]      ; value_lo[2]
0EDC 50                 push     ax
0EDD 9a4d002b00         lcall    0x2b, 0x4d        ; operand_word(slot 2)
0EE2 8946fc             mov      [bp-4], ax        ; base
0EE5 b003               mov      al, 3
0EE7 50                 push     ax
0EE8 9a25002b00         lcall    0x2b, 0x25        ; slot_value(3)
0EED 8946fe             mov      [bp-2], ax        ; index
0EF0 8b46fc             mov      ax, [bp-4]
0EF3 0346fe             add      ax, [bp-2]
0EF6 8946f8             mov      [bp-8], ax        ; dest = base + index
0EF9 ff76f8             push     [bp-8]            ; dest
0EFC ff76fa             push     [bp-6]            ; value
0EFF 9a52002b00         lcall    0x2b, 0x52        ; store_word(dest, value)
0F04 89ec               mov      sp, bp
0F06 5d                 pop      bp
0F07 cb                 retf
```

Semantic C (proposed decode; composes the existing `pr_rec_ecl_*` helpers):

```c
/* ECL opcode 0x35 SAVE TABLE. Far Pascal, no arguments, RETF.
 * dest = operand_word(slot 2) + slot_value(slot 3); value = slot_value(slot 1). */
void pr_rec_ecl_op_save_table(struct pr_ecl *e)
{
    pr_rec_ecl_load_operands(e, 3);                       /* 0EC2..0EC5 */
    uint16_t value = pr_rec_ecl_slot_value(e, 1);         /* 0ECA..0ED2 */
    uint16_t base  = pr_rec_ecl_operand_word(e, 2);       /* 0ED5..0EE2 */
    uint16_t index = pr_rec_ecl_slot_value(e, 3);         /* 0EE5..0EED */
    uint16_t dest  = (uint16_t)(base + index);            /* 0EF0..0EF6 */
    pr_rec_ecl_store_word(e, dest, value);                /* 0EF9..0EFF */
    /* 0F04 epilogue */
}
```

Note: `docs/re/U1C_GETTABLE_0E5E.md` already records that the `+0EBC` routine is a
separate `loader(3)` routine; this packet supplies the body-level reading. No credit.

---

## Candidate 3: OVL_001C_3171 - ECL run loop

BODY: OVL_001C_3171
UNIT: 0x1c
REL_OFFSET: 0x3171
LENGTH: 66
FILE_OFFSET: 16055
SHA256: 4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489
BYTES:
```bytes
55 89 e5 8b 46 06 a3 ed 49 c6 06 2e 44 00 80 3e
2e 44 00 75 24 80 3e ff 49 00 75 1d a0 7f 6f a2
7e 6f a1 ed 49 c4 3e de 49 03 f8 26 8a 85 00 67
a2 7f 6f 0e e8 85 fd eb d5 c6 06 2e 44 00 5d ca
02 00
```

Length is 66 bytes `[0x3171,0x31B3)`; the following `00` at `+0x31B3` is inter-function
padding (FUNCTION_LEDGER.md section 1.16). The span ends on `RETF 2`.

Disassembly (Capstone, 16-bit):

```
3171 55                 push     bp
3172 89e5               mov      bp, sp
3174 8b4606             mov      ax, [bp+6]        ; entry PC
3177 a3ed49             mov      [0x49ed], ax      ; PC = entry
317A c6062e4400         mov      byte [0x442e], 0  ; stop byte clear on entry
317F 803e2e4400         cmp      byte [0x442e], 0
3184 7524               jne      0x31aa
3186 803eff4900         cmp      byte [0x49ff], 0
318B 751d               jne      0x31aa
318D a07f6f             mov      al, [0x6f7f]      ; current opcode latch
3190 a27e6f             mov      [0x6f7e], al      ; previous opcode
3193 a1ed49             mov      ax, [0x49ed]      ; PC
3196 c43ede49           les      di, [0x49de]      ; script buffer far ptr
319A 03f8               add      di, ax
319C 268a850067         mov      al, es:[di+0x6700] ; fetch
31A1 a27f6f             mov      [0x6f7f], al      ; latch
31A4 0e                 push     cs
31A5 e885fd             call     0x2f2d            ; dispatcher
31A8 ebd5               jmp      0x317f
31AA c6062e4400         mov      byte [0x442e], 0  ; stop byte clear on exit
31AF 5d                 pop      bp
31B0 ca0200             retf     2
```

Semantic C (proposed decode; the dispatcher stays a callback boundary):

```c
/* ECL run loop. Far Pascal: word entry PC at [bp+6], RETF 2.
 * Clears the stop byte on entry and exit, loops while both stop bytes are zero. */
typedef int (*pr_ecl_dispatch_fn)(struct pr_ecl *e);

void pr_rec_ecl_run(struct pr_ecl *e, uint16_t entry, pr_ecl_dispatch_fn dispatch)
{
    e->pc = entry;                       /* 3174..3177: [0x49ED] */
    e->stop = 0;                         /* 317A: [0x442E] = 0 */
    while (e->stop == 0 && e->stop2 == 0) {   /* [0x442E], [0x49FF] */
        e->opcode_prev = e->opcode;      /* 318D..3190: [0x6F7E] = [0x6F7F] */
        uint16_t off = (uint16_t)(e->script_buf_off + e->pc + 0x6700u);
        e->opcode = pr_bus_rd8(e->script_buf_seg, off);   /* 3193..31A1 */
        dispatch(e);                     /* 31A4..31A5: push cs; call +2F2D */
    }
    e->stop = 0;                         /* 31AA: [0x442E] = 0 */
}
```

Prior art: `docs/re/FUNCTION_LEDGER.md` section 1.16 documents this body as the callback
model `pr_rec_ecl_run`, and `docs/re/DECOMP_FEASIBILITY.md` reads the same loop. This
proposal is the body-level decode; no credit.

---

## What this packet does NOT verify

- The semantic C is not compiled or run against the original. The byte compare proves
  only that the quoted bytes are the body at the claimed offset.
- Callee contracts (`load_operands`, `slot_value`, `operand_word`, `store_word`,
  `store_string`, and the dispatcher) are taken from existing docs/models, not re-proven
  here.
- Opcode assignment for 0x09/0x35 follows `docs/re/ECL_CONTRACTS.md`; the bodies are
  reached through the dispatcher cases recorded there, not re-traced in this packet.
- `SAVE TABLE`'s meaning (base + index) is read from the arithmetic; the name is the
  contract table's, not a behaviour this packet proves.

Suggested independent checks for a reviewer: re-locate each span from the inventory, hash
it, decode it with a second disassembler, and exercise the C against the existing
`pr_rec_ecl_*` models on the SAVE/SAVE TABLE shapes in FUNCTION_LEDGER 1.17.

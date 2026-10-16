#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_1262_ADD_NPC_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_1262_ADD_NPC_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x6F + 0x1262: public entry 7 (run-time stub 0x7C:0x43),
 * executable [1262,1426), 452 bytes, 174 instructions, 11 call sites, one
 * word argument (its low byte is the NPC id) consumed by `retf 2`. Evidence
 * and offsets: docs/re/U6F_1262_ADD_NPC.md. It is the callee of the ADD NPC
 * script handler (opcode 0x36, unit 0x1C + 0x2B7E, site +2B9B). Semantics:
 *   +1268  if the window-1 word at +0x67C (the party count) > 7, return;
 *   +1277  rec := GetMem(0x11D) (0xAF8:0x023F; the result is never tested);
 *   +1286  near +0EC4 (entry 6, the record initializer) with (rec, id);
 *   +129D  rec[0xAB] := Random(0x100) (0xAF8:0x11C0, AL);
 *   +12AE  count := 0; for k := 0 to 6: if the signed byte rec[0x96+k] > 0
 *          then begin count += 1; list[count] := k end;
 *   +12E2  if count = 0 then begin count := 1; list[1] := 2 end;
 *   +12F7  sel := list[Roll(1, count)] (unit 0x9E entry 8, 0xB0:0x48);
 *   +1310  v := Roll(1, 5); race := signed rec[0x9E];
 *          sel = 0: rec[0xBC] := DS:[0x08D7 + race*5 + v];
 *          sel = 5: rec[0xBC] := DS:[0x08EB + race*5 + v];
 *          sel = 6: rec[0xBC] := DS:[0x08F5 + race*5 + v];
 *          else:    rec[0xBC] := DS:[0x08E1 + race*5 + v];
 *   +13E3  w := Roll(1, 8); rec[0xBB] := DS:[0x08C7 + race*8 + w];
 *   +140F  near +1446 (the party insert) with rec;
 *   +1419  near +0482 (the picture builder) with 1; +1420 retf 2.
 * The allocation, the initializer, Random, the roll, the insert and the
 * picture builder are boundaries supplied as callbacks (the last two have
 * merged models: pr_u6f_party_insert_1446 and pr_u6f_0482_picture_build);
 * the class scan, the selection and the table reads are translated inline;
 * the five DS tables are supplied by the caller with their spans. The only
 * loop has a fixed trip count (7). */

#define PR_U6F_1262_RECORD_SIZE 0x11Du
#define PR_U6F_1262_PARTY_MAX 7u                /* the count word must be <= 7 to proceed */
#define PR_U6F_1262_CLASS_SLOTS 7u              /* rec[0x96..0x9C] */
#define PR_U6F_1262_OFF_CLASS_BASE 0x96u
#define PR_U6F_1262_OFF_RACE 0x9Eu
#define PR_U6F_1262_OFF_RANDOM 0xABu
#define PR_U6F_1262_OFF_PICTURE_B 0xBBu
#define PR_U6F_1262_OFF_PICTURE_CLASS 0xBCu
#define PR_U6F_1262_DEFAULT_CLASS 2u
#define PR_U6F_1262_RANDOM_RANGE 0x100u

typedef struct {
    const uint8_t *bytes;        /* DS table start */
    size_t size;                 /* supplied span */
} pr_u6f_1262_table;

typedef struct {
    uint16_t party_count;        /* the window-1 word at +0x67C ([0x49D6] + 0x67C) */
    pr_u6f_1262_table class_table_0;     /* DS:0x08D7, used when the selected class is 0 */
    pr_u6f_1262_table class_table_5;     /* DS:0x08EB, class 5 */
    pr_u6f_1262_table class_table_6;     /* DS:0x08F5, class 6 */
    pr_u6f_1262_table class_table_other; /* DS:0x08E1, every other class */
    pr_u6f_1262_table picture_table;     /* DS:0x08C7, stride 8 */
} pr_u6f_1262_state;

typedef struct {
    void *context;
    int (*alloc_record)(void *context, uint8_t **record, size_t *size);      /* 0xAF8:0x023F GetMem(0x11D) */
    int (*init_record)(void *context, uint8_t *record, size_t size, uint8_t id);   /* +0EC4, entry 6 */
    int (*random_byte)(void *context, uint16_t range, uint8_t *al);          /* 0xAF8:0x11C0 */
    int (*roll)(void *context, uint8_t count, uint8_t range, uint8_t *al);   /* unit 0x9E entry 8 */
    int (*insert_party)(void *context, uint8_t *record);                     /* +1446 */
    int (*build_pictures)(void *context, uint8_t recolor);                   /* +0482 with 1 */
} pr_u6f_1262_hooks;

typedef struct {
    uint8_t skipped;             /* the party count gate returned at once */
    uint8_t *record;             /* the host's record, once allocated */
    uint8_t initialized;
    uint8_t random_value;        /* stored at rec[0xAB] */
    uint8_t class_count;         /* positive class slots found (after the default) */
    uint8_t list[PR_U6F_1262_CLASS_SLOTS + 1u];   /* SS:BP-0xF: list[1..count] */
    uint8_t selected_class;      /* list[roll] */
    uint8_t class_roll;          /* the second roll, 1..5 */
    uint8_t picture_roll;        /* the third roll, 1..8 */
    uint8_t inserted;
    uint8_t built;
    uint16_t next_offset;        /* source offset the model stopped at, or 0x1426 after retf */
} pr_u6f_1262_result;

typedef enum {
    PR_U6F_1262_OK = 0,
    PR_U6F_1262_ARGUMENT,        /* record span, a roll outside 1..count, a table index outside its span */
    PR_U6F_1262_NEED_HOOK,       /* a reached boundary has no callback; next_offset = the call instruction */
    PR_U6F_1262_HOST_FAILURE     /* a callback returned non-zero; earlier effects kept */
} pr_u6f_1262_status;

/* id: the low byte of the word argument. The record the host allocates must
 * span 0x11D bytes; the engine never tests GetMem's result, so a NULL or short
 * record is a host contract break (ARGUMENT at +127B). A roll result outside
 * 1..count would read an unwritten local in the engine and is refused; a
 * table index outside the supplied span (a negative race byte reaches below
 * the table in the engine) is refused before the write. */
pr_u6f_1262_status pr_u6f_1262_add_npc(
    uint8_t id,
    const pr_u6f_1262_state *state,
    const pr_u6f_1262_hooks *hooks,
    pr_u6f_1262_result *out);

#define PR_U6F_1262_RETF_OFFSET 0x1423u
#define PR_U6F_1262_END_OFFSET 0x1426u

#endif

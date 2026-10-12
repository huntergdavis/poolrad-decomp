/* Bounded C model of GAME.OVR unit 0x68 +0x0294; see
 * docs/re/U68_ROLL_GATE_0294.md. */
#include "poolrad/recovered/overlay_u68_roll_gate_0294.h"

#include <string.h>

static int null_far(pr_u68_far p)
{
    return (p.offset | p.segment) == 0u;
}

static int signed_byte(uint8_t v)
{
    return v < 0x80u ? (int)v : (int)v - 256;
}

typedef struct {
    unsigned sides;     /* the second pushed byte: 6 or 8; 0 = no roll */
    unsigned limit;     /* found when roll <= limit (unsigned compare on AL) */
    int equal;          /* the original compares `cmp al, 1; jne` (== limit) instead of `ja` (<= limit) */
    int clear_flag;     /* [0x6DDE] := 0 */
    int found_no_roll;  /* found without a roll */
} rule;

static const rule none = { 0, 0, 0, 0, 0 };

/* entry 3 result 3 */
static rule rule_hit(uint8_t type, uint8_t value)
{
    rule r = none;
    if (type == 0x12u) {                               /* +0x02E3..+0x0328 */
        if (value >= 0x5Bu && value <= 0x63u) { r.sides = 6; r.limit = 1; r.equal = 1; }
        else if (value == 0x64u)               { r.sides = 6; r.limit = 2; }
        else r.clear_flag = 1;                         /* +0x0328 */
    } else if (type == 0x13u || type == 0x14u) { r.sides = 6; r.limit = 3; }   /* +0x032F..+0x034A */
    else if (type == 0x15u || type == 0x16u)   { r.sides = 6; r.limit = 4; }   /* +0x034C..+0x0367 */
    else if (type == 0x17u)                    { r.sides = 6; r.limit = 5; }   /* +0x0369..+0x0380 */
    else if (type == 0x18u)                    { r.sides = 8; r.limit = 7; }   /* +0x0382..+0x0399 */
    else if (type == 0x19u)                    r.found_no_roll = 1;            /* +0x039B..+0x03A3 */
    else r.clear_flag = 1;                             /* +0x03A5 */
    return r;
}

/* entry 3 result other than 3 */
static rule rule_miss(uint8_t type, uint8_t value)
{
    rule r = none;
    if (type >= 3u && type <= 7u)        { r.sides = 6; r.limit = 1; r.equal = 1; }         /* +0x03B4..+0x03CF */
    else if (type >= 8u && type <= 0xFu) { r.sides = 6; r.limit = 2; }         /* +0x03D2..+0x03ED */
    else if (type == 0x10u || type == 0x11u) { r.sides = 6; r.limit = 3; }     /* +0x03EF..+0x040A */
    else if (type == 0x12u) {                                                  /* +0x040C..+0x0464 */
        if (value <= 0x32u)       { r.sides = 6; r.limit = 3; }
        else if (value <= 0x63u)  { r.sides = 6; r.limit = 4; }
        else if (value == 0x64u)  { r.sides = 6; r.limit = 5; }
    }
    return r;
}

pr_u68_roll_gate_status pr_u68_roll_gate(pr_u68_roll_gate_state *st, uint8_t *al_out,
                                         pr_u68_roll_gate_report *report)
{
    pr_u68_roll_gate_report local;
    pr_u68_roll_gate_report *r = report ? report : &local;
    pr_u68_far cur;
    size_t seen = 0, i;
    uint8_t found = 0;                                  /* [bp-6] */
    memset(r, 0, sizeof *r);
    if (!st || !al_out)
        return PR_U68_ROLL_ARGUMENT;
    cur = st->head;                                     /* +0x029E..+0x02A8 */

    /* +0x02AB..+0x02BC: nil or found ends the loop */
    while (!null_far(cur) && !found) {
        const pr_u68_segment_view *view = NULL;
        uint8_t e3 = 0, type, value;
        rule ru;
        uint16_t next_at, next_seg_at;

        if (!st->views || !st->view_count || !st->visited || !st->visited_capacity)
            return PR_U68_ROLL_ARGUMENT;
        for (i = 0; i < seen; i++)
            if (st->visited[i].offset == cur.offset && st->visited[i].segment == cur.segment)
                return PR_U68_ROLL_CYCLE;
        if (seen == st->visited_capacity)
            return PR_U68_ROLL_CAPACITY;
        st->visited[seen++] = cur;
        for (i = 0; i < st->view_count; i++)
            if (st->views[i].selector == cur.segment) { view = &st->views[i]; break; }
        if (!view || !view->bytes || view->size < PR_U68_ROLL_SEGMENT_SIZE)
            return PR_U68_ROLL_ARGUMENT;
        r->nodes++;

        /* +0x02C8..+0x02D4: entry 3 (x, y, facing) */
        if (pr_u3bd_entry3(&st->field, st->party_x, st->party_y, st->facing, &e3, NULL) != PR_U3BD_FIELD_OK)
            return PR_U68_ROLL_ARGUMENT;
        if (r->nodes == 1u)
            r->first_entry3 = e3;
        type = view->bytes[(uint16_t)(cur.offset + PR_U68_ROLL_TYPE_OFFSET)];
        value = view->bytes[(uint16_t)(cur.offset + PR_U68_ROLL_VALUE_OFFSET)];
        ru = (e3 == 3u) ? rule_hit(type, value) : rule_miss(type, value);

        if (ru.clear_flag) {
            st->ds_6dde = 0;
            r->cleared_6dde++;
        }
        if (ru.found_no_roll) {
            found = 1;
        } else if (ru.sides) {
            uint8_t roll;
            if (!st->hooks.roll_9e_entry8)
                return PR_U68_ROLL_HOOK;
            /* set AL = 1, push AX; set AL = sides, push AX; lcall 0xB0:0x48; the compare is unsigned on AL */
            roll = st->hooks.roll_9e_entry8(st->hooks.ctx, (uint8_t)ru.sides, 1u);
            r->rolls++;
            if (ru.equal ? roll == ru.limit : roll <= ru.limit)
                found = 1;
        }

        /* +0x0468..+0x0478: advance, also after a hit */
        next_at = (uint16_t)(cur.offset + PR_U68_ROLL_NEXT_OFFSET);
        next_seg_at = (uint16_t)(cur.offset + PR_U68_ROLL_NEXT_OFFSET + 2u);
        if (next_at == 0xFFFFu || next_seg_at == 0xFFFFu)
            return PR_U68_ROLL_ARGUMENT;      /* host views do not represent a word at FFFF */
        cur.offset = (uint16_t)(view->bytes[next_at] | ((uint16_t)view->bytes[(uint16_t)(next_at + 1u)] << 8));
        cur.segment = (uint16_t)(view->bytes[next_seg_at] | ((uint16_t)view->bytes[(uint16_t)(next_seg_at + 1u)] << 8));
    }

    /* +0x047E..+0x04D2: found -> +0x0109 twice */
    if (found) {
        uint8_t x2, y2, sel2;
        if (!st->hooks.mark_0109)
            return PR_U68_ROLL_HOOK;
        st->hooks.mark_0109(st->hooks.ctx, st->party_x, st->party_y, st->facing);     /* +0x0484..+0x0491 */
        r->marks++;
        if (!st->direction_table || st->direction_table_size < (size_t)st->facing + 10u)
            return PR_U68_ROLL_ARGUMENT;
        x2 = (uint8_t)(signed_byte(st->party_x) + signed_byte(st->direction_table[st->facing]));         /* +0x0494..+0x04A8 */
        y2 = (uint8_t)(signed_byte(st->party_y) + signed_byte(st->direction_table[(size_t)st->facing + 9u]));  /* +0x04A9..+0x04BD */
        sel2 = (uint8_t)(((unsigned)st->facing + 4u) % 8u);                                                /* +0x04BE..+0x04CD */
        st->hooks.mark_0109(st->hooks.ctx, x2, y2, sel2);                                                  /* +0x04CE..+0x04CF */
        r->marks++;
    }
    *al_out = found;                                    /* +0x04D2..+0x04DE */
    return PR_U68_ROLL_OK;
}

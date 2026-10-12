/* Portable C model of GAME.OVR unit 0x9e +0x0160; see docs/re/U9E_0160.md.
 * Bounded model: unit 0x3c2 entry 6, the entry-1 handlers and (optionally)
 * the resident lookup are host boundaries. */
#include "poolrad/recovered/overlay_u9e_0160.h"

#include <string.h>

/* cs:0x0140: the 256-bit set. Its members, read from the 32 constant bytes (SHA-256 in the evidence
 * note; element n is bit n % 8 of byte n / 8), are the type values below. The bytes themselves are
 * not copied here. */
int pr_u9e0160_type_in_set(uint8_t type)
{
    /* 0xAF8:0x0DC0: test byte es:[bx + di], 1 << (x & 7) with bx = x >> 3, ZF clear = member */
    return type == 0x15u || type == 0x2Du || type == 0x2Eu || type == 0x31u;
}

static uint8_t pr_u9e0160_builtin_lookup(void *ctx, pr_u7d0_member *character, uint8_t key, pr_u7d0_item **cell)
{
    pr_u7d0_item *n;
    unsigned steps = 0;
    (void)ctx;
    for (n = character->head; n; n = n->next) {
        if (n->type == key) {
            *cell = n;
            return 1;
        }
        if (++steps > PR_U9E0160_STEP_CAP)
            break;
    }
    *cell = NULL;
    return 0;
}

static void pr_u9e0160_log(pr_u9e0160_report *r, pr_u9e0160_call c)
{
    if (r->calls_n < sizeof r->calls / sizeof r->calls[0])
        r->calls[r->calls_n] = c;
    r->calls_n++;
}

pr_u9e0160_status pr_u9e0160_run(pr_u9e0160_state *st, pr_u7d0_member *character, uint8_t type,
                                 pr_u9e0160_report *report)
{
    pr_u9e0160_report local;
    pr_u9e0160_report *r = report ? report : &local;
    pr_u9e0160_lookup lookup;
    pr_u7d0_item *cell = NULL;       /* [bp-0xE0] */
    pr_u7d0_member *cur;             /* [bp-0xDC] */
    uint8_t saved[PR_U9E0160_SAVE_BYTES];
    uint8_t found = 0;               /* [bp-0xE1] */
    unsigned steps = 0;
    pr_u9e0160_status status = PR_U9E0160_OK;
    memset(r, 0, sizeof *r);
    if (!st || !character)
        return PR_U9E0160_ARGUMENT;
    lookup = st->lookup ? st->lookup : pr_u9e0160_builtin_lookup;
    cur = st->party_head;

    /* +0x017B..+0x0199: cell := lookup(B, type); AL <> 0 -> found := 1, to +0x02D4 */
    pr_u9e0160_log(r, PR_U9E0160_CALL_LOOKUP);
    if (lookup(st->ctx, character, type, &cell)) {
        found = 1;
        r->found_on_character = 1;
    } else {
        /* +0x019C..+0x01AC: type in the set, else to +0x02D4 with found = 0 */
        pr_u9e0160_log(r, PR_U9E0160_CALL_SET_TEST);
        r->in_set = (unsigned)pr_u9e0160_type_in_set(type);
        if (r->in_set) {
            /* +0x01AF..+0x01BE: Move(DS:0x671B, frame, 0xD8) */
            pr_u9e0160_log(r, PR_U9E0160_CALL_SAVE);
            memcpy(saved, st->scratch + (PR_U9E0160_SAVE_DS - PR_U9E0160_SCRATCH_DS), sizeof saved);
            /* +0x01C3..+0x02BD: while cur <> nil and not found */
            while (cur && !found) {
                uint8_t ok;
                if (++steps > PR_U9E0160_STEP_CAP) {
                    status = PR_U9E0160_STEP_LIMIT;
                    break;
                }
                r->members_visited++;
                pr_u9e0160_log(r, PR_U9E0160_CALL_LOOKUP);
                ok = lookup(st->ctx, cur, type, &cell);               /* +0x01DA..+0x01F1 */
                if (ok) {
                    if (st->mode != PR_U9E0160_MODE_COMBAT) {          /* +0x01F8..+0x01FF, +0x02A2 */
                        found = 1;
                    } else {
                        uint16_t a, b, c;
                        uint8_t mode_word, count, k, mine;
                        mode_word = (type == PR_U9E0160_SPECIAL_TYPE) ? 6u : 1u;   /* +0x0202..+0x020F */
                        a = pr_u3c6_entry12(&st->slots, cur);              /* +0x0214..+0x021C */
                        b = pr_u3c6_entry13(&st->slots, cur);              /* +0x0222..+0x022A */
                        c = pr_u3c6_entry14(&st->slots, cur);              /* +0x023A..+0x0242 */
                        pr_u9e0160_log(r, PR_U9E0160_CALL_SCAN);
                        r->scans++;
                        if (st->scan)                                       /* +0x0248..+0x0250: far [0x6718]:[0x6716] */
                            st->scan(st->ctx, a, b, mode_word, 0xFFu, c,
                                     (uint16_t)(st->scratch[0] | (st->scratch[1] << 8)),
                                     (uint16_t)(st->scratch[2] | (st->scratch[3] << 8)), st->scratch);
                        count = st->scratch[PR_U9E0160_COUNT_DS - PR_U9E0160_SCRATCH_DS];   /* +0x0255 */
                        /* +0x025C..+0x029E: for k := 1 to count: if DS[0x6718 + 3k] = entry15(B) then found := 1 */
                        if (count >= 1) {
                            k = 1;
                            for (;;) {
                                mine = pr_u3c6_entry15(&st->slots, character);   /* +0x0274 (0x3E0:0x6B) */
                                if (st->scratch[PR_U9E0160_RECORDS_DS - PR_U9E0160_SCRATCH_DS + 3u * k] == mine)
                                    found = 1;
                                if (k == count)
                                    break;
                                k++;
                            }
                        }
                    }
                }
                cur = cur->next;                                           /* +0x02A7..+0x02BD */
            }
            /* +0x02C0..+0x02CF: Move(frame, DS:0x671B, 0xD8) */
            pr_u9e0160_log(r, PR_U9E0160_CALL_RESTORE);
            memcpy(st->scratch + (PR_U9E0160_SAVE_DS - PR_U9E0160_SCRATCH_DS), saved, sizeof saved);
        }
    }
    /* +0x02D4..+0x02F1: found -> +0x0000 (type, B, cell, 0) */
    r->found = found;
    if (found) {
        r->cell = cell;
        pr_u9e0160_log(r, PR_U9E0160_CALL_DISPATCH);
        pr_u9e_dispatch(st->handlers, type, character, cell, 0);
    }
    return status;
}

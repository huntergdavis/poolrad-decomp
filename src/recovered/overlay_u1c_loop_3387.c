/* Bounded C model of GAME.OVR unit 0x1c +0x3387..+0x3507; see
 * docs/re/U1C_LOOP_3387.md. The control flow follows the original's jump
 * structure with the same labels. */
#include "poolrad/recovered/overlay_u1c_loop_3387.h"

#include <string.h>

uint8_t pr_u1c_upcase(uint8_t c)
{
    return (c >= 0x61u && c <= 0x7Au) ? (uint8_t)(c - 0x20u) : c;     /* 0xAF8:0x182A */
}

static void log_call(pr_u1c_report *r, pr_u1c_call c)
{
    if (r->calls_n < sizeof r->calls / sizeof r->calls[0])
        r->calls[r->calls_n] = c;
    r->calls_n++;
}

static void put_word(uint8_t *w, uint16_t off, uint16_t v)
{
    w[off] = (uint8_t)(v & 0xFFu);
    w[off + 1u] = (uint8_t)(v >> 8);
}

static uint16_t get_word(const uint8_t *w, uint16_t off)
{
    return (uint16_t)(w[off] | (w[off + 1u] << 8));
}

#define NEED(h) do { if (!(h)) return PR_U1C_HOOK; } while (0)

pr_u1c_status pr_u1c_loop_3387(pr_u1c_state *st, pr_u1c_report *report)
{
    pr_u1c_report local;
    pr_u1c_report *r = report ? report : &local;
    const pr_u1c_hooks *h;
    uint8_t key = 0;                                  /* [bp-1] */
    unsigned outer = 0, inner = 0;
    memset(r, 0, sizeof *r);
    if (!st || !st->window0 || st->window0_size < PR_U1C_WINDOW0_MIN_SIZE || !st->window1 ||
        st->window1_size < PR_U1C_WINDOW1_MIN_SIZE)
        return PR_U1C_ARGUMENT;
    h = &st->hooks;

    /* +0x3387..+0x3390 */
    put_word(st->window0, 0x1E4u, st->ds_84dc);

L3395:
    if (++outer > PR_U1C_STEP_CAP)
        return PR_U1C_LIMIT;
    r->iterations++;
    NEED(h->entry2_key);                              /* +0x3395: lcall 0x72:0x2A */
    log_call(r, PR_U1C_CALL_ENTRY2);
    key = h->entry2_key(st->ctx, st);
    r->last_key = key;
    st->ds_4436 = st->ds_5d92;                        /* +0x339D..+0x33A7 */
    st->ds_4438 = st->ds_5d94;
    put_word(st->window0, 0x1E4u, st->ds_84dc);       /* +0x33AB..+0x33B4 */

L33B9:
    if (++inner > PR_U1C_STEP_CAP)
        return PR_U1C_LIMIT;
    r->key_loops++;
    if (get_word(st->window1, 0x594u) > 1u)           /* +0x33BD: cmp word, 1; ja +0x33D2 */
        goto L33D2;
    if (pr_u1c_upcase(key) != PR_U1C_KEY_SEARCH)      /* +0x33C5..+0x33D0 */
        goto L344C;
L33D2:
    if (st->ds_49ff != 0)                             /* +0x33D2: jne +0x344C */
        goto L344C;
    if (pr_u1c_upcase(key) == PR_U1C_KEY_SEARCH) {    /* +0x33D9..+0x33EA */
        NEED(h->near_2dd3);
        log_call(r, PR_U1C_CALL_2DD3);
        h->near_2dd3(st->ctx, st);
        goto L342C;
    }
    /* +0x33EC..+0x3427: save the low bit of word 0x594, force it to 1, view, script [0x49E5], restore */
    st->ds_84dd = (uint8_t)(get_word(st->window1, 0x594u) & 1u);
    put_word(st->window1, 0x594u, 1u);
    NEED(h->view_3ca_25);
    log_call(r, PR_U1C_CALL_VIEW);
    h->view_3ca_25(st->ctx, st);
    NEED(h->script_3171);
    log_call(r, PR_U1C_CALL_3171_E5);
    h->script_3171(st->ctx, st, st->ds_49e5);
    if (st->ds_442f != 0) {                           /* +0x3413..+0x341B */
        NEED(h->near_31b4);
        log_call(r, PR_U1C_CALL_31B4);
        h->near_31b4(st->ctx, st);
    }
    put_word(st->window1, 0x594u, st->ds_84dd);       /* +0x341E..+0x3427 */
L342C:
    if (st->ds_49ff == 0) {                           /* +0x342C..+0x3445: prompt again */
        NEED(h->entry2_key);
        log_call(r, PR_U1C_CALL_ENTRY2);
        key = h->entry2_key(st->ctx, st);
        r->last_key = key;
        st->ds_4436 = st->ds_5d92;
        st->ds_4438 = st->ds_5d94;
    }
    goto L33B9;                                       /* +0x3449 */

L344C:
    if (st->ds_49ff == 0) {                           /* +0x344C..+0x3458 */
        NEED(h->script_3171);
        log_call(r, PR_U1C_CALL_3171_E3);
        h->script_3171(st->ctx, st, st->ds_49e3);
    }
    if (st->ds_442f != 0) {                           /* +0x345B..+0x3466 */
        NEED(h->near_31b4);
        log_call(r, PR_U1C_CALL_31B4);
        h->near_31b4(st->ctx, st);
        goto L34F5;
    }
    if (st->ds_49ff != 0)                             /* +0x3469..+0x3470 */
        goto L34F5;
    /* +0x3473..+0x34F2: movement bookkeeping */
    r->movement_passes++;
    put_word(st->window0, 0x1E0u, (uint16_t)(st->party_x < 0x80u ? st->party_x : 0xFF00u | st->party_x));
    put_word(st->window0, 0x1E2u, (uint16_t)(st->party_y < 0x80u ? st->party_y : 0xFF00u | st->party_y));
    NEED(h->entry1);
    log_call(r, PR_U1C_CALL_ENTRY1);
    h->entry1(st->ctx, st);
    NEED(h->view_3ca_25);
    log_call(r, PR_U1C_CALL_VIEW);
    h->view_3ca_25(st->ctx, st);
    {
        /* +0x3497..+0x34CD: zero-extended stored word against the sign-extended byte, 32-bit compares;
           the y compare runs only when x matched */
        uint32_t old_x = get_word(st->window0, 0x1E0u);
        uint32_t new_x = (st->party_x < 0x80u) ? st->party_x : (0xFFFFFF00u | st->party_x);
        int same = (old_x == new_x);
        if (same) {
            uint32_t old_y = get_word(st->window0, 0x1E2u);
            uint32_t new_y = (st->party_y < 0x80u) ? st->party_y : (0xFFFFFF00u | st->party_y);
            same = (old_y == new_y);
        }
        if (!same) {                                  /* +0x34CF..+0x34D3 */
            NEED(h->resident_8ae_00dc);
            log_call(r, PR_U1C_CALL_8AE);
            r->position_changed++;
            h->resident_8ae_00dc(st->ctx, st, st->ds_2628);
        }
    }
    st->ds_84e0 = 0;                                  /* +0x34D8..+0x34E1 */
    st->ds_84e1 = 1;
    NEED(h->script_3171);
    log_call(r, PR_U1C_CALL_3171_E5);
    h->script_3171(st->ctx, st, st->ds_49e5);         /* +0x34E2..+0x34E7 */
    if (st->ds_442f != 0) {                           /* +0x34EA..+0x34F2 */
        NEED(h->near_31b4);
        log_call(r, PR_U1C_CALL_31B4);
        h->near_31b4(st->ctx, st);
    }
L34F5:
    if (st->ds_49ff == 0)                             /* +0x34F5..+0x34FC */
        goto L3395;
    st->ds_49ff = 0;                                  /* +0x34FF */
    return PR_U1C_OK;                                 /* +0x3504..+0x3507 */
}

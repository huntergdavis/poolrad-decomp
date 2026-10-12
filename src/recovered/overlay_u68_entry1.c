/* Portable C model of GAME.OVR unit 0x68 entry 1 (+0x0D6D); see
 * docs/re/U68_ENTRY1.md. Bounded model: the callees except the merged
 * entry 3 and the runtime string routines are host hooks. */
#include "poolrad/recovered/overlay_u68_entry1.h"

#include <string.h>

typedef struct {
    uint8_t len;
    uint8_t ch[255];
} pstr;

static void log_call(pr_u68_e1_report *r, pr_u68_e1_call c)
{
    if (r->calls_n < sizeof r->calls / sizeof r->calls[0])
        r->calls[r->calls_n] = c;
    r->calls_n++;
}

static void pstr_from(pstr *d, const uint8_t *src)    /* 0xAF8:0x0B0B (load); NULL = '' */
{
    d->len = src ? src[0] : 0;
    if (d->len)
        memcpy(d->ch, src + 1, d->len);
}

static void pstr_append(pstr *d, const uint8_t *src)  /* 0xAF8:0x0B98: length is capped at 255 */
{
    unsigned n = src ? src[0] : 0, room = 255u - d->len;
    if (n > room)
        n = room;
    if (n)
        memcpy(d->ch + d->len, src + 1, n);
    d->len = (uint8_t)(d->len + n);
}

static void pstr_store(pstr *d, const pstr *s, unsigned max)   /* 0xAF8:0x0B25: truncating store */
{
    d->len = (uint8_t)(s->len > max ? max : s->len);
    memcpy(d->ch, s->ch, d->len);
}

static int pstr_equal(const pstr *a, const uint8_t *b)       /* 0xAF8:0x0C10, ZF set */
{
    unsigned bl = b ? b[0] : 0;
    return a->len == bl && (bl == 0 || memcmp(a->ch, b + 1, bl) == 0);
}

/* s := s + lit, kept to 40 characters (load s, append lit, store with max 0x28) */
static void s_append(pstr *s, const uint8_t *lit)
{
    pstr tmp;
    tmp.len = s->len;
    memcpy(tmp.ch, s->ch, s->len);
    pstr_append(&tmp, lit);
    pstr_store(s, &tmp, PR_U68_E1_MSG_MAX);
}

#define HOOK(h, field) ((h) && (h)->field)

/* The two message blocks (+0x0DB5..+0x0EEA and +0x0EF4..+0x1026); `b` selects block B. */
static void message_block(pr_u68_e1_state *st, pr_u68_e1_report *r, unsigned b, uint8_t *flag)
{
    const pr_u68_e1_hooks *h = st->hooks;
    pstr s, tmp;
    uint8_t key;
    r->block = b ? 2u : 1u;
    s.len = 0;                                                         /* mov byte [bp-0x2b], 0 */
    if (st->ds_6dde[0] != 0) {                                         /* +0x0DB9 */
        pstr_from(&tmp, st->lit[0]);
        pstr_store(&s, &tmp, PR_U68_E1_MSG_MAX);                       /* StrStore(cs:0xD4C, s, 0x28) */
    }
    if (st->ds_6dde[1] != 0) {                                         /* +0x0DD3 */
        uint8_t v = 0;
        log_call(r, PR_U68_E1_CALL_0236);
        if (HOOK(h, near_0236)) v = h->near_0236(st->ctx, st, 6);
        if (v != 0)
            s_append(&s, st->lit[1]);                                  /* cs:0xD51 */
    }
    if (st->ds_6dde[2] != 0) {                                         /* +0x0E0D / +0x0F4C */
        int hit = 0;
        if (!b) {
            log_call(r, PR_U68_E1_CALL_05EE);
            if (HOOK(h, near_05ee)) hit = h->near_05ee(st->ctx, st, 0x1F);
        } else {
            uint8_t v = 0;
            log_call(r, PR_U68_E1_CALL_0236);
            if (HOOK(h, near_0236)) v = h->near_0236(st->ctx, st, 5);
            hit = v != 0;
        }
        if (hit)
            s_append(&s, st->lit[2]);                                  /* cs:0xD57 */
    }
    s_append(&s, st->lit[3]);                                          /* cs:0xD5E */
    r->message[0] = s.len;
    memcpy(r->message + 1, s.ch, s.len);
    r->message_equals_lit3 = (unsigned)pstr_equal(&s, st->lit[3]);     /* lcall 0xAF8:0xC10 */
    if (r->message_equals_lit3) {                                      /* +0x0E7E / +0x0FBD: je */
        log_call(r, PR_U68_E1_CALL_0000);
        if (HOOK(h, near_0000)) h->near_0000(st->ctx, st, st->party_x, st->party_y, st->facing);
        return;                                                        /* [bp-1] stays 0 */
    }
    /* prompt string [bp-0x35] := lit4, then 0x3C5:0x2A */
    r->prompted = 1;
    log_call(r, PR_U68_E1_CALL_KEY);
    key = 0;
    if (HOOK(h, key_input)) {
        uint8_t msg[0x29];
        memcpy(msg, r->message, sizeof msg);
        key = h->key_input(st->ctx, st, st->lit[4], msg, 0x0D, 0x0A, 0x0F, 0, 0);
    }
    r->key = key;
    if (key == PR_U68_E1_KEY_B) {
        log_call(r, PR_U68_E1_CALL_0294);
        *flag = HOOK(h, near_0294) ? h->near_0294(st->ctx, st) : 0;
    } else if (key == PR_U68_E1_KEY_P) {
        if (!b) {                                                      /* block A: +0x0ED4..+0x0EDA */
            log_call(r, PR_U68_E1_CALL_04DF);
            *flag = HOOK(h, near_04df) ? h->near_04df(st->ctx, st) : 0;
        } else {                                                       /* block B: +0x1013..+0x1015, no call */
            st->ds_6dde[1] = 0;                                        /* mov byte [0x6DDF], 0 */
            r->cleared_6ddf = 1;
        }
    } else if (key == PR_U68_E1_KEY_K) {
        log_call(r, PR_U68_E1_CALL_0648);
        *flag = HOOK(h, near_0648) ? h->near_0648(st->ctx, st) : 0;
    }
}

static void clear_word_592(pr_u68_e1_state *st)
{
    st->window1[PR_U68_E1_WINDOW1_WORD] = 0;
    st->window1[PR_U68_E1_WINDOW1_WORD + 1] = 0;
}

static void tail(pr_u68_e1_state *st, pr_u68_e1_report *r)      /* +0x107B..+0x10A6 */
{
    const pr_u68_e1_hooks *h = st->hooks;
    log_call(r, PR_U68_E1_CALL_3D0);
    if (HOOK(h, far_3d0_2f)) h->far_3d0_2f(st->ctx, st, PR_U68_E1_TAIL_BUFFER_DS);
    log_call(r, PR_U68_E1_CALL_464);
    if (HOOK(h, far_464_2595)) h->far_464_2595(st->ctx, st, PR_U68_E1_TAIL_REC1_DS);
    log_call(r, PR_U68_E1_CALL_464);
    if (HOOK(h, far_464_2595)) h->far_464_2595(st->ctx, st, PR_U68_E1_TAIL_REC2_DS);
    st->ds_6b00 = 0xFF;
    st->ds_6b05 = 0xFF;
}

static void resident_290b(pr_u68_e1_state *st, pr_u68_e1_report *r)
{
    log_call(r, PR_U68_E1_CALL_290B);
    if (HOOK(st->hooks, resident_ba_290b)) st->hooks->resident_ba_290b(st->ctx, st);
}

pr_u68_e1_status pr_u68_entry1(pr_u68_e1_state *st, pr_u68_e1_report *report)
{
    pr_u68_e1_report local;
    pr_u68_e1_report *r = report ? report : &local;
    const pr_u68_e1_hooks *h;
    uint8_t flag = 0;                    /* [bp-1] */
    uint16_t w592;
    memset(r, 0, sizeof *r);
    if (!st || !st->window1 || st->window1_size < PR_U68_E1_WINDOW1_MIN_SIZE)
        return PR_U68_E1_ARGUMENT;
    if (st->lit[4] && st->lit[4][0] > PR_U68_E1_PROMPT_MAX)
        return PR_U68_E1_ARGUMENT;   /* the prompt slot [bp-0x35] holds 8 characters */
    h = st->hooks;
    w592 = (uint16_t)(st->window1[PR_U68_E1_WINDOW1_WORD] | (st->window1[PR_U68_E1_WINDOW1_WORD + 1] << 8));

    if (st->selector != 1) {                                           /* +0x0D78..+0x0D7F */
        if (st->selector >= 2 && st->selector <= 4) {                  /* +0x1050..+0x1058 */
            r->selector_branch = 2;
            if (w592 < PR_U68_E1_WORD_LIMIT) {                         /* +0x105C..+0x1063 */
                log_call(r, PR_U68_E1_CALL_0818);
                if (HOOK(h, near_0818)) h->near_0818(st->ctx, st);
                resident_290b(st, r);
            } else {
                clear_word_592(st);                                    /* +0x1070 */
                r->window_cleared = 1;
            }
        }
        tail(st, r);
        return PR_U68_E1_OK;
    }
    r->selector_branch = 1;
    if (w592 >= PR_U68_E1_WORD_LIMIT) {                                /* +0x0D86..+0x0D8F, +0x1043 */
        clear_word_592(st);
        r->window_cleared = 1;
        tail(st, r);
        return PR_U68_E1_OK;
    }
    {
        uint8_t res = 0;
        if (pr_u3bd_entry3(&st->field, st->party_x, st->party_y, st->facing, &res, NULL) != PR_U3BD_FIELD_OK)
            return PR_U68_E1_ARGUMENT;
        log_call(r, PR_U68_E1_CALL_ENTRY3);
        r->entry3_result = res;
        if (res == 1)                                                  /* +0x0DA3..+0x0DAB */
            flag = 1;
        else if (res == 2)
            message_block(st, r, 0, &flag);
        else if (res == 3)
            message_block(st, r, 1, &flag);
    }
    /* +0x1027..+0x1042: the commit */
    r->move_flag = flag;
    if (flag != 0) {
        r->committed_move = 1;
        log_call(r, PR_U68_E1_CALL_073B);
        if (HOOK(h, near_073b)) h->near_073b(st->ctx, st);
    } else {
        log_call(r, PR_U68_E1_CALL_8AE);
        if (HOOK(h, resident_8ae_00dc)) h->resident_8ae_00dc(st->ctx, st, st->word_2622);
    }
    resident_290b(st, r);
    tail(st, r);
    return PR_U68_E1_OK;
}

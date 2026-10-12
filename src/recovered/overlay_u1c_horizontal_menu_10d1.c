/* Bounded model of GAME.OVR unit 0x1c +0x10D1 (ECL opcode 0x2B); see docs/re/U1C_HORIZONTAL_MENU_10D1.md. */
#include "poolrad/recovered/overlay_u1c_horizontal_menu_10d1.h"

#include <string.h>

/* The Turbo Pascal Pascal-string runtime used by the handler, translated from START.EXE image 0xBA8B, 0xBAA5, 0xBB18, 0xBB90. */
typedef struct { uint8_t len; uint8_t ch[255]; } pstr;                         /* a Pascal string value, 256-byte layout */

static void p_load(pstr *dst, const uint8_t *src)                               /* 0xAF8:0x0B0B: copy the length byte and the text */
{
    dst->len = src[0];
    memcpy(dst->ch, src + 1, src[0]);
}

static void p_cat(pstr *dst, const uint8_t *src)                                /* 0xAF8:0x0B98: append; the length is capped at 255 */
{
    unsigned room = 255u - dst->len, n = src[0] < room ? src[0] : room;
    memcpy(dst->ch + dst->len, src + 1, n);
    dst->len = (uint8_t)(dst->len + n);
}

static void p_store(uint8_t *dst, const pstr *src, unsigned maxlen)             /* 0xAF8:0x0B25: copy, truncated to the maximum */
{
    unsigned n = src->len <= maxlen ? src->len : maxlen;
    dst[0] = (uint8_t)n;
    memcpy(dst + 1, src->ch, n);
}

static int p_equal(const uint8_t *a, const uint8_t *b)                          /* 0xAF8:0x0C10 followed by `jne`: equal iff the compare's flags say equal */
{
    return a[0] == b[0] && memcmp(a + 1, b + 1, a[0]) == 0;
}

static pr_rec_ecl_handler_status window_to_handler(pr_rec_ecl_window_status ws)
{
    switch (ws) {
    case PR_REC_ECL_WINDOW_OK: return PR_REC_ECL_HANDLER_OK;
    case PR_REC_ECL_WINDOW_HOOK: return PR_REC_ECL_HANDLER_HOOK;
    case PR_REC_ECL_WINDOW_BOUNDS: return PR_REC_ECL_HANDLER_BOUNDS;
    case PR_REC_ECL_WINDOW_UNRESOLVED: return PR_REC_ECL_HANDLER_UNRESOLVED;
    default: return PR_REC_ECL_HANDLER_ARGUMENT;
    }
}

/* Reads string buffer k of the loader state as a Pascal string value. */
static void buf_get(const pr_rec_ecl_loader_state *st, unsigned k, pstr *out)
{
    out->len = st->string_len[k];
    memcpy(out->ch, st->string_text[k], PR_REC_ECL_TEXT_MAX);
}

static void append_item(pr_u1c_hmenu_report *r, const pr_rec_ecl_loader_state *st, unsigned k, const uint8_t *sep_before,
                        const uint8_t *sep_after)
{
    pstr tmp, item;
    uint8_t local[1 + 255];
    p_load(&tmp, r->text);                                                      /* 0xB0B: tmp := text */
    p_cat(&tmp, sep_before);                                                    /* 0xB98: + cs:0x10CD */
    buf_get(st, k, &item);
    local[0] = item.len; memcpy(local + 1, item.ch, item.len);
    p_cat(&tmp, local);                                                         /* 0xB98: + buffer k */
    if (sep_after)
        p_cat(&tmp, sep_after);                                                 /* 0xB98: + cs:0x10CF */
    p_store(r->text, &tmp, PR_U1C_HMENU_TEXT_MAX);                              /* 0xB25: text := tmp, maximum 0x32 */
}

pr_u1c_hmenu_status pr_u1c_op_horizontal_menu(pr_rec_ecl_loader_state *st, pr_u1c_hmenu_env *env,
                                              pr_u1c_hmenu_report *report)
{
    pr_u1c_hmenu_report local;
    pr_u1c_hmenu_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status vs;
    uint16_t v = 0u;
    uint8_t n, m, al = 0u, var5 = 0u;
    unsigned i;
    int rc;
    static const uint8_t region_args[4] = { 0x00u, 0x18u, 0x27u, 0x18u };
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status = r->store_status = PR_REC_ECL_WINDOW_OK;
    if (!st || !env)
        return PR_U1C_HMENU_ARGUMENT;

    ls = pr_rec_ecl_load_operands(st, 2u);                             /* +0x10E0..+0x10E3: loader(2) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_HMENU_CORE;
    }
    r->dest = pr_rec_ecl_operand_word(st->low[1], st->high[1]);        /* +0x10E8..+0x10F5: 0x2B:0x4D, slot 1 */
    vs = pr_rec_ecl_slot_value(st, 2u, &v);                            /* +0x10F8..+0x1100: slot value(2) -> AL */
    r->slot_status = vs;
    if (vs == PR_REC_ECL_WINDOW_BOUNDS || vs == PR_REC_ECL_WINDOW_UNRESOLVED || vs == PR_REC_ECL_WINDOW_ARGUMENT) {
        r->core_status = window_to_handler(vs);
        return PR_U1C_HMENU_CORE;
    }
    if (vs == PR_REC_ECL_WINDOW_HOOK) {                                /* the class-1 hook may override the word: n is not known, so stop
                                                                          before any n-dependent step (no PC decrement, no loader(n), no hooks) */
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_HMENU_CORE;
    }
    n = (uint8_t)(v & 0xFFu);
    r->n = n;
    if (n == 0u)
        return PR_U1C_HMENU_COUNT;
    st->pc = (uint16_t)(st->pc - 1u);                                  /* +0x1103: dec word [0x49ED] */
    ls = pr_rec_ecl_load_operands(st, n);                              /* +0x1107..+0x110B: loader(n) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_HMENU_CORE;
    }
    r->text[0] = 0u;                                                   /* +0x1110 */

    if (n == 1u) {                                                     /* +0x1114: cmp [bp-1], 1 */
        uint8_t b1[1 + 255];
        r->p3b = 1u; r->p3d = 0x0Fu; r->p3e = 0x0Fu;                   /* +0x111A..+0x1122 */
        if (!env->str_1085)
            return PR_U1C_HMENU_STRING;
        b1[0] = st->string_len[1]; memcpy(b1 + 1, st->string_text[1], PR_REC_ECL_TEXT_MAX);
        if (p_equal(b1, env->str_1085)) {                              /* +0x1126..+0x1135: 0xC10 ; jne */
            pstr src;
            if (!env->str_10aa)
                return PR_U1C_HMENU_STRING;
            p_load(&src, env->str_10aa);
            {
                uint8_t dst[1 + 255];
                p_store(dst, &src, PR_U1C_HMENU_BUF_MAX);              /* +0x1137..+0x1145: 0xB25, maximum 0xFF */
                st->string_len[1] = dst[0];
                memcpy(st->string_text[1], dst + 1, dst[0]);
            }
            r->buffer1_replaced = 1;
        }
    } else {
        r->p3b = 0u; r->p3d = 0x0Fu; r->p3e = 0x0Au;                   /* +0x114C..+0x1154 */
    }
    r->p3c = (env->b84e0 != 0u && env->b84e1 != 0u) ? 1u : 0u;         /* +0x1158..+0x116C */

    if (!env->str_10cd)
        return PR_U1C_HMENU_STRING;
    m = (uint8_t)(n - 1u);                                             /* +0x116F..+0x1175: dec ax ; AL */
    if (m >= 1u && !env->str_10cf)
        return PR_U1C_HMENU_STRING;
    for (i = 1u; i <= m; ++i)                                          /* +0x1178..+0x11D5 */
        append_item(r, st, i, env->str_10cd, env->str_10cf);
    append_item(r, st, n, env->str_10cd, NULL);                        /* +0x11D7..+0x1210 */

    if (!env->hooks.menu_25_0061)
        return PR_U1C_HMENU_HOOK;
    r->menu_called = 1;
    rc = env->hooks.menu_25_0061(env->ctx, env, &var5, r->text, PR_U1C_HMENU_KIND, r->p3e, r->p3d, r->p3b, r->p3c, &al);   /* +0x1215..+0x1237 */
    r->var5 = var5;
    if (rc != 0) { r->callback_status = rc; return PR_U1C_HMENU_CALLBACK; }
    r->result = al;

    r->stored = 1;
    r->store_status = pr_rec_ecl_store_word(st, r->dest, al);          /* +0x123A..+0x1243: 0x2B:0x52 (dest, AL) */
    if (r->store_status != PR_REC_ECL_WINDOW_OK && r->store_status != PR_REC_ECL_WINDOW_HOOK) {
        r->core_status = window_to_handler(r->store_status);
        return PR_U1C_HMENU_CORE;
    }
    if (!env->hooks.region_709_0dd3)
        return PR_U1C_HMENU_HOOK;
    r->region_called = 1;
    rc = env->hooks.region_709_0dd3(env->ctx, env, region_args);       /* +0x1248..+0x1254 */
    if (rc != 0) { r->callback_status = rc; return PR_U1C_HMENU_CALLBACK; }
    if (r->store_status == PR_REC_ECL_WINDOW_HOOK) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_HMENU_CORE;
    }
    return PR_U1C_HMENU_OK;
}

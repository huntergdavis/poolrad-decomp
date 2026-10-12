/* Bounded model of unit 0x3ce entry 4 (+0x01C8); see docs/re/U3CE_ENTRY4_01C8.md. */
#include "poolrad/recovered/overlay_u3ce_entry4_01c8.h"

#include <string.h>

static int pstr_eq(const uint8_t *a, const uint8_t *b)       /* 0xAF8:0x0C10: equal length and bytes */
{
    return a[0] == b[0] && memcmp(a + 1, b + 1, a[0]) == 0;
}

static void pstr_store(uint8_t *dst, const uint8_t *src)      /* 0xAF8:0x0B25 with maximum 255 (a length byte cannot exceed it) */
{
    memcpy(dst, src, 1u + src[0]);
}

static void pstr_copy(uint8_t *dst, const uint8_t *src, int index, int count)   /* 0xAF8:0x0B57 */
{
    int len = src[0], n;
    if (index <= 0) index = 1;
    if (count < 0) count = 0;
    n = len - index + 1;
    if (n < 0) n = 0;
    if (n > count) n = count;
    dst[0] = (uint8_t)n;
    if (n > 0) memcpy(dst + 1, src + index, (size_t)n);
}

static void pstr_cat(uint8_t *dst, const uint8_t *suffix)     /* 0xAF8:0x0B98: append, length capped at 255 */
{
    unsigned room = PR_U3CE_E4_STR_MAX - dst[0], n = suffix[0] < room ? suffix[0] : room;
    memcpy(dst + 1 + dst[0], suffix + 1, n);
    dst[0] = (uint8_t)(dst[0] + n);
}

typedef struct { const pr_u3ce_e4_env *env; pr_u3ce_e4_report *r; uint8_t slot; } Run;

static pr_u3ce_e4_status log_call(Run *u, pr_u3ce_e4_call k, int hook_present, int result)
{
    if (!hook_present) { u->r->failed_call = k; return PR_U3CE_E4_HOOK; }
    u->r->calls[u->r->n_calls++] = k;
    if (result != 0) { u->r->callback_status = result; u->r->failed_call = k; return PR_U3CE_E4_CALLBACK; }
    return PR_U3CE_E4_OK;
}

static pr_u3ce_e4_status do_load(Run *u, unsigned which)
{
    const pr_u3ce_e4_env *e = u->env;
    pr_u3ce_slot *sl = &e->slots[u->slot];
    uint8_t p3 = which ? (uint8_t)(u->r->p3_used + 0x80u) : u->r->p3_used;     /* the second load adds 0x80 (low byte is what the callee reads) */
    int res = 0;
    if (e->hooks.load_464_0111)
        res = e->hooks.load_464_0111(e->ctx, e, u->r->name, p3, which, which ? &sl->second : &sl->first);
    return log_call(u, PR_U3CE_E4_CALL_LOAD, e->hooks.load_464_0111 != NULL, res);
}

static pr_u3ce_e4_status do_palette(Run *u, unsigned which, uint16_t table)
{
    const pr_u3ce_e4_env *e = u->env;
    const pr_u3ce_slot *sl = &e->slots[u->slot];
    int res = 0;
    if (e->hooks.palette_464_1e5c)
        res = e->hooks.palette_464_1e5c(e->ctx, e, table, which ? sl->second : sl->first);   /* reads the slot word pair after the load wrote it */
    return log_call(u, PR_U3CE_E4_CALL_PALETTE, e->hooks.palette_464_1e5c != NULL, res);
}

pr_u3ce_e4_status pr_u3ce_entry4(const pr_u3ce_e4_env *env, const uint8_t *name, uint8_t p3, uint8_t slot, pr_u3ce_e4_report *report)
{
    pr_u3ce_e4_report local;
    pr_u3ce_e4_report *r = report ? report : &local;
    Run u;
    uint8_t s[1 + PR_U3CE_E4_STR_MAX], t[1 + PR_U3CE_E4_STR_MAX], suffix[2] = { 0, 0 };
    pr_u3ce_e4_status st;
    int has_palette = 0, interleaved = 0;
    uint16_t pal_table = 0;
    memset(r, 0, sizeof *r);
    if (!env || !name || !env->slots || !env->c1b0 || !env->c1b6 || !env->c1bc || !env->c1c3)
        return PR_U3CE_E4_ARGUMENT;
    u.env = env; u.r = r; u.slot = slot;

    pstr_store(s, name);                                                     /* +0x01CF..+0x01DE */
    r->p3_used = p3;
    pstr_copy(t, s, 1, 5);                                                   /* +0x01E3..+0x0206 */
    if (pstr_eq(t, env->c1b0)) {
        r->path = PR_U3CE_E4_PATH_A;
    } else {
        pstr_copy(t, s, 1, 5);                                               /* +0x0208..+0x022B */
        if (pstr_eq(t, env->c1b6)) r->path = PR_U3CE_E4_PATH_A;
    }
    if (r->path == PR_U3CE_E4_PATH_A) {
        uint8_t last = s[s[0]], up = (last >= 0x61 && last <= 0x7A) ? (uint8_t)(last - 0x20) : last;   /* +0x0230..+0x023D, UpCase 0x182A */
        if (up == 0x54) { r->p3_used = (uint8_t)(r->p3_used + 0x40u); r->t_suffix = 1; }               /* +0x0242..+0x0246 */
        pstr_copy(t, s, 1, (int)(uint16_t)(s[0] - 1u));                      /* +0x024A..+0x0262: Copy(s, 1, len - 1) */
        pstr_store(s, t);                                                    /* +0x0267..+0x0271 */
    } else if (pstr_eq(s, env->c1bc)) {                                      /* +0x02FB..+0x030B */
        r->path = PR_U3CE_E4_PATH_B6;
    } else if (pstr_eq(s, env->c1c3)) {                                      /* +0x030D..+0x031D */
        r->path = PR_U3CE_E4_PATH_B4; has_palette = 1; pal_table = 0x0CF2u;  /* +0x03B6..: the palette pair runs only for the 4-char constant */
    } else {
        int res = 0;
        r->path = PR_U3CE_E4_PATH_C;                                         /* +0x0409..+0x041F */
        if (env->hooks.str_af8_1366) res = env->hooks.str_af8_1366(env->ctx, env, (uint32_t)env->b5376, suffix);
        st = log_call(&u, PR_U3CE_E4_CALL_STR, env->hooks.str_af8_1366 != NULL, res);
        if (st != PR_U3CE_E4_OK) goto done;
        if (suffix[0] > 1u) suffix[0] = 1u;                                  /* string[1] */
        has_palette = 1; interleaved = 1; pal_table = 0x0CE2u;   /* path C interleaves load, palette, load, palette */
    }
    pstr_store(r->s, s);
    pstr_store(r->name, s); pstr_cat(r->name, suffix);                       /* 0x0B0B then 0x0B98 */
    if ((st = do_load(&u, 0)) != PR_U3CE_E4_OK) goto done;
    if (interleaved && (st = do_palette(&u, 0, pal_table)) != PR_U3CE_E4_OK) goto done;
    if ((st = do_load(&u, 1)) != PR_U3CE_E4_OK) goto done;
    if (has_palette && !interleaved && (st = do_palette(&u, 0, pal_table)) != PR_U3CE_E4_OK) goto done;   /* path B4: both loads, then both palettes */
    if (has_palette && (st = do_palette(&u, 1, pal_table)) != PR_U3CE_E4_OK) goto done;
    {
        int res = 0;
        if (env->hooks.flush_899_0104) res = env->hooks.flush_899_0104(env->ctx, env);   /* +0x04F1 */
        st = log_call(&u, PR_U3CE_E4_CALL_FLUSH, env->hooks.flush_899_0104 != NULL, res);
    }
done:
    return st;
}

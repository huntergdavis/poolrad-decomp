/* Synthetic parity test for the unit 0x1c +0x3387 loop slice. All DS state,
 * the two windows and every hook are fabricated. A program-counter
 * restatement of the original's jump structure (a different shape from the
 * model's labels) is run with identical deterministic hooks on a copy of the
 * state and compared for the call log, the final state and the report over
 * generated hook behaviours; hand-traced scenes pin the Up-key route. */
#include "poolrad/recovered/overlay_u1c_loop_3387.h"
#include "poolrad/recovered/overlay_u68_entry1.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define W 0x800u

typedef struct {
    uint32_t seed; unsigned calls;
    uint8_t keys[8]; unsigned nkeys, kidx;
    int log[300]; unsigned nlog;
    unsigned force_exit_after;
    uint16_t eight_ae_word; unsigned eight_ae_calls;
    uint16_t last_script_entry[2]; unsigned nscript;
} Wd;

static unsigned rnd(Wd *w) { w->seed = w->seed * 1664525u + 1013904223u; return w->seed >> 16; }
static void note(Wd *w, int v) { w->log[w->nlog++] = v; w->calls++; }
static void maybe_exit(Wd *w, pr_u1c_state *s) { if (w->calls > w->force_exit_after) s->ds_49ff = 1; }

static uint8_t h_key(void *c, pr_u1c_state *s)
{
    Wd *w = c; uint8_t k = w->keys[w->kidx++ % w->nkeys];
    note(w, 1); (void)s; return k;
}
static void h_entry1(void *c, pr_u1c_state *s)
{
    Wd *w = c; unsigned r = rnd(w) % 6;
    note(w, 2);
    if (r == 0) s->party_x = (uint8_t)(s->party_x + 1u);
    else if (r == 1) s->party_y = (uint8_t)(s->party_y - 1u);
    else if (r == 2) s->party_x = (uint8_t)(0x80u + (rnd(w) & 3u));
}
static void h_2dd3(void *c, pr_u1c_state *s) { Wd *w = c; note(w, 3); if (rnd(w) % 3 == 0) s->ds_49ff = 1; }
static void h_script(void *c, pr_u1c_state *s, uint16_t e)
{
    Wd *w = c; unsigned r = rnd(w) % 5;
    note(w, 400 + (int)(e & 0xFF)); w->last_script_entry[w->nscript++ & 1u] = e;
    s->ds_442f = (uint8_t)(r == 0 || r == 1);
    if (r == 2) s->ds_49ff = 1;
    maybe_exit(w, s);
}
static void h_31b4(void *c, pr_u1c_state *s) { Wd *w = c; note(w, 6); s->ds_442f = (uint8_t)(rnd(w) % 4 == 0); if (rnd(w) % 4 == 0) s->ds_49ff = 1; maybe_exit(w, s); }
static void h_view(void *c, pr_u1c_state *s) { Wd *w = c; (void)s; note(w, 7); }
static void h_8ae(void *c, pr_u1c_state *s, uint16_t word) { Wd *w = c; (void)s; note(w, 8); w->eight_ae_word = word; w->eight_ae_calls++; }


/* deterministic scene hooks */
typedef struct {
    uint8_t keys[4]; unsigned nkeys, kidx;
    int move_dx; unsigned nscripts; uint16_t script_entries[16]; uint16_t w8ae;
    int exit_on_third_script, set_442f_first_script, exit_in_31b4, exit_on_first_script, exit_in_2dd3_second, exit_on_first_e3;
    int record_w594; unsigned w594_in_view, w594_in_script;
} Sc;
static uint8_t sc_key(void *c, pr_u1c_state *s) { Sc *x = c; (void)s; return x->keys[x->kidx++ % x->nkeys]; }
static void sc_entry1(void *c, pr_u1c_state *s) { Sc *x = c; s->party_x = (uint8_t)(s->party_x + x->move_dx); }
static void sc_2dd3(void *c, pr_u1c_state *s) { Sc *x = c; (void)s; (void)x; }
static void sc_script(void *c, pr_u1c_state *s, uint16_t e)
{
    Sc *x = c; unsigned i = x->nscripts++;
    if (i < 16) x->script_entries[i] = e;
    if (x->record_w594 && e == s->ds_49e5) x->w594_in_script = (unsigned)(s->window1[0x594] | (s->window1[0x595] << 8));
    if (x->exit_on_third_script && i == 2) s->ds_49ff = 1;
    if (x->exit_on_first_script && i == 0) s->ds_49ff = 1;
    if (x->exit_on_first_e3 && e == s->ds_49e3) s->ds_49ff = 1;
    if (x->set_442f_first_script && i == 0) s->ds_442f = 1;
}
static void sc_31b4(void *c, pr_u1c_state *s) { Sc *x = c; if (x->exit_in_31b4) s->ds_49ff = 1; s->ds_442f = 0; }
static void sc_view(void *c, pr_u1c_state *s) { Sc *x = c; if (x->record_w594) x->w594_in_view = (unsigned)(s->window1[0x594] | (s->window1[0x595] << 8)); }
static void sc_8ae(void *c, pr_u1c_state *s, uint16_t word) { Sc *x = c; (void)s; x->w8ae = word; }

static const pr_u1c_hooks hooks = { h_key, h_entry1, h_2dd3, h_script, h_31b4, h_view, h_8ae };

/* the original's flow as a program-counter machine (different shape from the model) */
static void ref_run(pr_u1c_state *s, Wd *w)
{
    enum { P3387, P3395, P33B9, P33D2, P33EC, P342C, P344C, P3473, P34F5, PDONE } pc = P3387;
    uint8_t key = 0;
    while (pc != PDONE) {
        switch (pc) {
        case P3387: s->window0[0x1E4] = s->ds_84dc; s->window0[0x1E5] = 0; pc = P3395; break;
        case P3395:
            key = h_key(w, s); s->ds_4436 = s->ds_5d92; s->ds_4438 = s->ds_5d94;
            s->window0[0x1E4] = s->ds_84dc; s->window0[0x1E5] = 0; pc = P33B9; break;
        case P33B9: {
            unsigned w594 = (unsigned)(s->window1[0x594] | (s->window1[0x595] << 8));
            if (w594 <= 1 && pr_u1c_upcase(key) != 0x45) pc = P344C; else pc = P33D2; break; }
        case P33D2:
            if (s->ds_49ff != 0) pc = P344C;
            else if (pr_u1c_upcase(key) == 0x45) { h_2dd3(w, s); pc = P342C; }
            else pc = P33EC;
            break;
        case P33EC:
            s->ds_84dd = (uint8_t)(s->window1[0x594] & 1u); s->window1[0x594] = 1; s->window1[0x595] = 0;
            h_view(w, s); h_script(w, s, s->ds_49e5);
            if (s->ds_442f != 0) h_31b4(w, s);
            s->window1[0x594] = s->ds_84dd; s->window1[0x595] = 0;
            pc = P342C; break;
        case P342C:
            if (s->ds_49ff == 0) { key = h_key(w, s); s->ds_4436 = s->ds_5d92; s->ds_4438 = s->ds_5d94; }
            pc = P33B9; break;
        case P344C:
            if (s->ds_49ff == 0) h_script(w, s, s->ds_49e3);
            if (s->ds_442f != 0) { h_31b4(w, s); pc = P34F5; }
            else if (s->ds_49ff != 0) pc = P34F5;
            else pc = P3473;
            break;
        case P3473: {
            int xs = s->party_x < 0x80 ? s->party_x : (int)s->party_x - 256, ys = s->party_y < 0x80 ? s->party_y : (int)s->party_y - 256;
            unsigned ow, oy;
            s->window0[0x1E0] = (uint8_t)xs; s->window0[0x1E1] = (uint8_t)((unsigned)xs >> 8);
            s->window0[0x1E2] = (uint8_t)ys; s->window0[0x1E3] = (uint8_t)((unsigned)ys >> 8);
            h_entry1(w, s); h_view(w, s);
            ow = (unsigned)(s->window0[0x1E0] | (s->window0[0x1E1] << 8)); oy = (unsigned)(s->window0[0x1E2] | (s->window0[0x1E3] << 8));
            xs = s->party_x < 0x80 ? s->party_x : (int)s->party_x - 256; ys = s->party_y < 0x80 ? s->party_y : (int)s->party_y - 256;
            /* zero-extended 32-bit word equals the sign-extended 32-bit byte only for non-negative values */
            if (!(xs >= 0 && ow == (unsigned)xs && ys >= 0 && oy == (unsigned)ys)) {
                h_8ae(w, s, s->ds_2628);
            }
            s->ds_84e0 = 0; s->ds_84e1 = 1;
            h_script(w, s, s->ds_49e5);
            if (s->ds_442f != 0) h_31b4(w, s);
            pc = P34F5; break; }
        case P34F5:
            if (s->ds_49ff == 0) pc = P3395; else { s->ds_49ff = 0; pc = PDONE; }
            break;
        default: pc = PDONE; break;
        }
    }
}

static void init(pr_u1c_state *s, uint8_t *w0, uint8_t *w1)
{
    memset(s, 0, sizeof *s);
    memset(w0, 0x55, W); memset(w1, 0x66, W);
    w1[0x594] = 0; w1[0x595] = 0;        /* word 0x594 = 0 unless a case sets it */
    s->window0 = w0; s->window0_size = W; s->window1 = w1; s->window1_size = W;
    s->hooks = hooks;
}

int main(void)
{
    static uint8_t a0[W], a1[W], b0[W], b1[W];
    pr_u1c_state sa, sb; Wd wa, wb; pr_u1c_report rep;
    unsigned seed;

    /* generated: random state and hook behaviour; model vs the pc-machine */
    for (seed = 0; seed < 5000; seed++) {
        init(&sa, a0, a1);
        sa.ds_49ff = (uint8_t)((seed >> 3) & 1u); sa.ds_442f = (uint8_t)((seed >> 4) & 1u);
        sa.ds_84dc = (uint8_t)seed; sa.ds_2628 = (uint16_t)(seed * 7u); sa.ds_49e3 = 0x0103; sa.ds_49e5 = 0x0207;
        sa.ds_5d92 = (uint16_t)(seed * 3u); sa.ds_5d94 = (uint16_t)(seed + 9u);
        sa.party_x = (uint8_t)(seed * 5u); sa.party_y = (uint8_t)(seed * 11u);
        a1[0x594] = (uint8_t)((seed >> 5) % 4u); a1[0x595] = (uint8_t)(((seed >> 7) & 3u) == 0 ? 1 : 0);   /* word 0, 1, 2, 3 and 0x100.. */
        memset(&wa, 0, sizeof wa); wa.seed = seed * 2654435761u + 1u; wa.force_exit_after = 12 + seed % 20;
        wa.nkeys = 6;
        wa.keys[0] = 0x48; wa.keys[1] = 'E'; wa.keys[2] = 'e'; wa.keys[3] = 'h'; wa.keys[4] = 0x45; wa.keys[5] = 'x';
        /* identical copy for the reference run */
        sb = sa; memcpy(b0, a0, W); memcpy(b1, a1, W); sb.window0 = b0; sb.window1 = b1; wb = wa;
        /* the model */
        sa.ctx = &wa;
        CHECK(pr_u1c_loop_3387(&sa, &rep) == PR_U1C_OK);
        sb.ctx = &wb;
        ref_run(&sb, &wb);
        CHECK(wa.nlog == wb.nlog && memcmp(wa.log, wb.log, wa.nlog * sizeof(int)) == 0);
        CHECK(wa.eight_ae_calls == wb.eight_ae_calls && wa.eight_ae_word == wb.eight_ae_word);
        CHECK(sa.ds_49ff == 0 && sb.ds_49ff == 0);
        CHECK(sa.ds_442f == sb.ds_442f && sa.ds_84dd == sb.ds_84dd && sa.ds_84e0 == sb.ds_84e0 && sa.ds_84e1 == sb.ds_84e1);
        CHECK(sa.ds_4436 == sb.ds_4436 && sa.ds_4438 == sb.ds_4438 && sa.party_x == sb.party_x && sa.party_y == sb.party_y);
        CHECK(memcmp(a0, b0, W) == 0 && memcmp(a1, b1, W) == 0);
        CHECK(rep.position_changed == wa.eight_ae_calls);
    }

    /* hand-traced Up-key scenes with deterministic hooks (no saved tuple is assumed; the party bytes are inputs) */
    {
        static uint8_t w0[W], w1[W];
        Sc sc; pr_u1c_state st; pr_u1c_hooks hk;
        memset(&hk, 0, sizeof hk);
        hk.entry2_key = sc_key; hk.entry1 = sc_entry1; hk.script_3171 = sc_script; hk.near_31b4 = sc_31b4;
        hk.view_3ca_25 = sc_view; hk.resident_8ae_00dc = sc_8ae; hk.near_2dd3 = sc_2dd3;

        /* scene 1: key 0x48 (Up), word 0x594 = 0, [0x49FF] = 0, [0x442F] = 0: straight to +0x344C; script [0x49E3]; the
           movement bookkeeping; entry 1 moves x 15 -> 14 (position changed -> 0x8AE with [0x2628]); script [0x49E5];
           the loop test passes (flag 0) -> second prompt (key 'x'): +0x344C, script [0x49E3] sets [0x49FF] -> exit */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc;
        st.party_x = 15; st.party_y = 4; st.ds_84dc = 3; st.ds_49e3 = 0x1111; st.ds_49e5 = 0x2222; st.ds_2628 = 0xBEEF;
        st.ds_5d92 = 0x0A0A; st.ds_5d94 = 0x0B0B;
        w1[0x594] = 0; w1[0x595] = 0;
        sc.keys[0] = 0x48; sc.keys[1] = 'x'; sc.nkeys = 2; sc.move_dx = -1; sc.exit_on_third_script = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK);
        CHECK(rep.iterations == 2 && rep.movement_passes == 1 && rep.position_changed == 1);
        CHECK(rep.calls_n == 8);
        CHECK(rep.calls[0] == PR_U1C_CALL_ENTRY2 && rep.calls[1] == PR_U1C_CALL_3171_E3 && rep.calls[2] == PR_U1C_CALL_ENTRY1 &&
              rep.calls[3] == PR_U1C_CALL_VIEW && rep.calls[4] == PR_U1C_CALL_8AE && rep.calls[5] == PR_U1C_CALL_3171_E5 &&
              rep.calls[6] == PR_U1C_CALL_ENTRY2 && rep.calls[7] == PR_U1C_CALL_3171_E3);
        CHECK(sc.script_entries[0] == 0x1111 && sc.script_entries[1] == 0x2222 && sc.script_entries[2] == 0x1111 && sc.nscripts == 3);
        CHECK(st.party_x == 14 && st.party_y == 4 && sc.w8ae == 0xBEEF && st.ds_84e0 == 0 && st.ds_84e1 == 1);
        CHECK(w0[0x1E4] == 3 && w0[0x1E5] == 0 && w0[0x1E0] == 15 && w0[0x1E1] == 0 && w0[0x1E2] == 4);   /* the pre-move words */
        CHECK(st.ds_4436 == 0x0A0A && st.ds_4438 == 0x0B0B && st.ds_49ff == 0 && rep.last_key == 'x');

        /* scene 2: the same Up key but the entry-1 hook leaves the party in place: no 0x8AE call */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4; st.ds_2628 = 1;
        sc.keys[0] = 0x48; sc.keys[1] = 'x'; sc.nkeys = 2; sc.move_dx = 0; sc.exit_on_third_script = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK && rep.position_changed == 0 && rep.movement_passes == 1);
        CHECK(rep.calls_n == 7 && rep.calls[4] == PR_U1C_CALL_3171_E5);

        /* scene 3: [0x442F] set by the first script: +0x31B4 runs and the movement bookkeeping is skipped */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.set_442f_first_script = 1; sc.exit_in_31b4 = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK && rep.movement_passes == 0 && rep.calls_n == 3);
        CHECK(rep.calls[1] == PR_U1C_CALL_3171_E3 && rep.calls[2] == PR_U1C_CALL_31B4 && st.party_x == 15);

        /* scene 4: a coordinate byte >= 0x80 always reads as changed (32-bit compare of zero- and sign-extended values) */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 0x90; st.party_y = 2;
        sc.keys[0] = 0x48; sc.keys[1] = 'x'; sc.nkeys = 2; sc.move_dx = 0; sc.exit_on_third_script = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK && rep.position_changed == 1);

        /* scene 5: key 'e' (upcase 'E') with word 0x594 = 0 enters the search branch: near +0x2DD3, then the re-prompt */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; w1[0x594] = 0; w1[0x595] = 0;
        sc.keys[0] = 'e'; sc.keys[1] = 'x'; sc.nkeys = 2; sc.exit_on_third_script = 0; sc.exit_in_2dd3_second = 0;
        sc.exit_on_first_script = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK);
        CHECK(rep.calls[0] == PR_U1C_CALL_ENTRY2 && rep.calls[1] == PR_U1C_CALL_2DD3 && rep.calls[2] == PR_U1C_CALL_ENTRY2 &&
              rep.calls[3] == PR_U1C_CALL_3171_E3);

        /* scene 6: word 0x594 = 3 (> 1) with a non-'E' key still enters the +0x33D2 branch: [0x84DD] saves bit 0, word 0x594
           is forced to 1 during the view and script [0x49E5], and restored afterwards */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.ds_49e5 = 0x7777; w1[0x594] = 3; w1[0x595] = 0;
        sc.keys[0] = 'x'; sc.keys[1] = 'x'; sc.nkeys = 2; sc.record_w594 = 1; sc.exit_on_first_e3 = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK);
        CHECK(rep.calls[1] == PR_U1C_CALL_VIEW && rep.calls[2] == PR_U1C_CALL_3171_E5);
        CHECK(sc.w594_in_view == 1 && sc.w594_in_script == 1 && st.ds_84dd == 1 && w1[0x594] == 1);   /* 3 and 1 = 1 saved, restored to 1 */
        /* the same with word 0x594 = 2: bit 0 clear, restored to 0 */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; w1[0x594] = 2; w1[0x595] = 0;
        sc.keys[0] = 'x'; sc.keys[1] = 'x'; sc.nkeys = 2; sc.record_w594 = 1; sc.exit_on_first_e3 = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK && sc.w594_in_view == 1 && st.ds_84dd == 0 && w1[0x594] == 0);

        /* scene 7: a reached NULL hook stops explicitly; a loop that never ends hits the cap; arguments */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.hooks.entry1 = NULL; st.ctx = &sc; sc.keys[0] = 0x48; sc.nkeys = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_HOOK && rep.calls[1] == PR_U1C_CALL_3171_E3 && rep.movement_passes == 1);
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; sc.keys[0] = 0x48; sc.nkeys = 1; sc.move_dx = 0;                   /* never sets [0x49FF] */
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_LIMIT && rep.iterations == PR_U1C_STEP_CAP + 1u - 1u);
        init(&st, w0, w1); st.window0_size = 0x1E5;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT);
        CHECK(pr_u1c_loop_3387(NULL, &rep) == PR_U1C_ARGUMENT);
    }

    /* the upcase edges (runtime 0xAF8:0x182A) */
    CHECK(pr_u1c_upcase('a') == 'A' && pr_u1c_upcase('z') == 'Z' && pr_u1c_upcase('`') == '`' && pr_u1c_upcase('{') == '{' &&
          pr_u1c_upcase('E') == 'E' && pr_u1c_upcase(0x48) == 0x48 && pr_u1c_upcase(0xE5) == 0xE5);

    puts("overlay_u1c_loop_3387: ok");
    return 0;
}

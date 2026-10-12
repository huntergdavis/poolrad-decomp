/* Synthetic parity test for the unit 0x1c +0x3387 loop slice. All DS state,
 * the two windows and every hook are fabricated. A program-counter
 * restatement of the original's jump structure (a different shape from the
 * model's labels) is run with identical deterministic, fallible hooks on a
 * copy of the state and compared for the call log, the final state and the
 * status over generated hook behaviours (with and without an injected
 * callback failure); hand-traced scenes pin the Up-key route, the staged
 * window guard and the failure semantics. */
#include "poolrad/recovered/overlay_u1c_loop_3387.h"

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
    unsigned force_exit_after, fail_at;     /* fail_at: the call (1-based) whose hook returns nonzero; 0 = never */
    uint16_t eight_ae_word; unsigned eight_ae_calls;
} Wd;

static unsigned rnd(Wd *w) { w->seed = w->seed * 1664525u + 1013904223u; return w->seed >> 16; }
static int note(Wd *w, int v) { w->log[w->nlog++] = v; w->calls++; return w->fail_at != 0 && w->calls == w->fail_at; }
static void maybe_exit(Wd *w, pr_u1c_state *s) { if (w->calls > w->force_exit_after) s->ds_49ff = 1; }

static int h_key(void *c, pr_u1c_state *s, uint8_t *key)
{
    Wd *w = c; (void)s;
    if (note(w, 1)) return 7;                      /* no key supplied */
    *key = w->keys[w->kidx++ % w->nkeys];
    return 0;
}
static int h_entry1(void *c, pr_u1c_state *s)
{
    Wd *w = c; unsigned r = rnd(w) % 6;
    if (r == 0) s->party_x = (uint8_t)(s->party_x + 1u);
    else if (r == 1) s->party_y = (uint8_t)(s->party_y - 1u);
    else if (r == 2) s->party_x = (uint8_t)(0x80u + (rnd(w) & 3u));
    return note(w, 2) ? 7 : 0;
}
static int h_2dd3(void *c, pr_u1c_state *s) { Wd *w = c; if (rnd(w) % 3 == 0) s->ds_49ff = 1; return note(w, 3) ? 7 : 0; }
static int h_script(void *c, pr_u1c_state *s, uint16_t e)
{
    Wd *w = c; unsigned r = rnd(w) % 5;
    s->ds_442f = (uint8_t)(r == 0 || r == 1);
    if (r == 2) s->ds_49ff = 1;
    maybe_exit(w, s);
    return note(w, 400 + (int)(e & 0xFF)) ? 7 : 0;
}
static int h_31b4(void *c, pr_u1c_state *s)
{
    Wd *w = c; s->ds_442f = (uint8_t)(rnd(w) % 4 == 0); if (rnd(w) % 4 == 0) s->ds_49ff = 1; maybe_exit(w, s);
    return note(w, 6) ? 7 : 0;
}
static int h_view(void *c, pr_u1c_state *s) { Wd *w = c; (void)s; return note(w, 7) ? 7 : 0; }
static int h_8ae(void *c, pr_u1c_state *s, uint16_t word) { Wd *w = c; (void)s; w->eight_ae_word = word; w->eight_ae_calls++; return note(w, 8) ? 7 : 0; }

static const pr_u1c_hooks hooks = { h_key, h_entry1, h_2dd3, h_script, h_31b4, h_view, h_8ae };

/* the original's flow as a program-counter machine; returns 0 (done) or 5 (a hook failed) */
#define RC(x) do { if (x) return 5; } while (0)
static int ref_run(pr_u1c_state *s, Wd *w)
{
    enum { P3387, P3395, P33B9, P33D2, P33EC, P342C, P344C, P3473, P34F5, PDONE } pc = P3387;
    uint8_t key = 0;
    while (pc != PDONE) {
        switch (pc) {
        case P3387: s->window0[0x1E4] = s->ds_84dc; s->window0[0x1E5] = 0; pc = P3395; break;
        case P3395:
            RC(h_key(w, s, &key)); s->ds_4436 = s->ds_5d92; s->ds_4438 = s->ds_5d94;
            s->window0[0x1E4] = s->ds_84dc; s->window0[0x1E5] = 0; pc = P33B9; break;
        case P33B9: {
            unsigned w594 = (unsigned)(s->window1[0x594] | (s->window1[0x595] << 8));
            if (w594 <= 1 && pr_u1c_upcase(key) != 0x45) pc = P344C; else pc = P33D2; break; }
        case P33D2:
            if (s->ds_49ff != 0) pc = P344C;
            else if (pr_u1c_upcase(key) == 0x45) { RC(h_2dd3(w, s)); pc = P342C; }
            else pc = P33EC;
            break;
        case P33EC:
            s->ds_84dd = (uint8_t)(s->window1[0x594] & 1u); s->window1[0x594] = 1; s->window1[0x595] = 0;
            RC(h_view(w, s)); RC(h_script(w, s, s->ds_49e5));
            if (s->ds_442f != 0) RC(h_31b4(w, s));
            s->window1[0x594] = s->ds_84dd; s->window1[0x595] = 0;
            pc = P342C; break;
        case P342C:
            if (s->ds_49ff == 0) { RC(h_key(w, s, &key)); s->ds_4436 = s->ds_5d92; s->ds_4438 = s->ds_5d94; }
            pc = P33B9; break;
        case P344C:
            if (s->ds_49ff == 0) RC(h_script(w, s, s->ds_49e3));
            if (s->ds_442f != 0) { RC(h_31b4(w, s)); pc = P34F5; }
            else if (s->ds_49ff != 0) pc = P34F5;
            else pc = P3473;
            break;
        case P3473: {
            int xs = s->party_x < 0x80 ? s->party_x : (int)s->party_x - 256, ys = s->party_y < 0x80 ? s->party_y : (int)s->party_y - 256;
            unsigned ow, oy;
            s->window0[0x1E0] = (uint8_t)xs; s->window0[0x1E1] = (uint8_t)((unsigned)xs >> 8);
            s->window0[0x1E2] = (uint8_t)ys; s->window0[0x1E3] = (uint8_t)((unsigned)ys >> 8);
            RC(h_entry1(w, s)); RC(h_view(w, s));
            ow = (unsigned)(s->window0[0x1E0] | (s->window0[0x1E1] << 8)); oy = (unsigned)(s->window0[0x1E2] | (s->window0[0x1E3] << 8));
            xs = s->party_x < 0x80 ? s->party_x : (int)s->party_x - 256; ys = s->party_y < 0x80 ? s->party_y : (int)s->party_y - 256;
            /* the zero-extended 32-bit word equals the sign-extended 32-bit byte only for non-negative values */
            if (!(xs >= 0 && ow == (unsigned)xs && ys >= 0 && oy == (unsigned)ys)) RC(h_8ae(w, s, s->ds_2628));
            s->ds_84e0 = 0; s->ds_84e1 = 1;
            RC(h_script(w, s, s->ds_49e5));
            if (s->ds_442f != 0) RC(h_31b4(w, s));
            pc = P34F5; break; }
        case P34F5:
            if (s->ds_49ff == 0) pc = P3395; else { s->ds_49ff = 0; pc = PDONE; }
            break;
        default: pc = PDONE; break;
        }
    }
    return 0;
}

static void init(pr_u1c_state *s, uint8_t *w0, uint8_t *w1)
{
    memset(s, 0, sizeof *s);
    memset(w0, 0x55, W); memset(w1, 0x66, W);
    w1[0x594] = 0; w1[0x595] = 0;        /* word 0x594 = 0 unless a case sets it */
    s->window0 = w0; s->window0_size = W; s->window1 = w1; s->window1_size = W;
    s->hooks = hooks;
}

/* deterministic scene hooks */
typedef struct {
    uint8_t keys[4]; unsigned nkeys, kidx;
    int move_dx; unsigned nscripts; uint16_t script_entries[16]; uint16_t w8ae;
    int exit_on_third_script, set_442f_first_script, exit_in_31b4, exit_on_first_script, exit_on_first_e3;
    int record_w594; unsigned w594_in_view, w594_in_script;
    int fail_key_at, key_calls, fail_8ae, fail_script_e5, e5_calls;
    int kill_w0_in_key, kill_w1_in_view, kill_w0_in_view, kill_w1_in_e3, view_calls;
    size_t shrink_w0_in_e3, shrink_w0_in_e3_to; int exit_in_e5;   /* hooks that invalidate a window */
} Sc;
static int sc_key(void *c, pr_u1c_state *s, uint8_t *key)
{
    Sc *x = c; (void)s;
    x->key_calls++;
    if (x->kill_w0_in_key && x->key_calls == x->kill_w0_in_key) { s->window0 = NULL; s->ds_84dc = 6; }
    if (x->fail_key_at && x->key_calls == x->fail_key_at) return 3;   /* does not supply a key */
    *key = x->keys[x->kidx++ % x->nkeys];
    return 0;
}
static int sc_entry1(void *c, pr_u1c_state *s) { Sc *x = c; s->party_x = (uint8_t)(s->party_x + x->move_dx); return 0; }
static int sc_2dd3(void *c, pr_u1c_state *s) { (void)c; (void)s; return 0; }
static int sc_script(void *c, pr_u1c_state *s, uint16_t e)
{
    Sc *x = c; unsigned i = x->nscripts++;
    if (i < 16) x->script_entries[i] = e;
    if (x->record_w594 && e == s->ds_49e5) x->w594_in_script = (unsigned)(s->window1[0x594] | (s->window1[0x595] << 8));
    if (x->exit_on_third_script && i == 2) s->ds_49ff = 1;
    if (x->exit_on_first_script && i == 0) s->ds_49ff = 1;
    if (x->exit_on_first_e3 && e == s->ds_49e3) s->ds_49ff = 1;
    if (x->set_442f_first_script && i == 0) s->ds_442f = 1;
    if (x->kill_w1_in_e3 && e == s->ds_49e3) s->window1 = NULL;
    if (x->shrink_w0_in_e3 && e == s->ds_49e3) s->window0_size = x->shrink_w0_in_e3_to;
    if (x->exit_in_e5 && e == s->ds_49e5) s->ds_49ff = 1;
    if (e == s->ds_49e5) { x->e5_calls++; if (x->fail_script_e5 && x->e5_calls == x->fail_script_e5) { s->ds_84e0 = 9; return 4; } }
    return 0;
}
static int sc_31b4(void *c, pr_u1c_state *s) { Sc *x = c; if (x->exit_in_31b4) s->ds_49ff = 1; s->ds_442f = 0; return 0; }
static int sc_view(void *c, pr_u1c_state *s)
{
    Sc *x = c; x->view_calls++;
    if (x->kill_w1_in_view && x->view_calls == 1) s->window1 = NULL;
    if (x->kill_w0_in_view && x->view_calls == 1) s->window0 = NULL;
    if (x->record_w594 && s->window1) x->w594_in_view = (unsigned)(s->window1[0x594] | (s->window1[0x595] << 8));
    return 0;
}
static int sc_8ae(void *c, pr_u1c_state *s, uint16_t word) { Sc *x = c; (void)s; x->w8ae = word; return x->fail_8ae ? 5 : 0; }

int main(void)
{
    static uint8_t a0[W], a1[W], b0[W], b1[W];
    pr_u1c_state sa, sb; Wd wa, wb; pr_u1c_report rep;
    unsigned seed;

    /* generated: random state and hook behaviour, a third of the runs with an injected callback failure */
    for (seed = 0; seed < 6000; seed++) {
        int ref_rc;
        pr_u1c_status got;
        init(&sa, a0, a1);
        sa.ds_49ff = (uint8_t)((seed >> 3) & 1u); sa.ds_442f = (uint8_t)((seed >> 4) & 1u);
        sa.ds_84dc = (uint8_t)seed; sa.ds_2628 = (uint16_t)(seed * 7u); sa.ds_49e3 = 0x0103; sa.ds_49e5 = 0x0207;
        sa.ds_5d92 = (uint16_t)(seed * 3u); sa.ds_5d94 = (uint16_t)(seed + 9u);
        sa.party_x = (uint8_t)(seed * 5u); sa.party_y = (uint8_t)(seed * 11u);
        a1[0x594] = (uint8_t)((seed >> 5) % 4u); a1[0x595] = (uint8_t)(((seed >> 7) & 3u) == 0 ? 1 : 0);
        memset(&wa, 0, sizeof wa); wa.seed = seed * 2654435761u + 1u; wa.force_exit_after = 12 + seed % 20;
        wa.fail_at = (seed % 3 == 0) ? 1 + (seed / 3) % 14 : 0;
        wa.nkeys = 6;
        wa.keys[0] = 0x48; wa.keys[1] = 'E'; wa.keys[2] = 'e'; wa.keys[3] = 'h'; wa.keys[4] = 0x45; wa.keys[5] = 'x';
        sb = sa; memcpy(b0, a0, W); memcpy(b1, a1, W); sb.window0 = b0; sb.window1 = b1; wb = wa;
        sa.ctx = &wa; sb.ctx = &wb;
        got = pr_u1c_loop_3387(&sa, &rep);
        ref_rc = ref_run(&sb, &wb);
        CHECK((got == PR_U1C_OK && ref_rc == 0) || (got == PR_U1C_CALLBACK && ref_rc == 5));
        CHECK(wa.nlog == wb.nlog && memcmp(wa.log, wb.log, wa.nlog * sizeof(int)) == 0);
        CHECK(wa.eight_ae_calls == wb.eight_ae_calls && wa.eight_ae_word == wb.eight_ae_word);
        CHECK(sa.ds_49ff == sb.ds_49ff && sa.ds_442f == sb.ds_442f && sa.ds_84dd == sb.ds_84dd && sa.ds_84e0 == sb.ds_84e0 && sa.ds_84e1 == sb.ds_84e1);
        CHECK(sa.ds_4436 == sb.ds_4436 && sa.ds_4438 == sb.ds_4438 && sa.party_x == sb.party_x && sa.party_y == sb.party_y);
        CHECK(memcmp(a0, b0, W) == 0 && memcmp(a1, b1, W) == 0);
        CHECK(rep.position_changed == wa.eight_ae_calls || got == PR_U1C_CALLBACK);
        if (got == PR_U1C_CALLBACK) CHECK(rep.callback_status == 7 && wa.fail_at != 0 && wa.calls == wa.fail_at);
        else CHECK(rep.callback_status == 0 && sa.ds_49ff == 0);
    }

    {
        static uint8_t w0[W], w1[W];
        Sc sc; pr_u1c_state st; pr_u1c_hooks hk;
        memset(&hk, 0, sizeof hk);
        hk.entry2_key = sc_key; hk.entry1 = sc_entry1; hk.script_3171 = sc_script; hk.near_31b4 = sc_31b4;
        hk.view_3ca_25 = sc_view; hk.resident_8ae_00dc = sc_8ae; hk.near_2dd3 = sc_2dd3;

        /* scene 1: the Up route. key 0x48, word 0x594 = 0, [0x49FF] = 0, [0x442F] = 0: straight to +0x344C; script
           [0x49E3]; the movement bookkeeping; entry 1 moves x 15 -> 14 (changed -> 0x8AE with [0x2628]); script
           [0x49E5]; the loop test passes -> second prompt (key 'x'): +0x344C, the script sets [0x49FF] -> exit */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc;
        st.party_x = 15; st.party_y = 4; st.ds_84dc = 3; st.ds_49e3 = 0x1111; st.ds_49e5 = 0x2222; st.ds_2628 = 0xBEEF;
        st.ds_5d92 = 0x0A0A; st.ds_5d94 = 0x0B0B;
        sc.keys[0] = 0x48; sc.keys[1] = 'x'; sc.nkeys = 2; sc.move_dx = -1; sc.exit_on_third_script = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK);
        CHECK(rep.iterations == 2 && rep.movement_passes == 1 && rep.position_changed == 1 && rep.calls_n == 8);
        CHECK(rep.calls[0] == PR_U1C_CALL_ENTRY2 && rep.calls[1] == PR_U1C_CALL_3171_E3 && rep.calls[2] == PR_U1C_CALL_ENTRY1 &&
              rep.calls[3] == PR_U1C_CALL_VIEW && rep.calls[4] == PR_U1C_CALL_8AE && rep.calls[5] == PR_U1C_CALL_3171_E5 &&
              rep.calls[6] == PR_U1C_CALL_ENTRY2 && rep.calls[7] == PR_U1C_CALL_3171_E3);
        CHECK(sc.script_entries[0] == 0x1111 && sc.script_entries[1] == 0x2222 && sc.script_entries[2] == 0x1111 && sc.nscripts == 3);
        CHECK(st.party_x == 14 && st.party_y == 4 && sc.w8ae == 0xBEEF && st.ds_84e0 == 0 && st.ds_84e1 == 1);
        CHECK(w0[0x1E4] == 3 && w0[0x1E5] == 0 && w0[0x1E0] == 15 && w0[0x1E1] == 0 && w0[0x1E2] == 4);
        CHECK(st.ds_4436 == 0x0A0A && st.ds_4438 == 0x0B0B && st.ds_49ff == 0 && rep.last_key == 'x' && rep.callback_status == 0);

        /* scene 2: the entry-1 hook leaves the party in place: no 0x8AE call */
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

        /* scene 4: a coordinate byte >= 0x80 always reads as changed (zero- vs sign-extended 32-bit compare) */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 0x90; st.party_y = 2;
        sc.keys[0] = 0x48; sc.keys[1] = 'x'; sc.nkeys = 2; sc.move_dx = 0; sc.exit_on_third_script = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK && rep.position_changed == 1);

        /* scene 5: key 'e' (upcase 'E') with word 0x594 = 0 enters the search branch: near +0x2DD3, then the re-prompt */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc;
        sc.keys[0] = 'e'; sc.keys[1] = 'x'; sc.nkeys = 2; sc.exit_on_first_script = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK);
        CHECK(rep.calls[0] == PR_U1C_CALL_ENTRY2 && rep.calls[1] == PR_U1C_CALL_2DD3 && rep.calls[2] == PR_U1C_CALL_ENTRY2 &&
              rep.calls[3] == PR_U1C_CALL_3171_E3);

        /* scene 6: word 0x594 = 3 (> 1) with a non-'E' key: [0x84DD] saves bit 0, word 0x594 is 1 during the view and
           script [0x49E5], and restored afterwards; then with word 0x594 = 2 (bit 0 clear) */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.ds_49e5 = 0x7777; w1[0x594] = 3; w1[0x595] = 0;
        sc.keys[0] = 'x'; sc.keys[1] = 'x'; sc.nkeys = 2; sc.record_w594 = 1; sc.exit_on_first_e3 = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK);
        CHECK(rep.calls[1] == PR_U1C_CALL_VIEW && rep.calls[2] == PR_U1C_CALL_3171_E5);
        CHECK(sc.w594_in_view == 1 && sc.w594_in_script == 1 && st.ds_84dd == 1 && w1[0x594] == 1);
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; w1[0x594] = 2; w1[0x595] = 0;
        sc.keys[0] = 'x'; sc.keys[1] = 'x'; sc.nkeys = 2; sc.record_w594 = 1; sc.exit_on_first_e3 = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK && sc.w594_in_view == 1 && st.ds_84dd == 0 && w1[0x594] == 0);

        /* scene 7 (staged guard): window 1 missing or short is detected only at its first read (+0x33BD), after the
           +0x3387 write, the first entry-2 call and the +0x3395 copies, all of which are retained */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.ds_84dc = 9; st.ds_5d92 = 0x1234; st.ds_5d94 = 0x5678;
        sc.keys[0] = 0x48; sc.nkeys = 1;
        st.window1 = NULL;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT);
        CHECK(sc.key_calls == 1 && rep.calls_n == 1 && rep.calls[0] == PR_U1C_CALL_ENTRY2 && rep.last_key == 0x48);
        CHECK(w0[0x1E4] == 9 && w0[0x1E5] == 0 && st.ds_4436 == 0x1234 && st.ds_4438 == 0x5678 && rep.key_loops == 0);
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.window1_size = 0x595; sc.keys[0] = 0x48; sc.nkeys = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && sc.key_calls == 1);
        /* window 0 is still validated at entry: nothing at all happens */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.window0_size = 0x1E5; w0[0x1E4] = 0x77;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && sc.key_calls == 0 && w0[0x1E4] == 0x77 && rep.calls_n == 0);
        st.window0_size = W; st.window0 = NULL;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && sc.key_calls == 0);
        CHECK(pr_u1c_loop_3387(NULL, &rep) == PR_U1C_ARGUMENT);

        /* scene 8 (input callback supplies no key): the first prompt fails after the +0x3387 write: stop, no key
           used, nothing after it done */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.ds_84dc = 5; st.ds_5d92 = 0x1111; st.ds_4436 = 0xAAAA;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.fail_key_at = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_CALLBACK && rep.callback_status == 3 && rep.failed_call == PR_U1C_CALL_ENTRY2);
        CHECK(rep.last_key == 0 && rep.calls_n == 1 && w0[0x1E4] == 5 && st.ds_4436 == 0xAAAA && rep.iterations == 1);
        /* the re-prompt fails after a search-branch pass: earlier effects retained */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.ds_5d92 = 0x2222;
        sc.keys[0] = 'e'; sc.nkeys = 1; sc.fail_key_at = 2;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_CALLBACK && rep.failed_call == PR_U1C_CALL_ENTRY2 && rep.last_key == 'e');
        CHECK(rep.calls_n == 3 && rep.calls[1] == PR_U1C_CALL_2DD3 && st.ds_4436 == 0x2222);

        /* scene 9 (late failures): 0x8AE fails after the movement words and entry 1 ran: the words and the moved party
           are retained, [0x84E0]/[0x84E1] (written after it) are not */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4; st.ds_84e0 = 7; st.ds_84e1 = 8;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.move_dx = -1; sc.fail_8ae = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_CALLBACK && rep.failed_call == PR_U1C_CALL_8AE && rep.callback_status == 5);
        CHECK(st.party_x == 14 && w0[0x1E0] == 15 && st.ds_84e0 == 7 && st.ds_84e1 == 8 && rep.position_changed == 1);
        /* the closing script [0x49E5] fails after [0x84E0]/[0x84E1] were written: they are retained too */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4; st.ds_49e5 = 0x4444; st.ds_84e0 = 7; st.ds_84e1 = 8;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.move_dx = 0; sc.fail_script_e5 = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_CALLBACK && rep.failed_call == PR_U1C_CALL_3171_E5 && rep.callback_status == 4);
        CHECK(st.ds_84e0 == 9 && st.ds_84e1 == 1 && rep.calls_n == 5);          /* ENTRY2, E3, ENTRY1, VIEW, then the failing E5 (logged before it runs) */

        /* scene 11 (a hook invalidates a window): the next access returns ARGUMENT, earlier effects stay, nothing is
           dereferenced */
        /* (a) entry 2 drops window 0 and changes the map id: the +0x33B0 rewrite does not happen */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.ds_84dc = 5; st.ds_5d92 = 0x3333;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.kill_w0_in_key = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && st.window0 == NULL && w0[0x1E4] == 5 && st.ds_4436 == 0x3333);
        CHECK(rep.calls_n == 1 && rep.key_loops == 0);
        /* (b) the search-branch view hook drops window 1: the restore at +0x3423 does not happen */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.ds_49e5 = 0x5555; w1[0x594] = 2; w1[0x595] = 0;
        sc.keys[0] = 'x'; sc.nkeys = 1; sc.kill_w1_in_view = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && st.window1 == NULL);
        CHECK(w1[0x594] == 1 && st.ds_84dd == 0 && rep.calls_n == 3 && rep.calls[1] == PR_U1C_CALL_VIEW && rep.calls[2] == PR_U1C_CALL_3171_E5);   /* forced to 1, never restored to 0 */
        /* (c) the movement-pass view hook drops window 0: the compare reads at +0x3497 do not happen, no 0x8AE call */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.move_dx = -1; sc.kill_w0_in_view = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && st.window0 == NULL && w0[0x1E0] == 15 && st.party_x == 14);
        CHECK(rep.position_changed == 0 && rep.calls_n == 4 && rep.calls[3] == PR_U1C_CALL_VIEW);
        /* (d) the first script drops window 1 after the movement pass: the next read at +0x33BD is refused */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4; st.ds_49e3 = 0x6666; st.ds_49e5 = 0x6667;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.move_dx = -1; sc.kill_w1_in_e3 = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && st.window1 == NULL && rep.movement_passes == 1 && st.party_x == 14);
        CHECK(rep.iterations == 2 && sc.key_calls == 2 && rep.key_loops == 1);       /* the second prompt ran; its +0x33B9 read was refused */

        /* scene 12 (exact access extents): a hook shrinks window 0 to 0x1E4 bytes: the movement-only accesses (word
           0x1E0 needs 0x1E2, word 0x1E2 needs 0x1E4) are still valid, so the run completes (the closing script ends
           the loop before the next +0x1E4 rewrite); at 0x1E3 the word 0x1E2 access is refused */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4; st.ds_49e3 = 0x6001; st.ds_49e5 = 0x6002; st.ds_2628 = 7;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.move_dx = -1; sc.shrink_w0_in_e3 = 1; sc.shrink_w0_in_e3_to = 0x1E4; sc.exit_in_e5 = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_OK && st.window0_size == 0x1E4 && rep.movement_passes == 1);
        CHECK(rep.position_changed == 1 && sc.w8ae == 7 && st.party_x == 14 && w0[0x1E0] == 15 && w0[0x1E2] == 4 && rep.iterations == 1);
        /* 0x1E2 (< 0x1E4): the word +0x1E2 write at +0x3484 is refused after the +0x1E0 write went through */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4; st.ds_49e3 = 0x6001; st.ds_49e5 = 0x6002;
        w0[0x1E0] = 0; w0[0x1E2] = 0x33;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.shrink_w0_in_e3 = 1; sc.shrink_w0_in_e3_to = 0x1E3; sc.exit_in_e5 = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && w0[0x1E0] == 15 && w0[0x1E2] == 0x33 && rep.calls_n == 2);
        /* 0x1E1: even the word +0x1E0 write at +0x3477 is refused */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4; st.ds_49e3 = 0x6001; w0[0x1E0] = 0x44;
        sc.keys[0] = 0x48; sc.nkeys = 1; sc.shrink_w0_in_e3 = 1; sc.shrink_w0_in_e3_to = 0x1E1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && w0[0x1E0] == 0x44 && rep.movement_passes == 1);
        /* a window shrunk to 0x1E4 before the next iteration is refused at the +0x33B0 rewrite (word +0x1E4 needs 0x1E6) */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; st.party_x = 15; st.party_y = 4; st.ds_49e3 = 0x6001; st.ds_84dc = 4; w0[0x1E4] = 0x22;
        sc.keys[0] = 0x48; sc.keys[1] = 'x'; sc.nkeys = 2; sc.move_dx = -1; sc.shrink_w0_in_e3 = 1; sc.shrink_w0_in_e3_to = 0x1E4;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_ARGUMENT && rep.iterations == 2 && sc.key_calls == 2 && w0[0x1E4] == 4);

        /* scene 10: a reached NULL hook; the iteration cap */
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.hooks.entry1 = NULL; st.ctx = &sc; sc.keys[0] = 0x48; sc.nkeys = 1;
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_HOOK && rep.calls[1] == PR_U1C_CALL_3171_E3 && rep.movement_passes == 1);
        init(&st, w0, w1); memset(&sc, 0, sizeof sc);
        st.hooks = hk; st.ctx = &sc; sc.keys[0] = 0x48; sc.nkeys = 1; sc.move_dx = 0;                   /* never sets [0x49FF] */
        CHECK(pr_u1c_loop_3387(&st, &rep) == PR_U1C_LIMIT && rep.iterations == PR_U1C_STEP_CAP);
    }

    /* the upcase edges (runtime 0xAF8:0x182A) */
    CHECK(pr_u1c_upcase('a') == 'A' && pr_u1c_upcase('z') == 'Z' && pr_u1c_upcase('`') == '`' && pr_u1c_upcase('{') == '{' &&
          pr_u1c_upcase('E') == 'E' && pr_u1c_upcase(0x48) == 0x48 && pr_u1c_upcase(0xE5) == 0xE5);

    puts("overlay_u1c_loop_3387: ok");
    return 0;
}

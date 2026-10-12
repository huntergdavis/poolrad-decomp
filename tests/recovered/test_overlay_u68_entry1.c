/* Synthetic parity test for the unit 0x68 entry 1 (+0x0D6D) model. The
 * field buffer, window 1, string literals, DS bytes and every hook are
 * fabricated. A straight restatement of the flow (std C strings) is compared
 * with the model over generated cases; the selector=1 / entry-3 result=1
 * branch is also run through the merged coordinate writer. */
#include "poolrad/recovered/overlay_u68_entry1.h"
#include "poolrad/recovered/overlay_u68_coordinate_writer.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define WIN 0x800u

typedef struct {
    unsigned n;
    int log[64];
    uint8_t ret_0236_6, ret_0236_5, ret_05ee, ret_key, ret_0294, ret_04df, ret_0648;
    unsigned a0000; uint8_t x0000, y0000, f0000;
    uint8_t seen_msg[0x29], seen_prompt[16];
    uint16_t word_8ae;
    uint16_t ds_calls[4]; unsigned nds;
    unsigned step_calls;
} H;

enum { L0000 = 1, L0236, L0294, L04DF, L05EE, L0648, L073B, L0818, LKEY, L3D0, L464, L8AE, L290B };

static void h_0000(void *c, pr_u68_e1_state *s, uint8_t x, uint8_t y, uint8_t f) { H *h = c; (void)s; h->log[h->n++] = L0000; h->a0000++; h->x0000 = x; h->y0000 = y; h->f0000 = f; }
static uint8_t h_0236(void *c, pr_u68_e1_state *s, uint8_t a) { H *h = c; (void)s; h->log[h->n++] = L0236 * 100 + a; return a == 6 ? h->ret_0236_6 : h->ret_0236_5; }
static uint8_t h_0294(void *c, pr_u68_e1_state *s) { H *h = c; (void)s; h->log[h->n++] = L0294; return h->ret_0294; }
static uint8_t h_04df(void *c, pr_u68_e1_state *s) { H *h = c; (void)s; h->log[h->n++] = L04DF; return h->ret_04df; }
static int h_05ee(void *c, pr_u68_e1_state *s, uint8_t a) { H *h = c; (void)s; h->log[h->n++] = L05EE * 100 + a; return h->ret_05ee; }
static uint8_t h_0648(void *c, pr_u68_e1_state *s) { H *h = c; (void)s; h->log[h->n++] = L0648; return h->ret_0648; }
static void h_073b(void *c, pr_u68_e1_state *s) { H *h = c; (void)s; h->log[h->n++] = L073B; }
static void h_0818(void *c, pr_u68_e1_state *s) { H *h = c; (void)s; h->log[h->n++] = L0818; }
static uint8_t h_key(void *c, pr_u68_e1_state *s, const uint8_t *p, const uint8_t *m, uint8_t a, uint8_t b, uint8_t cc, uint8_t d, uint8_t e)
{
    H *h = c; (void)s;
    h->log[h->n++] = LKEY;
    memcpy(h->seen_msg, m, 0x29);
    h->seen_prompt[0] = p ? p[0] : 0; if (p) memcpy(h->seen_prompt + 1, p + 1, p[0]);
    if (a != 0x0D || b != 0x0A || cc != 0x0F || d != 0 || e != 0) return 0xEE;
    return h->ret_key;
}
static void h_3d0(void *c, pr_u68_e1_state *s, uint16_t o) { H *h = c; (void)s; h->log[h->n++] = L3D0; h->ds_calls[h->nds++] = o; }
static void h_464(void *c, pr_u68_e1_state *s, uint16_t o) { H *h = c; (void)s; h->log[h->n++] = L464; h->ds_calls[h->nds++] = o; }
static void h_8ae(void *c, pr_u68_e1_state *s, uint16_t w) { H *h = c; (void)s; h->log[h->n++] = L8AE; h->word_8ae = w; }
static void h_290b(void *c, pr_u68_e1_state *s) { H *h = c; (void)s; h->log[h->n++] = L290B; }

static const pr_u68_e1_hooks hooks = { h_0000, h_0236, h_0294, h_04df, h_05ee, h_0648, h_073b, h_0818, h_key, h_3d0, h_464, h_8ae, h_290b };


/* adapter: +0x073B = the merged coordinate writer, run on the entry-1 state */
typedef struct { uint8_t tab[265]; uint8_t win[0x800]; const uint8_t *buf; unsigned runs; uint8_t x_before, y_before; } W73;
typedef struct { H h; W73 w; } Combo;      /* H first: the other hooks cast ctx to H * */
static int writer_resident(void *c, uint16_t word) { (void)c; (void)word; return 0; }
static int writer_u7d(void *c, uint16_t first, uint16_t second)
{ (void)c; (void)first; (void)second; return 0; }
static void run_writer(void *c, pr_u68_e1_state *s)
{
    W73 *w = &((Combo *)c)->w; pr_u68_step_state ws; pr_u68_step_report wr;
    pr_u68_step_hooks wh = { writer_resident, writer_u7d, NULL };
    memset(&ws, 0, sizeof ws);
    ws.party_x = s->party_x; ws.party_y = s->party_y; ws.facing = s->facing;
    ws.window1 = w->win; ws.window1_size = sizeof w->win;
    ws.direction_table = w->tab; ws.direction_table_size = sizeof w->tab;
    ws.map_id = s->field.map_id; ws.buffer = w->buf;
    ws.hooks = &wh;
    w->x_before = s->party_x; w->y_before = s->party_y;
    if (pr_u68_step_coordinate_writer(&ws, &wr) == PR_U68_STEP_OK) {
        s->party_x = ws.party_x; s->party_y = ws.party_y;
        memcpy(s->ds_6dde, ws.ds_6dde, 3);
    }
    w->runs++;
}

/* synthetic Pascal literals */
static const uint8_t L0[] = { 3, 'a', 'b', 'c' };
static const uint8_t L1[] = { 2, 'd', 'e' };
static const uint8_t L2[] = { 4, 'f', 'g', 'h', 'i' };
static const uint8_t L3[] = { 2, 'z', 'z' };
static const uint8_t L4[] = { 5, 'p', 'r', 'o', 'm', 'p' };

/* buffer with entry 3 result `res` at cell (x,y) for selector `sel` (0,2,4,6): nibble nonzero, bits = res */
static void make_buffer(uint8_t *buf, unsigned x, unsigned y, unsigned sel, unsigned res)
{
    unsigned cell = 16u * y + x;
    memset(buf, 0, 0x400);
    if (res == 1) return;                              /* nibble 0 -> default 1 */
    buf[(sel & 4u ? 0x100u : 0u) + cell] = 0x11;       /* nibble nonzero for both halves */
    buf[0x300u + cell] = (uint8_t)(res << sel);
}

static void init(pr_u68_e1_state *st, uint8_t *win, uint8_t *buf, H *h)
{
    memset(st, 0, sizeof *st);
    memset(win, 0, WIN);
    st->selector = 1; st->window1 = win; st->window1_size = WIN;
    st->party_x = 15; st->party_y = 4; st->facing = 6;
    st->ds_6dde[0] = st->ds_6dde[1] = st->ds_6dde[2] = 1;
    st->word_2622 = 0x1234;
    st->lit[0] = L0; st->lit[1] = L1; st->lit[2] = L2; st->lit[3] = L3; st->lit[4] = L4;
    st->field.map_id = 3; st->field.buffer = buf;
    st->hooks = &hooks; st->ctx = h;
    memset(h, 0, sizeof *h);
}

/* straight restatement of the message block on C strings */
static void ref_message(const pr_u68_e1_state *st, int blockB, uint8_t r236_6, uint8_t r236_5, int r05ee, char *out)
{
    char s[300]; size_t n;
#define LITSTR(i, buf) do { memcpy(buf, st->lit[i] + 1, st->lit[i][0]); buf[st->lit[i][0]] = 0; } while (0)
    char t[64];
    s[0] = 0;
    if (st->ds_6dde[0]) { LITSTR(0, t); strcpy(s, t); s[40] = 0; }
    if (st->ds_6dde[1] && r236_6) { LITSTR(1, t); strcat(s, t); s[40] = 0; }
    if (st->ds_6dde[2] && (blockB ? r236_5 != 0 : r05ee)) { LITSTR(2, t); strcat(s, t); s[40] = 0; }
    LITSTR(3, t); strcat(s, t); n = strlen(s); if (n > 40) s[40] = 0;
    strcpy(out, s);
}

int main(void)
{
    uint8_t win[WIN], buf[0x400];
    H h;
    pr_u68_e1_state st;
    pr_u68_e1_report rep;
    pr_u68_e1_state st0;
    unsigned seed;

    /* ---- selector 1, entry 3 result 1: the move commit and tail, in order ---- */
    init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, 1);
    CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK);
    CHECK(rep.selector_branch == 1 && rep.entry3_result == 1 && rep.move_flag == 1 && rep.committed_move && rep.block == 0);
    CHECK(h.n == 5 && h.log[0] == L073B && h.log[1] == L290B && h.log[2] == L3D0 && h.log[3] == L464 && h.log[4] == L464);
    CHECK(h.nds == 3 && h.ds_calls[0] == 0x6ABE && h.ds_calls[1] == 0x6B01 && h.ds_calls[2] == 0x6B06);
    CHECK(st.ds_6b00 == 0xFF && st.ds_6b05 == 0xFF && h.word_8ae == 0);
    CHECK(win[0x592] == 0 && win[0x593] == 0);
    CHECK(rep.calls_n == 6 && rep.calls[0] == PR_U68_E1_CALL_ENTRY3 && rep.calls[1] == PR_U68_E1_CALL_073B);

    /* ---- entry 3 results 0 and others: no move, resident 0x8AE with [0x2622], then 290B ---- */
    {
        unsigned res;
        for (res = 0; res <= 3; res += 3) {
            if (res == 3) continue;
            init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, res);
            CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK && rep.entry3_result == res && rep.move_flag == 0);
            CHECK(h.log[0] == L8AE && h.word_8ae == 0x1234 && h.log[1] == L290B && h.log[2] == L3D0 && !rep.committed_move && h.n == 5);
        }
    }

    /* ---- generated message blocks (results 2 and 3) ---- */
    for (seed = 0; seed < 4096; seed++) {
        unsigned blockB = seed & 1, d0 = (seed >> 1) & 1, d1 = (seed >> 2) & 1, d2 = (seed >> 3) & 1;
        unsigned r6 = (seed >> 4) & 1, r5 = (seed >> 5) & 1, r5ee = (seed >> 6) & 1;
        static const uint8_t keys[5] = { 0x42, 0x50, 0x4B, 0x41, 0x00 };
        uint8_t key = keys[(seed >> 7) % 5];
        unsigned ret = (seed >> 10) & 3;               /* what the key handler returns: 0, 1, 2, 3 */
        char want[64], got[64];
        unsigned expect_calls, idx;
        int is_equal;
        init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, blockB ? 3 : 2);
        st.ds_6dde[0] = (uint8_t)d0; st.ds_6dde[1] = (uint8_t)d1; st.ds_6dde[2] = (uint8_t)d2;
        h.ret_0236_6 = (uint8_t)r6; h.ret_0236_5 = (uint8_t)r5; h.ret_05ee = (int)r5ee; h.ret_key = key;
        h.ret_0294 = (uint8_t)ret; h.ret_04df = (uint8_t)ret; h.ret_0648 = (uint8_t)ret;
        st0 = st;
        CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK);
        CHECK(rep.entry3_result == (blockB ? 3u : 2u) && rep.block == (blockB ? 2u : 1u));
        ref_message(&st0, (int)blockB, (uint8_t)(d1 && r6), (uint8_t)r5, (int)r5ee, want);
        memcpy(got, rep.message + 1, rep.message[0]); got[rep.message[0]] = 0;
        CHECK(strcmp(want, got) == 0);
        is_equal = strcmp(want, "zz") == 0;
        CHECK((int)rep.message_equals_lit3 == is_equal);
        idx = 0;
        if (d1) { CHECK(h.log[idx++] == L0236 * 100 + 6); }
        if (d2) { CHECK(h.log[idx++] == (blockB ? L0236 * 100 + 5 : L05EE * 100 + 0x1F)); }
        if (is_equal) {
            CHECK(h.log[idx++] == L0000 && h.x0000 == 15 && h.y0000 == 4 && h.f0000 == 6 && !rep.prompted && rep.move_flag == 0);
            CHECK(h.log[idx] == L8AE && h.word_8ae == 0x1234);
        } else {
            CHECK(rep.prompted && h.log[idx++] == LKEY && rep.key == key);
            CHECK(memcmp(h.seen_msg, rep.message, 0x29) == 0 && h.seen_prompt[0] == 5 && memcmp(h.seen_prompt + 1, "promp", 5) == 0);
            expect_calls = 0;
            if (key == 0x42) { CHECK(h.log[idx++] == L0294); expect_calls = ret; }
            else if (key == 0x50 && !blockB) { CHECK(h.log[idx++] == L04DF); expect_calls = ret; }
            else if (key == 0x50) { CHECK(rep.cleared_6ddf == 1 && st.ds_6dde[1] == 0); expect_calls = 0; }
            else if (key == 0x4B) { CHECK(h.log[idx++] == L0648); expect_calls = ret; }
            CHECK(rep.move_flag == (uint8_t)expect_calls);
            if (!(key == 0x50 && blockB)) CHECK(rep.cleared_6ddf == 0 && st.ds_6dde[1] == (uint8_t)d1);
            if (expect_calls) CHECK(h.log[idx] == L073B && rep.committed_move);
            else CHECK(h.log[idx] == L8AE);
        }
        CHECK(h.log[h.n - 4] == L290B && h.log[h.n - 3] == L3D0 && h.nds == 3);
    }

    /* ---- the 40-character truncation, with lit0 alone exceeding 40 ---- */
    {
        uint8_t big[256], small4[5] = { 4, 'x', 'y', 'z', 'w' };
        char want[64];
        unsigned i;
        big[0] = 50; for (i = 1; i <= 50; i++) big[i] = (uint8_t)('A' + (i % 26));
        init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, 2);
        st.lit[0] = big; st.lit[3] = small4;
        h.ret_key = 0;
        CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK && rep.message[0] == 40);
        memcpy(want, big + 1, 40);
        CHECK(memcmp(rep.message + 1, want, 40) == 0);                /* lit3 no longer fits: dropped by the cap */
        CHECK(!rep.message_equals_lit3 && rep.prompted);
        /* NULL literals are empty: s = '' + '' = '' equals lit3 = '' -> near +0x0000 */
        init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, 2);
        st.lit[3] = NULL; st.ds_6dde[0] = st.ds_6dde[1] = st.ds_6dde[2] = 0;
        CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK && rep.message_equals_lit3 && h.a0000 == 1 && !rep.prompted);
    }


    /* ---- connected: result 1 runs the merged coordinate writer on the same party bytes ---- */
    {
        static Combo cb; pr_u68_e1_hooks hk = hooks;
        W73 *w73 = &cb.w;
        memset(&cb, 0, sizeof cb);
        w73->tab[6] = 0xFF; w73->tab[6 + 9] = 0;                    /* synthetic step for facing 6: x - 1 */
        init(&st, win, buf, &cb.h); make_buffer(buf, 15, 4, 6, 1); w73->buf = buf;
        hk.near_073b = run_writer; st.hooks = &hk; st.ctx = &cb;
        st.ds_6dde[0] = st.ds_6dde[1] = st.ds_6dde[2] = 0;
        CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK && rep.committed_move && w73->runs == 1);
        CHECK(st.party_x == 14 && st.party_y == 4 && st.facing == 6);   /* 15 -> 14, the step west */
        CHECK(st.ds_6dde[0] == 1 && st.ds_6dde[1] == 1 && st.ds_6dde[2] == 1);   /* the writer's fill, read by the next call */
        /* the next call (result 2 cell) now sees the three flags the writer set: message block A runs */
        make_buffer(buf, 14, 4, 6, 2);
        memset(&cb.h, 0, sizeof cb.h); w73->runs = 0; cb.h.ret_key = 0;
        CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK && rep.entry3_result == 2 && w73->runs == 0);
        CHECK(rep.block == 1 && rep.message[0] > 2);                  /* lit0 was present: flag [0x6DDE] was set by the writer */
    }

    /* ---- gates: window-1 word, selector values ---- */
    {
        unsigned sel;
        for (sel = 0; sel < 256; sel++) {
            init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, 1);
            st.selector = (uint8_t)sel;
            win[0x592] = 0x10; win[0x593] = 0;            /* below 0xFF */
            CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK);
            if (sel == 1) { CHECK(h.log[0] == L073B); }
            else if (sel >= 2 && sel <= 4) { CHECK(rep.selector_branch == 2 && h.log[0] == L0818 && h.log[1] == L290B && h.log[2] == L3D0 && win[0x592] == 0x10); }
            else { CHECK(rep.selector_branch == 0 && h.log[0] == L3D0 && h.n == 3 && win[0x592] == 0x10); }
            CHECK(st.ds_6b00 == 0xFF && st.ds_6b05 == 0xFF);
            /* word 0x592 >= 0xFF: cleared; only the tail runs (selector 1 skips entry 3; 2..4 skips 0x818) */
            init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, 1);
            st.selector = (uint8_t)sel;
            win[0x592] = 0xFF; win[0x593] = 0;
            CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK);
            if (sel == 1 || (sel >= 2 && sel <= 4)) CHECK(win[0x592] == 0 && win[0x593] == 0 && rep.window_cleared && h.log[0] == L3D0 && h.n == 3 && rep.calls_n == 3);
            else CHECK(win[0x592] == 0xFF && !rep.window_cleared && h.n == 3);
            init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, 1);
            st.selector = (uint8_t)sel;
            win[0x592] = 0x00; win[0x593] = 0x01;         /* 0x100 >= 0xFF as a word */
            CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK);
            if (sel == 1 || (sel >= 2 && sel <= 4)) CHECK(win[0x593] == 0 && rep.window_cleared && h.n == 3);
        }
    }

    /* ---- ARGUMENT: nothing written, no hook ---- */
    init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, 1);
    st.window1_size = 0x593;
    CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_ARGUMENT && h.n == 0 && st.ds_6b00 == 0);
    st.window1_size = WIN; st.field.buffer = NULL;
    CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_ARGUMENT && h.n == 0 && st.ds_6b00 == 0);
    st.field.buffer = buf; { static const uint8_t longp[10] = { 9, 'a' }; st.lit[4] = longp; }
    CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_ARGUMENT && h.n == 0);
    CHECK(pr_u68_entry1(NULL, &rep) == PR_U68_E1_ARGUMENT);

    /* NULL hooks: the flow still completes with every value 0 */
    init(&st, win, buf, &h); make_buffer(buf, 15, 4, 6, 2); st.hooks = NULL;
    CHECK(pr_u68_entry1(&st, &rep) == PR_U68_E1_OK && rep.prompted && rep.key == 0 && st.ds_6b00 == 0xFF);

    puts("overlay_u68_entry1: ok");
    return 0;
}

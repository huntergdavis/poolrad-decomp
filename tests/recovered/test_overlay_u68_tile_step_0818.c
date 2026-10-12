/* Synthetic parity test for the unit 0x68 +0x0818 bounded model. Window 0,
 * the DS image, the direction table, the string and both hooks are
 * fabricated. A wide-integer restatement of the address arithmetic and
 * interval lists for the tile sets are compared with the model over many
 * positions, facings, selectors and every tile value; the clock hook is
 * also wired to the merged unit 0x7d entry 1. */
#include "poolrad/recovered/overlay_u68_coordinate_writer.h"   /* dual-header compile check: both headers in one TU */
#include "poolrad/recovered/overlay_u68_tile_step_0818.h"
#include "poolrad/recovered/overlay_u7d_entry1.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define W0 0x800u

static uint8_t ds_image[0x10000];
static uint8_t win0[W0];
static uint8_t tab[265];
static const uint8_t msg[] = { 5, 'h', 'e', 'l', 'l', 'o' };

typedef struct {
    unsigned msg_calls, clock_calls;
    const uint8_t *msg_ptr; uint8_t color, n, idx;
    uint16_t xw, yw;                  /* window-0 words as the clock hook saw them */
} H;

static void h_msg(void *c, const uint8_t *m, uint8_t color) { H *h = c; h->msg_calls++; h->msg_ptr = m; h->color = color; }
static void h_clock(void *c, uint8_t n, uint8_t idx)
{
    H *h = c; h->clock_calls++; h->n = n; h->idx = idx;
    h->xw = (uint16_t)(win0[0x186] | (win0[0x187] << 8)); h->yw = (uint16_t)(win0[0x188] | (win0[0x189] << 8));
}

static int sb(uint8_t b) { return b < 0x80u ? b : (int)b - 256; }

/* the tile sets as interval lists, independent of the model's chain */
static const unsigned blocked_iv[][2] = { {0x4E,0x4E},{0x7E,0x80},{0x82,0x84},{0xA8,0xAC},{0xAE,0xB3},{0xBC,0xBC},{0xDD,0xDE},{0xE1,0xE9},{0xF5,0xF5} };
static const unsigned special_iv[][2] = { {0x66,0x6C},{0x72,0x72} };
static int in_iv(const unsigned (*iv)[2], unsigned n, unsigned t)
{
    unsigned i;
    for (i = 0; i < n; i++) if (t >= iv[i][0] && t <= iv[i][1]) return 1;
    return 0;
}

static void setup(pr_u68_0818_state *st, H *h)
{
    memset(st, 0, sizeof *st); memset(h, 0, sizeof *h);
    st->window0 = win0; st->window0_size = W0;
    st->direction_table = tab; st->direction_table_size = sizeof tab;
    st->ds = ds_image; st->ds_size = sizeof ds_image;
    st->message = msg; st->message_hook = h_msg; st->clock_hook = h_clock; st->ctx = h;
}

static void put_words(unsigned xw, unsigned yw, unsigned flag)
{
    memset(win0, 0xA5, sizeof win0);
    win0[0x186] = (uint8_t)xw; win0[0x187] = (uint8_t)(xw >> 8);
    win0[0x188] = (uint8_t)yw; win0[0x189] = (uint8_t)(yw >> 8);
    win0[0x344] = (uint8_t)flag; win0[0x345] = (uint8_t)(flag >> 8);
}

/* wide-integer reference: returns the DS address of the tile and fills x, y, col */
static unsigned ref_addr(unsigned xw, unsigned yw, unsigned facing, unsigned selector, unsigned *ox, unsigned *oy, unsigned *ocol)
{
    long xr = (long)((xw + (unsigned)sb(tab[facing])) & 0xFFu);
    long yr = (long)((yw + (unsigned)sb(tab[facing + 9])) & 0xFFu);
    long xx = xr >= 128 ? xr - 256 : xr, yy = yr >= 128 ? yr - 256 : yr, c, a;
    if (xx < 0) xx = 0;
    if (xx > 15) xx = 15;
    if (yy < 0) yy = 0;
    if (yy > 35) yy = 35;
    c = (long)((xx + ds_image[(0x3C76u + selector) & 0xFFFFu]) & 0xFFu);
    a = yy * 44 + (c >= 128 ? c - 256 : c);
    *ox = (unsigned)xx; *oy = (unsigned)yy; *ocol = (unsigned)c;
    return (unsigned)(((a % 65536) + 65536 + 0x3648) % 65536);
}

int main(void)
{
    pr_u68_0818_state st; pr_u68_0818_report rep; H h;
    static const unsigned xws[] = { 0, 3, 14, 15, 16, 0xFF, 0x80, 0x100, 0x1005, 0xFFFF };
    static const unsigned yws[] = { 0, 7, 34, 35, 36, 0xFE, 0x7F, 0x8000, 0x0105 };
    static const unsigned facs[] = { 0, 2, 4, 6, 9, 255 };
    static const unsigned sels[] = { 2, 3, 4, 0, 255 };
    static const unsigned flags[] = { 0, 1, 0x100, 0xFFFF };
    unsigned fi, si, xi, yi, k;

    for (k = 0; k < sizeof tab; k++) tab[k] = (uint8_t)(k * 53u + 7u);    /* random-ish signed steps */
    for (k = 0; k < sizeof ds_image; k++) ds_image[k] = (uint8_t)(k * 31u + 9u);

    for (fi = 0; fi < 6; fi++)
        for (si = 0; si < 5; si++)
            for (xi = 0; xi < 10; xi++)
                for (yi = 0; yi < 9; yi++) {
                    unsigned x, y, col, addr, fl, tileset;
                    addr = ref_addr(xws[xi], yws[yi], facs[fi], sels[si], &x, &y, &col);
                    for (tileset = 0; tileset < 256; tileset++) {
                        uint8_t old = ds_image[addr];
                        int blocked = in_iv(blocked_iv, 9, tileset), special = in_iv(special_iv, 2, tileset);
                        fl = flags[(tileset + xi + yi) % 4];
                        setup(&st, &h);
                        st.facing = (uint8_t)facs[fi]; st.selector = (uint8_t)sels[si];
                        put_words(xws[xi], yws[yi], fl);
                        ds_image[addr] = (uint8_t)tileset;
                        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_OK);
                        CHECK(rep.x == x && rep.y == y && rep.column == col && rep.tile == tileset && rep.tile_address == addr);
                        CHECK((int)rep.blocked == blocked && (int)rep.special == special);
                        if (blocked) {
                            CHECK(h.msg_calls == 0 && h.clock_calls == 0 && !rep.moved);
                            CHECK(win0[0x186] == (uint8_t)xws[xi] && win0[0x188] == (uint8_t)yws[yi]);   /* untouched */
                        } else if (special && fl == 0) {
                            CHECK(rep.flag_word_zero && h.msg_calls == 1 && h.msg_ptr == msg && h.color == 0x0E && h.clock_calls == 0 && !rep.moved);
                            CHECK(win0[0x186] == (uint8_t)xws[xi] && win0[0x188] == (uint8_t)yws[yi]);
                        } else {
                            CHECK(!rep.flag_word_zero && h.msg_calls == 0 && h.clock_calls == 1 && rep.moved);
                            CHECK(h.n == 12 && h.idx == 3 && h.xw == x && h.yw == y);               /* words written before the call, high bytes 0 */
                            CHECK(win0[0x187] == 0 && win0[0x189] == 0);
                        }
                        /* nothing outside the two words changed */
                        CHECK(win0[0x185] == 0xA5 && win0[0x18A] == 0xA5 && win0[0x344] == (uint8_t)fl && win0[0x7FF] == 0xA5);
                        ds_image[addr] = old;
                    }
                }

    /* clamp edges pinned: raw x = -3 -> 0, 20 -> 15; y = -1 -> 0, 40 -> 35 (zero steps via a zero table) */
    memset(tab, 0, sizeof tab);
    {
        static const struct { unsigned xw, yw, x, y; } c[] = { {0xFD,0xFF,0,0}, {20,40,15,35}, {15,35,15,35}, {16,36,15,35}, {0,0,0,0}, {0x80,0x7F,0,35} };
        unsigned i;
        for (i = 0; i < 6; i++) {
            setup(&st, &h); st.selector = 2; put_words(c[i].xw, c[i].yw, 1);
            CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_OK && rep.x == c[i].x && rep.y == c[i].y);
        }
    }

    /* message slot: required (non-NULL, <= 35 characters) only on the reached special-tile + word 0x344 == 0 branch */
    {
        static uint8_t long36[37] = { 36 }, long35[36] = { 35 };
        unsigned a;
        CHECK(PR_U68_STEP_OK == 0 && PR_U68_0818_OK == 0);   /* the coordinate writer's and this header's constants coexist */
        for (k = 0; k < sizeof tab; k++) tab[k] = 0;
        ds_image[(0x3C76u + 2u) & 0xFFFF] = 0;
        a = 0x3648u + 0;                                     /* x = 0, y = 0, column 0 */
        ds_image[a] = 0x66;                                  /* special */
        setup(&st, &h); st.selector = 2; put_words(0, 0, 0);
        st.message = long35;
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_OK && h.msg_calls == 1);
        st.message = long36; memset(&h, 0, sizeof h);
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_MESSAGE && h.msg_calls == 0 && rep.flag_word_zero);
        CHECK(win0[0x186] == 0 && win0[0x344] == 0);
        st.message = NULL;
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_MESSAGE && h.msg_calls == 0);
        /* the message is not examined on any other path: NULL and oversize are ignored there */
        {
            const uint8_t *bad[2] = { NULL, long36 };
            unsigned bi;
            for (bi = 0; bi < 2; bi++) {
                st.message = bad[bi];
                put_words(0, 0, 1);                          /* special tile but word 0x344 != 0: the move */
                memset(&h, 0, sizeof h);
                CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_OK && rep.moved && h.clock_calls == 1 && h.msg_calls == 0);
                ds_image[a] = 0x4E;                          /* blocked tile */
                put_words(0, 0, 0); memset(&h, 0, sizeof h);
                CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_OK && rep.blocked && h.clock_calls == 0 && h.msg_calls == 0);
                ds_image[a] = 0x10;                          /* ordinary tile */
                put_words(0, 0, 0); memset(&h, 0, sizeof h);
                CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_OK && rep.moved && h.clock_calls == 1);
                ds_image[a] = 0x66;
            }
        }
        st.message = msg;
        st.message_hook = NULL;                              /* valid message, reached hook missing: no write */
        put_words(0, 0, 0);
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_HOOK && win0[0x186] == 0 && win0[0x344] == 0);
        ds_image[a] = 0x10;                                  /* an ordinary tile: the clock hook is the reached one */
        st.clock_hook = NULL; put_words(2, 3, 1);
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_HOOK && win0[0x186] == 2 && win0[0x188] == 3);
        st.message_hook = h_msg; st.clock_hook = h_clock; st.message = msg;
        st.window0_size = 0x345;
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_ARGUMENT);
        st.window0_size = W0; st.direction_table_size = 9;
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_ARGUMENT);        /* facing 0 needs 10 bytes */
        st.direction_table_size = sizeof tab; st.ds_size = 0x3C77;       /* selector table read at 0x3C78 */
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_ARGUMENT);
        st.ds_size = 0x3649; st.selector = 0;                            /* tile read in range, nothing else */
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_ARGUMENT);        /* sel table at 0x3C76 is outside */
        CHECK(pr_u68_0818_run(NULL, &rep) == PR_U68_0818_ARGUMENT);
        st.ds_size = sizeof ds_image; st.ds = NULL;
        CHECK(pr_u68_0818_run(&st, &rep) == PR_U68_0818_ARGUMENT);
    }

    /* connected: the clock hook is the merged unit 0x7d entry 1 (12 units of word index 3) */
    {
        static uint8_t w0[W0];
        pr_u7d_state us; pr_u7d_report ur;
        memset(&us, 0, sizeof us);
        us.window0 = w0; us.window0_size = W0;
        us.limit[0] = 60; us.limit[1] = 60; us.limit[2] = 24; us.limit[3] = 30; us.limit[4] = 12; us.limit[5] = 100; us.limit[6] = 3;
        w0[0x192] = 25;                                      /* word 3 = 25 */
        /* 12 increments with a limit of 30: 25 + 12 = 37 -> carry once into word 4 (+1), word 3 = 7 */
        CHECK(pr_u7d_entry1(&us, PR_U68_0818_CLOCK_N, PR_U68_0818_CLOCK_IDX, &ur) == PR_U7D_OK);
        CHECK(w0[0x192] == 7 && w0[0x194] == 1 && ur.iterations == 12 && ur.carries[3] == 1);
    }

    puts("overlay_u68_tile_step_0818: ok");
    return 0;
}

/* Synthetic test for the resident startup video-mode selection and its three callees. The BIOS is a fabricated hook. Checked: the exact INT 10h
 * register sequence and the DS bytes/words each of the three adapter paths leaves, the previous/current mode chain, the 16-bit page-segment
 * arithmetic, the Tandy-path word rewrite, every unknown adapter letter taking the CGA path, hook NULL/failure stops with earlier effects
 * standing, and the argument guards. */
#include "poolrad/recovered/resident_video_init_4426.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct { unsigned n; uint8_t ah[8], al[8], intno[8]; uint8_t host_mode; int fail_at, fail_val; } Bios;
static Bios bios;

static int h_intr(void *c, pr_vi_ds *ds, uint8_t intno, uint8_t regs[PR_VI_REGS_BYTES])
{
    Bios *b = c; (void)ds;
    b->ah[b->n] = regs[1]; b->al[b->n] = regs[0]; b->intno[b->n] = intno; b->n++;
    if (regs[1] == 0x0F) regs[0] = b->host_mode;                      /* the BIOS answers the mode query in AL */
    regs[10] = 0x34; regs[11] = 0x12;                                   /* DI word: proves the hook may change the record */
    return b->fail_at && (int)b->n == b->fail_at ? b->fail_val : 0;
}

static pr_vi_hooks hooks = { h_intr };
static pr_vi_ds ds;

static void setup(uint8_t letter)
{
    memset(&bios, 0, sizeof bios); bios.host_mode = 3;
    memset(&ds, 0, sizeof ds);
    ds.b484c = letter; ds.b4a1c = 0x77; ds.b4a1d = 0x78; ds.b4a1e = 0x79; ds.b4a1a = 0x7A; ds.b4a1b = 0x7B; ds.w4a16 = 0x1111; ds.w4a18 = 0x2222;
    ds.w4408 = 0x5000; ds.w440a = 0x6000;
}

int main(void)
{
    pr_vi_report rep;
    unsigned i;

    /* EGA: mode query, SetVideoMode(0x0D), SetActivePage(0), SetDrawPage(0) */
    setup(0x45);
    CHECK(pr_vi_select_mode(&ds, &bios, &hooks, &rep) == PR_VI_OK);
    CHECK(bios.n == 3 && bios.ah[0] == 0x0F && bios.ah[1] == 0x00 && bios.al[1] == 0x0D && bios.ah[2] == 0x05 && bios.al[2] == 0x00);
    CHECK(bios.intno[0] == 0x10 && bios.intno[1] == 0x10 && bios.intno[2] == 0x10 && rep.intr_calls == 3);
    CHECK(ds.b4a1c == 3 && ds.b4a1d == 0x0D && ds.b4a1e == 1 && ds.b4a1a == 0 && ds.b4a1b == 0 && ds.w4a16 == 0xA000 && ds.w4a18 == 0xA000);
    CHECK(ds.regs[1] == 0x05 && ds.regs[0] == 0x00 && ds.regs[10] == 0x34 && ds.regs[11] == 0x12);                        /* the last call left AX = 0x0500 */
    CHECK(ds.w4408 == 0x5000 && ds.w440a == 0x6000 && ds.w4848 == 0 && ds.w484a == 0);                                    /* the Tandy words are untouched */

    /* Tandy: class 2, w440a -= 0x400 through the scratch words, mode 9 only */
    setup(0x54);
    CHECK(pr_vi_select_mode(&ds, &bios, &hooks, &rep) == PR_VI_OK && bios.n == 2 && bios.ah[1] == 0 && bios.al[1] == 9);
    CHECK(ds.b4a1e == 2 && ds.b4a1c == 3 && ds.b4a1d == 9 && ds.w4408 == 0x5000 && ds.w440a == 0x5C00 && ds.w4848 == 0x5000 && ds.w484a == 0x5C00);
    CHECK(ds.b4a1a == 0x7A && ds.b4a1b == 0x7B && ds.w4a16 == 0x1111 && ds.w4a18 == 0x2222);                              /* no page work on this path */
    setup(0x54); ds.w440a = 0x0200;
    CHECK(pr_vi_select_mode(&ds, &bios, &hooks, &rep) == PR_VI_OK && ds.w440a == 0xFE00);                                 /* 16-bit wrap */

    /* any other letter: class 0, mode 4; the byte seen natively later in the run (0x46) is one of them */
    for (i = 0; i < 256; i++) {
        if (i == 0x45 || i == 0x54) continue;
        setup((uint8_t)i); bios.host_mode = 7;
        CHECK(pr_vi_select_mode(&ds, &bios, &hooks, NULL) == PR_VI_OK && ds.b4a1e == 0 && ds.b4a1d == 4 && ds.b4a1c == 7 && bios.n == 2 && bios.al[1] == 4);
    }

    /* SetVideoMode on its own: prev := cur before the call, cur := mode after it */
    setup(0); ds.b4a1d = 0x21;
    CHECK(pr_vi_set_video_mode(&ds, &bios, &hooks, 0x55, &rep) == PR_VI_OK && ds.b4a1c == 0x21 && ds.b4a1d == 0x55 && bios.ah[0] == 0 && bios.al[0] == 0x55);
    /* page arithmetic: (page << 9) + 0xA000 in 16 bits */
    setup(0);
    CHECK(pr_vi_set_active_page(&ds, &bios, &hooks, 1, &rep) == PR_VI_OK && ds.w4a16 == 0xA200 && bios.ah[0] == 5 && bios.al[0] == 1);
    CHECK(pr_vi_set_active_page(&ds, &bios, &hooks, 0x80, NULL) == PR_VI_OK && ds.w4a16 == 0xA000);                       /* 0x80 << 9 = 0x10000 wraps to 0 */
    CHECK(pr_vi_set_active_page(&ds, &bios, &hooks, 0xFF, NULL) == PR_VI_OK && ds.w4a16 == (uint16_t)(0xFE00 + 0xA000));
    pr_vi_set_draw_page(&ds, 2); CHECK(ds.w4a18 == 0xA400 && bios.n == 3);                                                 /* no INT call */
    pr_vi_set_draw_page(&ds, 0xFF); CHECK(ds.w4a18 == (uint16_t)(0xFE00 + 0xA000));

    /* stops: a failing BIOS call stops with the earlier effects standing */
    setup(0x45); bios.fail_at = 1; bios.fail_val = -1;
    CHECK(pr_vi_select_mode(&ds, &bios, &hooks, &rep) == PR_VI_CALLBACK && rep.callback_status == -1 && ds.b4a1c == 0x77 && ds.b4a1e == 0x79 && ds.b4a1d == 0x78);
    setup(0x45); bios.fail_at = 2; bios.fail_val = 5;                                                                       /* the mode set fails after prev := cur */
    CHECK(pr_vi_select_mode(&ds, &bios, &hooks, &rep) == PR_VI_CALLBACK && ds.b4a1e == 1 && ds.b4a1c == 3 && ds.b4a1d == 3 && ds.w4a16 == 0x1111);
    setup(0x45); bios.fail_at = 3; bios.fail_val = 6;                                                                       /* the page select fails: the segments are unset */
    CHECK(pr_vi_select_mode(&ds, &bios, &hooks, &rep) == PR_VI_CALLBACK && ds.b4a1d == 0x0D && ds.w4a16 == 0x1111 && ds.w4a18 == 0x2222);
    setup(0x45);
    { pr_vi_hooks none = { NULL };
      CHECK(pr_vi_select_mode(&ds, &bios, &none, NULL) == PR_VI_HOOK && ds.b4a1c == 0x77);
      CHECK(pr_vi_select_mode(&ds, &bios, NULL, NULL) == PR_VI_HOOK); }

    /* argument guards */
    CHECK(pr_vi_select_mode(NULL, &bios, &hooks, &rep) == PR_VI_ARGUMENT && pr_vi_set_video_mode(NULL, &bios, &hooks, 1, NULL) == PR_VI_ARGUMENT && pr_vi_set_active_page(NULL, &bios, &hooks, 1, NULL) == PR_VI_ARGUMENT);
    pr_vi_set_draw_page(NULL, 1);
    puts("resident_video_init_4426: ok");
    return 0;
}

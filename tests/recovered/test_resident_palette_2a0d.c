/* Synthetic test for the resident palette setter 0x464:0x2A0D. The BIOS is a fabricated hook. Checked: the record bytes the original writes
 * (AX = 0x1000, BL = register, BH = the shifted value) for every register/value byte pair class, the < 8 / >= 8 split with the 8-bit wrap,
 * that no other record byte or DS word changes, the INT number, the hook-visible record, the failure/NULL-hook stops with the writes standing,
 * and the argument guards. */
#include "poolrad/recovered/resident_palette_2a0d.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct { unsigned n; uint8_t intno, ax_l, ax_h, bl, bh; int fail; } Bios;

static int h_intr(void *c, pr_vi_ds *ds, uint8_t intno, uint8_t regs[PR_VI_REGS_BYTES])
{
    Bios *b = c; (void)ds;
    b->n++; b->intno = intno; b->ax_l = regs[0]; b->ax_h = regs[1]; b->bl = regs[2]; b->bh = regs[3];
    return b->fail;
}

int main(void)
{
    pr_vi_hooks hooks = { h_intr };
    pr_vi_ds ds, before;
    Bios bios;
    pr_vi_report rep;
    unsigned reg, val, i;

    for (reg = 0; reg < 256; reg += 5) {
        for (val = 0; val < 256; val++) {
            uint8_t bh = val < 8 ? (uint8_t)val : (uint8_t)(val + 8);
            memset(&bios, 0, sizeof bios);
            memset(&ds, 0xA5, sizeof ds);
            before = ds;
            CHECK(pr_res_set_palette(&ds, &bios, &hooks, (uint8_t)reg, (uint8_t)val, &rep) == PR_VI_OK);
            CHECK(bios.n == 1 && bios.intno == 0x10 && bios.ax_h == 0x10 && bios.ax_l == 0 && bios.bl == reg && bios.bh == bh);
            CHECK(rep.intr_calls == 1 && rep.intr_ah[0] == 0x10 && rep.intr_al[0] == 0);
            CHECK(ds.regs[0] == 0 && ds.regs[1] == 0x10 && ds.regs[2] == reg && ds.regs[3] == bh);
            for (i = 4; i < PR_VI_REGS_BYTES; i++) CHECK(ds.regs[i] == before.regs[i]);          /* CX..Flags untouched */
            CHECK(ds.w4a16 == before.w4a16 && ds.w4a18 == before.w4a18 && ds.b4a1a == before.b4a1a && ds.b4a1b == before.b4a1b);
            CHECK(ds.b4a1c == before.b4a1c && ds.b4a1d == before.b4a1d && ds.b4a1e == before.b4a1e && ds.w4408 == before.w4408 && ds.w440a == before.w440a);
        }
    }
    /* boundary values of the value split: 7 stays, 8 -> 16, 0xF7 -> 0xFF, 0xF8 -> 0 (8-bit wrap) */
    memset(&bios, 0, sizeof bios); memset(&ds, 0, sizeof ds);
    pr_res_set_palette(&ds, &bios, &hooks, 1, 7, NULL);    CHECK(bios.bh == 7);
    pr_res_set_palette(&ds, &bios, &hooks, 1, 8, NULL);    CHECK(bios.bh == 16);
    pr_res_set_palette(&ds, &bios, &hooks, 1, 0xF7, NULL); CHECK(bios.bh == 0xFF);
    pr_res_set_palette(&ds, &bios, &hooks, 1, 0xF8, NULL); CHECK(bios.bh == 0);
    /* the two unit 0x3C6 sequences and the unit 0x17 flicker: (register, value) -> (BL, BH) */
    pr_res_set_palette(&ds, &bios, &hooks, 0, 8, NULL); CHECK(bios.bl == 0 && bios.bh == 16);
    pr_res_set_palette(&ds, &bios, &hooks, 8, 0, NULL); CHECK(bios.bl == 8 && bios.bh == 0);
    pr_res_set_palette(&ds, &bios, &hooks, 1, 15, NULL); CHECK(bios.bl == 1 && bios.bh == 23);
    pr_res_set_palette(&ds, &bios, &hooks, 1, 1, NULL); CHECK(bios.bl == 1 && bios.bh == 1);

    /* failure: the record writes stand, the status carries the code; a NULL hook stops with the record already built */
    memset(&bios, 0, sizeof bios); bios.fail = -9; memset(&ds, 0, sizeof ds);
    CHECK(pr_res_set_palette(&ds, &bios, &hooks, 3, 9, &rep) == PR_VI_CALLBACK && rep.callback_status == -9 && ds.regs[3] == 17 && ds.regs[2] == 3 && ds.regs[1] == 0x10);
    { pr_vi_hooks none = { NULL }; memset(&ds, 0, sizeof ds);
      CHECK(pr_res_set_palette(&ds, &bios, &none, 3, 9, &rep) == PR_VI_HOOK && ds.regs[3] == 17 && rep.intr_calls == 0);
      CHECK(pr_res_set_palette(&ds, &bios, NULL, 3, 9, NULL) == PR_VI_HOOK); }
    CHECK(pr_res_set_palette(NULL, &bios, &hooks, 1, 1, &rep) == PR_VI_ARGUMENT);
    puts("resident_palette_2a0d: ok");
    return 0;
}

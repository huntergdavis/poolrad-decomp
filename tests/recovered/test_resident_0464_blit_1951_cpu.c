/* Focused synthetic test for the raw-CPU resident 0464:1951 blit (docs/re/RES_0464_BLIT_1951_CPU.md).
 * No game bytes: tables, bitmap and the two far-call segment words are synthetic; expectations are hand-derived. */
#include "poolrad/recovered/resident_0464_blit_1951_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, outs, writes;
static int refuse = -1;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "blit 1951 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }
static uint16_t last_port; static uint16_t last_value;
static int out(void *ctx, uint16_t port, uint16_t value, unsigned size, uint16_t cs, uint16_t ip)
{
    (void)ctx; (void)size; (void)cs; (void)ip;
    if (refuse >= 0 && outs == (unsigned)refuse) return 1;
    ++outs; last_port = port; last_value = value; return 0;
}
static void written(void *ctx, uint16_t offset, uint8_t value) { (void)ctx; (void)offset; (void)value; ++writes; }

enum { DS = 0x5000, SS = 0x2000, BSEG = 0x7000, BOFF = 0x0100 };
static uint8_t data(unsigned k) { return (uint8_t)(k * 29u + 7u); }

static pr_r0464_blit_cpu setup(pr_bus *b, unsigned mode, unsigned rows, unsigned cols, unsigned x, unsigned y)
{
    pr_r0464_blit_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x8000, 0x8800, 0x1951, DS, 0x1111, SS, 0x0002};
    unsigned i;
    memset(b->ram, 0x5A, PR_BUS_RAM_SIZE); pr_ega_reset(b->ega); memset(b->ega->plane, 0, sizeof b->ega->plane);
    put(b->ram, SS, 0x8000, 0x7000); put(b->ram, SS, 0x8002, 0x9001); put(b->ram, SS, 0x8004, BOFF);
    put(b->ram, SS, 0x8006, BSEG); put(b->ram, SS, 0x8008, x); put(b->ram, SS, 0x800A, y);
    put(b->ram, 0x8800, 0x19DF, 0x6000); put(b->ram, 0x8800, 0x1B3C, 0x6000);  /* far-call segment words */
    b->ram[p(DS, 0x4A1E)] = (uint8_t)mode;
    for (i = 0; i < 4u; ++i) b->ram[p(DS, 0x25F6 + i)] = (uint8_t)(1u << i);
    for (i = 0; i < 16u; ++i) {
        put(b->ram, DS, 0x4EC0 + 2u * i, 0x3000 + i * 0x10); put(b->ram, DS, 0x51E0 + 2u * i, 0x3400 + i * 0x10);
        put(b->ram, DS, 0x5050 + 2u * i, i * 40u);
    }
    put(b->ram, BSEG, BOFF, rows); put(b->ram, BSEG, BOFF + 2u, cols);
    for (i = 0; i < 64u; ++i) b->ram[p(BSEG, BOFF + 0x17u + i)] = data(i);
    outs = writes = 0; refuse = -1; return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_BUS_RAM_SIZE); pr_ega *ega = malloc(sizeof *ega);
    pr_port_env env = {NULL, out, NULL}; pr_bus bus = {ram, ega, &env, written, NULL};
    pr_r0464_blit_cpu c, before; pr_r0464_blit_cpu_diagnostic d; unsigned pl;
    CHECK(ram != NULL && ega != NULL);

    /* Null bitmap pointer: straight to the epilogue; RETF 8 pops the frame and four argument words. */
    c = setup(&bus, 1, 1, 1, 0, 5); put(ram, SS, 0x8004, 0); put(ram, SS, 0x8006, 0);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_OK);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x800C && c.bp == 0x7777 && outs == 0 && writes == 0 && d.move_calls == 0);

    /* Mode 3 is no mode: returns without OUTs or stores. */
    c = setup(&bus, 3, 1, 1, 0, 5);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_OK && outs == 0 && writes == 0);

    /* Mode 1, one row (x = 0, table 5050[0] = 0) and one column y = 5: index OUT, four plane masks, final 0Fh. */
    c = setup(&bus, 1, 1, 1, 0, 5);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_OK);
    CHECK(outs == 6 && writes == 4 && last_port == 0x3C5 && last_value == 0x0F && ega->seq[2] == 0x0F);
    for (pl = 0; pl < 4u; ++pl) CHECK(ega->plane[pl][5] == data(pl) && ega->plane[pl][4] == 0 && ega->plane[pl][6] == 0);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x800C);

    /* Mode 1 with zero rows: the index OUT, the skipped loop, the final 0Fh. */
    c = setup(&bus, 1, 0, 1, 0, 5);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_OK && outs == 2 && writes == 0);

    /* Mode 0, two rows, one column word: Move copies 2 bytes per row to table-segment:2y. */
    c = setup(&bus, 0, 2, 1, 0, 3);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_OK && d.move_calls == 2 && d.move_status == 0);
    CHECK(ram[p(0x3000, 6)] == data(0) && ram[p(0x3000, 7)] == data(1) && ram[p(0x3010, 6)] == data(2) && ram[p(0x3010, 7)] == data(3));
    CHECK(ram[p(0x3000, 8)] == 0x5A && outs == 0 && writes == 0 && c.cs == 0x9001);

    /* Mode 2, one row: 4 bytes per column word to table 51E0 segment:4y. */
    c = setup(&bus, 2, 1, 1, 0, 2);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_OK && d.move_calls == 1);
    for (pl = 0; pl < 4u; ++pl) CHECK(ram[p(0x3400, 8u + pl)] == data(pl));
    CHECK(ram[p(0x3400, 12)] == 0x5A);

    /* CONTRACT: a mode-0 row segment in A000 with an EGA attached stops at Move entry, nothing copied. */
    c = setup(&bus, 0, 1, 1, 0, 3); put(ram, DS, 0x4EC0, 0xA000);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_CONTRACT);
    CHECK(c.cs == 0x6000 && c.ip == 0x1685 && get(ram, SS, c.sp) == 0x19E1 && get(ram, SS, c.sp + 2u) == 0x8800 && d.move_calls == 0);

    /* LIMIT: budget 0 stops before the first backward JNE of the mode-0 row loop, after one Move. */
    c = setup(&bus, 0, 2, 1, 0, 3);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 0, &d) == PR_R0464_BLIT_CPU_LIMIT && c.ip == 0x19ED && d.move_calls == 1);

    /* PORT: refusing the first OUT stops at the index OUT (+1A2E) with nothing sent. */
    c = setup(&bus, 1, 1, 1, 0, 5); refuse = 0;
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_PORT && c.ip == 0x1A2E && writes == 0);

    /* ARGUMENT: rejected before any change. */
    c = setup(&bus, 1, 1, 1, 0, 5); before = c;
    CHECK(pr_r0464_blit_1951_cpu(NULL, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_ARGUMENT);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE - 1u, &c, 100, &d) == PR_R0464_BLIT_CPU_ARGUMENT);
    CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, NULL, 100, &d) == PR_R0464_BLIT_CPU_ARGUMENT);
    c.ip = 0x1952; CHECK(pr_r0464_blit_1951_cpu(&bus, PR_BUS_RAM_SIZE, &c, 100, &d) == PR_R0464_BLIT_CPU_ARGUMENT);
    c.ip = 0x1951; CHECK(memcmp(&c, &before, sizeof c) == 0 && outs == 0 && writes == 0);

    free(ega); free(ram);
    printf("blit 1951: %u semantic checks passed\n", checks);
    return 0;
}

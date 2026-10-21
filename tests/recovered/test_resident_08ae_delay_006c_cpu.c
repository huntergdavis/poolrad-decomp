#include "poolrad/recovered/resident_08ae_delay_006c_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Semantic checks of the delay loop's seams. The original-byte differential is
 * tools/overlay/res08ae_delay_006c_cpu_audit.py; the time source here is a
 * scripted stub, never the original RES_08AE_0000. */
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#define SIZE PR_R08AE_006C_RAM_SIZE
static uint8_t ram[SIZE];
typedef struct { const uint32_t *times; unsigned count, used; } script_t;
static size_t addr(unsigned s, unsigned o) { return (((size_t)s << 4) + (o & 65535u)) & (SIZE - 1u); }
static void put(unsigned s, unsigned o, unsigned v) { size_t a = addr(s, o); ram[a] = (uint8_t)v; ram[(a + 1u) & (SIZE - 1u)] = (uint8_t)(v >> 8); }
static unsigned get(unsigned s, unsigned o) { return ram[addr(s, o)] | ((unsigned)ram[(addr(s, o) + 1u) & (SIZE - 1u)] << 8); }
static int time_fn(void *ctx, uint8_t *m, pr_r08ae_006c_cpu *c)
{
    script_t *s = ctx; (void)m;
    if (s->used == s->count) return 1;
    c->ax = (uint16_t)s->times[s->used]; c->dx = (uint16_t)(s->times[s->used] >> 16); ++s->used;
    return 0;
}
static pr_r08ae_006c_cpu fixture(unsigned arg)
{
    pr_r08ae_006c_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8000, 0x2000, 0x006C, 0x3000, 0x4000, 0x5000, 0x0202};
    memset(ram, 0xA5, sizeof ram);
    ram[addr(c.cs, 0x0072)] = 0x0E; ram[addr(c.cs, 0x0073)] = 0xE8; put(c.cs, 0x0074, 0xFF8A);
    ram[addr(c.cs, 0x0089)] = 0x0E; ram[addr(c.cs, 0x008A)] = 0xE8; put(c.cs, 0x008B, 0xFF73);
    put(c.ss, c.sp, 0x7000); put(c.ss, c.sp + 2, 0x9001); put(c.ss, c.sp + 4, arg);
    return c;
}
int main(void)
{
    pr_r08ae_006c_cpu c, before; script_t s; unsigned bad;
    static const uint32_t reach[] = {1000, 1001, 1005, 1010};
    static const uint32_t exact[] = {100, 100, 110};
    static const uint32_t wrapped[] = {5000, 100};
    static const uint32_t never[] = {10, 10, 10, 10};
    static const uint32_t carry[] = {0xFFFFFFF0u, 0xFFFFFFF0u, 5u};
    uint8_t *copy = malloc(SIZE);
    CHECK(copy != NULL);
    /* Reaches first reading + argument after intermediate readings. */
    c = fixture(10); s = (script_t){reach, 4, 0};
    CHECK(pr_r08ae_delay_006c_cpu(ram, SIZE, &c, time_fn, &s, 1000) == PR_R08AE_006C_OK);
    CHECK(s.used == 4 && c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8006 && c.bp == 0x7777);
    CHECK(c.ax == 1010 && c.dx == 0 && c.cx == 10 && c.bx == 0);
    /* A later reading exactly on the target, argument 10. */
    c = fixture(10); s = (script_t){exact, 3, 0};
    CHECK(pr_r08ae_delay_006c_cpu(ram, SIZE, &c, time_fn, &s, 1000) == PR_R08AE_006C_OK && s.used == 3);
    /* A reading more than the argument below the target is a wrapped clock and exits. */
    c = fixture(10); s = (script_t){wrapped, 2, 0};
    CHECK(pr_r08ae_delay_006c_cpu(ram, SIZE, &c, time_fn, &s, 1000) == PR_R08AE_006C_OK && s.used == 2);
    /* Target past 2^32 wraps: 0xFFFFFFF0 + 0x20 = 0x10; the next reading 5 is below it by 11 <= 32, loop continues. */
    c = fixture(0x20); s = (script_t){carry, 3, 0};
    CHECK(pr_r08ae_delay_006c_cpu(ram, SIZE, &c, time_fn, &s, 1000) == PR_R08AE_006C_TIME && s.used == 3);
    /* Script end is the named TIME stop with the frame pushed and IP=0. */
    c = fixture(10); s = (script_t){never, 4, 0};
    CHECK(pr_r08ae_delay_006c_cpu(ram, SIZE, &c, time_fn, &s, 1000) == PR_R08AE_006C_TIME);
    CHECK(c.ip == 0 && c.cs == 0x2000 && c.sp == 0x8000 - 2 - 8 - 4 && s.used == 4);
    CHECK(get(0x5000, c.sp) == 0x008D && get(0x5000, c.sp + 2) == 0x2000);
    /* LIMIT stops before the next step with no frame pushed. */
    c = fixture(10); s = (script_t){never, 4, 0};
    CHECK(pr_r08ae_delay_006c_cpu(ram, SIZE, &c, time_fn, &s, 3) == PR_R08AE_006C_LIMIT && c.ip == 0x0072);
    /* CODE: each call site's PUSH CS, E8 opcode and displacement are checked before any push. */
    for (bad = 0; bad < 3; ++bad) {
        c = fixture(10); s = (script_t){never, 4, 0};
        if (bad == 0) ram[addr(c.cs, 0x0072)] = 0x90;
        if (bad == 1) ram[addr(c.cs, 0x0073)] = 0x90;
        if (bad == 2) put(c.cs, 0x0074, 0xFF8B);
        CHECK(pr_r08ae_delay_006c_cpu(ram, SIZE, &c, time_fn, &s, 1000) == PR_R08AE_006C_CODE);
        CHECK(c.ip == 0x0072 && c.sp == 0x8000 - 2 - 8 && s.used == 0);
    }
    /* ARGUMENT rejects before changing CPU or RAM. */
    for (bad = 0; bad < 4; ++bad) {
        c = fixture(10);
        if (bad == 0) c.ip = 0x006D;
        if (bad == 1) c.sp = 0x000D;
        if (bad == 2) c.sp = 0xFFFB;
        before = c; memcpy(copy, ram, SIZE);
        CHECK(pr_r08ae_delay_006c_cpu(ram, bad == 3 ? SIZE - 1u : SIZE, bad == 3 ? &c : &c, time_fn, &s, 10) == PR_R08AE_006C_ARGUMENT);
        CHECK(memcmp(&c, &before, sizeof c) == 0 && memcmp(copy, ram, SIZE) == 0);
    }
    c = fixture(10);
    CHECK(pr_r08ae_delay_006c_cpu(NULL, SIZE, &c, time_fn, &s, 10) == PR_R08AE_006C_ARGUMENT);
    CHECK(pr_r08ae_delay_006c_cpu(ram, SIZE, NULL, time_fn, &s, 10) == PR_R08AE_006C_ARGUMENT);
    /* A missing time source is the TIME stop, not a crash. */
    c = fixture(10);
    CHECK(pr_r08ae_delay_006c_cpu(ram, SIZE, &c, NULL, NULL, 10) == PR_R08AE_006C_TIME && c.ip == 0);
    free(copy);
    printf("%u semantic checks passed\n", checks);
    return 0;
}

#include "poolrad/recovered/resident_08ae_vector_137c_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Semantic checks with a declared scripted environment (never real DOS or PIT). The
 * original-byte differential is tools/overlay/res08ae_vector_137c_cpu_audit.py. */
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#define SIZE PR_R08AE_137C_RAM_SIZE
static uint8_t ram[SIZE];
typedef struct { uint16_t table[512]; uint16_t flags[2]; unsigned dos, ports; uint8_t port_log[8][2]; int refuse_ah, refuse_port; } env_t;
static size_t addr(unsigned s, unsigned o) { return (((size_t)s << 4) + (o & 65535u)) & (SIZE - 1u); }
static void put(unsigned s, unsigned o, unsigned v) { size_t a = addr(s, o); ram[a] = (uint8_t)v; ram[(a + 1u) & (SIZE - 1u)] = (uint8_t)(v >> 8); }
static unsigned get(unsigned s, unsigned o) { return ram[addr(s, o)] | ((unsigned)ram[(addr(s, o) + 1u) & (SIZE - 1u)] << 8); }
static int dos(void *x, uint8_t *m, pr_r08ae_137c_cpu *c, uint16_t *out)
{
    env_t *e = x; unsigned ah = c->ax >> 8, al = c->ax & 255u; (void)m;
    if (e->refuse_ah == (int)ah) return 1;
    if (ah == 0x35u) { c->bx = e->table[al * 2u]; c->es = e->table[al * 2u + 1u]; }
    else if (ah == 0x25u) { e->table[al * 2u] = c->dx; e->table[al * 2u + 1u] = c->ds; }
    else return 1;
    *out = e->flags[e->dos++ & 1u];
    return 0;
}
static int port_out(void *x, uint16_t port, uint8_t v)
{
    env_t *e = x;
    if (e->refuse_port == (int)port || (port != 0x40u && port != 0x43u)) return 1;
    e->port_log[e->ports][0] = (uint8_t)port; e->port_log[e->ports][1] = v; ++e->ports; return 0;
}
static pr_r08ae_137c_cpu fixture(void)
{
    pr_r08ae_137c_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8000, 0x2000, 0x137C, 0x3000, 0x4000, 0x5000, 0x0202};
    memset(ram, 0xA5, sizeof ram);
    put(c.cs, 0x137F, 0x6000); put(c.cs, 0x1394, 0x6100);
    put(c.ss, c.sp, 0x7000); put(c.ss, c.sp + 2, 0x9001);
    return c;
}
int main(void)
{
    pr_r08ae_137c_cpu c, before; env_t e; pr_r08ae_137c_env env = {&e, dos, port_out};
    uint8_t *copy = malloc(SIZE); unsigned bad;
    CHECK(copy != NULL);
    memset(&e, 0, sizeof e); e.refuse_ah = e.refuse_port = -1;
    e.table[16] = 0xABCD; e.table[17] = 0x1234; e.flags[0] = 0x0246; e.flags[1] = 0x0AD7;
    c = fixture();
    CHECK(pr_r08ae_vector_137c_cpu(ram, SIZE, &c, &env) == PR_R08AE_137C_OK);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8004 && c.flags == 0x0AD7);
    /* The old vector is stored four bytes BEFORE the entry; the new vector is DS:DX of the second call. */
    CHECK(get(0x2000, 0x1378) == 0xABCD && get(0x2000, 0x137A) == 0x1234);
    CHECK(e.table[16] == 0x13EA && e.table[17] == 0x6100);
    CHECK(c.ds == 0x3000 && c.es == 0x4000 && c.bx == 0xABCD && c.dx == 0x13EA);
    CHECK(c.ax == 0x1313 && e.dos == 2 && e.ports == 3);
    CHECK(e.port_log[0][0] == 0x43 && e.port_log[0][1] == 0xB6 && e.port_log[1][0] == 0x40 &&
          e.port_log[1][1] == 0xB1 && e.port_log[2][0] == 0x40 && e.port_log[2][1] == 0x13);
    /* The two INT frames leave their three stale words below the live stack. */
    CHECK(get(0x5000, 0x7FF8u) == 0x0246u && get(0x5000, 0x7FF6u) == 0x2000u && get(0x5000, 0x7FF4u) == 0x13A0u);
    CHECK(get(0x5000, 0x7FFAu) == 0x6000u);              /* second PUSH DS of the relocated segment */
    /* Fail closed: an unlisted AH or port is a named refusal, never a default. */
    for (bad = 0; bad < 4; ++bad) {
        memset(&e, 0, sizeof e); e.refuse_ah = e.refuse_port = -1; e.flags[0] = e.flags[1] = 2;
        c = fixture();
        if (bad == 0) e.refuse_ah = 0x35;
        if (bad == 1) e.refuse_ah = 0x25;
        if (bad == 2) e.refuse_port = 0x43;
        if (bad == 3) e.refuse_port = 0x40;
        {
            pr_r08ae_137c_status s = pr_r08ae_vector_137c_cpu(ram, SIZE, &c, &env);
            CHECK(s == (bad < 2 ? PR_R08AE_137C_ENV_DOS : PR_R08AE_137C_ENV_PORT));
            if (bad == 0) CHECK(c.ip == 0x1386 && c.sp == 0x8000 - 4 - 6);
            if (bad == 1) CHECK(c.ip == 0x139E);
            if (bad == 2) CHECK(c.ip == 0x13A3 && e.ports == 0);
            if (bad == 3) CHECK(c.ip == 0x13A8 && e.ports == 1);
        }
    }
    /* A missing environment refuses at the first INT. */
    c = fixture(); CHECK(pr_r08ae_vector_137c_cpu(ram, SIZE, &c, NULL) == PR_R08AE_137C_ENV_DOS);
    { pr_r08ae_137c_env no_port = {&e, dos, NULL}; memset(&e, 0, sizeof e); e.refuse_ah = e.refuse_port = -1;
      c = fixture(); CHECK(pr_r08ae_vector_137c_cpu(ram, SIZE, &c, &no_port) == PR_R08AE_137C_ENV_PORT); }
    /* ARGUMENT rejects before changing CPU or RAM. */
    for (bad = 0; bad < 4; ++bad) {
        c = fixture();
        if (bad == 0) c.ip = 0x137D;
        if (bad == 1) c.sp = 0x000B;
        if (bad == 2) c.sp = 0xFFFD;
        before = c; memcpy(copy, ram, SIZE);
        CHECK(pr_r08ae_vector_137c_cpu(ram, bad == 3 ? SIZE - 1u : SIZE, &c, &env) == PR_R08AE_137C_ARGUMENT);
        CHECK(memcmp(&c, &before, sizeof c) == 0 && memcmp(copy, ram, SIZE) == 0);
    }
    c = fixture();
    CHECK(pr_r08ae_vector_137c_cpu(NULL, SIZE, &c, &env) == PR_R08AE_137C_ARGUMENT);
    CHECK(pr_r08ae_vector_137c_cpu(ram, SIZE, NULL, &env) == PR_R08AE_137C_ARGUMENT);
    free(copy);
    printf("%u semantic checks passed\n", checks);
    return 0;
}

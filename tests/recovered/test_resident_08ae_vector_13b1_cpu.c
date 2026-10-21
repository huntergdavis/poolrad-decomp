#include "poolrad/recovered/resident_08ae_vector_13b1_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Semantic checks with a declared scripted environment (never real DOS, PIT, PPI or PSG).
 * The original-byte differential is tools/overlay/res08ae_vector_13b1_cpu_audit.py. */
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#define SIZE PR_R08AE_13B1_RAM_SIZE
static uint8_t ram[SIZE];
typedef struct {
    uint16_t table[512]; uint16_t flags; uint8_t al_after, in_value;
    unsigned dos, ports; uint8_t log[16][3]; int refuse_ah, refuse_port, refuse_in;
} env_t;
static size_t addr(unsigned s, unsigned o) { return (((size_t)s << 4) + (o & 65535u)) & (SIZE - 1u); }
static void put(unsigned s, unsigned o, unsigned v) { size_t a = addr(s, o); ram[a] = (uint8_t)v; ram[(a + 1u) & (SIZE - 1u)] = (uint8_t)(v >> 8); }
static unsigned get(unsigned s, unsigned o) { return ram[addr(s, o)] | ((unsigned)ram[(addr(s, o) + 1u) & (SIZE - 1u)] << 8); }
static int dos(void *x, uint8_t *m, pr_r08ae_13b1_cpu *c, uint16_t *out)
{
    env_t *e = x; unsigned ah = c->ax >> 8, al = c->ax & 255u; (void)m;
    if (e->refuse_ah == (int)ah || ah != 0x25u) return 1;
    e->table[al * 2u] = c->dx; e->table[al * 2u + 1u] = c->ds;
    c->ax = (uint16_t)((c->ax & 0xFF00u) | e->al_after); *out = e->flags; ++e->dos;
    return 0;
}
static int port_out(void *x, uint16_t port, uint8_t v)
{
    env_t *e = x;
    if (e->refuse_port == (int)port || (port != 0x40u && port != 0x61u && port != 0xC0u)) return 1;
    e->log[e->ports][0] = 'O'; e->log[e->ports][1] = (uint8_t)port; e->log[e->ports][2] = v; ++e->ports; return 0;
}
static int port_in(void *x, uint16_t port, uint8_t *v)
{
    env_t *e = x;
    if (e->refuse_in || port != 0x61u) return 1;
    e->log[e->ports][0] = 'I'; e->log[e->ports][1] = (uint8_t)port; e->log[e->ports][2] = e->in_value; ++e->ports;
    *v = e->in_value; return 0;
}
static pr_r08ae_13b1_cpu fixture(void)
{
    pr_r08ae_13b1_cpu c = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8000, 0x2000, 0x13B1, 0x3000, 0x4000, 0x5000, 0x0202};
    memset(ram, 0xA5, sizeof ram);
    put(c.cs, 0x13B3, 0x6000); put(c.cs, 0x1378, 0xABCD); put(c.cs, 0x137A, 0x1234);
    put(c.ss, c.sp, 0x7000); put(c.ss, c.sp + 2, 0x9001);
    return c;
}
static void fresh(env_t *e)
{ memset(e, 0, sizeof *e); e->refuse_ah = e->refuse_port = -1; e->flags = 0x0246; e->al_after = 0x77; e->in_value = 0xA7; }
int main(void)
{
    pr_r08ae_13b1_cpu c, before; env_t e; pr_r08ae_13b1_env env = {&e, dos, port_out, port_in};
    uint8_t *copy = malloc(SIZE); unsigned bad, i;
    static const uint8_t want[][3] = {{'O',0x40,0xFF},{'O',0x40,0xFF},{'I',0x61,0xA7},{'O',0x61,0xA4},
                                      {'O',0xC0,0x9F},{'O',0xC0,0xBF},{'O',0xC0,0xDF},{'O',0xC0,0xFF}};
    CHECK(copy != NULL);
    fresh(&e); c = fixture();
    CHECK(pr_r08ae_vector_13b1_cpu(ram, SIZE, &c, &env) == PR_R08AE_13B1_OK);
    CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == 0x8004);
    /* The vector restored is the one READ FROM CS:1378/137A, outside the span. */
    CHECK(e.table[16] == 0xABCD && e.table[17] == 0x1234 && e.dos == 1);
    CHECK(c.ax == 0xFFFF && c.dx == 0xABCD && c.ds == 0x3000 && c.bx == 0x2222);
    CHECK(e.ports == 8);
    /* Stale words below the live stack: the two PUSH DS and the INT frame (FLAGS, CS, next IP). */
    CHECK(get(0x5000, 0x7FFE) == 0x3000 && get(0x5000, 0x7FFC) == 0x6000 && get(0x5000, 0x7FFA) == 0x0202 &&
          get(0x5000, 0x7FF8) == 0x2000 && get(0x5000, 0x7FF6) == 0x13C8);
    for (i = 0; i < 8; ++i) CHECK(memcmp(e.log[i], want[i], 3) == 0);
    /* AND FCh sets the flags from the masked byte: 0xA7 & 0xFC = 0xA4 (SF, PF=odd -> clear). */
    CHECK((c.flags & 0x08D5u) == 0x0080u);
    /* Port 61h read of 0x03 masks to zero: ZF and PF set. */
    fresh(&e); e.in_value = 0x03; c = fixture();
    CHECK(pr_r08ae_vector_13b1_cpu(ram, SIZE, &c, &env) == PR_R08AE_13B1_OK && (c.flags & 0x08D5u) == 0x0044u);
    CHECK(e.log[3][2] == 0x00);
    /* The stack may overlap the saved-vector slot: the body reads what its pushes left there. */
    fresh(&e); c = fixture(); c.ss = 0x2000; c.sp = 0x137Eu;
    memset(ram, 0xA5, 0x30000); put(0x2000, 0x13B3, 0x6000); put(c.ss, c.sp, 0x7000); put(c.ss, c.sp + 2, 0x9001);
    CHECK(pr_r08ae_vector_13b1_cpu(ram, SIZE, &c, &env) == PR_R08AE_13B1_OK);
    CHECK(e.table[16] == 0xA5A5 && e.table[17] == 0x6000);   /* slot word 1378 untouched (A5A5), 137A holds the second push */
    /* Fail closed: AH=35h, an unlisted port or IN, and a missing environment are named refusals. */
    for (bad = 0; bad < 6; ++bad) {
        fresh(&e); c = fixture();
        if (bad == 0) e.refuse_ah = 0x25;
        if (bad == 1) e.refuse_port = 0x40;
        if (bad == 2) e.refuse_port = 0x61;
        if (bad == 3) e.refuse_port = 0xC0;
        if (bad == 4) e.refuse_in = 1;
        if (bad == 5) env.port_in = NULL;
        {
            pr_r08ae_13b1_status s = pr_r08ae_vector_13b1_cpu(ram, SIZE, &c, &env);
            CHECK(s == (bad == 0 ? PR_R08AE_13B1_ENV_DOS : PR_R08AE_13B1_ENV_PORT));
            if (bad == 0) CHECK(c.ip == 0x13C6 && c.sp == 0x8000 - 4 - 6);
            if (bad == 1) CHECK(c.ip == 0x13CC && e.ports == 0);
            if (bad == 2) CHECK(c.ip == 0x13D6 && e.ports == 3);
            if (bad == 3) CHECK(c.ip == 0x13DA && e.ports == 4);
            if (bad == 4 || bad == 5) CHECK(c.ip == 0x13D2 && e.ports == 2);
        }
        env.port_in = port_in;
    }
    c = fixture(); CHECK(pr_r08ae_vector_13b1_cpu(ram, SIZE, &c, NULL) == PR_R08AE_13B1_ENV_DOS);
    /* ARGUMENT rejects before changing CPU or RAM. */
    for (bad = 0; bad < 4; ++bad) {
        c = fixture();
        if (bad == 0) c.ip = 0x13B2;
        if (bad == 1) c.sp = 0x0009;
        if (bad == 2) c.sp = 0xFFFD;
        before = c; memcpy(copy, ram, SIZE);
        CHECK(pr_r08ae_vector_13b1_cpu(ram, bad == 3 ? SIZE - 1u : SIZE, &c, &env) == PR_R08AE_13B1_ARGUMENT);
        CHECK(memcmp(&c, &before, sizeof c) == 0 && memcmp(copy, ram, SIZE) == 0);
    }
    c = fixture();
    CHECK(pr_r08ae_vector_13b1_cpu(NULL, SIZE, &c, &env) == PR_R08AE_13B1_ARGUMENT);
    CHECK(pr_r08ae_vector_13b1_cpu(ram, SIZE, NULL, &env) == PR_R08AE_13B1_ARGUMENT);
    free(copy);
    printf("%u semantic checks passed\n", checks);
    return 0;
}

#include "poolrad/recovered/system_real_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#define SS 0x2800u
#define SP 0x8000u
#define SYSTEM 0x0AF8u

typedef pr_system_real_status (*entry_fn)(uint8_t *, size_t, pr_system_real_cpu *);

static unsigned at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static unsigned rd(const uint8_t *r, unsigned s, unsigned o)
{
    unsigned p = at(s, o);
    return r[p] | ((unsigned)r[(p + 1) & 0xFFFFF] << 8);
}
static pr_system_real_cpu setup(uint8_t *r, unsigned entry)
{
    pr_system_real_cpu c = {0, 0, 0, 0, 0, 0, 0x7777, SP, SYSTEM, entry, 0x1234, 0x5678, SS, 0x0202};
    unsigned p = at(SS, SP);
    memset(r, 0xA5, PR_SYSTEM_REAL_RAM_SIZE);
    r[p] = 0x00; r[p + 1] = 0x70; r[p + 2] = 0x01; r[p + 3] = 0x90;      /* return IP 7000, CS 9001 */
    return c;
}
static void returned(const pr_system_real_cpu *c, const uint8_t *r, unsigned near_ip)
{
    CHECK(c->cs == 0x9001 && c->ip == 0x7000 && c->sp == SP + 4);
    CHECK(rd(r, SS, SP - 2) == near_ip);                       /* the near CALL's return IP stays below SP */
}
/* A 6-byte Real as AL exponent, AH/BX/DX mantissa; DX bit 15 is the sign. */
static void load_real(pr_system_real_cpu *c, unsigned exponent, unsigned ah, unsigned bx, unsigned dx)
{
    c->ax = (uint16_t)(exponent | (ah << 8)); c->bx = (uint16_t)bx; c->dx = (uint16_t)dx;
}

int main(void)
{
    uint8_t *r = malloc(PR_SYSTEM_REAL_RAM_SIZE), *saved = malloc(PR_SYSTEM_REAL_RAM_SIZE);
    pr_system_real_cpu c, before;
    CHECK(r && saved);

    /* LongInt to Real (119A): 0 is the zero Real; 1 is exponent 81h; -1 sets the sign; 3 is 1.5 * 2. */
    c = setup(r, 0x119A);
    CHECK(pr_system_real_from_long_119a(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    returned(&c, r, 0x119D);
    CHECK((c.ax & 0xFF) == 0 && c.dx == 0);                    /* 0 -> exponent 0 */
    c = setup(r, 0x119A); c.ax = 1;
    CHECK(pr_system_real_from_long_119a(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    returned(&c, r, 0x119D);
    CHECK((c.ax & 0xFF) == 0x81 && c.bx == 0 && c.dx == 0);
    c = setup(r, 0x119A); c.ax = 0xFFFF; c.dx = 0xFFFF;        /* -1 */
    CHECK(pr_system_real_from_long_119a(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    CHECK((c.ax & 0xFF) == 0x81 && c.bx == 0 && c.dx == 0x8000);
    c = setup(r, 0x119A); c.ax = 3;
    CHECK(pr_system_real_from_long_119a(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    CHECK((c.ax & 0xFF) == 0x82 && c.bx == 0 && c.dx == 0x4000);
    c = setup(r, 0x119A); c.ax = 0; c.dx = 1;                  /* 65536 = 2^16 -> exponent 81h + 16 */
    CHECK(pr_system_real_from_long_119a(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    CHECK((c.ax & 0xFF) == 0x91 && c.bx == 0 && c.dx == 0);

    /* Real to LongInt (119E), truncating: 1.0 -> 1, 2.5 -> 2, -2.5 -> -2, below 1.0 -> 0, exponent >= A0h -> RunError 207. */
    c = setup(r, 0x119E); load_real(&c, 0x81, 0, 0, 0);
    CHECK(pr_system_real_trunc_119e(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    returned(&c, r, 0x11A3);
    CHECK(c.ax == 1 && c.dx == 0 && !(c.flags & 1));
    c = setup(r, 0x119E); load_real(&c, 0x82, 0, 0, 0x2000);   /* 1.25 * 2 */
    CHECK(pr_system_real_trunc_119e(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    CHECK(c.ax == 2 && c.dx == 0);
    c = setup(r, 0x119E); load_real(&c, 0x82, 0, 0, 0xA000);   /* -2.5 */
    CHECK(pr_system_real_trunc_119e(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    CHECK(c.ax == 0xFFFE && c.dx == 0xFFFF);
    c = setup(r, 0x119E); load_real(&c, 0x7F, 0, 0, 0);        /* 0.5 truncates to zero */
    CHECK(pr_system_real_trunc_119e(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    CHECK(c.ax == 0 && c.dx == 0);
    c = setup(r, 0x119E); load_real(&c, 0xA0, 0, 0, 0);        /* 2^32: does not fit */
    CHECK(pr_system_real_trunc_119e(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RUNERROR);
    CHECK(c.ax == 0x00CF && c.ip == 0x00E2 && c.cs == SYSTEM && c.sp == SP);   /* the near return IP was popped, the far frame was not */

    /* Real multiply (1186): 1.0 * 1.0 = 1.0; 3.0 * -2.0 = -6.0; a zero operand gives zero; two huge exponents overflow (RunError 205). */
    c = setup(r, 0x1186); load_real(&c, 0x81, 0, 0, 0); c.cx = 0x81; c.si = 0; c.di = 0;
    CHECK(pr_system_real_mul_1186(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    returned(&c, r, 0x1189);
    CHECK((c.ax & 0xFF) == 0x81 && c.bx == 0 && c.dx == 0);
    c = setup(r, 0x1186); load_real(&c, 0x82, 0, 0, 0x4000); c.cx = 0x82; c.si = 0; c.di = 0x8000;
    CHECK(pr_system_real_mul_1186(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    CHECK((c.ax & 0xFF) == 0x83 && c.bx == 0 && c.dx == 0xC000);
    c = setup(r, 0x1186); load_real(&c, 0, 1, 2, 3); c.cx = 0x90; c.si = 5; c.di = 6;
    CHECK(pr_system_real_mul_1186(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    CHECK(c.ax == 0 && c.bx == 0 && c.dx == 0);
    c = setup(r, 0x1186); load_real(&c, 0xFF, 0, 0, 0); c.cx = 0xFF; c.si = 0; c.di = 0;
    CHECK(pr_system_real_mul_1186(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RUNERROR);
    CHECK(c.ax == 0x00CD && c.ip == 0x00E2 && c.cs == SYSTEM);
    /* The general path's six saved registers stay in RAM below the returned SP. */
    c = setup(r, 0x1186); load_real(&c, 0x82, 0x11, 0x2222, 0x4444); c.cx = 0x82; c.si = 0x3333; c.di = 0x5555;
    CHECK(pr_system_real_mul_1186(r, PR_SYSTEM_REAL_RAM_SIZE, &c) == PR_SYSTEM_REAL_RETURNED);
    CHECK(rd(r, SS, SP - 4) == 0x7777 && rd(r, SS, SP - 2) == 0x1189);   /* the saved BP sits below the near return IP */

    /* ARGUMENT: wrong entry IP, SP out of range, bad host objects; nothing changes. */
    for (unsigned trial = 0; trial < 6; ++trial) {
        c = setup(r, 0x1186);
        if (trial == 0) c.ip = 0x1187;
        if (trial == 1) c.sp = 0x003F;
        if (trial == 2) c.sp = 0xFFFD;
        before = c; memcpy(saved, r, PR_SYSTEM_REAL_RAM_SIZE);
        CHECK(pr_system_real_mul_1186(trial == 3 ? NULL : r, trial == 4 ? 1 : PR_SYSTEM_REAL_RAM_SIZE, trial == 5 ? NULL : &c) == PR_SYSTEM_REAL_ARGUMENT);
        if (trial != 5) CHECK(memcmp(&before, &c, sizeof c) == 0);
        CHECK(memcmp(saved, r, PR_SYSTEM_REAL_RAM_SIZE) == 0);
    }
    free(saved); free(r);
    printf("%u semantic checks\n", checks);
    return 0;
}

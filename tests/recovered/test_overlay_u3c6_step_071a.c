#include "poolrad/recovered/overlay_u3c6_step_071a.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define SIZE PR_U6C_3F31_RAM_SIZE
static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (SIZE - 1u); }
static void word(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = at(seg, off); ram[p] = (uint8_t)value;
    ram[(p + 1u) & (SIZE - 1u)] = (uint8_t)(value >> 8);
}
static pr_u3c6_071a_cpu setup(uint8_t *ram, uint8_t mode, uint8_t direction,
                             uint8_t x, uint8_t delta)
{
    pr_u3c6_071a_cpu c = {0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                         0x7777,0x8000,0x6000,0x071A,0x1000,0x1111,0x2000,0x202};
    unsigned i;
    memset(ram, 0xA5, SIZE);
    ram[at(c.ds, 0x5F2A)] = 3;
    word(ram,c.ds,0x65BD,0x1234); word(ram,c.ds,0x65BF,0x5678);
    ram[at(c.ds,0x5F2E)] = direction;
    ram[at(c.ds,0x604B)] = x; ram[at(c.ds,0x6093)] = 3;
    for (i=0; i<4; ++i) {
        ram[at(c.ds,(uint16_t)(0x2844+8u*direction+2u*i))] = delta;
        ram[at(c.ds,(uint16_t)(0x2845+8u*direction+2u*i))] = 0;
    }
    word(ram,c.ss,0x8000,0x9000); word(ram,c.ss,0x8002,0x7000);
    word(ram,c.ss,0x8004,mode); word(ram,c.ss,0x8006,0x1234); word(ram,c.ss,0x8008,0x5678);
    return c;
}
int main(void)
{
    uint8_t *ram = malloc(SIZE), *before = malloc(SIZE);
    pr_u3c6_071a_cpu c, saved;
    unsigned mode, x;
    assert(ram && before);
    for (mode=0; mode<2; ++mode) for (x=0; x<9; ++x) {
        c=setup(ram,(uint8_t)mode,1,(uint8_t)x,0);
        assert(pr_u3c6_step_071a(ram,SIZE,&c,5)==PR_U3C6_071A_OK);
        assert((c.ax & 255u)==(x<=6));
        assert(c.cs==0x7000 && c.ip==0x9000 && c.sp==0x800A && c.bp==0x7777);
        assert(c.bx==0x3456 && c.si==0x9ABC && c.ds==0x1000 && c.ss==0x2000);
    }
    c=setup(ram,0,0,3,0);
    assert(pr_u3c6_step_071a(ram,SIZE,&c,1)==PR_U3C6_071A_OK && (c.ax&255u)==0);
    c=setup(ram,1,1,3,128);
    assert(pr_u3c6_step_071a(ram,SIZE,&c,5)==PR_U3C6_071A_OK && (c.ax&255u)==1);
    c=setup(ram,1,1,3,0);
    assert(pr_u3c6_step_071a(ram,SIZE,&c,0)==PR_U3C6_071A_LIMIT && c.ip==0x0C71);
    c=setup(ram,1,1,3,0);
    assert(pr_u3c6_step_071a(ram,SIZE,&c,1)==PR_U3C6_071A_LIMIT && c.ip==0x0753);
    c=setup(ram,0,1,3,0); c.ip=0;
    saved=c; memcpy(before,ram,SIZE);
    assert(pr_u3c6_step_071a(ram,SIZE,&c,512)==PR_U3C6_071A_ARGUMENT);
    assert(!memcmp(&c,&saved,sizeof c) && !memcmp(ram,before,SIZE));
    c=setup(ram,0,1,3,0); c.ss=c.cs; c.sp=0x0730; saved=c; memcpy(before,ram,SIZE);
    assert(pr_u3c6_step_071a(ram,SIZE,&c,512)==PR_U3C6_071A_ARGUMENT);
    assert(!memcmp(&c,&saved,sizeof c) && !memcmp(ram,before,SIZE));
    free(before); free(ram); return 0;
}

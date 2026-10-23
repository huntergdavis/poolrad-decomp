#include "loaded_stub_fixture.h"
#include "poolrad/recovered/overlay_u38_pair_04ba.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define SIZE PR_U9E_10E4_RAM_SIZE
static size_t at(uint16_t seg,uint16_t off) { return (((size_t)seg<<4)+off)&(SIZE-1u); }
static void word(uint8_t *ram,uint16_t seg,uint16_t off,uint16_t value)
{ size_t p=at(seg,off); ram[p]=(uint8_t)value; ram[(p+1u)&(SIZE-1u)]=(uint8_t)(value>>8); }
static pr_u38_04ba_cpu setup(uint8_t *ram,uint8_t source,uint8_t first,uint8_t second)
{
    pr_u38_04ba_cpu c={0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x6000,0x04BA,0x1000,0x1111,0x2000,0x202};
    memset(ram,0xA5,SIZE);
    test_loaded_stub(ram, c.cs, 0x04DDu, (uint16_t)(0x5000u + 0x100u), 0x0075u, 0x5000u, 0x110Du);
    test_loaded_stub(ram, c.cs, 0x0552u, (uint16_t)(0x5000u + 0x100u), 0x0075u, 0x5000u, 0x110Du);
    test_loaded_stub(ram, c.cs, 0x05B0u, (uint16_t)(0x5000u + 0x100u), 0x0070u, 0x5000u, 0x10E4u);
    ram[at(0x3000,0x103)]=source;
    ram[at(0x3000,0x300)]=12; ram[at(0x3000,0x303)]=first;
    ram[at(0x3000,0x400)]=38; ram[at(0x3000,0x403)]=second;
    word(ram,0x3000,0x305,0x400); word(ram,0x3000,0x307,0x3000);
    word(ram,0x3000,0x405,0); word(ram,0x3000,0x407,0);
    word(ram,0x4000,0x17F,0x300); word(ram,0x4000,0x181,0x3000);
    word(ram,c.ss,0x8000,0x9000); word(ram,c.ss,0x8002,0x7000);
    word(ram,c.ss,0x8004,0xCAFE); word(ram,c.ss,0x8006,0x100);
    word(ram,c.ss,0x8008,0x3000); word(ram,c.ss,0x800A,0x100); word(ram,c.ss,0x800C,0x4000);
    return c;
}
static void returned(pr_u38_04ba_cpu c)
{
    assert(c.cs==0x7000 && c.ip==0x9000 && c.sp==0x800E && c.bp==0x7777);
    assert(c.bx==0x3456 && c.si==0x9ABC && c.ds==0x1000 && c.ss==0x2000);
}
int main(void)
{
    uint8_t *ram=malloc(SIZE),*before=malloc(SIZE);
    pr_u38_04ba_cpu c,saved;
    assert(ram && before);
    c=setup(ram,128,6,106);
    assert(pr_u38_pair_04ba(ram,SIZE,&c,0x5000,0)==PR_U38_04BA_OK);
    returned(c); assert(ram[at(0x4000,0x110)]==0xA5);
    c=setup(ram,5,6,106);
    assert(pr_u38_pair_04ba(ram,SIZE,&c,0x5000,3)==PR_U38_04BA_OK);
    returned(c); assert(ram[at(0x4000,0x110)]==18 && ram[at(0x4000,0x116)]==5);
    assert(ram[at(0x3000,0x303)]==5 && ram[at(0x3000,0x403)]==106);
    c=setup(ram,106,106,127);
    assert(pr_u38_pair_04ba(ram,SIZE,&c,0x5000,3)==PR_U38_04BA_OK);
    returned(c); assert(ram[at(0x4000,0x110)]==27 && ram[at(0x4000,0x116)]==0);
    assert(ram[at(0x3000,0x403)]==106);
    c=setup(ram,5,6,106); word(ram,0x4000,0x17F,0); word(ram,0x4000,0x181,0);
    assert(pr_u38_pair_04ba(ram,SIZE,&c,0x5000,1)==PR_U38_04BA_OK);
    assert(ram[at(0x4000,0x110)]==18 && ram[at(0x4000,0x116)]==4);
    c=setup(ram,5,6,106); word(ram,0x3000,0x405,0x300); word(ram,0x3000,0x407,0x3000);
    assert(pr_u38_pair_04ba(ram,SIZE,&c,0x5000,8)==PR_U38_04BA_LIMIT && c.ip==0x0518);
    c=setup(ram,5,6,106); c.ip=0; saved=c; memcpy(before,ram,SIZE);
    assert(pr_u38_pair_04ba(ram,SIZE,&c,0x5000,8)==PR_U38_04BA_ARGUMENT);
    assert(!memcmp(&c,&saved,sizeof c) && !memcmp(ram,before,SIZE));
    c=setup(ram,5,6,106); c.ss=c.cs; c.sp=0x04D0; saved=c; memcpy(before,ram,SIZE);
    assert(pr_u38_pair_04ba(ram,SIZE,&c,0x5000,8)==PR_U38_04BA_ARGUMENT);
    assert(!memcmp(&c,&saved,sizeof c) && !memcmp(ram,before,SIZE));
    free(before); free(ram); return 0;
}

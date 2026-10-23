#include "loaded_stub_fixture.h"
#include "poolrad/recovered/overlay_u5e_neighbor_1bf8.h"
#include <stdio.h>
#include <string.h>

static uint8_t ram[PR_U5E_1BF8_RAM_SIZE], before[PR_U5E_1BF8_RAM_SIZE];
static unsigned checks;
#define CHECK(v) do { ++checks; if (!(v)) { fprintf(stderr, "line %d: %s\n", __LINE__, #v); return 1; } } while (0)
static size_t phys(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & (PR_U5E_1BF8_RAM_SIZE - 1u); }
static void word(uint16_t s, uint16_t o, uint16_t v)
{
    size_t p = phys(s,o);
    ram[p]=(uint8_t)v; ram[(p+1u)&(PR_U5E_1BF8_RAM_SIZE-1u)]=(uint8_t)(v>>8);
}
static uint16_t read_word(uint16_t s, uint16_t o)
{
    size_t p=phys(s,o);
    return (uint16_t)(ram[p] | ((uint16_t)ram[(p+1u)&(PR_U5E_1BF8_RAM_SIZE-1u)]<<8));
}
static pr_u5e_1bf8_cpu setup(uint16_t sp, uint8_t occupant)
{
    pr_u5e_1bf8_cpu c={0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,
                       sp,0x6000,0x1BF8,0x1000,0x1111,0x2000,0x202};
    unsigned i;
    memset(ram,0xA5,sizeof ram);
    test_loaded_stub(ram, c.cs, 0x1C1Du, (uint16_t)(0x7000u + 0x100u), 0x005Cu, 0x7000u, 0x0BE7u);
    test_loaded_stub(ram, c.cs, 0x1C3Cu, (uint16_t)(0x7000u + 0x100u), 0x0061u, 0x7000u, 0x0C0Fu);
    test_loaded_stub(ram, c.cs, 0x1C67u, (uint16_t)(0x7000u + 0x100u), 0x003Eu, 0x7000u, 0x04BEu);
    for(i=0;i<1250u;++i) {
        ram[phys(0x1000,(uint16_t)(0x60DBu+i))]=occupant;
        ram[phys(0x4000,(uint16_t)(0x107u+i))]=0u;
    }
    for(i=0;i<9u;++i) {
        ram[phys(0x1000,(uint16_t)(0x2736u+i))]=0u;
        ram[phys(0x1000,(uint16_t)(0x273Fu+i))]=0u;
    }
    word(0x1000,0x65BD,0x2000);word(0x1000,0x65BF,0x5000);
    word(0x1000,0x65C1,0x2400);word(0x1000,0x65C3,0x5000);
    ram[phys(0x1000,0x5F2A)]=2;
    ram[phys(0x1000,0x5F2B)]=10;ram[phys(0x1000,0x5F2C)]=10;
    word(0x1000,0x6716,0x100);word(0x1000,0x6718,0x4000);
    ram[phys(0x5000,0x210E)]=1;ram[phys(0x5000,0x250E)]=1;
    ram[phys(0x5000,0x251B)]=3;ram[phys(0x5000,0x2432)]=20;
    word(0x2000,sp,0x4000);word(0x2000,(uint16_t)(sp+2u),0x9000);
    word(0x2000,(uint16_t)(sp+4u),0x800);word(0x2000,(uint16_t)(sp+6u),0x5000);
    word(0x2000,(uint16_t)(sp+8u),0x2000);word(0x2000,(uint16_t)(sp+10u),0x5000);
    return c;
}
int main(void)
{
    const pr_u5e_1bf8_layout layout={0x7000,0x8000};
    pr_u5e_1bf8_cpu c,saved;
    unsigned i;
    const uint16_t frames[]={42u,0x8000u,0xFFF4u};
    for(i=0;i<3u;++i) {
        c=setup(frames[i],2);
        CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,27)==PR_U5E_1BF8_OK);
        CHECK(read_word(0x5000,0x800)==0x2400 && read_word(0x5000,0x802)==0x5000);
        CHECK(c.ax==0x7401 && c.flags==0x206);
        CHECK(c.cs==0x9000 && c.ip==0x4000 && c.sp==(uint16_t)(frames[i]+12u) && c.bp==0x7777);
    }
    c=setup(0x8000,0);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,27)==PR_U5E_1BF8_OK);
    CHECK(c.ax==0 && c.flags==0x246 && read_word(0x5000,0x800)==0 && read_word(0x5000,0x802)==0);
    c=setup(0x8000,2);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,1)==PR_U5E_1BF8_LIMIT);
    CHECK(c.cs==0x7000 && c.ip==0xC71 && c.sp==0x7FD6);
    c=setup(0x8000,2);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,3)==PR_U5E_1BF8_LIMIT);
    CHECK(c.cs==0x6000 && c.ip==0x1C17 && ram[phys(0x2000,0x7FF4)]==1);
    c=setup(0x8000,2);
    /* The final far return reads the live frame overwritten by the output. */
    word(0x2000,0x8004,0x8000);word(0x2000,0x8006,0x2000);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,27)==PR_U5E_1BF8_OK);
    CHECK(c.cs==0x5000 && c.ip==0x2400 && c.sp==0x800C);
    c=setup(0x8000,2);
    word(0x2000,0x8004,0x1BD6);word(0x2000,0x8006,0x6000);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,27)==PR_U5E_1BF8_CODE_ALIAS);
    CHECK(c.cs==0x6000 && c.ip==0x1C8E);
    CHECK(read_word(0x6000,0x1BD6)==0x2400 && read_word(0x6000,0x1BD8)==0xA5A5);
    c=setup(0x8000,0);saved=c;memcpy(before,ram,sizeof ram);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,0)==PR_U5E_1BF8_ARGUMENT);
    CHECK(memcmp(&c,&saved,sizeof c)==0 && memcmp(ram,before,sizeof ram)==0);
    CHECK(pr_u5e_neighbor_1bf8(NULL,sizeof ram,&c,&layout,1)==PR_U5E_1BF8_ARGUMENT);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,NULL,&layout,1)==PR_U5E_1BF8_ARGUMENT);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,NULL,1)==PR_U5E_1BF8_ARGUMENT);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram-1u,&c,&layout,1)==PR_U5E_1BF8_ARGUMENT);
    c.sp=41;saved=c;
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,1)==PR_U5E_1BF8_ARGUMENT);
    CHECK(memcmp(&c,&saved,sizeof c)==0 && memcmp(ram,before,sizeof ram)==0);
    c.sp=0xFFF5;saved=c;
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,1)==PR_U5E_1BF8_ARGUMENT);
    CHECK(memcmp(&c,&saved,sizeof c)==0 && memcmp(ram,before,sizeof ram)==0);
    c=setup(0x1C20,0);c.ss=0x6000;saved=c;memcpy(before,ram,sizeof ram);
    CHECK(pr_u5e_neighbor_1bf8(ram,sizeof ram,&c,&layout,1)==PR_U5E_1BF8_CODE_ALIAS);
    CHECK(memcmp(&c,&saved,sizeof c)==0 && memcmp(ram,before,sizeof ram)==0);
    printf("u5e +1BF8: %u checks passed\n",checks);
    return 0;
}

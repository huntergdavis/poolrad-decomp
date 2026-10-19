#include "poolrad/recovered/overlay_u38_clear_cycle_2e08_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t ram[1u<<20];
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static const uint16_t sites[]={0x2E0F,0x2E16,0x2E1D,0x2E24};
static const uint8_t keys[]={0x0B,0x35,0x34,0x37};
static size_t at(unsigned s,unsigned o) {return ((s<<4)+o)&((1u<<20)-1u);}
static void put(unsigned s,unsigned o,unsigned v) {size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1u)&((1u<<20)-1u)]=(uint8_t)(v>>8);}
static unsigned get(unsigned s,unsigned o) {size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1u)&((1u<<20)-1u)]<<8);}
static void calls(unsigned cs)
{
    for(unsigned i=0;i<4;i++) {ram[at(cs,sites[i])]=0xE8;put(cs,sites[i]+1,(uint16_t)(0u-sites[i]-3u));}
}
static pr_u38_2e08_cpu fixture(unsigned latch,unsigned gate,unsigned ah)
{
    pr_u38_2e08_cpu c={0,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x5000,0x2E08,0x1800,0x3300,0x2800,0x8D7};
    memset(ram,0xA5,sizeof ram);c.ax=(uint16_t)((ah<<8)|0xCD);
    ram[at(c.ds,0x6816)]=0x12;ram[at(c.ds,0x6817)]=(uint8_t)latch;ram[at(c.ds,0x6818)]=0xA7;ram[at(c.ds,0x682A)]=(uint8_t)gate;
    put(c.ss,c.sp,0x7000);put(c.ss,c.sp+2,0x9001);
    for(unsigned i=0;i<5;i++) put(c.ss,c.sp+4+2*i,0xA100+i);
    calls(c.cs);return c;
}
static unsigned cmp_zero_flags(unsigned gate)
{
    unsigned ones=0;
    for(unsigned i=0;i<8;i++) ones+=(gate>>i)&1u;
    return 2u | (!(ones&1u)?4u:0u) | (!gate?0x40u:0u) | (gate&0x80u);
}
int main(void)
{
    const unsigned gates[]={0,1,2,0x7F,0x80,0xFF};
    int status;
    pr_u38_2e08_cpu c,before;
    for(unsigned latch=0;latch<256;latch++) for(unsigned g=0;g<6;g++) for(unsigned ah=0;ah<=255;ah+=255) {
        unsigned match=latch==0x0B||latch==0x35||latch==0x34||latch==0x37;
        unsigned final_al=match&&latch!=0x37?0:latch,gate=gates[g];
        c=fixture(latch,gate,ah);before=c;status=-99;
        CHECK(pr_u38_clear_cycle_2e08_cpu(ram,sizeof ram,&c,&status)==PR_U38_2E08_RETURNED);
        CHECK(status==0);
        CHECK(c.ax==((ah<<8)|final_al));
        CHECK(c.flags==cmp_zero_flags(gate));
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800E && c.bp==before.bp);
        CHECK(c.bx==before.bx && c.cx==before.cx && c.dx==before.dx && c.si==before.si && c.di==before.di);
        CHECK(c.ds==before.ds && c.es==before.es && c.ss==before.ss);
        CHECK(ram[at(c.ds,0x6817)]==(match?0:latch));
        CHECK(ram[at(c.ds,0x6818)]==(match?0:0xA7));
        CHECK(ram[at(c.ds,0x6816)]==(gate?0x12:0x64));
        CHECK(get(c.ss,0x7FF8)==0x2E27 && get(c.ss,0x7FFA)==0x5000 && get(c.ss,0x7FFC)==((ah<<8)|0x37));
        for(unsigned i=0;i<5;i++) CHECK(get(c.ss,0x8004+2*i)==0xA100+i);
    }
    for(unsigned i=0;i<4;i++) {
        c=fixture(0x37,0,0xA5);ram[at(c.cs,sites[i])]=0x90;
        CHECK(pr_u38_clear_cycle_2e08_cpu(ram,sizeof ram,&c,&status)==PR_U38_2E08_CODE);
        CHECK(c.ip==sites[i] && c.cs==0x5000 && c.sp==0x7FFA && c.bp==0x7FFE);
        CHECK(c.ax==(0xA500u|keys[i]) && status==(i?0:-1));
        CHECK(ram[at(c.ds,0x6817)]==0x37 && ram[at(c.ds,0x6818)]==0xA7 && ram[at(c.ds,0x6816)]==0x12);
        c=fixture(0x37,0,0xA5);c.cs=(uint16_t)(0x5000u|keys[i]);c.ss=c.ds;c.sp=0x681D;calls(c.cs);put(c.ss,c.sp,0x7000);put(c.ss,c.sp+2,0x9001);
        CHECK(pr_u38_clear_cycle_2e08_cpu(ram,sizeof ram,&c,&status)==PR_U38_2E08_TRANSFER);
        CHECK(c.cs==0 && c.ip==sites[i]+3 && c.sp==0x681B && status==0);
        CHECK(ram[at(c.ds,0x6817)]==0 && ram[at(c.ds,0x6818)]==0);
    }
    c=fixture(0,0,0);before=c;status=123;
    CHECK(pr_u38_clear_cycle_2e08_cpu(NULL,sizeof ram,&c,&status)==PR_U38_2E08_ARGUMENT);
    CHECK(pr_u38_clear_cycle_2e08_cpu(ram,sizeof ram,NULL,&status)==PR_U38_2E08_ARGUMENT);
    CHECK(pr_u38_clear_cycle_2e08_cpu(ram,sizeof ram-1,&c,&status)==PR_U38_2E08_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && status==123);
    c.sp=9;CHECK(pr_u38_clear_cycle_2e08_cpu(ram,sizeof ram,&c,NULL)==PR_U38_2E08_ARGUMENT);
    c.sp=0xFFFD;CHECK(pr_u38_clear_cycle_2e08_cpu(ram,sizeof ram,&c,NULL)==PR_U38_2E08_ARGUMENT);
    c.sp=0x8000;c.ip=0;CHECK(pr_u38_clear_cycle_2e08_cpu(ram,sizeof ram,&c,NULL)==PR_U38_2E08_ARGUMENT);
    c=fixture(0x34,0x80,0);CHECK(pr_u38_clear_cycle_2e08_cpu(ram,sizeof ram,&c,NULL)==PR_U38_2E08_RETURNED);
    printf("U38 2E08: %u checks\n",checks);return 0;
}

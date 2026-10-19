#include "poolrad/recovered/overlay_u7d_update_047a_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t ram[1u << 20];
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static size_t at(unsigned s, unsigned o) { return ((s << 4) + o) & ((1u << 20) - 1u); }
static void put(unsigned s, unsigned o, unsigned v)
{ size_t p=at(s,o); ram[p]=(uint8_t)v;ram[(p+1)&((1u<<20)-1)]=(uint8_t)(v>>8); }
static unsigned get(unsigned s, unsigned o)
{ size_t p=at(s,o);return ram[p] | ((unsigned)ram[(p+1)&((1u<<20)-1)]<<8); }
static pr_u7d_047a_cpu fixture(unsigned value, unsigned index, unsigned amount)
{
    pr_u7d_047a_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x5000,0x047A,0x1800,0x3300,0x2800,0x402};
    memset(ram,0xA5,sizeof ram);
    for(unsigned i=0;i<7;i++) {put(c.ds,0x6DC2+2*i,value%10);value/=10;put(c.ds,0x363A+2*i,10);}
    put(c.ds,0x5D96,0);put(c.ds,0x5D98,0);
    put(c.ss,c.sp,0x7000);put(c.ss,c.sp+2,0x9001);put(c.ss,c.sp+4,index);put(c.ss,c.sp+6,amount);
    ram[at(c.cs,0x4F0)]=0x9A;put(c.cs,0x4F1,0x16A8);put(c.cs,0x4F3,0xAF8);
    ram[at(c.cs,0x55A)]=0xE8;put(c.cs,0x55B,0xFE46);
    return c;
}
int main(void)
{
    static const unsigned values[]={0,1,9,10,11,99,100,999,1000,9999,10000,10001,54321,99999};
    pr_u7d_047a_cpu c,before;pr_u7d_047a_diagnostic d;
    for(unsigned j=0;j<sizeof values/sizeof values[0];j++) for(unsigned index=0,power=1;index<5;index++,power*=10) for(unsigned amount=0;amount<256;amount+=17) {
        unsigned initial=values[j],sub=amount*power,want=initial<10?initial:initial>sub?initial-sub:0;
        c=fixture(initial,index|0xAB00,amount|0xCD00);
        CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram,&c,100000,&d)==PR_U7D_047A_RETURNED);
        for(unsigned k=0;k<7;k++) { CHECK(get(c.ds,0x6DC2+2*k)==want%10);want/=10; }
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8008 && c.bp==0x7777);
        CHECK(get(c.ds,0x5D96)==0 && get(c.ds,0x5D98)==0);
        CHECK(d.callee==(initial<10?-1:1));
    }
    c=fixture(1000,0,20);before=c;
    CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram,&c,0,&d)==PR_U7D_047A_LIMIT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && d.callee==-1 && d.status==-1);
    CHECK(get(c.ss,0x7FFE)==0xA5A5);
    c=fixture(1000,0,20);
    CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram,&c,3,&d)==PR_U7D_047A_LIMIT);
    CHECK(c.ip==0x480 && c.bp==0x7FFE && c.sp==0x7FFA && get(c.ss,0x7FFE)==0x7777);
    c=fixture(1000,0,20);ram[at(c.cs,0x55A)]=0x90;
    CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram,&c,10000,&d)==PR_U7D_047A_CODE);
    CHECK(c.ip==0x55A && d.callee==-1);
    c=fixture(10000,4,2);ram[at(c.cs,0x4F0)]=0x90;
    CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram,&c,10000,&d)==PR_U7D_047A_CODE);
    CHECK(c.ip==0x4F0 && d.callee==-1);
    c=fixture(100,0,3);before=c;
    CHECK(pr_u7d_update_047a_cpu(NULL,sizeof ram,&c,10,&d)==PR_U7D_047A_ARGUMENT);
    CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram,NULL,10,&d)==PR_U7D_047A_ARGUMENT);
    CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram-1,&c,10,&d)==PR_U7D_047A_ARGUMENT);
    CHECK(memcmp(&before,&c,sizeof c)==0);
    c.sp=0x1F;CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram,&c,10,NULL)==PR_U7D_047A_ARGUMENT);
    c.sp=0xFFF9;CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram,&c,10,NULL)==PR_U7D_047A_ARGUMENT);
    c.sp=0x8000;c.ip=0;CHECK(pr_u7d_update_047a_cpu(ram,sizeof ram,&c,10,NULL)==PR_U7D_047A_ARGUMENT);
    printf("U7D 047A: %u checks\n",checks);return 0;
}

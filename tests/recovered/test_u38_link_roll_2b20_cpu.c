#include "poolrad/recovered/overlay_u38_link_roll_2b20.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t ram[1u<<20],saved[1u<<20];
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static size_t at(unsigned s,unsigned o){return ((s<<4)+o)&((1u<<20)-1u);}
static void put(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&((1u<<20)-1)]=(uint8_t)(v>>8);}
static unsigned get(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&((1u<<20)-1)]<<8);}
static void seed_put(uint32_t seed){put(0x1800,0x4428,(uint16_t)seed);put(0x1800,0x442A,(uint16_t)(seed>>16));}
static uint32_t seed_get(void){return get(0x1800,0x4428)|((uint32_t)get(0x1800,0x442A)<<16);}
static unsigned roll(unsigned count,unsigned sides,uint32_t *seed)
{
    unsigned total=0;
    while(count--){*seed=*seed*UINT32_C(0x08088405)+1u;total=(total+1+(sides?(*seed>>16)%sides:0))&255u;}
    return total;
}
static pr_roll_chain_cpu fixture(unsigned latch,unsigned count,unsigned sides,unsigned kind,uint32_t seed)
{
    pr_roll_chain_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x5000,0x2B20,0x1800,0x3300,0x2800,0x8D7};
    memset(ram,0xA5,sizeof ram);
    if(latch){c.cs=0x6000;c.ip=0xE2E;}
    put(c.ds,0x5D92,0x1000);put(c.ds,0x5D94,0x3000);
    put(0x3000,0x10CC,0x2000);put(0x3000,0x10CE,0x3000);
    ram[at(0x3000,0x202E)]=(uint8_t)kind;
    ram[at(c.ds,0x6818)]=0xA7;ram[at(c.ds,0x681C)]=0x97;seed_put(seed);
    put(c.ss,c.sp,0x7000);put(c.ss,c.sp+2,0x9001);
    if(latch){put(c.ss,c.sp+4,0xA500|sides);put(c.ss,c.sp+6,0x5A00|count);}
    else for(unsigned i=0;i<5;i++)put(c.ss,c.sp+4+2*i,0xA100+i);
    ram[at(0x5000,0x2B52)]=0x9A;put(0x5000,0x2B53,0x4D);put(0x5000,0x2B55,0x7000);
    ram[at(0x7000,0x4D)]=0xEA;put(0x7000,0x4E,0xE2E);put(0x7000,0x50,0x6000);
    ram[at(0x6000,0xE43)]=0xE8;put(0x6000,0xE44,0xFF9D);put(0xAF8,0x1245,0x8405);
    ram[at(0x6000,0xE0E)]=0x9A;put(0x6000,0xE0F,0x11C0);put(0x6000,0xE11,0xAF8);   /* the 0E0E LCALL: its segment word is read live */
    return c;
}
static int run(unsigned latch,pr_roll_chain_cpu *c,unsigned budget,pr_roll_chain_diagnostic *d)
{return latch?pr_u9e_roll_latch_0e2e_cpu(ram,sizeof ram,c,budget,d):pr_u38_link_roll_2b20_cpu(ram,sizeof ram,c,budget,d);}
int main(void)
{
    pr_roll_chain_cpu c,before;pr_roll_chain_diagnostic d;
    for(unsigned kind=0;kind<256;kind++)for(unsigned v=0;v<3;v++){
        uint32_t seed=UINT32_C(0xFEDCBA98)*v+kind,want=seed;
        unsigned amount=kind==0x55?roll(1,6,&want)+1:0xA7;
        c=fixture(0,1,6,kind,seed);before=c;
        CHECK(run(0,&c,256,&d)==0);
        CHECK(c.cs==0x9001&&c.ip==0x7000&&c.sp==0x800E&&c.bp==before.bp);
        CHECK(c.si==before.si&&c.ds==before.ds&&c.ss==before.ss);
        CHECK(ram[at(c.ds,0x6818)]==amount&&seed_get()==want);
        CHECK(ram[at(c.ds,0x681C)]==(kind==0x55?1:0x97));
        CHECK(d.latch_status==(kind==0x55?0:-1)&&d.dice_status==(kind==0x55?0:-1));
        for(unsigned j=0;j<5;j++)CHECK(get(c.ss,0x8004+2*j)==0xA100+j);
        if(kind==0x55){CHECK(get(c.ss,0x7FF8)==0x3001&&get(c.ss,0x7FF6)==0x3006);CHECK(c.ax==amount);}
    }
    for(unsigned count=0;count<256;count++)for(unsigned v=0;v<4;v++){
        const unsigned ranges[]={0,1,6,255};unsigned sides=ranges[v];uint32_t seed=count*UINT32_C(0x01020304)+v,want=seed;
        unsigned value=roll(count,sides,&want);c=fixture(1,count,sides,0x55,seed);before=c;
        CHECK(run(1,&c,256,&d)==0);
        CHECK(c.cs==0x9001&&c.ip==0x7000&&c.sp==0x8008&&c.bp==before.bp);
        CHECK((c.ax&255)==value&&seed_get()==want);
        CHECK(ram[at(c.ds,0x681C)]==count&&ram[at(c.ds,0x6818)]==0xA7);
        CHECK(d.latch_status==-1&&d.dice_status==0);
    }
    /* The latch store aliases the range argument, which is read afterwards. */
    c=fixture(1,3,6,0x55,0);c.ss=c.ds;c.sp=0x6818;
    put(c.ss,c.sp,0x7000);put(c.ss,c.sp+2,0x9001);put(c.ss,c.sp+4,6);put(c.ss,c.sp+6,3);
    uint32_t seed=0;unsigned value=roll(3,3,&seed);
    CHECK(run(1,&c,256,&d)==0);CHECK((c.ax&255)==value&&seed_get()==seed);
    CHECK(get(c.ss,0x681C)==3);
    /* The null link skips even the zero-budget dice chain. */
    c=fixture(0,1,6,0x55,123);put(0x3000,0x10CC,0);put(0x3000,0x10CE,0);
    CHECK(run(0,&c,0,&d)==0);CHECK(d.latch_status==-1&&d.dice_status==-1);
    CHECK(seed_get()==123&&ram[at(c.ds,0x6818)]==0xA7&&c.ax==0&&c.dx==0);
    for(unsigned latch=0;latch<2;latch++){
        c=fixture(latch,3,6,0x55,123);
        CHECK(run(latch,&c,0,&d)==PR_ROLL_CHAIN_CALLEE);
        CHECK(c.cs==0x6000&&c.ip==0xE0E&&c.sp==(latch?0x7FEC:0x7FDE));
        CHECK(d.dice_status==PR_U9E_ENTRY8_COMPOSE_LIMIT&&d.latch_status==(latch?-1:PR_ROLL_CHAIN_CALLEE));
        CHECK(seed_get()==123&&ram[at(c.ds,0x681C)]==(latch?3:1));
        CHECK((c.flags&0x8D5u)==0x44u);
    }
    c=fixture(1,16,6,0x55,123);
    CHECK(run(1,&c,15,&d)==PR_ROLL_CHAIN_CALLEE);
    CHECK(d.dice_status==PR_U9E_ENTRY8_COMPOSE_LIMIT&&c.ip==0xE0E);
    CHECK((c.flags&0x8D5u)==0x44u); /* INC index 15->16 must not leak AF. */
    c=fixture(1,0,6,0x55,123);CHECK(run(1,&c,0,&d)==0);CHECK(seed_get()==123&&(c.ax&255)==0);
    for(unsigned site=0;site<6;site++){
        const unsigned segs[]={0x5000,0x5000,0x7000,0x7000,0x6000,0x6000};
        const unsigned offs[]={0x2B52,0x2B53,0x4D,0x4E,0xE43,0xE44};
        c=fixture(0,1,6,0x55,123);ram[at(segs[site],offs[site])]^=1;
        CHECK(run(0,&c,256,&d)==(site<4?PR_ROLL_CHAIN_CODE:PR_ROLL_CHAIN_CALLEE));
        CHECK(c.ip==(site<4?0x2B52:0xE43)&&c.sp==(site<4?0x7FF6:0x7FE8));
        CHECK(d.latch_status==(site<4?-1:PR_ROLL_CHAIN_CODE)&&d.dice_status==-1);
        CHECK(seed_get()==123&&ram[at(c.ds,0x6818)]==0xA7);
    }
    for(unsigned latch=0;latch<2;latch++)for(unsigned bad=0;bad<7;bad++){
        c=fixture(latch,1,6,0x55,123);if(bad==0)c.ip++;if(bad==1)c.sp=latch?0x1F:0x3F;if(bad==2)c.sp=0xFFFD;
        before=c;memcpy(saved,ram,sizeof ram);d.latch_status=d.dice_status=123;
        size_t size=bad==4?sizeof ram-1:bad==5?sizeof ram+1:sizeof ram;
        int s=latch?pr_u9e_roll_latch_0e2e_cpu(bad==3?NULL:ram,size,bad==6?NULL:&c,256,&d):
                    pr_u38_link_roll_2b20_cpu(bad==3?NULL:ram,size,bad==6?NULL:&c,256,&d);
        CHECK(s==PR_ROLL_CHAIN_ARGUMENT);CHECK(!memcmp(saved,ram,sizeof ram)&&!memcmp(&c,&before,sizeof c));
        CHECK(d.latch_status==123&&d.dice_status==123);
    }
    for(unsigned latch=0;latch<2;latch++){
        c=fixture(latch,3,6,0x55,123);CHECK(run(latch,&c,256,&d)==0);before=c;memcpy(saved,ram,sizeof ram);
        c=fixture(latch,3,6,0x55,123);CHECK(run(latch,&c,256,NULL)==0);CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(saved,ram,sizeof ram));
    }
    printf("U38 +2B20 / U9E +0E2E raw chain: %u checks\n",checks);return 0;
}

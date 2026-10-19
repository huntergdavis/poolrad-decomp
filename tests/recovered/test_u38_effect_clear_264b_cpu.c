#include "poolrad/recovered/overlay_u38_effect_clear_264b.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t ram[1u<<20], saved[1u<<20];
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static size_t at(unsigned s,unsigned o){return ((s<<4)+o)&((1u<<20)-1u);}
static void put(unsigned s,unsigned o,unsigned v)
{size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1u)&((1u<<20)-1u)]=(uint8_t)(v>>8);}
static unsigned get(unsigned s,unsigned o)
{size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1u)&((1u<<20)-1u)]<<8);}
static pr_target_cpu fixture(unsigned bits,unsigned amount,unsigned nil,unsigned null_alts)
{
    pr_target_cpu c={0xA5CD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x5000,0x264B,0x1800,0x3300,0x2800,0x8D7};
    memset(ram,0xA5,sizeof ram);
    put(0x3000,0x10CC,nil?0:0x2000);put(0x3000,0x10CE,nil?0:0x3000);
    put(0x3000,0x10F8,null_alts?0:0x3000);put(0x3000,0x10FA,null_alts?0:0x3000);
    put(0x3000,0x10FC,null_alts?0:0x4000);put(0x3000,0x10FE,null_alts?0:0x3000);
    ram[at(0x3000,0x202E)]=7;
    ram[at(c.ds,0x5600)]=(uint8_t)bits;
    ram[at(c.ds,0x6818)]=(uint8_t)amount;put(c.ds,0x5D92,0x1000);put(c.ds,0x5D94,0x3000);
    put(c.ss,c.sp,0x7000);put(c.ss,c.sp+2,0x9001);
    for(unsigned i=0;i<5;i++)put(c.ss,c.sp+4+2*i,0xA100+i);
    ram[at(c.cs,0x1045)]=0x9A;put(c.cs,0x1046,0x2E8B);put(c.cs,0x1048,0xBA);
    ram[at(c.cs,0x265A)]=0xE8;put(c.cs,0x265B,0xE9C0);
    return c;
}
static int run(pr_target_cpu *c,pr_target_diagnostic *d)
{return pr_u38_effect_clear_264b_cpu(ram,sizeof ram,c,d);}
int main(void)
{
    pr_target_cpu c,before;pr_target_diagnostic d;
    /* Distinct records make selector precedence observable in wrapper output. */
    for(unsigned bits=0;bits<256;bits++)for(unsigned variation=0;variation<8;variation++){
        unsigned nil=variation==1,null_alts=variation==2,off=0,amount=(bits*7+variation)&255;
        if(!nil){
            off=0x2000;
            if(!null_alts && (bits&9)==9)off=0x3000;
            if((bits&0x88)==0x88)off=null_alts?0x2000:0x4000;
        }
        c=fixture(bits,amount,nil,null_alts);before=c;d.selector_status=d.resident_status=123;
        for(unsigned j=0;j<3;j++){
            unsigned p=0x2000+0x1000*j;
            ram[at(0x3000,p+0x31)]=(uint8_t)((variation+j)%3==0?0xB1:0xB0);
            ram[at(0x3000,p+0x32)]=(uint8_t)((variation+j)%3==2?0x80:0);
        }
        unsigned keep=off && (ram[at(0x3000,off+0x32)] || ram[at(0x3000,off+0x31)]==0xB1);
        CHECK(run(&c,&d)==PR_TARGET_RETURNED);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800E && c.bp==before.bp);
        CHECK(c.bx==before.bx && c.si==before.si && c.ds==before.ds && c.ss==before.ss);
        CHECK(d.resident_status==(nil?-1:0) && d.selector_status==0);
        CHECK(ram[at(c.ds,0x6818)]==(keep?amount:0));
        CHECK(c.dx==(nil?0:0x3000) && c.ax==(nil?0:(off|0x3000)));
        if(!nil)CHECK(c.di==off && c.es==0x3000);
        for(unsigned j=0;j<5;j++)CHECK(get(c.ss,0x8004+2*j)==0xA100+j);
    }
    /* All values of each byte used by the two CMPs, including signed edges. */
    for(unsigned value=0;value<256;value++)for(unsigned field=0;field<2;field++){
        c=fixture(0x10,0xFF,0,0);
        ram[at(0x3000,0x2031)]=(uint8_t)(field?0:value);
        ram[at(0x3000,0x2032)]=(uint8_t)(field?value:0);
        CHECK(run(&c,NULL)==PR_TARGET_RETURNED);
        CHECK(ram[at(c.ds,0x6818)]==((field?value!=0:value==0xB1)?0xFF:0));
    }
    for(unsigned nested=0;nested<2;nested++)for(unsigned operand=0;operand<2;operand++){
        c=fixture(0x99,0x81,0,0);ram[at(c.cs,(nested?0x1045:0x265A)+operand)]^=1;
        CHECK(run(&c,&d)==(nested?PR_TARGET_CALLEE:PR_TARGET_CODE));
        CHECK(c.ip==(nested?0x1045:0x265A));
        CHECK(d.selector_status==(nested?PR_TARGET_CODE:-1) && d.resident_status==-1);
        CHECK(ram[at(c.ds,0x6818)]==0x81);
        if(!nested){CHECK(c.sp==0x7FF4 && c.bp==0x7FFE);CHECK(get(c.ss,c.sp)==0x5000);}
    }
    for(unsigned invalid=0;invalid<9;invalid++){
        c=fixture(0,0,0,0);d.selector_status=d.resident_status=123;
        if(invalid==0)c.ip++;
        if(invalid==1)c.sp=0x2D;
        if(invalid==2)c.sp=0xFFFD;
        before=c;memcpy(saved,ram,sizeof ram);
        CHECK(pr_u38_effect_clear_264b_cpu(invalid==3?NULL:ram,
              invalid==4?sizeof ram-1:invalid==5?sizeof ram+1:invalid==8?0:sizeof ram,
              invalid==6||invalid==7?NULL:&c,&d)==PR_TARGET_ARGUMENT);
        CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(saved,ram,sizeof ram));
        CHECK(d.selector_status==123 && d.resident_status==123);
    }
    c=fixture(0x99,0x81,0,0);CHECK(run(&c,&d)==PR_TARGET_RETURNED);
    before=c;memcpy(saved,ram,sizeof ram);c=fixture(0x99,0x81,0,0);
    CHECK(run(&c,NULL)==PR_TARGET_RETURNED);
    CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(saved,ram,sizeof ram));
    printf("U38 +264B raw chain: %u checks\n",checks);return 0;
}

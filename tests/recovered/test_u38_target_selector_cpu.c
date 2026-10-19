#include "poolrad/recovered/overlay_u38_target_mask81_25bc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t ram[1u<<20];static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static size_t at(unsigned s,unsigned o){return ((s<<4)+o)&((1u<<20)-1u);}
static void put(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1u)&((1u<<20)-1u)]=(uint8_t)(v>>8);}
static unsigned get(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1u)&((1u<<20)-1u)]<<8);}
static pr_target_cpu fixture(unsigned mode,unsigned bits,unsigned mask,unsigned amount,unsigned nil,unsigned null_alts)
{
    pr_target_cpu c={0xA5CD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x5000,0,0x1800,0x3300,0x2800,0x8D7};
    memset(ram,0xA5,sizeof ram);c.ip=(uint16_t)(mode==0?0x2E8B:mode==1?0x101D:0x25BC);if(!mode)c.cs=0xBA;
    put(0x3000,0x10CC,nil?0:0x2000);put(0x3000,0x10CE,nil?0:0x3000);
    put(0x3000,0x10F8,null_alts?0:0x3000);put(0x3000,0x10FA,null_alts?0:0x3000);
    put(0x3000,0x10FC,null_alts?0:0x4000);put(0x3000,0x10FE,null_alts?0:0x3000);
    ram[at(0x3000,0x202E)]=7;ram[at(0x3000,0x302E)]=9;ram[at(0x3000,0x402E)]=11;
    ram[at(c.ds,0x5600)]=(uint8_t)bits;ram[at(c.ds,0x55F9)]=(uint8_t)mask;ram[at(c.ds,0x5619)]=(uint8_t)mask;ram[at(c.ds,0x5639)]=(uint8_t)mask;
    ram[at(c.ds,0x6818)]=(uint8_t)amount;put(c.ds,0x5D92,0x1000);put(c.ds,0x5D94,0x3000);
    put(c.ss,c.sp,0x7000);put(c.ss,c.sp+2,0x9001);
    if(!mode){put(c.ss,c.sp+4,0x6000);put(c.ss,c.sp+6,c.ds);put(c.ss,c.sp+8,0x1000);put(c.ss,c.sp+10,0x3000);}
    else if(mode==1){put(c.ss,c.sp+4,0x1000);put(c.ss,c.sp+6,0x3000);}
    else for(unsigned i=0;i<5;i++)put(c.ss,c.sp+4+2*i,0xA100+i);
    ram[at(0x5000,0x1045)]=0x9A;put(0x5000,0x1046,0x2E8B);put(0x5000,0x1048,0xBA);
    ram[at(0x5000,0x25CB)]=0xE8;put(0x5000,0x25CC,0xEA4F);
    return c;
}
static int run(unsigned mode,pr_target_cpu *c,pr_target_diagnostic *d)
{return !mode?pr_res_target_select_2e8b_cpu(ram,sizeof ram,c):mode==1?pr_u38_target_select_101d_cpu(ram,sizeof ram,c,d):pr_u38_target_mask81_25bc_cpu(ram,sizeof ram,c,PR_TARGET_SHIFT_UNICORN,d);}
int main(void)
{
    pr_target_cpu c,before;pr_target_diagnostic d;
    for(unsigned mode=0;mode<3;mode++)for(unsigned bits=0;bits<256;bits++)for(unsigned variation=0;variation<8;variation++){
        unsigned nil=variation==1,null_alts=variation==2,mask=variation==3?0:variation==4?0x7E:variation==5?1:variation==6?0x80:0x81;
        unsigned amount=(bits*7+variation)&255,off=0,seg=0;
        if(!nil){if(bits&0x10){off=0x2000;seg=0x3000;}if((bits&9)==9){off=null_alts?0:0x3000;seg=null_alts?0:0x3000;}if((bits&0x88)==0x88){off=null_alts?0:0x4000;seg=null_alts?0:0x3000;}}
        c=fixture(mode,bits,mask,amount,nil,null_alts);before=c;d.selector_status=d.resident_status=123;
        CHECK(run(mode,&c,&d)==PR_TARGET_RETURNED);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8000u+(mode==0?12u:mode==1?8u:14u) && c.bp==before.bp);
        CHECK(c.bx==before.bx && c.si==before.si && c.ds==before.ds && c.ss==before.ss);
        if(!mode){CHECK(get(c.ds,0x6000)==off && get(c.ds,0x6002)==seg);CHECK(c.ax==(((off|seg)&0xFF00u)|((off|seg)!=0||(!nil&&bits==0xA))));}
        else {
            if(!nil && !(off|seg)){off=0x2000;seg=0x3000;}
            CHECK(d.resident_status==(nil?-1:0));
            CHECK(d.selector_status==(mode==1?-1:0));
            if(mode==1){CHECK(c.ax==off && c.dx==seg);CHECK(ram[at(c.ds,0x6818)]==amount);}
            else {unsigned want=(!nil&&(mask&0x81))?amount/2:amount;CHECK(ram[at(c.ds,0x6818)]==want);if(!nil&&(mask&0x81))CHECK(c.ax==want);}
        }
    }
    for(unsigned mode=1;mode<3;mode++){
        c=fixture(mode,0x99,0x81,255,0,0);ram[at(c.cs,0x1045)]=0x90;
        CHECK(run(mode,&c,&d)==(mode==1?PR_TARGET_CODE:PR_TARGET_CALLEE));
        CHECK(c.ip==0x1045 && d.resident_status==-1 && d.selector_status==(mode==1?-1:PR_TARGET_CODE));
        CHECK(ram[at(c.ds,0x6818)]==255);
    }
    c=fixture(2,0x99,0x81,255,0,0);ram[at(c.cs,0x25CB)]=0x90;
    CHECK(run(2,&c,&d)==PR_TARGET_CODE);CHECK(c.ip==0x25CB && d.selector_status==-1 && d.resident_status==-1);
    for(unsigned mode=0;mode<3;mode++){
        c=fixture(mode,0,0,0,0,0);before=c;d.selector_status=d.resident_status=123;c.sp=0;
        CHECK(run(mode,&c,&d)==PR_TARGET_ARGUMENT);CHECK(d.selector_status==123 && d.resident_status==123);c=before;c.ip++;
        CHECK(run(mode,&c,&d)==PR_TARGET_ARGUMENT);
    }
    CHECK(pr_res_target_select_2e8b_cpu(NULL,sizeof ram,&c)==PR_TARGET_ARGUMENT);
    CHECK(pr_u38_target_select_101d_cpu(ram,sizeof ram,NULL,NULL)==PR_TARGET_ARGUMENT);
    CHECK(pr_u38_target_mask81_25bc_cpu(ram,sizeof ram-1,&c,PR_TARGET_SHIFT_UNICORN,NULL)==PR_TARGET_ARGUMENT);
    for(unsigned amount=0;amount<256;amount++)for(unsigned take=0;take<2;take++){
        c=fixture(2,0x99,take?0x81:0,amount,0,0);
        CHECK(pr_u38_target_mask81_25bc_cpu(ram,sizeof ram,&c,PR_TARGET_SHIFT_DOSBOX386,&d)==PR_TARGET_RETURNED);
        CHECK((c.flags&0x10u)==(take?0x10u:0u));
        CHECK(ram[at(c.ds,0x6818)]==(take?amount/2:amount));
        before=c;c=fixture(2,0x99,take?0x81:0,amount,0,0);
        CHECK(run(2,&c,&d)==PR_TARGET_RETURNED);
        CHECK((uint16_t)(c.flags^(take?0x10u:0u))==before.flags);
    }
    c=fixture(2,0,0,0,0,0);before=c;d.selector_status=d.resident_status=123;
    CHECK(pr_u38_target_mask81_25bc_cpu(ram,sizeof ram,&c,(pr_target_shift_profile)2,&d)==PR_TARGET_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && d.selector_status==123 && d.resident_status==123);
    printf("Selector chain: %u checks\n",checks);return 0;
}

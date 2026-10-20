/* Synthetic heaps and instruction binding operands; no original game body. */
#include "poolrad/recovered/overlay_u3ce_bitmap_pair_release_014e_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_SYSTEM_FREEMEM_RAM_SIZE
#define CHECK(x) do {if(!(x)){fprintf(stderr,"014E line %d: %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
enum {DS=0x1800,SS=0x2800,CS=0x4000,RES=0x5000,SYS=0xAF8,SP=0x8000};
static uint8_t *ram,*backup;static unsigned checks;
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535))&(RAM-1);}
static void w(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1)]=(uint8_t)(v>>8);}
static unsigned r(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1)]<<8);}
static pr_u3ce_014e_cpu world(unsigned n,unsigned mode,int aux){
    pr_u3ce_014e_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x14E,DS,0x8888,SS,0xED7};
    const unsigned sites[]={0x25F0,0x2605,0x261B};
    memset(ram,0xA5,RAM);
    w(DS,0x4400,0);w(DS,0x4402,0x3000);w(DS,0x4404,0);w(DS,0x4406,0x3400);
    w(DS,0x440C,0);w(DS,0x440E,0x3400);w(DS,0x4410,0);w(DS,0x4412,0);
    w(DS,0x5E5A,0);w(DS,0x5E5C,0x3100);w(DS,0x5E5E,0);w(DS,0x5E60,0x3300);
    ram[at(DS,0x4A1E)]=(uint8_t)mode;
    ram[at(0x3100,8)]=1;w(0x3100,0x11,n);w(0x3100,0x13,0);w(0x3100,0x15,aux?0x3200:0);
    w(SS,SP,0x7000);w(SS,SP+2,0x9001);w(SS,SP+4,0xA500);
    for(unsigned i=0;i<3;++i){ram[at(RES,sites[i])]=0x9A;w(RES,sites[i]+1,0x254);w(RES,sites[i]+3,SYS);}
    ram[at(CS,0x17A)]=0x9A;w(CS,0x17B,0x2595);w(CS,0x17D,RES);
    ram[at(CS,0x1A5)]=0x9A;w(CS,0x1A6,0x2595);w(CS,0x1A8,RES);
    ram[at(0x3300,8)]=1;w(0x3300,0x11,16);w(0x3300,0x13,0);w(0x3300,0x15,0);
    return c;
}
int main(void){
    pr_u3ce_014e_cpu c,before;pr_u3ce_014e_diagnostic d;
    ram=malloc(RAM);backup=malloc(RAM);CHECK(ram && backup);
    for(unsigned index=0;index<256;++index)for(unsigned mask=0;mask<4;++mask){
        unsigned base=0x5E5A+index*8;c=world(16,0,0);w(SS,SP+4,0xA500+index);
        w(DS,base,0);w(DS,base+2,mask&1?0x3100:0);w(DS,base+4,0);w(DS,base+6,mask&2?0x3300:0);
        CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM,&c,RES,SYS,64,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+6 && c.bp==0x789A);
        CHECK(d.calls==(unsigned)((mask&1)!=0)+(unsigned)((mask&2)!=0));
        CHECK(d.call_site==(mask&2?0x1A5:mask&1?0x17A:0));
        CHECK(r(DS,base)==0 && r(DS,base+2)==0 && r(DS,base+4)==0 && r(DS,base+6)==0);
        CHECK(r(DS,0x440E)==(mask&1?0x3100:mask&2?0x3300:0x3400));
        if(mask==0)CHECK(d.resident_status==-1 && d.resident.freemem_status==-1 && d.resident.calls==0);
    }
    for(unsigned mode=0;mode<3;++mode){
        c=world(16,mode,1);CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM,&c,RES,SYS,64,&d)==0);
        CHECK(d.calls==2 && d.resident.calls==1 && r(DS,0x5E5C)==0 && r(DS,0x5E60)==0);
        CHECK(r(DS,0x440E)==0x3100 && r(0x3100,2)==0x3200 && r(0x3200,2)==0x3300);
    }
    c=world(16,0,1);w(0x3100,0x13,1);
    CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM,&c,RES,SYS,64,&d)==PR_U3CE_014E_CALLEE);
    CHECK(d.calls==1 && d.resident.freemem_status==PR_SYSTEM_FREEMEM_RUNERROR && c.cs==SYS && c.ip==0xE2);
    CHECK(r(DS,0x5E5C)==0x3100 && r(DS,0x5E60)==0x3300);
    c=world(16,0,0);w(DS,0x440E,0x3001);w(0x3001,0,0);w(0x3001,2,0x3001);w(0x3001,4,0);w(0x3001,6,1);
    CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM,&c,RES,SYS,0,&d)==PR_U3CE_014E_CALLEE);
    CHECK(d.calls==1 && d.resident.freemem_status==PR_SYSTEM_FREEMEM_LIMIT && c.ip==0x40F);
    for(unsigned site=0;site<2;++site)for(unsigned field=0;field<3;++field){
        const unsigned sites[]={0x17A,0x1A5};c=world(16,0,0);
        ram[at(CS,sites[site]+(field==0?0:field==1?1:3))]^=1;
        before=c;memcpy(backup,ram,RAM);memset(&d,0x55,sizeof d);
        CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM,&c,RES,SYS,64,&d)==PR_U3CE_014E_ARGUMENT);
        CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(ram,backup,RAM)==0 && d.resident_status==0x55555555);
    }
    c=world(16,0,0);before=c;memset(&d,0x55,sizeof d);
    CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(NULL,RAM,&c,RES,SYS,64,&d)==PR_U3CE_014E_ARGUMENT);
    CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM,NULL,RES,SYS,64,&d)==PR_U3CE_014E_ARGUMENT);
    CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM-1,&c,RES,SYS,64,&d)==PR_U3CE_014E_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && d.resident_status==0x55555555);
    c.sp=0x7F;CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM,&c,RES,SYS,64,&d)==PR_U3CE_014E_ARGUMENT);
    c.sp=0xFFFB;CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM,&c,RES,SYS,64,&d)==PR_U3CE_014E_ARGUMENT);
    c=world(16,0,0);CHECK(pr_u3ce_bitmap_pair_release_014e_cpu(ram,RAM,&c,RES,SYS,64,NULL)==0);
    free(backup);free(ram);printf("PASS: U3CE 014E %u checks\n",checks);return 0;
}

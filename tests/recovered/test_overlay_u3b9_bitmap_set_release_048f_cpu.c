/* Synthetic heaps and instruction binding operands; no original game body. */
#include "poolrad/recovered/overlay_u3b9_bitmap_set_release_048f_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_SYSTEM_FREEMEM_RAM_SIZE
#define CHECK(x) do {if(!(x)){fprintf(stderr,"048F line %d: %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
enum {DS=0x1800,SS=0x2800,CS=0x4000,RES=0x5000,SYS=0xAF8,SP=0x8000};
static uint8_t *ram,*backup;static unsigned checks;
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535))&(RAM-1);}
static void w(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1)]=(uint8_t)(v>>8);}
static unsigned r(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1)]<<8);}
static pr_u3b9_048f_cpu world(unsigned n,unsigned mode,int aux){
    pr_u3b9_048f_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x48F,DS,0x8888,SS,0xED7};
    const unsigned sites[]={0x25F0,0x2605,0x261B};
    memset(ram,0xA5,RAM);
    w(DS,0x4400,0);w(DS,0x4402,0x3000);w(DS,0x4404,0);w(DS,0x4406,0x3400);
    w(DS,0x440C,0);w(DS,0x440E,0x3400);w(DS,0x4410,0);w(DS,0x4412,0);
    for(unsigned i=0;i<1030;++i)ram[at(DS,0x6000+i)]=0;
    ram[at(DS,0x6000)]=1;w(DS,0x6006,0);w(DS,0x6008,0x3100);ram[at(DS,0x4A1E)]=(uint8_t)mode;
    ram[at(0x3100,8)]=1;w(0x3100,0x11,n);w(0x3100,0x13,0);w(0x3100,0x15,aux?0x3200:0);
    w(SS,SP,0x7000);w(SS,SP+2,0x9001);w(SS,SP+4,0x6000);w(SS,SP+6,DS);
    for(unsigned i=0;i<3;++i){ram[at(RES,sites[i])]=0x9A;w(RES,sites[i]+1,0x254);w(RES,sites[i]+3,SYS);}
    ram[at(CS,0x4CA)]=0x9A;w(CS,0x4CB,0x2595);w(CS,0x4CD,RES);
    ram[at(CS,0x4E3)]=0x9A;w(CS,0x4E4,0x16A8);w(CS,0x4E6,SYS);
    return c;
}
int main(void){
    pr_u3b9_048f_cpu c,before;pr_u3b9_048f_diagnostic d;
    ram=malloc(RAM);backup=malloc(RAM);CHECK(ram && backup);
    for(unsigned n=0;n<256;++n){
        c=world(16,0,0);ram[at(DS,0x6000)]=(uint8_t)n;w(DS,0x6008,0);
        CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,&c,RES,SYS,256,64,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+8 && c.bp==0x789A);
        CHECK(d.release_calls==(n<128?n:0) && d.fill_calls==1);
        for(unsigned i=0;i<66;++i)CHECK(ram[at(DS,0x6000+i)]==0);
        CHECK(ram[at(DS,0x6B0A)]==0 && ram[at(DS,0x6B1C)]==255);
        CHECK(ram[at(DS,0x70C7)]==(n>0 && n<128?n:0xA5));
        CHECK(r(DS,0x440E)==0x3400);
    }
    for(unsigned mode=0;mode<3;++mode){
        c=world(16,mode,1);CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,&c,RES,SYS,256,64,&d)==0);
        CHECK(d.release_calls==1 && d.resident.calls==2 && d.fill_calls==1);
        CHECK(r(DS,0x440E)==0x3100 && r(0x3100,2)==0x3200);
        CHECK(r(0x3200,4)==(mode==1?8:0) && r(0x3200,6)==(mode==1?0:1));
    }
    c=world(16,0,0);ram[at(DS,0x6000)]=2;
    CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,&c,RES,SYS,0,64,&d)==PR_U3B9_048F_LIMIT);
    CHECK(d.release_calls==0 && d.fill_calls==0 && c.cs==CS && c.ip==0x4CA && c.sp==SP-12);
    CHECK(r(DS,0x6008)==0x3100 && ram[at(DS,0x70C7)]==1);
    c=world(16,0,0);ram[at(DS,0x6000)]=2;
    CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,&c,RES,SYS,1,64,&d)==PR_U3B9_048F_LIMIT);
    CHECK(d.release_calls==1 && d.fill_calls==0 && r(DS,0x6008)==0 && ram[at(DS,0x70C7)]==2);
    c=world(16,0,1);w(0x3100,0x13,1);
    CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,&c,RES,SYS,256,64,&d)==PR_U3B9_048F_CALLEE);
    CHECK(d.resident.freemem_status==PR_SYSTEM_FREEMEM_RUNERROR && c.cs==SYS && c.ip==0xE2 && d.fill_calls==0);
    for(unsigned site=0;site<2;++site)for(unsigned field=0;field<3;++field){
        const unsigned sites[]={0x4CA,0x4E3};c=world(16,0,0);
        ram[at(CS,sites[site]+(field==0?0:field==1?1:3))]^=1;
        before=c;memcpy(backup,ram,RAM);memset(&d,0x55,sizeof d);
        CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,&c,RES,SYS,256,64,&d)==PR_U3B9_048F_ARGUMENT);
        CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(ram,backup,RAM)==0 && d.resident_status==0x55555555);
    }
    c=world(16,0,0);before=c;memset(&d,0x55,sizeof d);
    CHECK(pr_u3b9_bitmap_set_release_048f_cpu(NULL,RAM,&c,RES,SYS,256,64,&d)==PR_U3B9_048F_ARGUMENT);
    CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,NULL,RES,SYS,256,64,&d)==PR_U3B9_048F_ARGUMENT);
    CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM-1,&c,RES,SYS,256,64,&d)==PR_U3B9_048F_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && d.resident_status==0x55555555);
    c.sp=0x7F;CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,&c,RES,SYS,256,64,&d)==PR_U3B9_048F_ARGUMENT);
    c.sp=0xFFF9;CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,&c,RES,SYS,256,64,&d)==PR_U3B9_048F_ARGUMENT);
    c=world(16,0,0);CHECK(pr_u3b9_bitmap_set_release_048f_cpu(ram,RAM,&c,RES,SYS,256,64,NULL)==0);
    free(backup);free(ram);printf("PASS: U3B9 048F %u checks\n",checks);return 0;
}

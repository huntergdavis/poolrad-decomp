/* Synthetic heaps and instruction binding operands; no original game body. */
#include "poolrad/recovered/resident_0464_bitmap_release_2595_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_SYSTEM_FREEMEM_RAM_SIZE
#define CHECK(x) do {if(!(x)){fprintf(stderr,"2595 line %d: %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
enum {DS=0x1800,SS=0x2800,CS=0x5000,SYS=0xAF8,SP=0x8000};
static uint8_t *ram,*backup;static unsigned checks;
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535))&(RAM-1);}
static void w(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1)]=(uint8_t)(v>>8);}
static unsigned r(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1)]<<8);}
static pr_r0464_2595_cpu world(unsigned n,unsigned mode,int aux){
    pr_r0464_2595_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x2595,DS,0x8888,SS,0xED7};
    const unsigned sites[]={0x25F0,0x2605,0x261B};
    memset(ram,0xA5,RAM);
    w(DS,0x4400,0);w(DS,0x4402,0x3000);w(DS,0x4404,0);w(DS,0x4406,0x3400);
    w(DS,0x440C,0);w(DS,0x440E,0x3400);w(DS,0x4410,0);w(DS,0x4412,0);
    w(DS,0x6000,0);w(DS,0x6002,0x3100);ram[at(DS,0x4A1E)]=(uint8_t)mode;
    ram[at(0x3100,8)]=1;w(0x3100,0x11,n);w(0x3100,0x13,0);w(0x3100,0x15,aux?0x3200:0);
    w(SS,SP,0x7000);w(SS,SP+2,0x9001);w(SS,SP+4,0x6000);w(SS,SP+6,DS);
    for(unsigned i=0;i<3;++i){ram[at(CS,sites[i])]=0x9A;w(CS,sites[i]+1,0x254);w(CS,sites[i]+3,SYS);}
    return c;
}
int main(void){
    pr_r0464_2595_cpu c,before;pr_r0464_2595_diagnostic d,sentinel={77,88,99};
    ram=malloc(RAM);backup=malloc(RAM);CHECK(ram && backup);
    for(unsigned n=1;n<=128;++n)for(unsigned mode=0;mode<3;++mode){
        unsigned a=mode==1?n/4:n; c=world(n,mode,1);
        CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,64,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+8 && c.bp==0x789A);
        CHECK(d.calls==2 && d.call_site==0x261B && d.freemem_status==0);
        CHECK(r(DS,0x6000)==0 && r(DS,0x6002)==0);
        CHECK(r(DS,0x440E)==0x3100);
        CHECK(r(0x3100,4)==((n+23+7)&8) && r(0x3100,6)==((n+23+7)>>4));
        CHECK(r(0x3100,2)==(a?0x3200:0x3400));
        if(a)CHECK(r(0x3200,4)==((a+7)&8) && r(0x3200,6)==((a+7)>>4));
    }
    c=world(16,1,0);CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,64,&d)==0 && d.calls==1);
    c=world(16,1,1);w(DS,0x6002,0);CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,64,&d)==0 && d.calls==0 && d.freemem_status==-1);
    CHECK(r(DS,0x440E)==0x3400);
    c=world(16,0,1);w(0x3100,0x13,1);
    CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,64,&d)==PR_R0464_2595_CALLEE);
    CHECK(d.calls==1 && d.call_site==0x2605 && d.freemem_status==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(c.cs==SYS && c.ip==0xE2 && c.ax==204 && r(DS,0x6002)==0x3100);
    c=world(16,0,1);w(DS,0x440E,0x3100);w(0x3100,0,0);w(0x3100,2,0x3400);w(0x3100,4,0);w(0x3100,6,4);
    CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,64,&d)==PR_R0464_2595_CALLEE);
    CHECK(d.calls==2 && d.freemem_status==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(r(0x3100,2)==0x3200 && r(0x3200,2)==0x3400 && r(DS,0x6002)==0x3100);
    c=world(16,0,0);w(DS,0x440E,0x3001);w(0x3001,0,0);w(0x3001,2,0x3001);w(0x3001,4,0);w(0x3001,6,1);
    CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,0,&d)==PR_R0464_2595_CALLEE);
    CHECK(d.freemem_status==PR_SYSTEM_FREEMEM_LIMIT && c.ip==0x40F && r(DS,0x6002)==0x3100);
    for(unsigned site=0;site<3;++site)for(unsigned which=0;which<3;++which){
        const unsigned sites[]={0x25F0,0x2605,0x261B};
        c=world(16,0,0);ram[at(CS,sites[site]+(which==0?0:which==1?1:3))]^=1;
        before=c;d=sentinel;memcpy(backup,ram,RAM);
        CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,64,&d)==PR_R0464_2595_ARGUMENT);
        CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(ram,backup,RAM)==0);
        CHECK(d.freemem_status==77 && d.call_site==88 && d.calls==99);
    }
    c=world(16,0,0);before=c;d=sentinel;
    CHECK(pr_r0464_bitmap_release_2595_cpu(NULL,RAM,&c,SYS,64,&d)==PR_R0464_2595_ARGUMENT);
    CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,NULL,SYS,64,&d)==PR_R0464_2595_ARGUMENT);
    CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM-1,&c,SYS,64,&d)==PR_R0464_2595_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && d.freemem_status==77);
    c.sp=0x3F;CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,64,&d)==PR_R0464_2595_ARGUMENT);
    c.sp=0xFFF9;CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,64,&d)==PR_R0464_2595_ARGUMENT);
    c=world(16,0,0);CHECK(pr_r0464_bitmap_release_2595_cpu(ram,RAM,&c,SYS,64,NULL)==0);
    free(backup);free(ram);printf("PASS: resident2595 %u checks\n",checks);return 0;
}

/* Synthetic heap/stack inputs; no original code bytes. */
#include "poolrad/recovered/system_freemem_0254_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_SYSTEM_FREEMEM_RAM_SIZE
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FreeMem line %d: %s\n",__LINE__,#x);exit(1);}++checks; } while(0)
enum { DS=0x1800,SS=0x2800,SP=0x8000,SYS=0x0AF8 };
static uint8_t *ram;static unsigned checks;
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535u))&(RAM-1u);}
static void w(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1)]=(uint8_t)(v>>8);}
static unsigned r(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1)]<<8);}
static void node(unsigned seg,unsigned next,unsigned paras,unsigned granule){w(seg,0,0);w(seg,2,next);w(seg,4,granule);w(seg,6,paras);}
static pr_system_freemem_cpu world(unsigned size,unsigned seg,unsigned off){
    pr_system_freemem_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,SYS,0x254,DS,0x8888,SS,0xED7};
    memset(ram,0xA5,RAM);w(DS,0x4400,0);w(DS,0x4402,0x3000);w(DS,0x4404,0);w(DS,0x4406,0x3400);
    w(DS,0x440C,0);w(DS,0x440E,0x3400);w(DS,0x4410,0);w(DS,0x4412,0);
    w(SS,SP,0x7000);w(SS,SP+2,0x9001);w(SS,SP+4,size);w(SS,SP+6,off);w(SS,SP+8,seg);return c;
}
int main(void){
    pr_system_freemem_cpu c,before;ram=malloc(RAM);CHECK(ram!=NULL);
    for(unsigned n=1;n<=512;++n){
        c=world(n,0x3100,0);CHECK(pr_system_freemem_0254(ram,RAM,&c,0)==PR_SYSTEM_FREEMEM_RETURNED);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+10 && c.bp==0x789A);
        CHECK(r(DS,0x440E)==0x3100 && r(0x3100,2)==0x3400);
        CHECK(r(0x3100,4)==((n+7)&8) && r(0x3100,6)==((n+7)>>4));
        CHECK((c.flags&0x401)==0x400 && r(DS,0x4406)==0x3400);
    }
    c=world(0xFFFF,0x3100,0);CHECK(pr_system_freemem_0254(ram,RAM,&c,0)==0);
    CHECK(r(0x3100,4)==0 && r(0x3100,6)==0x1000);
    c=world(0,0,0xFFFF);CHECK(pr_system_freemem_0254(ram,RAM,&c,0)==0);
    CHECK(c.ax==0 && c.bx==0 && c.cx==0xFFFF && (c.flags&0x8D5)==0x44 && r(DS,0x440E)==0x3400);
    c=world(16,0x3100,1);CHECK(pr_system_freemem_0254(ram,RAM,&c,64)==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(c.ax==204 && c.cs==SYS && c.ip==0xE2 && c.sp==SP && c.bp==0x789A && (c.flags&1));
    CHECK(r(SS,SP)==0x7000 && r(DS,0x440E)==0x3400);
    c=world(9,0x3100,0);w(DS,0x440E,0x3100);node(0x3100,0x3400,5,8);
    CHECK(pr_system_freemem_0254(ram,RAM,&c,64)==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(r(0x3100,4)==0 && r(0x3100,6)==1 && r(0x3100,2)==0x3400);
    c=world(16,0x3100,0);w(DS,0x440E,0x30FF);node(0x30FF,0x3101,1,0);node(0x3101,0x3400,2,0);
    CHECK(pr_system_freemem_0254(ram,RAM,&c,64)==0);
    CHECK(r(DS,0x440E)==0x30FF && r(0x30FF,2)==0x3400 && r(0x30FF,6)==4 && r(0x30FF,4)==0);
    CHECK((c.flags&0x401)==0 && c.ds==DS);
    c=world(16,0x33FF,0);w(DS,0x440E,0x33FD);node(0x33FD,0x3400,2,0);
    CHECK(pr_system_freemem_0254(ram,RAM,&c,64)==0);
    CHECK(r(DS,0x4406)==0x33FD && r(DS,0x440E)==0x33FD);
    c=world(16,0x3100,0);w(DS,0x440E,0x3001);node(0x3001,0x3001,1,0);
    CHECK(pr_system_freemem_0254(ram,RAM,&c,0)==PR_SYSTEM_FREEMEM_LIMIT && c.ip==0x40F);
    CHECK(r(0x3100,6)==1 && c.sp==SP-4);
    c=world(16,0x3100,0);c.ss=0x3000;c.sp=0x1008;
    w(c.ss,c.sp,0x7000);w(c.ss,c.sp+2,0x9001);w(c.ss,c.sp+4,16);w(c.ss,c.sp+6,0);w(c.ss,c.sp+8,0x3100);
    CHECK(pr_system_freemem_0254(ram,RAM,&c,64)==PR_SYSTEM_FREEMEM_TRANSFER && c.ip==0 && c.sp==0x1006);
    c=world(16,0x3100,0);before=c;
    CHECK(pr_system_freemem_0254(NULL,RAM,&c,64)==PR_SYSTEM_FREEMEM_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_system_freemem_0254(ram,RAM,NULL,64)==PR_SYSTEM_FREEMEM_ARGUMENT);
    CHECK(pr_system_freemem_0254(ram,RAM-1,&c,64)==PR_SYSTEM_FREEMEM_ARGUMENT);
    c.sp=0x1F;CHECK(pr_system_freemem_0254(ram,RAM,&c,64)==PR_SYSTEM_FREEMEM_ARGUMENT);
    c.sp=0xFFF7;CHECK(pr_system_freemem_0254(ram,RAM,&c,64)==PR_SYSTEM_FREEMEM_ARGUMENT);
    c=before;c.ip=0;CHECK(pr_system_freemem_0254(ram,RAM,&c,64)==PR_SYSTEM_FREEMEM_ARGUMENT);
    free(ram);printf("PASS: SYSTEM FreeMem %u checks\n",checks);return 0;
}

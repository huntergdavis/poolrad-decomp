/* Synthetic caller/heap states; no original media bytes. */
#include "poolrad/recovered/overlay_u3b0_dispose_11f3_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM (1u<<20)
#define CHECK(x) do { if(!(x)){fprintf(stderr,"11F3 line %d: %s\n",__LINE__,#x);exit(1);}++checks; } while(0)
enum { DS=0x1800,SS=0x2800,SP=0x8000,CS=0x5000,SYS=0xAF8 };
static uint8_t *ram;static unsigned checks;
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535u))&(RAM-1u);}
static void w(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1u)&(RAM-1u)]=(uint8_t)(v>>8);}
static unsigned r(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1u)&(RAM-1u)]<<8);}
static void link(unsigned seg,unsigned next){w(seg,0x2A,0);w(seg,0x2C,next);}
static pr_u3b0_11f3_cpu world(unsigned first,unsigned top){
    pr_u3b0_11f3_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x11F3,DS,0x8888,SS,0xED7};
    memset(ram,0xA5,RAM);w(DS,0x4400,0);w(DS,0x4402,0x3000);w(DS,0x4404,0);w(DS,0x4406,top);
    w(DS,0x440C,0);w(DS,0x440E,top);w(DS,0x4410,0);w(DS,0x4412,0);
    w(SS,SP,0x7000);w(SS,SP+2,0x9001);w(SS,SP+4,0x6000);w(SS,SP+6,DS);
    w(DS,0x6000,0);w(DS,0x6002,first);link(first,0);
    /* Synthetic binding descriptor; C checks this live CALL site. */
    ram[at(CS,0x1238)]=0x9A;w(CS,0x1239,0x254);w(CS,0x123B,SYS);return c;
}
static int run(pr_u3b0_11f3_cpu *c,unsigned loops,unsigned heap,int *status){return pr_u3b0_dispose_11f3_cpu(ram,RAM,c,loops,heap,status);}
int main(void){
    pr_u3b0_11f3_cpu c,before;int inner;ram=malloc(RAM);CHECK(ram!=NULL);
    c=world(0,0x3400);CHECK(run(&c,0,0,&inner)==0 && inner==-1);
    CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+8 && c.bp==0x789A);
    CHECK(c.ax==0 && (c.flags&0x8D5)==0x44 && (c.flags&0x400));CHECK(r(DS,0x440E)==0x3400);
    for(unsigned count=1;count<=64;++count)for(unsigned reverse=0;reverse<2;++reverse){
        unsigned first=0x3100+3*(reverse?count-1:0);c=world(first,0x3100+3*count);
        for(unsigned j=0;j<count;++j){unsigned index=reverse?count-j-1:j;unsigned next=j+1==count?0:0x3100+3*(reverse?index-1:index+1);link(0x3100+3*index,next);}
        CHECK(run(&c,64,64,&inner)==0 && inner==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+8 && c.bp==0x789A);
        CHECK(r(DS,0x6000)==0 && r(DS,0x6002)==first); /* No explicit clear. */
        CHECK(r(DS,0x4404)==0 && r(DS,0x4406)==0x3100);
        CHECK(r(DS,0x440C)==0 && r(DS,0x440E)==0x3100);
        CHECK(c.ax==0 && (c.flags&0x8D5)==0x44);
    }
    c=world(0x3100,0x3400);link(0x3100,0x3200);link(0x3200,0);
    CHECK(run(&c,0,64,&inner)==PR_U3B0_11F3_LIMIT && inner==0);
    CHECK(c.cs==CS && c.ip==0x1209 && c.sp==SP-10 && c.bp==SP-2);
    CHECK(r(SS,SP-6)==0 && r(SS,SP-4)==0x3200);
    CHECK(r(0x3100,4)==0 && r(0x3100,6)==3 && r(DS,0x440E)==0x3100 && r(0x3200,6)==0xA5A5);
    CHECK(r(DS,0x6002)==0x3100);
    c=world(0x3100,0x3400);link(0x3100,0x3100);
    CHECK(run(&c,64,64,&inner)==PR_U3B0_11F3_CALLEE && inner==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(c.cs==SYS && c.ip==0xE2 && c.ax==204 && (c.flags&1) && c.sp==SP-20);
    CHECK(r(0x3100,6)==3 && r(SS,SP-20)==0x123D && r(SS,SP-18)==CS);
    c=world(0x3100,0x3400);w(DS,0x6000,1);w(0x3100,0x2B,0);w(0x3100,0x2D,0);
    CHECK(run(&c,64,64,&inner)==PR_U3B0_11F3_CALLEE && inner==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(r(0x3100,5)==0xA5A5 && c.ax==204);
    c=world(0x3100,0x3400);w(DS,0x440E,0x3001);w(0x3001,0,0);w(0x3001,2,0x3001);
    CHECK(run(&c,64,0,&inner)==PR_U3B0_11F3_CALLEE && inner==PR_SYSTEM_FREEMEM_LIMIT);
    CHECK(c.cs==SYS && c.ip==0x40F && r(0x3100,6)==3);
    c=world(0x3100,0x3400);ram[at(CS,0x1238)]=0x90;
    CHECK(run(&c,64,64,&inner)==PR_U3B0_11F3_CODE && inner==-1);
    CHECK(c.cs==CS && c.ip==0x1238 && c.sp==SP-16 && r(DS,0x440E)==0x3400 && r(0x3100,6)==0xA5A5);
    c=world(0,0x3400);ram[at(CS,0x1238)]=0x90;CHECK(run(&c,0,0,NULL)==0);
    c=world(0x3100,0x3400);c.ss=0x3000;c.sp=0x101C;
    w(c.ss,c.sp,0x7000);w(c.ss,c.sp+2,0x9001);w(c.ss,c.sp+4,0x6000);w(c.ss,c.sp+6,DS);
    CHECK(run(&c,64,64,&inner)==PR_U3B0_11F3_CALLEE && inner==PR_SYSTEM_FREEMEM_TRANSFER);
    CHECK(c.cs==SYS && c.ip==0 && r(0x3100,6)==3);
    c=world(0x3100,0x3400);c.ss=0x3000;c.sp=0x1014;
    w(c.ss,c.sp,0x7000);w(c.ss,c.sp+2,0x9001);w(c.ss,c.sp+4,0x6000);w(c.ss,c.sp+6,DS);
    CHECK(run(&c,64,64,&inner)==PR_U3B0_11F3_TRANSFER && inner==0);
    CHECK(c.cs==0x3400 && c.ip==0 && c.sp==0x100A);
    CHECK(r(0x3100,6)==3 && r(DS,0x440E)==0x3100);
    c=world(0x3100,0x3400);before=c;
    CHECK(pr_u3b0_dispose_11f3_cpu(NULL,RAM,&c,0,0,&inner)==PR_U3B0_11F3_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_u3b0_dispose_11f3_cpu(ram,RAM,NULL,0,0,&inner)==PR_U3B0_11F3_ARGUMENT);
    CHECK(pr_u3b0_dispose_11f3_cpu(ram,RAM-1,&c,0,0,&inner)==PR_U3B0_11F3_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    c.sp=0x33;CHECK(run(&c,0,0,&inner)==PR_U3B0_11F3_ARGUMENT);
    c.sp=0xFFF9;CHECK(run(&c,0,0,&inner)==PR_U3B0_11F3_ARGUMENT);
    c=before;c.ip=0;CHECK(run(&c,0,0,&inner)==PR_U3B0_11F3_ARGUMENT);
    free(ram);printf("PASS: U3B0 disposer %u checks\n",checks);return 0;
}

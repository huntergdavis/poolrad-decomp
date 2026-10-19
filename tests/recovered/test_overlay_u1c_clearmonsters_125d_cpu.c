/* Synthetic caller/heap states; no original media bytes. */
#include "poolrad/recovered/overlay_u1c_clearmonsters_125d_cpu.h"
#include "poolrad/recovered/overlay_u1c_clearmonsters_125d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM (1u<<20)
#define CHECK(x) do { if(!(x)){fprintf(stderr,"125D line %d: %s\n",__LINE__,#x);exit(1);}++checks; } while(0)
enum { DS=0x1800,SS=0x2800,SP=0x8000,CS=0x5000,SYS=0xAF8 };
static uint8_t *ram;static unsigned checks;
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535u))&(RAM-1u);}
static void w(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1u)&(RAM-1u)]=(uint8_t)(v>>8);}
static unsigned r(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1u)&(RAM-1u)]<<8);}
static void link(unsigned seg,unsigned next){w(seg,0x2A,0);w(seg,0x2C,next);}
static pr_u1c_cmon_cpu world(unsigned first,unsigned top){
    pr_u1c_cmon_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x125D,DS,0x8888,SS,0xED7};
    memset(ram,0xA5,RAM);w(DS,0x4400,0);w(DS,0x4402,0x3000);w(DS,0x4404,0);w(DS,0x4406,top);
    w(DS,0x440C,0);w(DS,0x440E,top);w(DS,0x4410,0);w(DS,0x4412,0);
    w(SS,SP,0x7000);w(SS,SP+2,0x9001);w(DS,0x49ED,0x1234);
    w(DS,0x6810,0);w(DS,0x6812,first);link(first,0);
    /* Synthetic call descriptors, not copied executable bodies. */
    ram[at(CS,0x1282)]=0x9A;w(CS,0x1283,0x16A8);w(CS,0x1285,SYS);
    ram[at(CS,0x12AE)]=0x9A;w(CS,0x12AF,0x254);w(CS,0x12B1,SYS);return c;
}
static int run(pr_u1c_cmon_cpu *c,unsigned loops,unsigned heap,pr_u1c_cmon_cpu_diagnostic *d){return pr_u1c_clearmonsters_125d_cpu(ram,RAM,c,loops,heap,d);}
int main(void){
    pr_u1c_cmon_cpu c,before;pr_u1c_cmon_cpu_diagnostic d;ram=malloc(RAM);CHECK(ram!=NULL);
    c=world(0,0x3400);before=c;
    for(unsigned pc=0;pc<=65535;pc+=257){
        c=before;w(DS,0x49ED,pc);memset(ram+at(DS,0x67F4),0xA5,28);
        CHECK(run(&c,0,0,&d)==0 && d.callee==0 && d.status==0);
        CHECK(r(DS,0x49ED)==((pc+1u)&65535u) && r(DS,0x6810)==0 && r(DS,0x6812)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+4 && c.bp==0x789A);
        CHECK((c.flags&0xCD5)==0x44 && r(DS,0x440E)==0x3400);
        CHECK(ram[at(DS,0x4435)]==0 && ram[at(DS,0x84E7)]==0 && ram[at(DS,0x6F83)]==8);
        for(unsigned j=0;j<28;++j)CHECK(ram[at(DS,0x67F4u+j)]==0);
        CHECK(ram[at(DS,0x67F3)]==0xA5 && ram[at(DS,0x6814)]==0xA5);
    }
    for(unsigned count=1;count<=64;++count)for(unsigned reverse=0;reverse<2;++reverse){
        unsigned first=0x3100+4*(reverse?count-1:0);c=world(first,0x3100+4*count);
        for(unsigned j=0;j<count;++j){unsigned index=reverse?count-j-1:j;link(0x3100+4*index,j+1==count?0:0x3100+4*(reverse?index-1:index+1));}
        CHECK(run(&c,64,64,&d)==0 && d.callee==1 && d.status==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+4 && c.bp==0x789A);
        CHECK(r(DS,0x6810)==0 && r(DS,0x6812)==0 && r(DS,0x49ED)==0x1235);
        CHECK(r(DS,0x4404)==0 && r(DS,0x4406)==0x3100 && r(DS,0x440E)==0x3100);
    }
    c=world(0x3100,0x3400);link(0x3100,0x3200);link(0x3200,0);
    CHECK(run(&c,0,64,&d)==PR_U1C_CMON_CPU_LIMIT && d.callee==1 && d.status==0);
    CHECK(c.cs==CS && c.ip==0x1287 && c.sp==SP-8 && r(DS,0x6812)==0x3200);
    CHECK(r(0x3100,4)==0 && r(0x3100,6)==4 && r(0x3200,6)==0xA5A5);
    c=world(0x3100,0x3400);link(0x3100,0x3100);
    CHECK(run(&c,64,64,&d)==PR_U1C_CMON_CPU_CALLEE && d.callee==1 && d.status==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(c.cs==SYS && c.ip==0xE2 && c.ax==204 && c.sp==SP-18 && r(DS,0x6812)==0x3100);
    c=world(0x3100,0x3400);w(DS,0x440E,0x3001);w(0x3001,0,0);w(0x3001,2,0x3001);
    CHECK(run(&c,64,0,&d)==PR_U1C_CMON_CPU_CALLEE && d.status==PR_SYSTEM_FREEMEM_LIMIT);
    CHECK(c.cs==SYS && c.ip==0x40F && r(0x3100,6)==4 && r(DS,0x6812)==0x3100);
    c=world(0x3100,0x3400);ram[at(CS,0x1282)]=0x90;w(DS,0x49ED,0x7FFF);
    CHECK(run(&c,64,64,&d)==PR_U1C_CMON_CPU_CODE && d.callee==-1 && d.status==-1);
    CHECK(c.ip==0x1282 && c.sp==SP-16 && r(DS,0x49ED)==0x8000 && (c.flags&0xCD5)==0xC94);
    CHECK(ram[at(DS,0x67F4)]==0xA5 && ram[at(DS,0x4435)]==0);
    c=world(0x3100,0x3400);ram[at(CS,0x12AE)]=0x90;
    CHECK(run(&c,64,64,&d)==PR_U1C_CMON_CPU_CODE && d.callee==0 && d.status==0);
    CHECK(c.ip==0x12AE && c.sp==SP-14 && ram[at(DS,0x67F4)]==0 && r(0x3100,6)==0xA5A5);
    c=world(0,0x3400);ram[at(CS,0x12AE)]=0x90;CHECK(run(&c,0,0,NULL)==0);
    c=world(0x3100,0x3400);c.ss=DS;c.sp=0x6808;w(c.ss,c.sp,0x7000);w(c.ss,c.sp+2,0x9001);
    CHECK(run(&c,64,64,&d)==PR_U1C_CMON_CPU_TRANSFER && d.callee==0 && d.status==0);
    CHECK(c.cs==0 && c.ip==0 && c.sp==0x6800 && !(c.flags&0x400));
    c=world(0x3100,0x3400);c.ss=0x3000;c.sp=0x101A;w(c.ss,c.sp,0x7000);w(c.ss,c.sp+2,0x9001);
    CHECK(run(&c,64,64,&d)==PR_U1C_CMON_CPU_CALLEE && d.callee==1 && d.status==PR_SYSTEM_FREEMEM_TRANSFER);
    CHECK(c.cs==SYS && c.ip==0 && r(0x3100,6)==4);
    c=world(0x3100,0x3400);c.ss=0x3000;c.sp=0x1012;w(c.ss,c.sp,0x7000);w(c.ss,c.sp+2,0x9001);
    CHECK(run(&c,64,64,&d)==PR_U1C_CMON_CPU_TRANSFER && d.callee==1 && d.status==0);
    CHECK(c.cs==0x3400 && c.ip==0 && c.sp==0x100A && r(DS,0x6812)==0x3100);
    c=world(0x3100,0x3400);before=c;
    CHECK(pr_u1c_clearmonsters_125d_cpu(NULL,RAM,&c,0,0,&d)==PR_U1C_CMON_CPU_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_u1c_clearmonsters_125d_cpu(ram,RAM,NULL,0,0,&d)==PR_U1C_CMON_CPU_ARGUMENT);
    CHECK(pr_u1c_clearmonsters_125d_cpu(ram,RAM-1,&c,0,0,&d)==PR_U1C_CMON_CPU_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    c.sp=0x31;CHECK(run(&c,0,0,&d)==PR_U1C_CMON_CPU_ARGUMENT);
    c.sp=0xFFFD;CHECK(run(&c,0,0,&d)==PR_U1C_CMON_CPU_ARGUMENT);
    c=before;c.ip=0;CHECK(run(&c,0,0,&d)==PR_U1C_CMON_CPU_ARGUMENT);
    free(ram);printf("PASS: U1C CLEARMONSTERS %u checks\n",checks);return 0;
}

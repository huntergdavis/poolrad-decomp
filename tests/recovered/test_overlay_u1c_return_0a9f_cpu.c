/* Synthetic caller/heap states; no original media bytes. */
#include "poolrad/recovered/overlay_u1c_return_0a9f_cpu.h"
#include "poolrad/recovered/overlay_u1c_return_0a9f.h" /* Both APIs coexist. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM (1u<<20)
#define CHECK(x) do { if(!(x)){fprintf(stderr,"0A9F line %d: %s\n",__LINE__,#x);exit(1);}++checks; } while(0)
enum { DS=0x1800,SS=0x2800,SP=0x8000,CS=0x5000,SYS=0xAF8 };
static uint8_t *ram;static unsigned checks;
static size_t at(unsigned s,unsigned o){return (((size_t)s<<4)+(o&65535u))&(RAM-1u);}
static void w(unsigned s,unsigned o,unsigned v){size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1u)&(RAM-1u)]=(uint8_t)(v>>8);}
static unsigned r(unsigned s,unsigned o){size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1u)&(RAM-1u)]<<8);}
static pr_u1c_return_cpu world(unsigned seg,unsigned off){
    pr_u1c_return_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0xA9F,DS,0x8888,SS,0xED7};
    memset(ram,0xA5,RAM);w(DS,0x4400,0);w(DS,0x4402,0x3000);w(DS,0x4404,0);w(DS,0x4406,0x3400);
    w(DS,0x440C,0);w(DS,0x440E,0x3400);w(DS,0x4410,0);w(DS,0x4412,0);
    w(seg,off,0xBEEF);w(seg,off+2,8);w(seg,off+4,0x3200);
    w(DS,0x49ED,0x1234);w(DS,0x6F70,off);w(DS,0x6F72,seg);
    w(SS,SP,0x7000);w(SS,SP+2,0x9001);
    /* Synthetic live call binding; no executable body bytes. */
    ram[at(CS,0xAD8)]=0x9A;w(CS,0xAD9,0x254);w(CS,0xADB,SYS);return c;
}
static int run(pr_u1c_return_cpu *c,unsigned heap,int *status){return pr_u1c_return_0a9f_cpu(ram,RAM,c,heap,status);}
int main(void){
    pr_u1c_return_cpu c,before;int inner;ram=malloc(RAM);CHECK(ram!=NULL);
    c=world(0,0);before=c;
    for(unsigned pc=0;pc<=65535;pc+=257){
        c=before;w(DS,0x49ED,pc);CHECK(run(&c,0,&inner)==0 && inner==-1);
        CHECK(r(DS,0x49ED)==((pc+1u)&65535u) && r(DS,0x6F70)==0 && r(DS,0x6F72)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+4 && c.bp==0x789A);
        CHECK((c.flags&0x8D5)==0x44 && (c.flags&0x400) && r(DS,0x440E)==0x3400);
    }
    for(unsigned n=0;n<128;++n){
        unsigned off=(n&1u)*8;c=world(0x3100,off);w(0x3100,off,n*511);w(0x3100,off+2,n*257);w(0x3100,off+4,0x4000+n);
        CHECK(run(&c,64,&inner)==0 && inner==0);
        CHECK(r(DS,0x49ED)==((n*511u)&65535u));
        CHECK(r(DS,0x6F70)==n*257 && r(DS,0x6F72)==0x4000+n);
        CHECK(c.ax==n*257 && c.dx==0x4000+n && c.bp==0x789A && c.sp==SP+4);
        CHECK(r(0x3100,off+4)==8 && r(0x3100,off+6)==0 && r(DS,0x440E)==0x3100 && r(DS,0x440C)==off);
    }
    c=world(0x1EF7,0);w(DS,0x4402,DS);
    CHECK(run(&c,64,&inner)==0 && inner==0);
    CHECK(r(DS,0x49ED)==0 && r(DS,0x6F74)==8 && r(DS,0x6F76)==0);
    CHECK(c.ax==8 && c.dx==0 && r(DS,0x6F70)==8 && r(DS,0x6F72)==0); /* Reload after heap writes. */
    c=world(0x33FF,8);CHECK(run(&c,64,&inner)==0);
    CHECK(r(DS,0x4404)==8 && r(DS,0x4406)==0x33FF && r(DS,0x6F72)==0x3200);
    c=world(0x3100,1);CHECK(run(&c,64,&inner)==PR_U1C_RETURN_CPU_CALLEE && inner==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(c.cs==SYS && c.ip==0xE2 && c.ax==204 && c.sp==SP-12 && c.bp==SP-2);
    CHECK(r(DS,0x49ED)==0xBEEF && r(DS,0x6F70)==1 && r(DS,0x6F72)==0x3100 && r(DS,0x6F74)==8 && r(DS,0x6F76)==0x3200);
    c=world(0x3100,0);w(DS,0x440E,0x3100);
    CHECK(run(&c,64,&inner)==PR_U1C_RETURN_CPU_CALLEE && inner==PR_SYSTEM_FREEMEM_RUNERROR);
    CHECK(r(0x3100,4)==8 && r(0x3100,6)==0 && r(DS,0x6F76)==0x3200);
    c=world(0x3100,0);w(DS,0x440E,0x3001);w(0x3001,0,0);w(0x3001,2,0x3001);
    CHECK(run(&c,0,&inner)==PR_U1C_RETURN_CPU_CALLEE && inner==PR_SYSTEM_FREEMEM_LIMIT);
    CHECK(c.cs==SYS && c.ip==0x40F && r(DS,0x6F72)==0x3100 && r(0x3100,4)==8);
    c=world(0x3100,0);ram[at(CS,0xAD8)]=0x90;
    CHECK(run(&c,64,&inner)==PR_U1C_RETURN_CPU_CODE && inner==-1);
    CHECK(c.cs==CS && c.ip==0xAD8 && c.sp==SP-8 && r(DS,0x440E)==0x3400 && r(DS,0x49ED)==0xBEEF);
    c=world(0,0);ram[at(CS,0xAD8)]=0x90;CHECK(run(&c,0,NULL)==0);
    c=world(0x3100,0);c.ss=0x3000;c.sp=0x1014;w(c.ss,c.sp,0x7000);w(c.ss,c.sp+2,0x9001);
    CHECK(run(&c,64,&inner)==PR_U1C_RETURN_CPU_CALLEE && inner==PR_SYSTEM_FREEMEM_TRANSFER);
    CHECK(c.cs==SYS && c.ip==8 && r(DS,0x6F72)==0x3100);
    c=world(0x3100,0);c.ss=0x3000;c.sp=0x100C;w(c.ss,c.sp,0x7000);w(c.ss,c.sp+2,0x9001);
    CHECK(run(&c,64,&inner)==PR_U1C_RETURN_CPU_TRANSFER && inner==0);
    CHECK(c.cs==0x3400 && c.ip==0 && c.sp==0x100A && r(DS,0x6F72)==0x3100);
    c=world(0x3100,0);before=c;
    CHECK(pr_u1c_return_0a9f_cpu(NULL,RAM,&c,0,&inner)==PR_U1C_RETURN_CPU_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_u1c_return_0a9f_cpu(ram,RAM,NULL,0,&inner)==PR_U1C_RETURN_CPU_ARGUMENT);
    CHECK(pr_u1c_return_0a9f_cpu(ram,RAM-1,&c,0,&inner)==PR_U1C_RETURN_CPU_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    c.sp=0x2B;CHECK(run(&c,0,&inner)==PR_U1C_RETURN_CPU_ARGUMENT);
    c.sp=0xFFFD;CHECK(run(&c,0,&inner)==PR_U1C_RETURN_CPU_ARGUMENT);
    c=before;c.ip=0;CHECK(run(&c,0,&inner)==PR_U1C_RETURN_CPU_ARGUMENT);
    free(ram);printf("PASS: U1C RETURN %u checks\n",checks);return 0;
}

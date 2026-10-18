/* Synthetic ECL operands and heap; no original game bytes. */
#include "poolrad/recovered/overlay_u1c_gosub_0111_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U1C_GOSUB_RAM_SIZE
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"GOSUB +0111 line %d: %s\n",__LINE__,#x);exit(1); } ++checks; } while(0)
enum { CS=0x8800,U25=0x7800,STUB=0x002B,SYS=0x0AF8,DS=0x1800,SS=0x2800,SP=0x8000,HEAP=0x3400 };
static uint8_t *ram;
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void w8(unsigned s,unsigned o,unsigned v) { ram[at(s,o)]=(uint8_t)v; }
static void w16(unsigned s,unsigned o,unsigned v) { size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1u)&(RAM-1u)]=(uint8_t)(v>>8); }
static unsigned r16(unsigned s,unsigned o) { size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1u)&(RAM-1u)]<<8); }
static void binding(unsigned site,unsigned off,unsigned target) {
    w8(CS,site,0x9A);w16(CS,site+1,off);w16(CS,site+3,STUB);
    w8(STUB,off,0xEA);w16(STUB,off+1,target);w16(STUB,off+3,U25);
}
static pr_u1c_gosub_cpu world(unsigned tag,unsigned low,unsigned high,unsigned old) {
    pr_u1c_gosub_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x111,DS,0x8888,SS,0x0603};
    unsigned j;
    memset(ram,0xA5,RAM);binding(0x117,0x2A,0);binding(0x11F,0x75,0x1951);
    for(j=0;j<2;++j) { unsigned site=j?0x198F:0x1961;w8(U25,site,0x9A);w16(U25,site+1,0x23F);w16(U25,site+3,SYS); }
    w16(DS,0x49DE,0x1000);w16(DS,0x49E0,0x6300);w16(DS,0x49ED,0x9900);
    w8(0x6300,0x1001,tag);w8(0x6300,0x1002,low);w8(0x6300,0x1003,high);
    w16(DS,0x6F70,old?0x100:0);w16(DS,0x6F72,old?0x3380:0);
    w16(DS,0x4404,0);w16(DS,0x4406,HEAP);w16(DS,0x4408,0);w16(DS,0x440A,HEAP+0x100);
    w16(DS,0x440C,0);w16(DS,0x440E,HEAP);w16(DS,0x4414,0xA9);w16(DS,0x4416,SYS);
    w16(SS,SP,0x7000);w16(SS,SP+2,0x9001);return c;
}
static unsigned flags(unsigned value) {
    unsigned n=value&255u,ones=0,f=0;
    for(unsigned j=0;j<8;++j)ones+=(n>>j)&1u;
    if(!(ones&1u))f|=4u;
    if(value==0u)f|=0x40u;
    if(value&0x8000u)f|=0x80u;
    return f;
}
int main(void) {
    const unsigned tags[]={0,1,2,3,0x55};
    pr_u1c_gosub_cpu c,before;pr_u1c_gosub_diagnostic d;
    pr_u1c_gosub_limits limits={SYS,300,4000,64},tight;
    unsigned n,tag,low,high,target,old;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(n=0;n<256;++n) {
        tag=tags[n%5];low=(n*37)&255;high=(n*11)&255;old=n%2;
        target=((tag>=1 && tag<=3?high:0xA5)<<8)|low;c=world(tag,low,high,old);
        CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_GOSUB_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+4 && c.bp==0x789A);
        CHECK(c.ax==target && c.dx==low && r16(DS,0x49ED)==target);
        CHECK(r16(DS,0x6F70)==0 && r16(DS,0x6F72)==HEAP);
        CHECK(r16(HEAP,0)==(tag>=1 && tag<=3?0x9904u:0x9903u));
        CHECK(r16(HEAP,2)==(old?0x100u:0u) && r16(HEAP,4)==(old?0x3380u:0u));
        CHECK((c.flags&0x8D5u)==flags(target) && (c.flags&~0x8D5u)==(0x603u&~0x8D5u));
        CHECK(d.callee==PR_U1C_GOSUB_ALLOC && d.status==0 && d.nested_callee==1 && d.nested_status==0);
    }
    c=world(2,0,0,0);CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,NULL)==PR_U1C_GOSUB_OK && c.ax==0 && (c.flags&0x44)==0x44);
    c=world(2,3,4,1);w16(DS,0x440A,HEAP);
    CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_GOSUB_CALLEE);
    CHECK(d.callee==PR_U1C_GOSUB_ALLOC && d.status==3 && d.nested_callee==0 && d.nested_status==2);
    CHECK(c.cs==SYS && c.ip==0xE2 && c.ax==203 && r16(DS,0x49ED)==0x9904 && r16(DS,0x6F72)==0x3380);
    c=world(2,3,4,0);w16(DS,0x4414,0x1234);
    CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_GOSUB_CALLEE && d.nested_status==3);
    c=world(2,3,4,0);w8(CS,0x117,0x90);
    CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_GOSUB_CODE);
    CHECK(c.ip==0x117 && c.flags==0x603 && r16(SS,c.sp)==0x1201 && d.callee==-1);
    c=world(2,3,4,0);w16(STUB,0x76,0x1952);
    CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_GOSUB_CODE && c.ip==0x11F && d.callee==0);
    c=world(2,3,4,0);c.ss=DS;c.sp=0x708E;w16(DS,c.sp,0x7000);w16(DS,c.sp+2,0x9001);
    CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_GOSUB_TRANSFER && c.ip==0x0103);
    c=world(2,3,4,0);before=c;
    CHECK(pr_u1c_gosub_0111_cpu(NULL,RAM,&c,&limits,&d)==PR_U1C_GOSUB_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && d.callee==-1 && d.status==-1);
    CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,NULL,&limits,NULL)==PR_U1C_GOSUB_ARGUMENT);
    CHECK(pr_u1c_gosub_0111_cpu(ram,RAM-1,&c,&limits,NULL)==PR_U1C_GOSUB_ARGUMENT);
    CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,NULL,NULL)==PR_U1C_GOSUB_ARGUMENT);
    tight=limits;tight.max_operands=0;CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&tight,NULL)==PR_U1C_GOSUB_ARGUMENT);
    tight=limits;tight.max_string_iterations=0;CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&tight,NULL)==PR_U1C_GOSUB_ARGUMENT);
    c.sp=0x231;CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,NULL)==PR_U1C_GOSUB_ARGUMENT);
    c.sp=0xFFFD;CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,NULL)==PR_U1C_GOSUB_ARGUMENT);
    c=before;c.ip=0;CHECK(pr_u1c_gosub_0111_cpu(ram,RAM,&c,&limits,NULL)==PR_U1C_GOSUB_ARGUMENT);
    free(ram);printf("PASS: GOSUB +0111 %u checks\n",checks);return 0;
}

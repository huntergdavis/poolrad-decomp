/* Synthetic operands, bindings and heap only; no original game bytes. */
#include "poolrad/recovered/overlay_u1c_on_goto_18f8_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U1C_18F8_RAM_SIZE
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"U1C +18F8 line %d: %s\n",__LINE__,#x);exit(1); } ++checks; } while(0)
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
static pr_u1c_18f8_cpu world(unsigned selector,unsigned count,unsigned opcode,unsigned old) {
    pr_u1c_18f8_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x18F8,DS,0x8888,SS,0x0603};
    unsigned j,p=0x1001;
    memset(ram,0xA5,RAM);
    binding(0x1905,0x2A,0);binding(0x190D,0x25,0x136);binding(0x1918,0x25,0x136);
    binding(0x1928,0x2A,0);binding(0x1968,0x4D,0x6AF);binding(0x1979,0x75,0x1951);
    for(j=0;j<2;++j) { unsigned site=j?0x198F:0x1961;w8(U25,site,0x9A);w16(U25,site+1,0x23F);w16(U25,site+3,SYS); }
    w16(DS,0x49DE,0x1000);w16(DS,0x49E0,0x6300);w16(DS,0x49ED,0x9900);
    w8(0x6300,p++,0);w8(0x6300,p++,selector);w8(0x6300,p++,0);w8(0x6300,p++,count);
    for(j=0;j<count;++j) { w8(0x6300,p++,2);w8(0x6300,p++,j+0x40);w8(0x6300,p++,0x90); }
    w8(DS,0x6F7F,opcode);w16(DS,0x6F70,old?0x100:0);w16(DS,0x6F72,old?0x3380:0);
    w16(DS,0x4404,0);w16(DS,0x4406,HEAP);w16(DS,0x4408,0);w16(DS,0x440A,HEAP+0x100);
    w16(DS,0x440C,0);w16(DS,0x440E,HEAP);w16(DS,0x4414,0xA9);w16(DS,0x4416,SYS);
    w16(SS,SP,0x7000);w16(SS,SP+2,0x9001);return c;
}
int main(void) {
    pr_u1c_18f8_cpu c,before;pr_u1c_18f8_diagnostic d;
    pr_u1c_18f8_limits limits={SYS,300,4000,300,64},tight;
    unsigned selector,mode,old,count;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(count=0;count<=7;++count) for(selector=0;selector<=9;++selector) for(mode=0;mode<2;++mode) for(old=0;old<2;++old) {
        unsigned jumped=selector<count,after=0x9905+3*count;
        c=world(selector,count,mode?0x26:0x25,old);
        CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_18F8_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+4 && c.bp==0x789A);
        CHECK(r16(DS,0x49ED)==(jumped?0x9040+selector:after));
        if(jumped && mode) {
            CHECK(r16(DS,0x6F70)==0 && r16(DS,0x6F72)==HEAP);
            CHECK(r16(HEAP,0)==after && r16(HEAP,2)==(old?0x100u:0u) && r16(HEAP,4)==(old?0x3380u:0u));
            CHECK(d.callee==PR_U1C_18F8_ALLOC && d.nested_callee==1 && d.nested_status==0);
        } else CHECK(r16(DS,0x6F70)==(old?0x100u:0u) && r16(DS,0x6F72)==(old?0x3380u:0u));
    }
    c=world(0,3,0x26,1);w16(DS,0x440A,HEAP);
    CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_18F8_CALLEE);
    CHECK(d.callee==PR_U1C_18F8_ALLOC && d.status==3 && d.nested_callee==0 && d.nested_status==2);
    CHECK(c.cs==SYS && c.ip==0xE2 && c.ax==203 && r16(DS,0x6F72)==0x3380);
    c=world(0,3,0x25,0);tight=limits;tight.max_operands=1;
    CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,&tight,&d)==PR_U1C_18F8_CALLEE);
    CHECK(c.cs==U25 && c.ip==0x22 && d.callee==PR_U1C_18F8_LOAD && d.status==2);
    c=world(2,3,0x25,0);tight=limits;tight.max_caller_iterations=1;
    CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,&tight,&d)==PR_U1C_18F8_CALLER_LIMIT);
    CHECK(c.cs==CS && c.ip==0x1931 && ram[at(SS,SP-6)]==1);
    c=world(0,3,0x25,0);w8(CS,0x1968,0x90);
    CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_18F8_CODE && c.ip==0x1968);
    c=world(0,3,0x25,0);w16(STUB,0x2B,1);
    CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_18F8_CODE && d.callee==-1);
    c=world(0,3,0x25,0);c.ss=DS;c.sp=0x7094;w16(DS,c.sp,0x7000);w16(DS,c.sp+2,0x9001);
    CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,&limits,&d)==PR_U1C_18F8_TRANSFER);
    /* Selector 0 and count 3 overwrite both bytes of the loader return IP. */
    CHECK(c.cs==CS && c.ip==0x0300);
    c=world(0,3,0x25,0);before=c;
    CHECK(pr_u1c_on_goto_18f8_cpu(NULL,RAM,&c,&limits,&d)==PR_U1C_18F8_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && d.callee==-1);
    CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,NULL,&limits,NULL)==PR_U1C_18F8_ARGUMENT);
    CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM-1,&c,&limits,NULL)==PR_U1C_18F8_ARGUMENT);
    CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,NULL,NULL)==PR_U1C_18F8_ARGUMENT);
    for(selector=0;selector<3;++selector) {
        c=before;if(selector==0)c.ip=0;else c.sp=selector==1?0x237:0xFFFD;
        CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,&limits,NULL)==PR_U1C_18F8_ARGUMENT);
    }
    c=world(0,3,0x25,0);CHECK(pr_u1c_on_goto_18f8_cpu(ram,RAM,&c,&limits,NULL)==PR_U1C_18F8_OK);
    free(ram);printf("PASS: U1C +18F8 %u checks\n",checks);return 0;
}

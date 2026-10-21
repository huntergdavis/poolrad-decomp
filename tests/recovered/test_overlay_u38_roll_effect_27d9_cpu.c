/* Synthetic heap/records and binding operands; no original instruction body. */
#include "poolrad/recovered/overlay_u38_roll_effect_27d9_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U38_27D9_RAM_SIZE
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"U38 +27D9 line %d: %s\n",__LINE__,#x);exit(1); } ++checks; } while(0)
enum { CS=0x8800,DICE=0x7800,STUB=0xB0,DS=0x1800,SS=0x2800,SP=0x8000,SYS=0x0AF8,REC=0x5000,HEAP=0x3400 };
static uint8_t *ram;
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void w8(unsigned s,unsigned o,unsigned v) { ram[at(s,o)]=(uint8_t)v; }
static void w16(unsigned s,unsigned o,unsigned v) { size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8); }
static unsigned r16(unsigned s,unsigned o) { size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1u)]<<8); }
static void binding(unsigned site,unsigned off,unsigned target) {
    w8(CS,site,0x9A);w16(CS,site+1,off);w16(CS,site+3,STUB);
    w8(STUB,off,0xEA);w16(STUB,off+1,target);w16(STUB,off+3,DICE);
}
static pr_u38_27d9_cpu world(uint32_t seed,unsigned old) {
    pr_u38_27d9_cpu c={0xA534,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x27D9,DS,0x8888,SS,0x0603};
    memset(ram,0xA5,RAM);binding(0x27FD,0x48,0xDE3);binding(0x280B,0x52,0xE52);
    w8(DICE,0xE5C,0x9A);w16(DICE,0xE5D,0x23F);w16(DICE,0xE5F,SYS);w16(SYS,0x1245,0x8405);w8(DICE,0xE0E,0x9A);w16(DICE,0xE0F,0x11C0);w16(DICE,0xE11,SYS);
    w8(DS,0x6819,0);w16(DS,0x4428,seed&65535u);w16(DS,0x442A,seed>>16);
    w16(DS,0x4404,0);w16(DS,0x4406,HEAP);w16(DS,0x4408,0);w16(DS,0x440A,HEAP+0x100);
    w16(DS,0x440C,0);w16(DS,0x440E,HEAP);w16(DS,0x4414,0xA9);w16(DS,0x4416,SYS);
    w16(REC,0x17F,0);w16(REC,0x181,old?0x5200:0);w16(0x5200,5,0);w16(0x5200,7,0);
    w16(SS,SP,0x7000);w16(SS,SP+2,0x9001);
    w16(SS,SP+4,0x1111);w16(SS,SP+6,0x2222);w16(SS,SP+8,0x3333);w16(SS,SP+10,0x100);w16(SS,SP+12,REC);
    return c;
}
int main(void) {
    pr_u38_27d9_limits limits={256,64,64},tight;pr_u38_27d9_cpu c,before;pr_u38_27d9_diagnostic d;
    unsigned seen=0;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(unsigned n=0;n<400;++n) {
        uint32_t seed=n*0x01000193u,next=seed;unsigned roll=0,old=n%2;
        for(unsigned j=0;j<3;++j) { next=next*0x08088405u+1u;roll+=((next>>16)%6u)+1u; }
        seen|=1u<<roll;
        c=world(seed,old);CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,&d)==PR_U38_27D9_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+14 && c.bp==0x789A);
        CHECK(r16(DS,0x4428)==(next&65535u) && r16(DS,0x442A)==(next>>16));
        CHECK((c.flags&0x0600u)==0x0600u);
        CHECK(d.callee==1 && d.status==0 && d.getmem_status==0);
        CHECK(ram[at(HEAP,0)]==0x66 && r16(HEAP,1)==roll && ram[at(HEAP,3)]==0xFF && ram[at(HEAP,4)]==1);
        CHECK(r16(HEAP,5)==0 && r16(HEAP,7)==0 && (c.flags&0x8D5u)==0x44 && c.ax==0xFF);
        if(old)CHECK(r16(REC,0x181)==0x5200 && r16(0x5200,5)==0 && r16(0x5200,7)==HEAP);
        else CHECK(r16(REC,0x17F)==0 && r16(REC,0x181)==HEAP);
    }
    CHECK(seen==0x7FFF8u);
    for(unsigned gate=0;gate<256;++gate) {
        c=world(0x12345678u,0);w8(DS,0x6819,gate);before=c;
        CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,&d)==PR_U38_27D9_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+14 && c.bp==before.bp);
        if(gate&0x11u) {
            CHECK(c.ax==(0xA500u|((gate&1u)?1u:16u)) && (c.flags&0x8D5u)==0);
            CHECK(c.bx==before.bx && c.cx==before.cx && c.dx==before.dx && c.si==before.si && c.di==before.di && c.es==before.es);
            CHECK(d.callee==-1 && d.status==-1 && d.getmem_status==-1);
            CHECK(r16(DS,0x4428)==0x5678 && r16(DS,0x442A)==0x1234 && r16(DS,0x4406)==HEAP && r16(DS,0x4404)==0);
        } else CHECK(d.callee==1 && ram[at(HEAP,0)]==0x66);
    }
    for(unsigned cap=0;cap<3;++cap) {
        uint32_t next=0;
        for(unsigned j=0;j<cap;++j)next=next*0x08088405u+1u;
        c=world(0,0);tight=limits;tight.max_system_calls=cap;
        CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&tight,&d)==PR_U38_27D9_CALLEE && d.callee==0 && d.status==11 && c.ip==0xE0E);
        CHECK(r16(DS,0x4428)==(next&65535u) && r16(DS,0x442A)==(next>>16));
        CHECK(r16(REC,0x181)==0 && r16(DS,0x4404)==0);
    }
    c=world(0,0);tight=limits;tight.max_system_calls=0;
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&tight,&d)==PR_U38_27D9_CALLEE && d.callee==0 && d.status==11 && c.ip==0xE0E);
    CHECK(ram[at(SS,SP-7)]==0xA5 && ram[at(SS,SP-9)]==0xA5 && r16(DS,0x4428)==0);
    c=world(0,0);w16(DS,0x440A,HEAP);
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,&d)==PR_U38_27D9_CALLEE && d.callee==1 && d.status==3 && d.getmem_status==2 && c.ip==0xE2 && c.ax==203);
    CHECK(r16(REC,0x181)==0 && r16(DS,0x4428)==0xAC1F);
    c=world(0,1);w16(0x5200,7,0x5200);tight=limits;tight.max_links=0;
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&tight,&d)==PR_U38_27D9_CALLEE && d.status==5 && d.getmem_status==0 && c.ip==0xEA8);
    c=world(0,0);w8(CS,0x27FD,0x90);
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,&d)==PR_U38_27D9_CODE && c.ip==0x27FD && c.sp==SP-12 && d.callee==-1);
    c=world(0,0);w8(STUB,0x52,0);
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,&d)==PR_U38_27D9_CODE && c.ip==0x280B && d.callee==0 && r16(DS,0x4428)==0xAC1F);
    c=world(0,0);w8(DS,0x6819,1);w8(CS,0x27FD,0);w8(STUB,0x52,0);
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,&d)==PR_U38_27D9_OK && d.callee==-1);
    c=world(0,0);w16(DS,0x4414,0x1234);w16(DS,0x4416,0x5678);
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,&d)==PR_U38_27D9_CALLEE && d.getmem_status==3);
    c=world(0,1);w8(DICE,0xE5C,0x90);
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,&d)==PR_U38_27D9_CALLEE && d.status==2 && d.getmem_status==-1);
    c=world(0,0);before=c;
    CHECK(pr_u38_roll_effect_27d9_cpu(NULL,RAM,&c,&limits,&d)==PR_U38_27D9_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && d.callee==-1);
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,NULL,&limits,NULL)==PR_U38_27D9_ARGUMENT);
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM-1,&c,&limits,NULL)==PR_U38_27D9_ARGUMENT);
    CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,NULL,NULL)==PR_U38_27D9_ARGUMENT);
    c.sp=0x3F;CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,NULL)==PR_U38_27D9_ARGUMENT);
    c.sp=0xFFF3;CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,NULL)==PR_U38_27D9_ARGUMENT);
    c=before;c.ip=0;CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,NULL)==PR_U38_27D9_ARGUMENT);
    c=world(0,0);CHECK(pr_u38_roll_effect_27d9_cpu(ram,RAM,&c,&limits,NULL)==PR_U38_27D9_OK);
    free(ram);printf("PASS: U38 +27D9 %u checks\n",checks);return 0;
}

/* Synthetic binding operands, heap and lists; no original instruction body. */
#include "poolrad/recovered/overlay_u38_sidecar_1472_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U38_1472_RAM_SIZE
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"U38 +1472 line %d: %s\n",__LINE__,#x);exit(1); } ++checks; } while(0)
enum { CS=0x8800,DICE=0x7800,STUB=0xB0,DS=0x1800,SS=0x2800,SP=0x8000,SYS=0x0AF8,REC=0x5000,HEAP=0x3400 };
static uint8_t *ram;
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void w8(unsigned s,unsigned o,unsigned v) { ram[at(s,o)]=(uint8_t)v; }
static void w16(unsigned s,unsigned o,unsigned v) { size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8); }
static unsigned r16(unsigned s,unsigned o) { size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1u)]<<8); }
static pr_u38_1472_cpu world(unsigned high,unsigned old) {
    pr_u38_1472_cpu c={0xA534,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x1472,DS,0x8888,SS,0x0ED7};
    c.ax=(uint16_t)((high<<8)|0x34);memset(ram,0xA5,RAM);
    w8(CS,0x1487,0x9A);w16(CS,0x1488,0x52);w16(CS,0x148A,STUB);
    w8(STUB,0x52,0xEA);w16(STUB,0x53,0xE52);w16(STUB,0x55,DICE);
    w8(DICE,0xE5C,0x9A);w16(DICE,0xE5D,0x23F);w16(DICE,0xE5F,SYS);
    w16(DS,0x4404,0);w16(DS,0x4406,HEAP);w16(DS,0x4408,0);w16(DS,0x440A,HEAP+0x100);
    w16(DS,0x440C,0);w16(DS,0x440E,HEAP);w16(DS,0x4414,0xA9);w16(DS,0x4416,SYS);
    w16(REC,0x17F,0);w16(REC,0x181,old?0x5200:0);w16(0x5200,5,0);w16(0x5200,7,0);
    w16(SS,SP,0x7000);w16(SS,SP+2,0x9001);
    w16(SS,SP+4,0x1111);w16(SS,SP+6,0x2222);w16(SS,SP+8,0x3333);w16(SS,SP+10,0x100);w16(SS,SP+12,REC);
    return c;
}
int main(void) {
    pr_u38_1472_cpu c,before;pr_u38_1472_diagnostic d;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(unsigned high=0;high<256;++high) {
        unsigned old=high%2;c=world(high,old);
        CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,64,&d)==PR_U38_1472_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+14 && c.bp==0x789A);
        CHECK(d.append_status==0 && d.getmem_status==0 && c.ax==0xFF && c.dx==HEAP);
        CHECK(c.flags==0x0646 && ram[at(HEAP,0)]==0x62 && r16(HEAP,1)==0 && ram[at(HEAP,3)]==0xFF && ram[at(HEAP,4)]==0);
        CHECK(r16(HEAP,5)==0 && r16(HEAP,7)==0);
        if(old)CHECK(r16(REC,0x181)==0x5200 && r16(0x5200,5)==0 && r16(0x5200,7)==HEAP);
        else CHECK(r16(REC,0x17F)==0 && r16(REC,0x181)==HEAP);
        c=world(high,0);w8(CS,0x1487,0x90);
        CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,64,&d)==PR_U38_1472_CODE && c.ip==0x1487 && c.sp==SP-14 && d.append_status==-1);
        CHECK(r16(SS,SP-8)==((high<<8)|0x62) && r16(SS,SP-10)==0 && r16(SS,SP-12)==0xFF && r16(SS,SP-14)==0 && c.flags==0x0646);
    }
    c=world(0,0);w16(DS,0x440A,HEAP);
    CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,64,&d)==PR_U38_1472_CALLEE && d.append_status==3 && d.getmem_status==2 && c.ip==0xE2 && c.ax==203);
    CHECK(r16(REC,0x181)==0);
    c=world(0,0);w16(DS,0x4414,0x1234);
    CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,64,&d)==PR_U38_1472_CALLEE && d.getmem_status==3);
    c=world(0,1);w16(0x5200,7,0x5200);
    CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,0,&d)==PR_U38_1472_CALLEE && d.append_status==5 && d.getmem_status==0 && c.ip==0xEA8);
    c=world(0,0);w8(STUB,0x52,0);
    CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,64,&d)==PR_U38_1472_CODE && d.append_status==-1);
    c=world(0,0);w8(DICE,0xE5C,0);
    CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,64,&d)==PR_U38_1472_CALLEE && d.append_status==2 && d.getmem_status==-1);
    c=world(0,0);before=c;
    CHECK(pr_u38_sidecar_1472_cpu(NULL,RAM,&c,64,64,&d)==PR_U38_1472_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && d.append_status==-1);
    CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,NULL,64,64,NULL)==PR_U38_1472_ARGUMENT);
    CHECK(pr_u38_sidecar_1472_cpu(ram,RAM-1,&c,64,64,NULL)==PR_U38_1472_ARGUMENT);
    c.sp=0x3F;CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,64,NULL)==PR_U38_1472_ARGUMENT);
    c.sp=0xFFF3;CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,64,NULL)==PR_U38_1472_ARGUMENT);
    c=before;c.ip=0;CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,64,64,NULL)==PR_U38_1472_ARGUMENT);
    c=world(0,0);CHECK(pr_u38_sidecar_1472_cpu(ram,RAM,&c,0,0,NULL)==PR_U38_1472_OK);
    free(ram);printf("PASS: U38 +1472 %u checks\n",checks);return 0;
}

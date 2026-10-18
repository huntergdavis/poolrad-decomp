/* Synthetic sidecars, object slots, strings and heap; no game bytes. */
#include "poolrad/recovered/overlay_u86_sidecar_gate_051c_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U86_051C_RAM_SIZE
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"U86 +051C line %d: %s\n",__LINE__,#x);exit(1); } ++checks; } while(0)
enum { CS=0x8000,DS=0x1800,SS=0x2800,SP=0x8000,SYSTEM=0x0AF8,RES=0xBA,OBJ=0x5200,HEAP=0x3400 };
static uint8_t *ram;
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void w8(unsigned s,unsigned o,unsigned v) { ram[at(s,o)]=(uint8_t)v; }
static void w16(unsigned s,unsigned o,unsigned v) { size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8); }
static unsigned r16(unsigned s,unsigned o) { size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1u)]<<8); }
static pr_u86_051c_cpu world(unsigned mode,unsigned first) {
    pr_u86_051c_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x51C,DS,0x8888,SS,0x0603};
    const unsigned sites[]={0x2AE,0x31F,0x38B,0x39C,0x3DE,0x3F3,0x403,0x41C,0x42A};
    const unsigned targets[]={0x23F,0x23F,0xB25,0x23F,0xB25,0xB25,0xB0B,0xB98,0xB25};
    memset(ram,0,RAM);
    w8(CS,0x572,0xE8);w16(CS,0x573,0x2CC8);
    w8(CS,0x57E,0xE8);w16(CS,0x57F,0xFECD);
    w8(CS,0x464,0x9A);w16(CS,0x465,0x2199);w16(CS,0x467,RES);
    w8(CS,0x4EE,0xE8);w16(CS,0x4EF,0xFD9C);
    for(unsigned j=0;j<9;++j) { w8(CS,sites[j],0x9A);w16(CS,sites[j]+1,targets[j]);w16(CS,sites[j]+3,SYSTEM); }
    w8(CS,0x287,2);w8(CS,0x288,'A');w8(CS,0x289,' ');
    w8(CS,0x28A,2);w8(CS,0x28B,'B');w8(CS,0x28C,' ');
    w16(DS,0x5D92,0x300);w16(DS,0x5D94,OBJ);w16(DS,0x6B80,0x100);w16(DS,0x6B82,OBJ);
    w16(OBJ,0x3C8,0x100);w16(OBJ,0x3CA,OBJ);
    w8(DS,0x55A2,12);w8(DS,0x4843,255);
    for(unsigned j=0;j<196;++j)w8(DS,0x477F+j,0xA5);
    w8(OBJ,0x12E,2);w8(OBJ,0x13C,first);w8(OBJ,0x13D,0x80);w8(OBJ,0x13E,0x81);
    w16(DS,0x4406,HEAP);w16(DS,0x440A,HEAP+0x100);w16(DS,0x440E,HEAP);
    w16(DS,0x4414,0xA9);w16(DS,0x4416,SYSTEM);
    w16(SS,SP,0x7000);w16(SS,SP+2,0x9001);w16(SS,SP+4,mode);return c;
}
int main(void) {
    pr_u86_051c_limits limits={400,400,{400,400,64,64}},tight;
    pr_u86_051c_diagnostic d;pr_u86_051c_cpu c,before;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(unsigned mode=0;mode<2;++mode)for(unsigned kind=0;kind<256;++kind) {
        unsigned want=(kind>=11 && kind<=13)?(mode?1:3):0;
        c=world(mode,1);w8(DS,0x55A2,kind);
        CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&limits,&d)==PR_U86_051C_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+6 && c.bp==0x789A);
        CHECK(ram[at(DS,0x4843)]==want && (c.flags&0x40)!=0);
        CHECK(r16(DS,0x6B80)==0 && r16(DS,0x6B82)==0);
        for(unsigned j=0;j<49;++j)CHECK(r16(DS,0x477F+4*j)==(j<want?0x100:0) && r16(DS,0x4781+4*j)==(j<want?OBJ:0));
        CHECK(d.callee==(want?PR_U86_051C_SIDECAR:PR_U86_051C_PREDICATE) && d.status==0);
    }
    c=world(0,1);w16(OBJ,0x3C8,0);w16(OBJ,0x3CA,0);
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&limits,&d)==0 && d.callee==-1);
    CHECK(ram[at(DS,0x4843)]==0 && r16(DS,0x4841)==0);
    c=world(0,1);w16(DS,0x440A,HEAP);
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&limits,&d)==PR_U86_051C_CALLEE);
    CHECK(d.callee==1 && d.status==3 && d.sidecar.callee==1 && d.sidecar.status==3 &&
          d.sidecar.nested_callee==0 && d.sidecar.nested_status==2 && c.ip==0xE2 && c.ax==203);
    c=world(0,1);w8(CS,0x572,0x90);
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&limits,&d)==PR_U86_051C_CODE && c.ip==0x572 && d.callee==-1);
    c=world(0,1);w8(CS,0x57E,0x90);
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&limits,&d)==PR_U86_051C_CODE && c.ip==0x57E && d.callee==0);
    c=world(0,1);tight=limits;tight.max_clear_visits=1;
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&tight,&d)==PR_U86_051C_LOOP_LIMIT && c.ip==0x52B);
    CHECK(r16(DS,0x477F)==0 && r16(DS,0x4783)==0xA5A5 && ram[at(SS,SP-3)]==1);
    c=world(0,1);tight=limits;tight.max_record_visits=1;
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&tight,&d)==PR_U86_051C_LOOP_LIMIT && c.ip==0x560);
    CHECK(ram[at(DS,0x4843)]==3 && r16(DS,0x6B80)==0 && r16(DS,0x6B82)==0);
    c=world(0,0);w8(OBJ,0x13D,0);w8(OBJ,0x13E,0);w16(OBJ,0x12A,0x100);w16(OBJ,0x12C,OBJ);
    tight=limits;tight.max_record_visits=3;
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&tight,&d)==PR_U86_051C_LOOP_LIMIT && c.ip==0x560);
    c=world(0,1);before=c;
    CHECK(pr_u86_sidecar_gate_051c_cpu(NULL,RAM,&c,&limits,&d)==PR_U86_051C_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && d.callee==-1);
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,NULL,&limits,NULL)==PR_U86_051C_ARGUMENT);
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM-1,&c,&limits,NULL)==PR_U86_051C_ARGUMENT);
    CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,NULL,NULL)==PR_U86_051C_ARGUMENT);
    tight=limits;tight.max_clear_visits=0;CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&tight,NULL)==PR_U86_051C_ARGUMENT);
    tight=limits;tight.max_record_visits=0;CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&tight,NULL)==PR_U86_051C_ARGUMENT);
    tight=limits;tight.sidecar.max_caller_visits=0;CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&tight,NULL)==PR_U86_051C_ARGUMENT);
    tight=limits;tight.sidecar.max_list_visits=0;CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&tight,NULL)==PR_U86_051C_ARGUMENT);
    c.sp=0x15F;CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&limits,NULL)==PR_U86_051C_ARGUMENT);
    c.sp=0xFFF9;CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&limits,NULL)==PR_U86_051C_ARGUMENT);
    c=before;c.ip=0;CHECK(pr_u86_sidecar_gate_051c_cpu(ram,RAM,&c,&limits,NULL)==PR_U86_051C_ARGUMENT);
    free(ram);printf("PASS: U86 +051C %u checks\n",checks);return 0;
}

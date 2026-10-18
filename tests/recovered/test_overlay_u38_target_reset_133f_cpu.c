#include "poolrad/recovered/overlay_u38_target_reset_133f_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_RES_2637_RAM_SIZE
#define CHECK(x) do { if(!(x)){fprintf(stderr,"U38 reset line%d: %s\n",__LINE__,#x);exit(1);}++checks; } while(0)
enum { CS=0x8800,RES=0x7000,DS=0x1800,SS=0x2800,SP=0x8000,REC=0x5000,TARGET=0x6000 };
static uint8_t *ram;
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void put(unsigned s,unsigned o,unsigned v) { size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8); }
static pr_u38_133f_cpu world(unsigned high,unsigned direct) {
    pr_u38_133f_cpu c={(uint16_t)((high<<8)|0x34),0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x133F,DS,0x9999,SS,0xED7};
    memset(ram,0xA5,RAM);ram[at(CS,0x1348)]=0x9A;put(CS,0x1349,0x2637);put(CS,0x134B,RES);
    put(SS,SP,0x7000);put(SS,SP+2,0x9001);put(SS,SP+4,0x1111);put(SS,SP+6,0x2222);put(SS,SP+8,0x3333);put(SS,SP+10,0x100);put(SS,SP+12,REC);
    if(direct){c.cs=RES;c.ip=0x2637;put(SS,SP+4,0x100);put(SS,SP+6,REC);}
    put(REC,0x208,0x200);put(REC,0x20A,TARGET);return c;
}
int main(void) {
    pr_u38_133f_cpu c,before;ram=malloc(RAM);CHECK(ram!=NULL);
    for(unsigned high=0;high<256;++high)for(unsigned direct=0;direct<2;++direct){
        c=world(high,direct);
        CHECK((direct?(int)pr_res_00ba_target_reset_2637_cpu(ram,RAM,&c):(int)pr_u38_target_reset_133f_cpu(ram,RAM,&c))==0);
        CHECK(c.ax==((high<<8)|1) && c.bp==0x789A && c.sp==SP+(direct?8:14) && c.cs==0x9001 && c.ip==0x7000);
        CHECK(c.bx==0x2345 && c.cx==0x3456 && c.dx==0x4567 && c.si==0x5678 && c.di==0x200 && c.es==TARGET && c.ds==DS && c.ss==SS);
        CHECK(c.flags==(direct?0x606:0x602));
        CHECK(ram[at(TARGET,0x200)]==0 && ram[at(TARGET,0x203)]==0 && ram[at(TARGET,0x206)]==0 && ram[at(TARGET,0x207)]==0);
        CHECK(ram[at(TARGET,0x201)]==0xA5 && ram[at(TARGET,0x202)]==0xA5 && ram[at(TARGET,0x204)]==0xA5 && ram[at(TARGET,0x205)]==0xA5);
        CHECK(ram[at(SS,direct?SP-3:SP-13)]==1);
    }
    c=world(0,0);put(REC,0x208,0x205);put(REC,0x20A,REC);
    CHECK(pr_u38_target_reset_133f_cpu(ram,RAM,&c)==0);
    CHECK(ram[at(REC,0x208)]==0 && ram[at(REC,0x200)]==0 && ram[at(REC,0x207)]==0 && ram[at(REC,0x206)]==0 && c.di==0x200 && c.es==REC);
    c=world(0,0);put(REC,0x208,0x7FF3);put(REC,0x20A,SS);
    CHECK(pr_u38_target_reset_133f_cpu(ram,RAM,&c)==PR_U38_133F_TRANSFER && c.cs==0 && c.ip==0x1300);
    c=world(0,0);ram[at(CS,0x1348)]=0;
    CHECK(pr_u38_target_reset_133f_cpu(ram,RAM,&c)==PR_U38_133F_CODE && c.ip==0x1348 && c.sp==SP-6 && c.flags==0xED7);
    c=world(0,0);before=c;
    CHECK(pr_u38_target_reset_133f_cpu(NULL,RAM,&c)==PR_U38_133F_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_u38_target_reset_133f_cpu(ram,RAM,NULL)==PR_U38_133F_ARGUMENT);
    CHECK(pr_u38_target_reset_133f_cpu(ram,RAM-1,&c)==PR_U38_133F_ARGUMENT);
    c.sp=0xF;CHECK(pr_u38_target_reset_133f_cpu(ram,RAM,&c)==PR_U38_133F_ARGUMENT);
    c.sp=0xFFF3;CHECK(pr_u38_target_reset_133f_cpu(ram,RAM,&c)==PR_U38_133F_ARGUMENT);
    c=world(0,1);before=c;
    CHECK(pr_res_00ba_target_reset_2637_cpu(NULL,RAM,&c)==PR_RES_2637_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_res_00ba_target_reset_2637_cpu(ram,RAM,NULL)==PR_RES_2637_ARGUMENT);
    c.sp=3;CHECK(pr_res_00ba_target_reset_2637_cpu(ram,RAM,&c)==PR_RES_2637_ARGUMENT);
    c.sp=0xFFF9;CHECK(pr_res_00ba_target_reset_2637_cpu(ram,RAM,&c)==PR_RES_2637_ARGUMENT);
    c=before;c.ip=0;CHECK(pr_res_00ba_target_reset_2637_cpu(ram,RAM,&c)==PR_RES_2637_ARGUMENT);
    free(ram);printf("PASS U38 +133F / RES2637: %u checks\n",checks);return 0;
}

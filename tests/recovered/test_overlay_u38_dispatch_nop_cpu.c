#include "poolrad/recovered/overlay_u38_dispatch_nop_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U38_NOP_RAM_SIZE
#define CHECK(x) do { if(!(x)){fprintf(stderr,"U38 nop line%d: %s\n",__LINE__,#x);exit(1);}++checks; } while(0)
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void put(uint8_t *ram,unsigned s,unsigned o,unsigned v) { size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8); }
int main(void) {
    const uint16_t entries[]={0x647,0x1355,0x135C,0x319B},stacks[]={2,3,0x100,0x8001,0xFFF0,0xFFFC};
    uint8_t *ram=malloc(RAM),*want=malloc(RAM);CHECK(ram!=NULL && want!=NULL);
    for(unsigned e=0;e<4;++e)for(unsigned n=0;n<48;++n){
        pr_u38_nop_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,(uint16_t)(0x789A+n),stacks[n%6],0x8800,entries[e],0x1800,0x5000,(uint16_t)(n%2?0xFFFF:0x2800),(uint16_t)(n*1373u)};
        pr_u38_nop_cpu expected=c;memset(ram,(int)n,RAM);put(ram,c.ss,c.sp,0x1234+n);put(ram,c.ss,c.sp+2,0x4000+n);
        memcpy(want,ram,RAM);put(want,c.ss,c.sp-2,c.bp);expected.ip=(uint16_t)(0x1234+n);expected.cs=(uint16_t)(0x4000+n);expected.sp=(uint16_t)(c.sp+14);
        CHECK(pr_u38_dispatch_nop_cpu(ram,RAM,&c)==PR_U38_NOP_OK);
        CHECK(memcmp(&c,&expected,sizeof c)==0);CHECK(memcmp(ram,want,RAM)==0);
    }
    pr_u38_nop_cpu c={0},before;c.ip=0x647;c.sp=0x8000;before=c;memcpy(want,ram,RAM);
    CHECK(pr_u38_dispatch_nop_cpu(NULL,RAM,&c)==PR_U38_NOP_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_u38_dispatch_nop_cpu(ram,RAM,NULL)==PR_U38_NOP_ARGUMENT);
    CHECK(pr_u38_dispatch_nop_cpu(ram,RAM-1,&c)==PR_U38_NOP_ARGUMENT);
    CHECK(pr_u38_dispatch_nop_cpu(ram,RAM+1,&c)==PR_U38_NOP_ARGUMENT);
    for(unsigned n=0;n<4;++n){c=before;if(n<3)c.sp=n==0?0:n==1?1:0xFFFD;else c.ip=0;pr_u38_nop_cpu saved=c;
        CHECK(pr_u38_dispatch_nop_cpu(ram,RAM,&c)==PR_U38_NOP_ARGUMENT && memcmp(&c,&saved,sizeof c)==0);}
    CHECK(memcmp(ram,want,RAM)==0);free(want);free(ram);printf("PASS U38 dispatch leaves: %u checks\n",checks);return 0;
}

#include "poolrad/replay/slot_a_save_loader_compose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
static void word(uint8_t *r,size_t p,unsigned v) { r[p]=(uint8_t)v; r[p+1]=(uint8_t)(v>>8); }
int main(void)
{
    uint8_t *r=calloc(1,0x100000u), *saved=malloc(0x100000u), stub[5]={0xea,0,0,0x00,0x30}, body[310]={0};
    pr_u1c_3171_cpu c={0},before; pr_u25_0000_status st=PR_U25_0000_TRANSFER; size_t ds=0x10000,ss=0x20000;
    CHECK(r && saved); c.cs=0x1234; c.ip=0x2a; c.ds=0x1000; c.ss=0x2000; c.sp=0x8000; c.bp=0x7777;
    memcpy(r+0x1236a,stub,5); word(r,ss+0x8000,0x02a0); word(r,ss+0x8002,0x3582); word(r,ss+0x8004,2);
    word(r,ds+0x49de,0x1000); word(r,ds+0x49e0,0x6300); word(r,ds+0x49ed,0x9900);
    r[0x64001]=1; r[0x64002]=0x34; r[0x64003]=0x12; r[0x64004]=2; r[0x64005]=0x78; r[0x64006]=0x56;
    before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_loader_compose(r,0x100000u,stub,5,body,309,0x4000,2,1,&c,&st)==PR_SLOT_A_LOAD_REFUSED);
    CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u) && st==PR_U25_0000_TRANSFER);
    CHECK(pr_slot_a_save_loader_compose(r,0x100000u,stub,5,body,310,0x4000,2,1,&c,&st)==PR_SLOT_A_LOAD_REACHED);
    CHECK(st==PR_U25_0000_OK && c.cs==0x3582 && c.ip==0x02a0 && c.sp==0x8006);
    CHECK(r[ds+0x7006]==1 && r[ds+0x7007]==2 && r[ds+0x7086]==0x34 && r[ds+0x7046]==0x12 && r[ds+0x7087]==0x78 && r[ds+0x7047]==0x56);
    memcpy(r,saved,0x100000u); c=before;
    CHECK(pr_slot_a_save_loader_compose(r,0x100000u,stub,5,body,310,0x4000,1,1,&c,&st)==PR_SLOT_A_LOAD_REACHED);
    CHECK(st==PR_U25_0000_OPERAND_LIMIT && c.ip==0x22 && r[ds+0x7006]==1);
    free(saved); free(r); puts("conditional SAVE loader composition passed"); return 0;
}

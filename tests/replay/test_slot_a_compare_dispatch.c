#include "poolrad/replay/slot_a_compare_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),pro[6]={0},branch[4]={0},select[26]={0}; pr_u1c_3171_cpu c={0},before;
    CHECK(r&&saved); r[0x389c4]=0x0e;r[0x389c5]=0xe8;r[0x389c6]=0x85;r[0x389c7]=0xfd; r[0x26f7f]=3;
    c.cs=0x3582;c.ip=0x31a4;c.ds=0x2000;c.ss=0x4000;c.sp=0x8000;c.bp=0x1234;c.ax=0xab03;c.flags=0x7282; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f58); CHECK(c.ip==0x2f58 && c.sp==0x7ffa && c.bp==0x7ffa && c.ax==0xab03 && (c.flags&0x08d5u)==0x44u);
    CHECK(r[0x47ffe]==0x82 && r[0x47fff]==0x35 && r[0x47ffc]==0xa8 && r[0x47ffd]==0x31 && r[0x47ffa]==0x34 && r[0x47ffb]==0x12);
    memcpy(r,saved,0x100000u); c=before; r[0x3874d]=1; CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f2d); CHECK(c.ip==0x2f2d && c.sp==0x7ffc && r[0x47ffc]==0xa8);
    memcpy(r,saved,0x100000u); c=before; r[0x26f7f]=4; CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f3e); CHECK(c.ip==0x2f3e && (uint8_t)c.ax==4);
    free(saved); free(r); puts("conditional COMPARE dispatch passed"); return 0;
}

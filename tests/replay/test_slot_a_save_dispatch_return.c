#include "poolrad/replay/slot_a_save_dispatch_return.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),jump[3]={0},tail[2]={0}; pr_u1c_3171_cpu c={0},before;
    CHECK(r&&saved); c.cs=0x1000; c.ip=0x2f89; c.ss=0x3000; c.sp=0x100; c.bp=0x8888; c.flags=0x7246;
    r[0x30100]=0x78; r[0x30101]=0x56; r[0x30102]=0xa8; r[0x30103]=0x31; r[0x30104]=0x34; r[0x30105]=0x12;
    before=c; memcpy(saved,r,0x100000u); CHECK(pr_slot_a_save_dispatch_return(r,0x100000u,jump,2,tail,2,&c)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_save_dispatch_return(r,0x100000u,jump,3,tail,2,&c)); CHECK(c.bp==0x5678 && c.sp==0x106 && c.cs==0x1234 && c.ip==0x31a8 && c.flags==0x7246 && !memcmp(r,saved,0x100000u));
    c=before; c.sp=0xfffb; before=c; CHECK(pr_slot_a_save_dispatch_return(r,0x100000u,jump,3,tail,2,&c)); CHECK(!memcmp(&c,&before,sizeof c));
    free(saved); free(r); puts("SAVE dispatcher return passed"); return 0;
}

#include "poolrad/replay/slot_a_dispatch_first_branch.h"
#include "poolrad/replay/slot_a_dispatch_prologue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u), *saved=malloc(0x100000u), code[4]={0}, prologue[6]={0};
    unsigned i,j; pr_u1c_3171_cpu c, before;
    CHECK(r && saved); memcpy(saved,r,0x100000u);
    for(i=0;i<256;i++) {
        unsigned ones=0; uint16_t expected;
        for(j=0;j<8;j++) ones+=(i>>j)&1u;
        memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x2f33; c.ax=(uint16_t)(0xab00u|i); c.flags=0xffff;
        before=c; CHECK(pr_slot_a_dispatch_first_branch(r,0x100000u,code,3,&c)); CHECK(!memcmp(&c,&before,sizeof c));
        CHECK(!pr_slot_a_dispatch_first_branch(r,0x100000u,code,4,&c));
        expected=(uint16_t)((0xffffu & ~0x08d5u) | ((ones%2u)==0u?4u:0u) | (i==0?0x40u:0u) | (i&0x80u));
        CHECK(c.flags==expected && c.ip==(i?0x2f3e:0x2f37) && c.ax==before.ax && !memcmp(r,saved,0x100000u));
    }
    memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x2f2d; c.ss=0x3000; c.ds=0x2000; c.sp=0x100; r[0x26f7f]=9;
    CHECK(!pr_slot_a_dispatch_prologue(r,0x100000u,prologue,6,&c));
    CHECK(!pr_slot_a_dispatch_first_branch(r,0x100000u,code,4,&c)); CHECK(c.ip==0x2f3e);
    free(saved); free(r); puts("dispatcher first comparison passed"); return 0;
}

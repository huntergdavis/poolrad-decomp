#include "poolrad/replay/slot_a_save_call_frame.h"
#include "poolrad/replay/slot_a_dispatch_save_select.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u), *saved=malloc(0x100000u), code[4]={0}, select[0x47]={0};
    pr_u1c_3171_cpu c={0}, before;
    CHECK(r && saved); c.cs=0x1000; c.ip=0x2f85; c.ss=0x3000; c.sp=0x100; c.flags=0x7246; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_call_frame(r,0x100000u,code,3,&c)); CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_save_call_frame(r,0x100000u,code,4,&c));
    CHECK(c.ip==0x0292 && c.sp==0xfc && c.flags==0x7246);
    saved[0x300fe]=0; saved[0x300ff]=0x10; saved[0x300fc]=0x89; saved[0x300fd]=0x2f; CHECK(!memcmp(r,saved,0x100000u));
    memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x2f3e; c.ss=0xffff; c.sp=0x12; c.ax=9;
    CHECK(!pr_slot_a_dispatch_save_select(r,0x100000u,select,sizeof select,&c));
    CHECK(!pr_slot_a_save_call_frame(r,0x100000u,code,sizeof code,&c));
    CHECK(c.ip==0x0292 && c.sp==0x0e && r[0]==0 && r[1]==0x10 && r[0xffffe]==0x89 && r[0xfffff]==0x2f);
    free(saved); free(r); puts("SAVE call frame passed"); return 0;
}

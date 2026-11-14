#include "poolrad/replay/slot_a_save_loader_call.h"
#include "poolrad/replay/slot_a_save_handler_prefix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),code[5]={0x9a,0x2a,0,0x34,0x12},prefix[9]={0};
    pr_u1c_3171_cpu c={0},before; size_t pos=0x1029b;
    CHECK(r && saved); memcpy(r+pos,code,5); c.cs=0x1000; c.ip=0x029b; c.ss=0x3000; c.sp=0x100; c.flags=0x7246;
    before=c; memcpy(saved,r,0x100000u); CHECK(pr_slot_a_save_loader_call(r,0x100000u,code,4,&c)); CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_save_loader_call(r,0x100000u,code,5,&c)); CHECK(c.cs==0x1234 && c.ip==0x2a && c.sp==0xfc && c.flags==0x7246);
    saved[0x300fe]=0; saved[0x300ff]=0x10; saved[0x300fc]=0xa0; saved[0x300fd]=2; CHECK(!memcmp(r,saved,0x100000u));
    memset(r,0,0x100000u); memcpy(r+pos,code,5); memset(&c,0,sizeof c); c.cs=c.ss=0x1000; c.ip=0x029b; c.sp=0x029f; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_loader_call(r,0x100000u,code,5,&c)); CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
    memset(r,0,0x100000u); memcpy(r+pos,code,5); memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x0292; c.ss=0x3000; c.sp=0x100; c.ax=9;
    CHECK(!pr_slot_a_save_handler_prefix(r,0x100000u,prefix,9,&c)); CHECK(!pr_slot_a_save_loader_call(r,0x100000u,code,5,&c)); CHECK(c.cs==0x1234 && c.ip==0x2a);
    free(saved); free(r); puts("SAVE operand-loader call frame passed"); return 0;
}

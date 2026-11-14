#include "poolrad/replay/slot_a_save_loader_stub.h"
#include "poolrad/replay/slot_a_save_loader_call.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u), *saved=malloc(0x100000u), stub[5]={0xea,0,0,0x78,0x56}, call[5]={0x9a,0x2a,0,0x34,0x12};
    pr_u1c_3171_cpu c={0},before; size_t pos=0x1236a;
    CHECK(r && saved); memcpy(r+pos,stub,5); c.cs=0x1234; c.ip=0x2a; c.sp=0x100; c.flags=0x7246; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_loader_stub(r,0x100000u,stub,4,&c)); CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_save_loader_stub(r,0x100000u,stub,5,&c)); CHECK(c.cs==0x5678 && c.ip==0 && c.sp==before.sp && c.flags==before.flags && !memcmp(r,saved,0x100000u));
    r[pos+1]=1; stub[1]=1; before=c; c.cs=0x1234; c.ip=0x2a; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_loader_stub(r,0x100000u,stub,5,&c)); CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
    stub[1]=0; memset(r,0,0x100000u); memcpy(r+0x1029b,call,5); memcpy(r+pos,stub,5); memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x029b; c.ss=0x3000; c.sp=0x100;
    CHECK(!pr_slot_a_save_loader_call(r,0x100000u,call,5,&c)); CHECK(!pr_slot_a_save_loader_stub(r,0x100000u,stub,5,&c)); CHECK(c.cs==0x5678 && c.ip==0 && c.sp==0xfc);
    free(saved); free(r); puts("SAVE loader stub passed"); return 0;
}

#include "poolrad/replay/slot_a_save_store_args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),code[9]={0}; pr_u1c_3171_cpu c={0},before;
    CHECK(r&&saved); c.cs=0x1000; c.ip=0x02bf; c.ss=0x3000; c.bp=0x100; c.sp=0x80; c.ax=0x1234; c.flags=0x7246; r[0x300fc]=0x78; r[0x300fd]=0x56;
    before=c; memcpy(saved,r,0x100000u); CHECK(pr_slot_a_save_store_args(r,0x100000u,code,8,&c)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_save_store_args(r,0x100000u,code,9,&c)); CHECK(c.ip==0x02c8 && c.sp==0x7c && c.flags==0x7246);
    saved[0x300fe]=0x34; saved[0x300ff]=0x12; saved[0x3007e]=0x78; saved[0x3007f]=0x56; saved[0x3007c]=0x34; saved[0x3007d]=0x12; CHECK(!memcmp(r,saved,0x100000u));
    memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x02bf; c.ss=0x3000; c.bp=0x100; c.sp=0x100; c.ax=0x1234; r[0x300fc]=0x78; r[0x300fd]=0x56;
    CHECK(!pr_slot_a_save_store_args(r,0x100000u,code,9,&c)); CHECK(r[0x300fc]==0x78 && r[0x300fd]==0x56 && r[0x300fe]==0x78 && r[0x300ff]==0x56);
    memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=c.ss=0x1000; c.ip=0x02bf; c.bp=0x02c1; c.sp=0x8000; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_store_args(r,0x100000u,code,9,&c)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    free(saved); free(r); puts("SAVE store argument setup passed"); return 0;
}

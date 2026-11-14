#include "poolrad/replay/slot_a_save_word_args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),code[8]={0}; pr_u1c_3171_cpu c={0},before;
    CHECK(r && saved); c.cs=0x1000; c.ip=0x02a0; c.ds=0x2000; c.ss=0x3000; c.sp=0x100; c.ax=0xab00; c.flags=0x7246;
    r[0x27087]=0x34; r[0x27047]=0x12; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_word_args(r,0x100000u,code,7,&c)); CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_save_word_args(r,0x100000u,code,8,&c)); CHECK(c.ip==0x02a8 && c.sp==0xfc && c.ax==0xab12 && c.flags==0x7246);
    saved[0x300fe]=0x34; saved[0x300ff]=0xab; saved[0x300fc]=0x12; saved[0x300fd]=0xab; CHECK(!memcmp(r,saved,0x100000u));
    memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x02a0; c.ds=c.ss=0x2000; c.sp=0x7049; c.ax=0xab00; r[0x27087]=0x34; r[0x27047]=0x12;
    CHECK(!pr_slot_a_save_word_args(r,0x100000u,code,8,&c)); CHECK(c.ax==0xab34 && r[0x27045]==0x34);
    memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=c.ss=0x1000; c.ip=0x02a0; c.sp=0x02a4; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_word_args(r,0x100000u,code,8,&c)); CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
    free(saved); free(r); puts("SAVE word argument setup passed"); return 0;
}

#include "poolrad/replay/slot_a_first_save_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
static void word(uint8_t *r,size_t p,unsigned v) { r[p]=(uint8_t)v; r[p+1]=(uint8_t)(v>>8); }
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),call[5]={0x9a,0x52,0,0x34,0x12},stub[5]={0xea,0x77,0x0c,0,0x30},body[405]={0},hook[515]={0},classify[91]={0};
    pr_u1c_3171_cpu c={0},before; pr_u25_store_word_status st=PR_U25_STORE_WORD_TRANSFER;
    CHECK(r&&saved); memcpy(r+0x102c8,call,5); memcpy(r+0x12392,stub,5); c.cs=0x1000; c.ip=0x02c8; c.ds=0x1000; c.ss=0x2000; c.sp=0x8000;
    word(r,0x28000,11); word(r,0x28002,0x6e7d); word(r,0x149d6,0x100); word(r,0x149d8,0x4000); before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_first_save_store(r,0x100000u,call,5,stub,5,body,404,hook,515,classify,91,&c,&st)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u)&&st==PR_U25_STORE_WORD_TRANSFER);
    CHECK(!pr_slot_a_first_save_store(r,0x100000u,call,5,stub,5,body,405,hook,515,classify,91,&c,&st)); CHECK(st==PR_U25_STORE_WORD_OK && c.cs==0x1000 && c.ip==0x02cd && c.sp==0x8004 && r[0x407fa]==11 && r[0x407fb]==0);
    memcpy(r,saved,0x100000u); c=before; word(r,0x28002,0xc04d); memcpy(saved,r,0x100000u); CHECK(pr_slot_a_first_save_store(r,0x100000u,call,5,stub,5,body,405,hook,515,classify,91,&c,&st)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    memcpy(r,saved,0x100000u); c=before; word(r,0x28002,0x6e7d); word(r,0x149d8,0x1000); memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_first_save_store(r,0x100000u,call,5,stub,5,body,405,hook,515,classify,91,&c,&st)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    c.ss=c.ds=0x1000; before=c; word(r,0x18000,11); word(r,0x18002,0x6e7d); memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_first_save_store(r,0x100000u,call,5,stub,5,body,405,hook,515,classify,91,&c,&st)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    free(saved); free(r); puts("bounded first SAVE store passed"); return 0;
}

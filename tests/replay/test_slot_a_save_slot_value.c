#include "poolrad/replay/slot_a_save_slot_value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),call[8]={0,0,0,0x9a,0x25,0,0x34,0x12},stub[5]={0xea,0x36,1,0,0x30},body[128]={0},word[36]={0};
    pr_u1c_3171_cpu c={0},before; pr_u25_slot_value_status st=PR_U25_SLOT_VALUE_TRANSFER;
    CHECK(r&&saved); memcpy(r+0x102b7,call,8); memcpy(r+0x12365,stub,5); c.cs=0x1000; c.ip=0x02b7; c.ds=0x2000; c.ss=0x4000; c.sp=0x8000; c.ax=0xab00;
    r[0x27006]=2; r[0x27086]=0x34; r[0x27046]=0x12; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_slot_value(r,0x100000u,call,8,stub,5,body,127,word,36,&c,&st)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u)&&st==PR_U25_SLOT_VALUE_TRANSFER);
    CHECK(!pr_slot_a_save_slot_value(r,0x100000u,call,8,stub,5,body,128,word,36,&c,&st)); CHECK(st==PR_U25_SLOT_VALUE_OK && c.cs==0x1000 && c.ip==0x02bf && c.sp==0x8000 && c.ax==0x1234);
    memcpy(r,saved,0x100000u); c=before; r[0x27006]=1; memcpy(saved,r,0x100000u); CHECK(pr_slot_a_save_slot_value(r,0x100000u,call,8,stub,5,body,128,word,36,&c,&st)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    r[0x27006]=2; c.ss=c.ds=0x2000; c.sp=0x7010; before=c; memcpy(saved,r,0x100000u); CHECK(pr_slot_a_save_slot_value(r,0x100000u,call,8,stub,5,body,128,word,36,&c,&st)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    free(saved); free(r); puts("bounded SAVE slot-value composition passed"); return 0;
}

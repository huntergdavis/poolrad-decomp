#include "poolrad/replay/slot_a_compare_first_slot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
static void word(uint8_t *r,size_t p,unsigned v) { r[p]=(uint8_t)v;r[p+1]=(uint8_t)(v>>8); }
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),call[8]={0,0,0,0x9a,0x25,0,0x34,0x12},stub[5]={0xea,0x36,1,0,0x30},slot[128]={0},wordcode[36]={0},load[327]={0},classify[91]={0},hook[696]={0},party[102]={0};
    pr_compare_slot_refs refs={call,stub,slot,wordcode,load,classify,hook,party};pr_u1c_3171_cpu c={0},before;pr_u25_slot_value_status st=PR_U25_SLOT_VALUE_TRANSFER;
    CHECK(r&&saved);memcpy(r+0x1017e,call,8);memcpy(r+0x12365,stub,5);c.cs=0x1000;c.ip=0x017e;c.ds=0x2000;c.ss=0x4000;c.sp=0x8000;c.ax=0xab00;
    r[0x27006]=1;r[0x27086]=0xc9;r[0x27046]=0x49;word(r,0x249d2,0x100);word(r,0x249d4,0x6000);word(r,0x60292,10);before=c;memcpy(saved,r,0x100000u);
    CHECK(!pr_slot_a_compare_first_slot(r,0x100000u,&refs,&c,&st));CHECK(st==PR_U25_SLOT_VALUE_OK && c.cs==0x1000 && c.ip==0x0186 && c.sp==0x8000 && c.ax==10);
    memcpy(r,saved,0x100000u);c=before;r[0x27006]=2;memcpy(saved,r,0x100000u);CHECK(pr_slot_a_compare_first_slot(r,0x100000u,&refs,&c,&st));CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    r[0x27006]=1;c.ss=c.ds;c.sp=0x8000;before=c;memcpy(saved,r,0x100000u);CHECK(pr_slot_a_compare_first_slot(r,0x100000u,&refs,&c,&st));CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    free(saved);free(r);puts("first COMPARE slot passed");return 0;
}

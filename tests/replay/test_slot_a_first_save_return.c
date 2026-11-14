#include "poolrad/replay/slot_a_first_save_compose.h"
#include "poolrad/replay/slot_a_save_epilogue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
static void word(uint8_t *r,size_t p,unsigned v) { r[p]=(uint8_t)v; r[p+1]=(uint8_t)(v>>8); }
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),sc[8]={0,0,0,0x9a,0x25,0,0x34,0x12},ss[5]={0xea,0x36,1,0,0x30},sb[128]={0},wb[36]={0},sa[9]={0},tc[5]={0x9a,0x52,0,0x34,0x12},ts[5]={0xea,0x77,0x0c,0,0x30},tb[405]={0},th[515]={0},cl[91]={0};
    uint8_t jump[2]={0},tail[4]={0}; pr_slot_a_first_save_refs refs={sc,ss,sb,wb,sa,tc,ts,tb,th,cl}; pr_u1c_3171_cpu c={0},before; pr_slot_a_first_save_report report;
    CHECK(r&&saved); memcpy(r+0x102b7,sc,8); memcpy(r+0x12365,ss,5); memcpy(r+0x102c8,tc,5); memcpy(r+0x12392,ts,5);
    c.cs=0x1000; c.ip=0x02b7; c.ds=0x2000; c.ss=0x4000; c.sp=0x8000; c.bp=0x8010; c.ax=0xab00;
    r[0x27006]=0; r[0x27086]=11; word(r,0x4800c,0x6e7d); word(r,0x48010,0x4567); word(r,0x48012,0x2f89); word(r,0x48014,0x1000); word(r,0x249d6,0x100); word(r,0x249d8,0x6000); before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_first_save_compose(r,0x100000u,&refs,&c,&report)==PR_FIRST_SAVE_AT_02CD);
    CHECK(report.slot==PR_U25_SLOT_VALUE_OK && report.store==PR_U25_STORE_WORD_OK && c.cs==0x1000 && c.ip==0x02cd && c.sp==0x8000 && r[0x607fa]==11 && r[0x607fb]==0);
    CHECK(!pr_slot_a_save_epilogue(r,0x100000u,jump,2,tail,4,&c)); CHECK(c.cs==0x1000 && c.ip==0x2f89 && c.sp==0x8016 && c.bp==0x4567 && r[0x607fa]==11);
    memcpy(r,saved,0x100000u); c=before; r[0x27006]=1; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_first_save_compose(r,0x100000u,&refs,&c,&report)==PR_FIRST_SAVE_SLOT_REFUSED); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    memcpy(r,saved,0x100000u); c=before; r[0x27006]=0; r[0x102bf]=1;
    CHECK(pr_slot_a_first_save_compose(r,0x100000u,&refs,&c,&report)==PR_FIRST_SAVE_ARGS_REFUSED); CHECK(report.slot==PR_U25_SLOT_VALUE_OK && c.cs==0x1000 && c.ip==0x02bf && c.ax==11 && r[0x607fa]==0);
    free(saved); free(r); puts("conditional first SAVE return passed"); return 0;
}

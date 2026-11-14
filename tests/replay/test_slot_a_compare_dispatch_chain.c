#include "poolrad/replay/slot_a_first_save_compose.h"
#include "poolrad/replay/slot_a_save_epilogue.h"
#include "poolrad/replay/slot_a_save_dispatch_return.h"
#include "poolrad/replay/slot_a_save_runner_jump.h"
#include "poolrad/replay/slot_a_save_runner_loop.h"
#include "poolrad/replay/slot_a_runner_opcode_prefix.h"
#include "poolrad/replay/slot_a_runner_opcode_fetch.h"
#include "poolrad/replay/slot_a_compare_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
static void word(uint8_t *r,size_t p,unsigned v) { r[p]=(uint8_t)v; r[p+1]=(uint8_t)(v>>8); }
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),sc[8]={0,0,0,0x9a,0x25,0,0x34,0x12},ss[5]={0xea,0x36,1,0,0x30},sb[128]={0},wb[36]={0},sa[9]={0},tc[5]={0x9a,0x52,0,0x34,0x12},ts[5]={0xea,0x77,0x0c,0,0x30},tb[405]={0},th[515]={0},cl[91]={0};
    uint8_t jump[2]={0},tail[4]={0},dj[3]={0},dt[2]={0},loop[14]={0},fetch[10]={0},pro[6]={0},branch[4]={0},select[26]={0}; pr_slot_a_first_save_refs refs={sc,ss,sb,wb,sa,tc,ts,tb,th,cl}; pr_u1c_3171_cpu c={0},before; pr_slot_a_first_save_report report;
    CHECK(r&&saved); memcpy(r+0x35ad7,sc,8); memcpy(r+0x12365,ss,5); memcpy(r+0x35ae8,tc,5); memcpy(r+0x12392,ts,5);
    c.cs=0x3582; c.ip=0x02b7; c.ds=0x2000; c.ss=0x4000; c.sp=0x8000; c.bp=0x8010; c.ax=0xab00;
    r[0x27006]=0; r[0x27086]=11; word(r,0x4800c,0x6e7d); word(r,0x48010,0x4567); word(r,0x48012,0x2f89); word(r,0x48014,0x3582); word(r,0x48016,0x2222); word(r,0x48018,0x31a8); word(r,0x4801a,0x3582); word(r,0x249d6,0x100); word(r,0x249d8,0x6000); r[0x389c4]=0x0e; r[0x389c5]=0xe8; r[0x389c6]=0x85; r[0x389c7]=0xfd; r[0x389c8]=0xeb; r[0x389c9]=0xd5; { const uint8_t op[]={0xa0,0x7f,0x6f,0xa2,0x7e,0x6f,0xa1,0xed,0x49,0xc4,0x3e,0xde,0x49}; memcpy(r+0x389ad,op,sizeof op); } r[0x26f7f]=9; word(r,0x249ed,0x991a); word(r,0x249de,0x1000); word(r,0x249e0,0x6300); r[0x6401a]=3; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_first_save_compose(r,0x100000u,&refs,&c,&report)==PR_FIRST_SAVE_AT_02CD);
    CHECK(report.slot==PR_U25_SLOT_VALUE_OK && report.store==PR_U25_STORE_WORD_OK && c.cs==0x3582 && c.ip==0x02cd && c.sp==0x8000 && r[0x607fa]==11 && r[0x607fb]==0);
    CHECK(!pr_slot_a_save_epilogue(r,0x100000u,jump,2,tail,4,&c)); CHECK(c.cs==0x3582 && c.ip==0x2f89 && c.sp==0x8016 && c.bp==0x4567 && r[0x607fa]==11);
    CHECK(!pr_slot_a_save_dispatch_return(r,0x100000u,dj,3,dt,2,&c)); CHECK(c.cs==0x3582 && c.ip==0x31a8 && c.sp==0x801c && c.bp==0x2222 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x389c8]^=1; CHECK(!pr_slot_a_save_runner_jump(&c,r,0x100000u)); CHECK(!memcmp(&c,&at,sizeof c)); r[0x389c8]^=1; CHECK(pr_slot_a_save_runner_jump(&c,r,0x100000u)); CHECK(c.cs==0x3582 && c.ip==0x317f && c.sp==0x801c && r[0x607fa]==11); }
    CHECK(!pr_slot_a_save_runner_loop(r,0x100000u,loop,14,&c)); CHECK(c.cs==0x3582 && c.ip==0x318d && r[0x607fa]==11);
    CHECK(pr_slot_a_runner_opcode_prefix(r,0x100000u,&c)==0x319a); CHECK(c.ip==0x319a && c.ax==0x991a && c.di==0x1000 && c.es==0x6300 && r[0x26f7e]==9 && r[0x607fa]==11);
    CHECK(!pr_slot_a_runner_opcode_fetch(r,0x100000u,fetch,10,&c)); CHECK(c.ip==0x31a4 && c.di==0xa91a && c.ax==0x9903 && r[0x26f7f]==3 && r[0x607fa]==11);
    CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f58); CHECK(c.ip==0x2f58 && c.sp==0x8016 && (uint8_t)c.ax==3 && r[0x607fa]==11);
    memcpy(r,saved,0x100000u); c=before; r[0x27006]=1; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_first_save_compose(r,0x100000u,&refs,&c,&report)==PR_FIRST_SAVE_SLOT_REFUSED); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    memcpy(r,saved,0x100000u); c=before; r[0x27006]=0; r[0x35adf]=1;
    CHECK(pr_slot_a_first_save_compose(r,0x100000u,&refs,&c,&report)==PR_FIRST_SAVE_ARGS_REFUSED); CHECK(report.slot==PR_U25_SLOT_VALUE_OK && c.cs==0x3582 && c.ip==0x02bf && c.ax==11 && r[0x607fa]==0);
    free(saved); free(r); puts("conditional COMPARE dispatch chain passed"); return 0;
}

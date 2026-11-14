#include "poolrad/replay/slot_a_compare_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
static void word(uint8_t *r,size_t p,unsigned v) { r[p]=(uint8_t)v;r[p+1]=(uint8_t)(v>>8); }
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),call[5]={0x9a,0x2a,0,0x34,0x12},stub[5]={0xea,0,0,0,0x30},body[310]={0};
    pr_u1c_3171_cpu c={0},before;pr_slot_a_compare_load_report report;
    CHECK(r&&saved); memcpy(r+0x10130,call,5);memcpy(r+0x1236a,stub,5);c.cs=0x1000;c.ip=0x0130;c.ds=0x2000;c.ss=0x4000;c.sp=0x8000;c.ax=2;word(r,0x48000,2);
    word(r,0x249de,0x1000);word(r,0x249e0,0x6300);word(r,0x249ed,0x991a);r[0x6401b]=1;r[0x6401c]=0xc9;r[0x6401d]=0x49;r[0x6401e]=0;r[0x6401f]=14;
    before=c;memcpy(saved,r,0x100000u);r[0x10130]^=1;CHECK(pr_slot_a_compare_loader(r,0x100000u,call,stub,body,0x5000,2,1,&c,&report)==PR_COMPARE_LOAD_CALL_REFUSED);CHECK(!memcmp(&c,&before,sizeof c));r[0x10130]^=1;
    CHECK(pr_slot_a_compare_loader(r,0x100000u,call,stub,body,0x5000,2,1,&c,&report)==PR_COMPARE_LOAD_REACHED);CHECK(report.loader==PR_U25_0000_OK && c.cs==0x1000 && c.ip==0x0135 && c.sp==0x8002);
    CHECK(r[0x27006]==1 && r[0x27007]==0 && r[0x27086]==0xc9 && r[0x27046]==0x49 && r[0x27087]==14 && r[0x249ed]==0x20 && r[0x249ee]==0x99);
    memcpy(r,saved,0x100000u);c=before;CHECK(pr_slot_a_compare_loader(r,0x100000u,call,stub,body,0x5000,1,1,&c,&report)==PR_COMPARE_LOAD_REACHED);CHECK(report.loader==PR_U25_0000_OPERAND_LIMIT && c.ip==0x22);
    memcpy(r,saved,0x100000u);c=before;r[0x1236a]^=1;CHECK(pr_slot_a_compare_loader(r,0x100000u,call,stub,body,0x5000,2,1,&c,&report)==PR_COMPARE_LOAD_STUB_REFUSED);CHECK(c.cs==0x1234 && c.ip==0x002a && c.sp==0x7ffc && r[0x47ffc]==0x35 && r[0x47ffd]==1);
    memcpy(r,saved,0x100000u);c=before;c.ss=c.cs;c.sp=0x0134;before=c;memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_compare_loader(r,0x100000u,call,stub,body,0x5000,2,1,&c,&report)==PR_COMPARE_LOAD_CALL_REFUSED);CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    free(saved);free(r);puts("COMPARE operand loader passed");return 0;
}

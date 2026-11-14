#include "poolrad/replay/slot_a_compare_tag_tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),code[14]={0};pr_u1c_3171_cpu c,before;unsigned k,v,j;
    CHECK(r&&saved);
    for(k=0;k<2;k++) for(v=0;v<256;v++){ unsigned last=k?v:(v>=128u?v:129u),res=(last-128u)&255u,ones=0;uint16_t f;
        for(j=0;j<8;j++) ones+=(res>>j)&1u;
        memset(&c,0,sizeof c);c.cs=0x1000;c.ip=0x0135;c.ds=0x2000;c.flags=0xffff;r[0x27006]=k?1:(uint8_t)v;r[0x27007]=k?(uint8_t)v:129;before=c;memcpy(saved,r,0x100000u);
        CHECK(pr_slot_a_compare_tag_tests(r,0x100000u,code,13,&c));CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
        CHECK(!pr_slot_a_compare_tag_tests(r,0x100000u,code,14,&c)); f=(uint16_t)((0xffffu&~0x08d5u)|(last<128u?0x801u:0u)|(ones%2u==0?4u:0u)|(res==0?0x40u:0u)|(res&0x80u));
        CHECK(c.flags==f && c.ip==((!k||v>=128u)?0x0143:0x017e) && !memcmp(r,saved,0x100000u));
    }
    free(saved);free(r);puts("COMPARE tag tests passed");return 0;
}

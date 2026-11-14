#include "poolrad/replay/slot_a_runner_first_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u), code[66]={0};
    pr_u1c_3171_cpu c={0}, before;
    size_t ds=0x20000u, st=0x30000u;
    CHECK(r); c.cs=0x1000; c.ip=0x3171; c.ds=0x2000; c.ss=0x3000; c.sp=0x100; c.bp=0x9876;
    r[st+0x104]=0x14; r[st+0x105]=0x99;
    r[ds+0x49de]=0; r[ds+0x49df]=0; r[ds+0x49e0]=0; r[ds+0x49e1]=0x40;
    r[0x40000+0x14]=9; /* (9914 + 6700) wraps to 0014 */
    before=c; r[ds+0x49ff]=1;
    CHECK(pr_slot_a_runner_first_dispatch(r,0x100000u,code,sizeof code,&c)==PR_SLOT_A_RUNNER_REFUSED);
    CHECK(!memcmp(&c,&before,sizeof c) && r[st+0xfe]==0);
    r[ds+0x49ff]=0;
    CHECK(pr_slot_a_runner_first_dispatch(r,0x100000u,code,sizeof code,&c)==PR_SLOT_A_RUNNER_FIRST_DISPATCH);
    CHECK(c.ip==0x2f2d && c.sp==0xfa && c.bp==0xfe && r[ds+0x49ed]==0x14 && r[ds+0x49ee]==0x99 && r[ds+0x6f7f]==9);
    CHECK(r[st+0xfe]==0x76 && r[st+0xff]==0x98 && r[st+0xfa]==0xa8 && r[st+0xfb]==0x31);
    {
        uint8_t *saved=malloc(0x100000u);
        CHECK(saved); memset(r,0,0x100000u); memset(&c,0,sizeof c);
        c.cs=0x1000; c.ip=0x3171; c.ds=0x2000; c.ss=0x2000;
        c.sp=0x4a01; c.bp=1; before=c; memcpy(saved,r,0x100000u);
        CHECK(pr_slot_a_runner_first_dispatch(r,0x100000u,code,sizeof code,&c)==PR_SLOT_A_RUNNER_REFUSED);
        CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
        free(saved);
    }
    free(r); puts("runner first dispatcher boundary passed"); return 0;
}

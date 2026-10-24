#include "poolrad/recovered/overlay_u7f_063a_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t at(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & 0xFFFFFu; }
static void put(uint8_t *r, uint16_t s, uint16_t o, uint16_t v) {
    size_t p = at(s, o); r[p] = (uint8_t)v; r[(p + 1) & 0xFFFFFu] = (uint8_t)(v >> 8);
}
static uint16_t get(const uint8_t *r, uint16_t s, uint16_t o) {
    size_t p = at(s, o); return (uint16_t)(r[p] | (r[(p + 1) & 0xFFFFFu] << 8));
}
static pr_u7f_063a_cpu setup(uint8_t *r, unsigned count, uint16_t seg, uint16_t total, uint16_t pool) {
    static const uint16_t near_sites[][2] = {
        {0x652,0x5E0},{0x75D,0x54},{0x7A8,0x3A},{0x7D0,0x54},{0x805,0x3A},
        {0x86C,0x3A},{0x8C7,0},{0x91D,0x3A},{0x97E,0x3A},{0x61,0},{0x80,0}
    };
    static const uint16_t far_sites[][2] = {{0x664,0x16A8},{0x675,0x16A8},{0x6BA,0xA76},{0x6E9,0xA76},{0xC,0x13AB}};
    pr_u7f_063a_cpu c; unsigned i, k;
    memset(r, 0xA5, PR_BUS_RAM_SIZE); memset(&c, 0, sizeof c);
    c.cs=0x6000; c.ip=0x63A; c.ds=0x1000; c.ss=0x2000; c.sp=0x8000; c.bp=0x7777; c.flags=0x202;
    put(r,c.ss,c.sp,0x9000); put(r,c.ss,c.sp+2,0x8000);
    put(r,c.ds,0x5D96,count ? 0x100 : 0); put(r,c.ds,0x5D98,count ? seg : 0);
    for(i=0;i<7;i++) { put(r,c.ds,(uint16_t)(0x67F4+4*i),i ? 0 : pool); put(r,c.ds,(uint16_t)(0x67F6+4*i),0); }
    for(i=0;i<count;i++) {
        uint16_t off=(uint16_t)(0x100+0x200*i);
        for(k=0;k<0x11D;k++)r[at(seg,(uint16_t)(off+k))]=0;
        r[at(seg,off+0x10)]=0x0C; put(r,seg,off+0x102,total);
        put(r,seg,off+0x104,i+1<count ? off+0x200 : 0); put(r,seg,off+0x106,i+1<count ? seg : 0);
    }
    for(i=0;i<sizeof near_sites/sizeof *near_sites;i++) {
        uint16_t p=near_sites[i][0]; r[at(c.cs,p)]=0xE8; put(r,c.cs,p+1,(uint16_t)(near_sites[i][1]-p-3));
    }
    for(i=0;i<sizeof far_sites/sizeof *far_sites;i++) {
        uint16_t p=far_sites[i][0]; r[at(c.cs,p)]=0x9A; put(r,c.cs,p+1,far_sites[i][1]); put(r,c.cs,p+3,p==0xC ? 0x7000 : 0x7400);
    }
    return c;
}
int main(void) {
    uint8_t *r=malloc(PR_BUS_RAM_SIZE), *before=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega); pr_bus bus; pr_u7f_063a_cpu c,saved;
    pr_u7f_063a_diagnostic d; size_t budget; unsigned p;
    assert(r && before && ega); memset(&bus,0,sizeof bus); bus.ram=r;
    /* Equal shares plus the remainder go to the first eligible node. */
    c=setup(r,2,0x3000,0,19); put(r,c.ds,0x67F8,7); budget=100000;
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,&budget,&d)==PR_U7F_063A_RETURNED);
    assert(get(r,0x3000,0x188)==10 && get(r,0x3000,0x388)==9);
    assert(get(r,0x3000,0x18A)==4 && get(r,0x3000,0x38A)==3);
    assert(get(r,0x3000,0x202)==14 && get(r,0x3000,0x402)==12);
    assert(get(r,0x1000,0x67F4)==0 && get(r,0x1000,0x67F8)==0 && r[at(0x1000,0x6DC0)]==0);
    assert(c.cs==0x8000 && c.ip==0x9000 && c.sp==0x8004 && c.bp==0x7777);
    /* Class 0Ch yields threshold 1600; the excess remains in the pool. */
    c=setup(r,1,0x3000,1599,17); budget=100000;
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,&budget,&d)==PR_U7F_063A_RETURNED);
    assert(get(r,0x3000,0x188)==1 && get(r,0x3000,0x202)==1600);
    assert(get(r,0x1000,0x67F4)==16 && r[at(0x1000,0x6DC0)]==1);
    c=setup(r,0,0x3000,0,1); budget=100000;
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,&budget,&d)==PR_U7F_063A_CALLEE);
    assert(d.callee==2 && d.callee_status==2 && c.cs==0x7400 && c.ip==0xE2 && c.ax==0xC8);
    c=setup(r,2,0x3000,0,19); saved=c; memcpy(before,r,PR_BUS_RAM_SIZE); budget=0;
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,&budget,&d)==PR_U7F_063A_LIMIT);
    assert(!memcmp(&saved,&c,sizeof c) && !memcmp(before,r,PR_BUS_RAM_SIZE));
    c=setup(r,2,0x3000,0,19); r[at(c.cs,0x652)]=0x9A; budget=100000;
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,&budget,&d)==PR_U7F_063A_CODE);
    assert(c.ip==0x652 && c.sp==0x7FD2 && d.site==0x652);
    c=setup(r,1,0x3000,0,19); r[at(c.cs,0xC)]=0xE8; budget=100000;
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,&budget,&d)==PR_U7F_063A_CALLEE && d.callee==3);
    /* Bus mechanism probe only: flat callees keep their declared contracts.
       The caller's node-word write updates planes, not RAM underneath. */
    c=setup(r,1,0xA000,0,5); pr_ega_reset(ega); bus.ega=ega;
    ega->plane[0][0x188]=0x34; ega->plane[0][0x189]=0x12; budget=100000;
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,&budget,&d)==PR_U7F_063A_RETURNED);
    for(p=0;p<4;p++)assert(ega->plane[p][0x188]==0x39 && ega->plane[p][0x189]==0x12);
    assert(get(r,0xA000,0x188)==0); bus.ega=NULL;
    c=setup(r,1,0x3000,0,1); c.sp=0x1FF; saved=c; budget=100000; memcpy(before,r,PR_BUS_RAM_SIZE);
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,&budget,&d)==PR_U7F_063A_ARGUMENT);
    assert(!memcmp(&saved,&c,sizeof c) && !memcmp(before,r,PR_BUS_RAM_SIZE) && budget==100000);
    c.sp=0x8000;
    assert(pr_u7f_063a_run(NULL,PR_BUS_RAM_SIZE,&c,&budget,&d)==PR_U7F_063A_ARGUMENT);
    assert(pr_u7f_063a_run(&bus,0,&c,&budget,&d)==PR_U7F_063A_ARGUMENT);
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,NULL,&budget,&d)==PR_U7F_063A_ARGUMENT);
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,NULL,&d)==PR_U7F_063A_ARGUMENT);
    assert(pr_u7f_063a_run(&bus,PR_BUS_RAM_SIZE,&c,&budget,NULL)==PR_U7F_063A_ARGUMENT);
    free(ega); free(before); free(r); puts("U7F063A distribution, residual, error, bus and guard cases passed"); return 0;
}

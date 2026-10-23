#include "loaded_stub_fixture.h"
#include "poolrad/recovered/overlay_u9e_sidecar_groups_02fa_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static unsigned rd(const uint8_t *m,unsigned s,unsigned o) { size_t a=p(s,o);return m[a]|((unsigned)m[(a+1u)&1048575u]<<8); }
static void put(uint8_t *m,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8); }
static const pr_u9e_0160_layout layout={0x3000,0x4000,0x6000,0x9800};
static const pr_u9e_0160_limits defaults={64,512,256,20000,512,64,65536};
static pr_u9e_0160_cpu setup(uint8_t *m,unsigned group,unsigned key,unsigned direct)
{
    pr_u9e_0160_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8800,0x160,0x5000,0x1111,0x2000,0x602};
    static const unsigned calls[][3]={{0x18B,0x2199,0x9800},{0x1A5,0xDC0,0x6000},{0x1BE,0x1685,0x6000},
        {0x1EC,0x2199,0x9800},{0x21C,0xBE7,0x3000},{0x22A,0xC0F,0x3000},{0x242,0xC37,0x3000},
        {0x250,0x909,0x4000},{0x274,0xC5F,0x3000},{0x2CF,0x1685,0x6000}};
    unsigned i;memset(m,0xA5,PR_U9E_0160_RAM_SIZE);
    test_loaded_stub(m, layout.chain_cs, 0x092Au, (uint16_t)(layout.slots_cs + 0x100u), 0x0025u, layout.slots_cs, 0x0000u);
    test_loaded_stub(m, layout.chain_cs, 0x09D8u, (uint16_t)(layout.slots_cs + 0x100u), 0x0025u, layout.slots_cs, 0x0000u);
    for(i=0;i<10u;++i){m[p(c.cs,calls[i][0])]=0x9A;put(m,c.cs,calls[i][0]+1u,calls[i][1]);put(m,c.cs,calls[i][0]+3u,calls[i][2]);}
    m[p(c.cs,0x2F1)]=0xE8;put(m,c.cs,0x2F2,0xFD0C);
    memset(m+p(c.cs,0x140),0,32);
    {const unsigned set[]={21,45,46,49};for(i=0;i<4u;++i)m[p(c.cs,0x140u+set[i]/8u)]|=(uint8_t)(1u<<(set[i]%8u));}
    put(m,0x7000,0x147F,direct?0x8000:0);put(m,0x7000,0x1481,direct?0x7000:0);
    m[p(0x7000,0x8000)]=(uint8_t)key;put(m,0x7000,0x8005,0);put(m,0x7000,0x8007,0);
    put(m,c.ds,0x5D96,0);put(m,c.ds,0x5D98,0);
    for(i=0;i<216u;++i)m[p(c.ds,0x671Bu+i)]=(uint8_t)(i*29u+7u);
    for(i=0;i<256u;++i){put(m,c.ds,0x6828u+4u*i,0x1000u+i);put(m,c.ds,0x682Au+4u*i,0xA000u+i);}
    put(m,c.ss,c.sp,0x7000);put(m,c.ss,c.sp+2u,0x9001);put(m,c.ss,c.sp+4u,0xA500u|key);
    put(m,c.ss,c.sp+6u,0x1400);put(m,c.ss,c.sp+8u,0x7000);
    { static const unsigned sites[]={0x311,0x325,0x332,0x33f,0x34c,0x360,0x36d,0x37a,0x387,0x394,0x3a1,0x3ae,0x3bb,0x3c8,0x3df,0x3ec,0x3f9,0x406,0x413,0x420,0x42d,0x43a,0x447,0x454,0x461,0x475,0x482,0x48f,0x4a6,0x4b3,0x4c0,0x4cd,0x4da,0x4e7,0x4f4,0x501,0x50e,0x51b,0x528,0x535,0x542,0x54f,0x566,0x573,0x580,0x58d,0x59a,0x5a7,0x5b4,0x5c1,0x5ce,0x5db,0x5e8,0x5f5,0x602,0x60f,0x61c,0x629,0x63d,0x64a,0x657,0x664,0x678,0x685,0x692,0x69f,0x6ac,0x6b9,0x6d0,0x6dd,0x6ea,0x6f7,0x704,0x711,0x71e,0x72b,0x738,0x745,0x759,0x766,0x773,0x780,0x78d,0x79a,0x7a7,0x7b4,0x7c1,0x7d5,0x7e2,0x7ef,0x7fc,0x809,0x816,0x823,0x83a,0x847,0x854,0x861,0x86e,0x87b,0x888,0x895,0x8a2,0x8af,0x8bc,0x8c9,0x8d6,0x8e3,0x8f0,0x904,0x911,0x91e,0x92b,0x938,0x94c,0x959,0x966,0x973,0x987,0x994,0x9a1,0x9ae,0x9bb,0x9cf,0x9dc,0x9e9,0x9f6,0xa03,0xa10,0xa24,0xa31,0xa3e,0xa51,0xa5e,0xa6b,0xa7e,0xa8b,0xa98,0xaa5,0xab2};
      for(i=0;i<sizeof sites/sizeof sites[0];++i){m[p(c.cs,sites[i])]=0xE8;put(m,c.cs,sites[i]+1u,0x160u-sites[i]-3u);} }
    c.ip=0x2FA;
    put(m,c.ss,c.sp+4u,0x1400);put(m,c.ss,c.sp+6u,0x7000);put(m,c.ss,c.sp+8u,0xA500u|group);
    return c;
}
static int run(uint8_t *m,pr_u9e_0160_cpu *c,const pr_u9e_0160_limits *l,pr_u9e_0160_diagnostic *d)
{ return pr_u9e_sidecar_groups_02fa_cpu(m,PR_U9E_0160_RAM_SIZE,c,&layout,l,d); }
int main(void)
{
    uint8_t *m=malloc(PR_U9E_0160_RAM_SIZE),*copy=malloc(PR_U9E_0160_RAM_SIZE);
    pr_u9e_0160_cpu c,before;pr_u9e_0160_limits limits;pr_u9e_0160_diagnostic d;unsigned i;
    CHECK(m!=NULL && copy!=NULL);
    for(i=0;i<20u;++i) {
        c=setup(m,i,0,0);
        CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800A && c.bp==0x7777);
    }
    c=setup(m,255,0,0);memcpy(copy,m,PR_U9E_0160_RAM_SIZE);put(copy,c.ss,c.sp-2u,c.bp);
    CHECK(run(m,&c,&defaults,NULL)==PR_U9E_0160_OK);
    CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800A && c.bp==0x7777);
    CHECK(memcmp(copy,m,PR_U9E_0160_RAM_SIZE)==0);
    c=setup(m,1,71,1);
    CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_PENDING);
    CHECK(c.cs==0x8800 && c.ip==0x1E && c.bp==0x7EF8 && c.sp==0x7EEE);
    CHECK(c.ax==71 && c.di==4u*71u && (c.flags&0x10u)!=0u);
    CHECK(d.callee==0 && d.status==0 && d.handler_offset==0x1047 && d.handler_segment==0xA047);
    CHECK(rd(m,c.ss,c.sp+2u)==0x8000 && rd(m,c.ss,c.sp+4u)==0x7000);
    CHECK(rd(m,c.ss,c.sp+6u)==0x1400 && rd(m,c.ss,c.sp+8u)==0x7000);
    CHECK(rd(m,c.ss,0x7FF0)==0x34F && rd(m,c.ss,0x7FF2)==0x8800);
    /* Both fallback types 45 and 46 need one party visit: reset per +0160. */
    c=setup(m,11,0,0);limits=defaults;limits.party_visits=1;
    CHECK(run(m,&c,&limits,&d)==PR_U9E_0160_OK);
    c=setup(m,15,21,0);limits=defaults;limits.party_visits=0;
    CHECK(run(m,&c,&limits,&d)==PR_U9E_0160_LIMIT && c.ip==0x1C3);
    c=setup(m,0,126,1);limits=defaults;limits.lookup_loopbacks=0;
    CHECK(run(m,&c,&limits,&d)==PR_U9E_0160_CALLEE && d.callee==0 && d.status==2);
    CHECK(c.cs==0x9800 && c.ip==0x21B9);
    c=setup(m,0,126,1);m[p(c.cs,0x311)]=0x90;
    CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_CODE && c.ip==0x311 && c.sp==0x7FF2 && d.callee==-1);
    c=setup(m,0,126,1);put(m,c.cs,0x312,0);
    CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_CODE && c.ip==0x311);
    c=setup(m,0,126,1);m[p(c.cs,0x18B)]=0x90;
    CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_CODE && c.ip==0x18B);
    c=setup(m,0,126,1);put(m,c.ds,0x6828u+4u*126u,0xBEEF);put(m,c.ds,0x682Au+4u*126u,0xCAFE);
    CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_PENDING && d.handler_offset==0xBEEF && d.handler_segment==0xCAFE);
    c=setup(m,0,126,1);before=c;memcpy(copy,m,PR_U9E_0160_RAM_SIZE);
    CHECK(pr_u9e_sidecar_groups_02fa_cpu(NULL,PR_U9E_0160_RAM_SIZE,&c,&layout,&defaults,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(pr_u9e_sidecar_groups_02fa_cpu(m,PR_U9E_0160_RAM_SIZE,NULL,&layout,&defaults,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(pr_u9e_sidecar_groups_02fa_cpu(m,1,&c,&layout,&defaults,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(pr_u9e_sidecar_groups_02fa_cpu(m,PR_U9E_0160_RAM_SIZE,&c,NULL,&defaults,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(pr_u9e_sidecar_groups_02fa_cpu(m,PR_U9E_0160_RAM_SIZE,&c,&layout,NULL,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(copy,m,PR_U9E_0160_RAM_SIZE)==0);
    for(i=0;i<6u;++i) {
        c=before;limits=defaults;if(i==0)c.ip=0x2FB;if(i==1)c.sp=0x195;if(i==2)c.sp=0xFFF7;
        if(i==3)limits.chain_iterations=0;
        if(i==4)limits.line_iterations=0;
        if(i==5)limits.sort_loopbacks=0;
        {pr_u9e_0160_cpu rejected=c;CHECK(run(m,&c,&limits,&d)==PR_U9E_0160_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && memcmp(copy,m,PR_U9E_0160_RAM_SIZE)==0);
         CHECK(d.callee==-1 && d.status==-1 && d.handler_offset==0 && d.handler_segment==0);}
    }
    free(copy);free(m);printf("%u semantic checks passed\n",checks);return 0;
}

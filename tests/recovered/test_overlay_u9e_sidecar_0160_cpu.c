#include "poolrad/recovered/overlay_u9e_sidecar_0160_cpu.h"
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
static pr_u9e_0160_cpu setup(uint8_t *m,unsigned key,unsigned direct)
{
    pr_u9e_0160_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8800,0x160,0x5000,0x1111,0x2000,0x602};
    static const unsigned calls[][3]={{0x18B,0x2199,0x9800},{0x1A5,0xDC0,0x6000},{0x1BE,0x1685,0x6000},
        {0x1EC,0x2199,0x9800},{0x21C,0xBE7,0x3000},{0x22A,0xC0F,0x3000},{0x242,0xC37,0x3000},
        {0x250,0x909,0x4000},{0x274,0xC5F,0x3000},{0x2CF,0x1685,0x6000}};
    unsigned i;memset(m,0xA5,PR_U9E_0160_RAM_SIZE);
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
    return c;
}
static int run(uint8_t *m,pr_u9e_0160_cpu *c,const pr_u9e_0160_limits *l,pr_u9e_0160_diagnostic *d)
{ return pr_u9e_sidecar_0160_cpu(m,PR_U9E_0160_RAM_SIZE,c,&layout,l,d); }
int main(void)
{
    uint8_t *m=malloc(PR_U9E_0160_RAM_SIZE),*copy=malloc(PR_U9E_0160_RAM_SIZE),scratch[216];
    pr_u9e_0160_cpu c,before;pr_u9e_0160_limits limits;pr_u9e_0160_diagnostic d;unsigned i;
    CHECK(m!=NULL && copy!=NULL);
    for(i=0;i<256u;++i) {
        c=setup(m,i,1);CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_PENDING);
        CHECK(c.cs==0x8800 && c.ip==0x1E && c.bp==0x7F08 && c.sp==0x7EFE);
        CHECK(c.ax==i && c.di==4u*i && (c.flags&0x10u)!=0u && (c.flags&0x600u)==0x600u);
        CHECK(d.callee==0 && d.status==0 && d.handler_offset==0x1000u+i && d.handler_segment==0xA000u+i);
        CHECK(rd(m,c.ss,c.sp)==0xF000 && rd(m,c.ss,c.sp+2u)==0x8000 && rd(m,c.ss,c.sp+4u)==0x7000);
        CHECK(rd(m,c.ss,c.sp+6u)==0x1400 && rd(m,c.ss,c.sp+8u)==0x7000);
        CHECK(rd(m,c.ss,0x7F0A)==0x2F4 && rd(m,c.ss,0x7F0C)==0x8800);
    }
    for(i=0;i<256u;++i) {
        c=setup(m,i,0);memcpy(scratch,m+p(c.ds,0x671B),sizeof scratch);
        CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800A && c.bp==0x7777);
        CHECK(memcmp(scratch,m+p(c.ds,0x671B),sizeof scratch)==0);
    }
    c=setup(m,21,0);put(m,c.ds,0x5D96,0x1200);put(m,c.ds,0x5D98,0x7000);
    put(m,0x7000,0x127F,0x8200);put(m,0x7000,0x1281,0x7000);m[p(0x7000,0x8200)]=21;
    put(m,0x7000,0x8205,0);put(m,0x7000,0x8207,0);put(m,0x7000,0x1304,0);put(m,0x7000,0x1306,0);
    m[p(c.ds,0x49F3)]=0;memcpy(scratch,m+p(c.ds,0x671B),sizeof scratch);
    CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_PENDING);
    CHECK(rd(m,c.ss,c.sp+2u)==0x8200 && memcmp(scratch,m+p(c.ds,0x671B),sizeof scratch)==0);
    CHECK(memcmp(scratch,m+p(c.ss,0x7F26),sizeof scratch)==0);
    c=setup(m,21,1);limits=defaults;limits.lookup_loopbacks=0;
    CHECK(run(m,&c,&limits,&d)==PR_U9E_0160_CALLEE && d.callee==0 && d.status==2);
    CHECK(c.cs==0x9800 && c.ip==0x21B9);
    c=setup(m,21,0);limits=defaults;limits.party_visits=0;
    CHECK(run(m,&c,&limits,&d)==PR_U9E_0160_LIMIT && c.ip==0x1C3);
    c=setup(m,21,1);m[p(c.cs,0x18B)]=0x90;
    CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_CODE && c.ip==0x18B && d.callee==-1);
    c=setup(m,21,1);m[p(c.cs,0x2F1)]=0x90;
    CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_CODE && c.ip==0x2F1);
    c=setup(m,21,1);put(m,c.ds,0x6828u+4u*21u,0xBEEF);put(m,c.ds,0x682Au+4u*21u,0xCAFE);
    CHECK(run(m,&c,&defaults,&d)==PR_U9E_0160_PENDING && d.handler_offset==0xBEEF && d.handler_segment==0xCAFE);
    c=setup(m,21,1);before=c;memcpy(copy,m,PR_U9E_0160_RAM_SIZE);
    CHECK(pr_u9e_sidecar_0160_cpu(NULL,PR_U9E_0160_RAM_SIZE,&c,&layout,&defaults,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(pr_u9e_sidecar_0160_cpu(m,PR_U9E_0160_RAM_SIZE,NULL,&layout,&defaults,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(pr_u9e_sidecar_0160_cpu(m,1,&c,&layout,&defaults,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(pr_u9e_sidecar_0160_cpu(m,PR_U9E_0160_RAM_SIZE,&c,NULL,&defaults,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(pr_u9e_sidecar_0160_cpu(m,PR_U9E_0160_RAM_SIZE,&c,&layout,NULL,&d)==PR_U9E_0160_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(copy,m,PR_U9E_0160_RAM_SIZE)==0);
    for(i=0;i<6u;++i) {
        c=before;limits=defaults;if(i==0)c.ip=0x161;if(i==1)c.sp=0x185;if(i==2)c.sp=0xFFF7;
        if(i==3)limits.chain_iterations=0;
        if(i==4)limits.line_iterations=0;
        if(i==5)limits.sort_loopbacks=0;
        {pr_u9e_0160_cpu rejected=c;CHECK(run(m,&c,&limits,&d)==PR_U9E_0160_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && memcmp(copy,m,PR_U9E_0160_RAM_SIZE)==0);}
    }
    free(copy);free(m);printf("%u semantic checks passed\n",checks);return 0;
}

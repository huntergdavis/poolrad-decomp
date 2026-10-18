#include "poolrad/recovered/overlay_u5e_caller_32e6.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned seg,unsigned off) { return ((size_t)seg*16u+(off&65535u))&1048575u; }
static void put(uint8_t *m,unsigned seg,unsigned off,unsigned v) { size_t a=p(seg,off);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8u); }
static pr_u5e_32e6_cpu setup(uint8_t *m,unsigned count,unsigned sp,unsigned ss)
{
    pr_u5e_32e6_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,(uint16_t)sp,0x1000,0x32E6,0x5000,0x1111,(uint16_t)ss,0x602};
    static const unsigned shape[8]={0,0,1,0,0,1,255,0};
    static const unsigned dx[9]={0,1,1,1,0,255,255,255,0},dy[9]={255,255,0,1,1,1,0,255,0};
    unsigned i,j;unsigned frame[5]={0x9000,0xA000,0xE000,0x1020,0x7000};
    memset(m,0xA5,PR_U5E_32E6_RAM_SIZE);
    for(i=0;i<256u;++i)for(j=0;j<8u;++j)m[p(c.ds,0x2844u+8u*i+j)]=(uint8_t)shape[j];
    for(i=0;i<9u;++i){m[p(c.ds,0x2736u+i)]=(uint8_t)dx[i];m[p(c.ds,0x273Fu+i)]=(uint8_t)dy[i];m[p(c.ds,0x25C0u+i)]=0;}
    for(i=0;i<64u;++i){m[p(c.ds,0x2745u+4u*i)]=0;m[p(c.ds,0x2746u+4u*i)]=0;}
    for(i=0;i<0x1800u;++i)m[p(c.ds,0x8000u+i)]=0;
    m[p(c.ds,0x8006u)]=1;m[p(c.ds,0x5F2Au)]=(uint8_t)count;
    for(i=1;i<=count;++i){m[p(c.ds,0x5F27u+4u*i)]=12;m[p(c.ds,0x5F28u+4u*i)]=12;m[p(c.ds,0x5F2Au+4u*i)]=1;}
    if(count){m[p(c.ds,0x5F2B)]=10;m[p(c.ds,0x5F2C)]=10;}
    for(i=0;i<256u;++i){put(m,c.ds,0x65B9u+4u*i,0x1000u+32u*i);put(m,c.ds,0x65BBu+4u*i,0x7000);}
    put(m,c.ds,0x6716,0x8000);put(m,c.ds,0x6718,c.ds);
    for(i=0;i<5u;++i)put(m,ss,sp+2u*i,frame[i]);
    /* The accepted sorter validates these actual Move call operands in RAM. */
    for(i=0;i<3u;++i){
        static const unsigned sites[3]={0x131,0x160,0x181};
        unsigned at=sites[i];
        m[p(0x4000,at)]=0x9A;put(m,0x4000,at+1u,0xA3F);put(m,0x4000,at+3u,0x6000);
    }
    return c;
}
static const pr_u5e_32e6_layout layout={0x3000,0x4000,0x6000};
static const pr_u5e_32e6_limits defaults={256,20000,512,65536,256};
static int run(uint8_t *m,pr_u5e_32e6_cpu *c,const pr_u5e_32e6_limits *limits)
{ return (int)pr_u5e_caller_32e6(m,PR_U5E_32E6_RAM_SIZE,c,&layout,limits); }
int main(void)
{
    uint8_t *m=malloc(PR_U5E_32E6_RAM_SIZE),*copy=malloc(PR_U5E_32E6_RAM_SIZE);
    pr_u5e_32e6_cpu c,before;pr_u5e_32e6_limits limits;unsigned i;
    const uint8_t records[6]={1,0,0,2,5,3};
    CHECK(m!=NULL && copy!=NULL);
    c=setup(m,2,0xF000,0x2000);CHECK(run(m,&c,&defaults)==0);
    CHECK(m[p(c.ds,0x671A)]==2 && memcmp(m+p(c.ds,0x671B),records,6)==0);
    CHECK(m[p(c.ss,0xDF26)]==2 && m[p(c.ss,0xDF27)]==0xA5);
    CHECK(memcmp(m+p(c.ss,0xDF28),records,6)==0 && m[p(c.ss,0xDF2E)]==0xA5);
    CHECK(c.cs==0xA000 && c.ip==0x9000 && c.sp==0xF00A && c.bp==0x7777);
    CHECK(c.ds==0x5000 && c.ss==0x2000 && c.es==0x2000 && c.flags==0x246);
    CHECK(c.cx==0 && c.si==0x6721 && c.di==0xDF2E && c.bx==0xEFEE);
    c=setup(m,0,0xF000,0x2000);CHECK(run(m,&c,&defaults)==0);
    CHECK(m[p(c.ss,0xDF26)]==0 && m[p(c.ss,0xDF28)]==0xA5 && c.flags==0x602);
    c=setup(m,2,0xF000,0x2000);put(m,c.ss,c.sp+4u,0xF0DA);
    CHECK(run(m,&c,&defaults)==0);
    CHECK(c.cs==1 && c.ip==0x9002 && c.sp==0xF00A);
    /* The first copy overwrites the parent pointer in the caller arguments;
     * the second copy must reload it, redirecting the output into EFxx. */
    CHECK(c.di==0xEF2E && m[p(c.ss,0xEF2B)]==2 && m[p(c.ss,0xEF2C)]==5);
    c=setup(m,2,0xF000,0x2000);put(m,c.ss,c.sp+6u,0x3000);limits=defaults;limits.slot_iterations=1;
    CHECK(run(m,&c,&limits)==PR_U5E_32E6_SLOT_LIMIT);
    CHECK(c.cs==0x3000 && c.ip==0xC71 && c.sp==0xEFE2 && c.bp==0xEFE6);
    CHECK(m[p(c.ss,0xEFE3)]==1 && m[p(c.ss,0xDF26)]==0xA5);
    c=setup(m,2,0xF000,0x2000);limits=defaults;limits.chain_iterations=1;
    CHECK(run(m,&c,&limits)==PR_U5E_32E6_CHAIN_LIMIT);
    CHECK(c.cs==0x4000 && c.ip==0x918 && c.sp==0xEFC8 && c.bp==0xEFE8);
    c=setup(m,2,0xF000,0x2000);limits=defaults;limits.line_iterations=1;
    CHECK(run(m,&c,&limits)==PR_U5E_32E6_LINE_LIMIT);
    CHECK(c.cs==0x4000 && c.ip==0x4DD && c.sp==0xEF74 && c.bp==0xEFAE);
    c=setup(m,5,0xF000,0x2000);
    for(i=1;i<=5u;++i){m[p(c.ds,0x5F27u+4u*i)]=(uint8_t)(21u-i);m[p(c.ds,0x5F28u+4u*i)]=(uint8_t)(21u-i);}
    limits=defaults;limits.sort_loopbacks=1;CHECK(run(m,&c,&limits)==PR_U5E_32E6_SORT_LIMIT);
    CHECK(c.cs==0x4000 && c.ip==0x72);
    c=setup(m,2,0xF000,0x2000);limits=defaults;limits.copy_iterations=1;
    CHECK(run(m,&c,&limits)==PR_U5E_32E6_COPY_LIMIT);
    CHECK(c.cs==0x1000 && c.ip==0x3349 && c.sp==0xEFFC && c.bp==0xEFFE);
    CHECK(m[p(c.ss,0xDF26)]==2 && memcmp(m+p(c.ss,0xDF28),records,3)==0 && m[p(c.ss,0xDF2B)]==0xA5);
    CHECK(m[p(c.ss,0xEFFD)]==2 && c.flags==0x203);
    for(i=0;i<4u;++i){
        static const unsigned sp[4]={0xA4,0xA5,0xFFF5,0xFFF6};
        c=setup(m,2,sp[i],0x2000);CHECK(run(m,&c,&defaults)==0);
        CHECK(c.sp==(uint16_t)(sp[i]+10u) && c.cs==0xA000 && c.ip==0x9000);
    }
    c=setup(m,2,0xF000,0x2000);before=c;memcpy(copy,m,PR_U5E_32E6_RAM_SIZE);
    CHECK(pr_u5e_caller_32e6(NULL,PR_U5E_32E6_RAM_SIZE,&c,&layout,&defaults)==PR_U5E_32E6_ARGUMENT);
    CHECK(pr_u5e_caller_32e6(m,PR_U5E_32E6_RAM_SIZE,NULL,&layout,&defaults)==PR_U5E_32E6_ARGUMENT);
    CHECK(pr_u5e_caller_32e6(m,1,&c,&layout,&defaults)==PR_U5E_32E6_ARGUMENT);
    CHECK(pr_u5e_caller_32e6(m,PR_U5E_32E6_RAM_SIZE,&c,NULL,&defaults)==PR_U5E_32E6_ARGUMENT);
    CHECK(pr_u5e_caller_32e6(m,PR_U5E_32E6_RAM_SIZE,&c,&layout,NULL)==PR_U5E_32E6_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof(c))==0 && memcmp(m,copy,PR_U5E_32E6_RAM_SIZE)==0);
    for(i=0;i<8u;++i){
        limits=defaults;c=before;
        if(i==0)limits.slot_iterations=0;
        if(i==1)limits.chain_iterations=0;
        if(i==2)limits.line_iterations=0;
        if(i==3)limits.sort_loopbacks=0;
        if(i==4)limits.copy_iterations=0;
        if(i==5)c.sp=0xA3;
        if(i==6)c.sp=0xFFF7;
        if(i==7)c.ip=0x32E7;
        {pr_u5e_32e6_cpu rejected=c;CHECK(run(m,&c,&limits)==PR_U5E_32E6_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof(c))==0 && memcmp(m,copy,PR_U5E_32E6_RAM_SIZE)==0);}
    }
    free(copy);free(m);printf("%u semantic checks passed\n",checks);return 0;
}

#include "loaded_stub_fixture.h"
#include "poolrad/recovered/overlay_u3c2_caller_0909.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned seg,unsigned off) { return ((size_t)seg*16u+(off&65535u))&1048575u; }
static void put(uint8_t *m,unsigned seg,unsigned off,unsigned v) { size_t a=p(seg,off);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8u); }
static pr_u3c2_0909_cpu setup(uint8_t *m,unsigned count,unsigned sp,unsigned ss)
{
    pr_u3c2_0909_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,(uint16_t)sp,0x3000,0x909,0x5000,0x1111,(uint16_t)ss,0x602};
    static const unsigned shape[8]={0,0,1,0,0,1,255,0};
    static const unsigned dx[9]={0,1,1,1,0,255,255,255,0},dy[9]={255,255,0,1,1,1,0,255,0};
    unsigned i,j;unsigned frame[9]={0x9000,0xA000,0x8000,0x5000,1,8,100,10,10};
    memset(m,0xA5,PR_U3C2_0909_RAM_SIZE);
    test_loaded_stub(m, c.cs, 0x092Au, (uint16_t)(0x4000u + 0x100u), 0x0025u, 0x4000u, 0x0000u);
    test_loaded_stub(m, c.cs, 0x09D8u, (uint16_t)(0x4000u + 0x100u), 0x0025u, 0x4000u, 0x0000u);
    for(i=0;i<256u;++i)for(j=0;j<8u;++j)m[p(c.ds,0x2844u+8u*i+j)]=(uint8_t)shape[j];
    for(i=0;i<9u;++i){m[p(c.ds,0x2736u+i)]=(uint8_t)dx[i];m[p(c.ds,0x273Fu+i)]=(uint8_t)dy[i];m[p(c.ds,0x25C0u+i)]=0;}
    for(i=0;i<64u;++i){m[p(c.ds,0x2745u+4u*i)]=0;m[p(c.ds,0x2746u+4u*i)]=0;}
    for(i=0;i<0x1800u;++i)m[p(c.ds,0x8000u+i)]=0;
    m[p(c.ds,0x8006u)]=1;m[p(c.ds,0x5F2Au)]=(uint8_t)count;
    for(i=1;i<=count;++i){m[p(c.ds,0x5F27u+4u*i)]=12;m[p(c.ds,0x5F28u+4u*i)]=12;m[p(c.ds,0x5F2Au+4u*i)]=1;}
    for(i=0;i<9u;++i)put(m,ss,sp+2u*i,frame[i]);
    /* The accepted sorter validates these actual Move call operands in RAM. */
    for(i=0;i<3u;++i){
        static const unsigned sites[3]={0x131,0x160,0x181};
        unsigned at=sites[i];
        m[p(c.cs,at)]=0x9A;put(m,c.cs,at+1u,0xA3F);put(m,c.cs,at+3u,0x6000);
    }
    return c;
}
static int run(uint8_t *m,pr_u3c2_0909_cpu *c,size_t caller,size_t line,unsigned sort)
{ return (int)pr_u3c2_caller_0909(m,PR_U3C2_0909_RAM_SIZE,c,0x4000,caller,line,sort); }
int main(void)
{
    uint8_t *m=malloc(PR_U3C2_0909_RAM_SIZE),*copy=malloc(PR_U3C2_0909_RAM_SIZE);
    pr_u3c2_0909_cpu c,before;unsigned i;
    CHECK(m!=NULL && copy!=NULL);
    c=setup(m,1,0xF000,0x2000);CHECK(run(m,&c,20000,512,65536)==0);
    CHECK(m[p(c.ds,0x671A)]==1);
    CHECK(m[p(c.ds,0x671B)]==1 && m[p(c.ds,0x671C)]==5 && m[p(c.ds,0x671D)]==3);
    CHECK(c.cs==0xA000 && c.ip==0x9000 && c.sp==0xF012 && c.bp==0x7777);
    CHECK(c.ds==0x5000 && c.ss==0x2000 && c.flags==0x646);
    c=setup(m,0,0xF000,0x2000);CHECK(run(m,&c,20000,512,65536)==0);
    CHECK(m[p(c.ds,0x671A)]==0 && c.bx==0x3456 && c.si==0x9ABC);
    CHECK(c.flags==0x697);
    c=setup(m,1,0xF000,0x2000);put(m,c.ss,c.sp+8u,0);
    CHECK(run(m,&c,20000,512,65536)==0);CHECK(m[p(c.ds,0x671A)]==0);
    c=setup(m,3,0xF000,0x2000);
    m[p(c.ds,0x5F2B)]=20;m[p(c.ds,0x5F2C)]=20;
    m[p(c.ds,0x5F2F)]=11;m[p(c.ds,0x5F30)]=11;m[p(c.ds,0x5F36)]=0;
    CHECK(run(m,&c,20000,512,65536)==0);
    CHECK(m[p(c.ds,0x671A)]==2);
    { const uint8_t expected[6]={2,2,4,1,29,3};CHECK(memcmp(m+p(c.ds,0x671B),expected,6)==0); }
    CHECK(c.flags==0x246 && c.cx==0 && c.ds==0x5000);
    c=setup(m,1,0xF000,0x2000);CHECK(run(m,&c,1,512,65536)==PR_U3C2_0909_CALLER_LIMIT);
    CHECK(c.cs==0x3000 && c.ip==0x918 && c.bp==0xEFFE && c.sp==0xEFDE);
    CHECK(m[p(c.ss,0xEFFB)]==1 && m[p(c.ds,0x671A)]==0xA5);
    c=setup(m,1,0xF000,0x2000);CHECK(run(m,&c,25,512,65536)==PR_U3C2_0909_CALLER_LIMIT);
    CHECK(c.ip==0xB73 && m[p(c.ds,0x671A)]==1);
    c=setup(m,1,0xF000,0x2000);CHECK(run(m,&c,20000,1,65536)==PR_U3C2_0909_LINE_LIMIT);
    CHECK(c.cs==0x3000 && c.ip==0x4DD && c.bp==0xEFC4 && c.sp==0xEF8A);
    CHECK(m[p(c.ds,0x671A)]==0);
    c=setup(m,5,0xF000,0x2000);
    for(i=1;i<=5u;++i){m[p(c.ds,0x5F27u+4u*i)]=(uint8_t)(21u-i);m[p(c.ds,0x5F28u+4u*i)]=(uint8_t)(21u-i);}
    CHECK(run(m,&c,20000,512,1)==PR_U3C2_0909_SORT_LIMIT);
    CHECK(c.ip==0x72 && m[p(c.ds,0x671A)]==5 && c.sp==0xEFCE);
    c=setup(m,1,0x671B,0x5000);CHECK(run(m,&c,256,32,128)==0);
    CHECK(c.cs==0xA003 && c.ip==0x501 && c.bp==0x177);
    c=setup(m,1,0xF000,0x2000);before=c;memcpy(copy,m,PR_U3C2_0909_RAM_SIZE);
    CHECK(pr_u3c2_caller_0909(NULL,PR_U3C2_0909_RAM_SIZE,&c,0x4000,1,1,1)==PR_U3C2_0909_ARGUMENT);
    CHECK(pr_u3c2_caller_0909(m,PR_U3C2_0909_RAM_SIZE,NULL,0x4000,1,1,1)==PR_U3C2_0909_ARGUMENT);
    CHECK(pr_u3c2_caller_0909(m,1,&c,0x4000,1,1,1)==PR_U3C2_0909_ARGUMENT);
    CHECK(run(m,&c,0,1,1)==PR_U3C2_0909_ARGUMENT);
    CHECK(run(m,&c,1,0,1)==PR_U3C2_0909_ARGUMENT);
    CHECK(run(m,&c,1,1,0)==PR_U3C2_0909_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof(c))==0 && memcmp(m,copy,PR_U3C2_0909_RAM_SIZE)==0);
    for(i=0;i<3u;++i){
        c=setup(m,1,0xF000,0x2000);if(i==0)c.sp=0x8D;if(i==1)c.sp=0xFFEF;if(i==2)c.ip=0x90A;before=c;
        CHECK(run(m,&c,1,1,1)==PR_U3C2_0909_ARGUMENT);
        CHECK(memcmp(&c,&before,sizeof(c))==0 && memcmp(m,copy,PR_U3C2_0909_RAM_SIZE)==0);
    }
    free(copy);free(m);printf("%u semantic checks passed\n",checks);return 0;
}

#include "poolrad/recovered/overlay_u7f_caller_1010.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} }while(0)
static size_t physical(unsigned s,unsigned o){return ((size_t)s*16u+(o&65535u))&1048575u;}
static void put(uint8_t *m,unsigned s,unsigned o,unsigned v)
{size_t a=physical(s,o);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8u);}
static unsigned get(const uint8_t *m,unsigned s,unsigned o)
{size_t a=physical(s,o);return m[a]+256u*m[(a+1u)&1048575u];}
static pr_u7f_1010_cpu setup(uint8_t *m,unsigned type,unsigned seg,unsigned off)
{
    pr_u7f_1010_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
        0x7777,0xF000,0x3000,0x1010,0x5000,0x1111,0x2000,0x602};
    unsigned i;memset(m,0xA5,PR_U7F_1010_RAM_SIZE);
    put(m,c.ss,c.sp,0x9000);put(m,c.ss,c.sp+2u,0xA000);
    put(m,c.ss,c.sp+4u,off);put(m,c.ss,c.sp+6u,seg);put(m,c.ss,c.sp+8u,type);
    for(i=0;i<512u;++i)put(m,c.ds,0x970u+2u*i,i*317u+19u);
    put(m,c.ds,0x4428,0);put(m,c.ds,0x442A,0);
    put(m,0xAF8,0x1245,0x8405);
    /* Synthetic loaded call and jump binding, not copied game code. */
    m[physical(c.cs,0xFDE)]=0x9A;put(m,c.cs,0xFDF,0x48);put(m,c.cs,0xFE1,0x4100);
    m[physical(0x4100,0x48)]=0xEA;put(m,0x4100,0x49,0xDE3);put(m,0x4100,0x4B,0x4000);
    return c;
}
int main(void)
{
    uint8_t *m=malloc(PR_U7F_1010_RAM_SIZE),*copy=malloc(PR_U7F_1010_RAM_SIZE);
    pr_u7f_1010_cpu c,before;unsigned i;
    CHECK(m!=NULL && copy!=NULL);
    c=setup(m,0,0x5000,0x8000);
    CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,1024,256)==PR_U7F_1010_OK);
    CHECK(c.cs==0xA000 && c.ip==0x9000 && c.sp==0xF00A && c.bp==0x7777);
    CHECK(c.ds==0x5000 && c.ss==0x2000 && c.si==0x9ABC);
    for(i=0;i<63u;++i)CHECK(m[physical(0x5000,0x8000+i)]==(i==0x35u?6u:0u));
    CHECK(m[physical(0x5000,0x7FFF)]==0xA5 && m[physical(0x5000,0x803F)]==0xA5);
    c=setup(m,1,0x5000,0x8000);
    CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,1024,256)==PR_U7F_1010_OK);
    CHECK(m[physical(c.ds,0x8032)]==1 && get(m,c.ds,0x8037)==75 && get(m,c.ds,0x803A)==2000);
    CHECK(get(m,c.ds,0x4428)==1 && get(m,c.ds,0x442A)==0);
    for(i=0;i<2u;++i){
        unsigned t=i==0u?63u:92u,base=i==0u?0x49u:0x61u;
        c=setup(m,t,0x5000,0x8000);
        CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,1024,256)==PR_U7F_1010_OK);
        CHECK(m[physical(c.ds,0x802E)]==t);
        CHECK(m[physical(c.ds,0x802F)]==(uint8_t)(base*317u+19u));
        CHECK(get(m,c.ds,0x8037)==(base+3u)*317u+19u);
        CHECK(get(m,c.ds,0x803A)==(base+4u)*317u+19u);
        CHECK(m[physical(c.ds,0x8032)]==i+1u && m[physical(c.ds,0x8033)]==i+1u);
        CHECK(m[physical(c.ds,0x803E)]==(uint8_t)((base+7u)*317u+19u));
    }
    c=setup(m,63,0x5000,0x8000);
    CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,1,256)==PR_U7F_1010_CALLER_LIMIT);
    CHECK(c.ip==0x181A && c.cs==0x3000 && c.sp==0xEFF4);
    CHECK(m[physical(c.ds,0x802F)]==(uint8_t)(0x49u*317u+19u));
    CHECK(m[physical(c.ds,0x8030)]==(uint8_t)(0x4Au*317u+19u));
    CHECK(m[physical(c.ds,0x8031)]==0);
    c=setup(m,1,0x5000,0x8000);
    CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,1024,0)==PR_U7F_1010_DICE_LIMIT);
    CHECK(c.cs==0x4000 && c.ip==0xE0E);
    CHECK(get(m,c.ds,0x4428)==0 && get(m,c.ds,0x442A)==0);
    /* FillChar really destroys its own far return, exposing that transfer. */
    c=setup(m,0,0x2000,0xEFE8);
    CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,1024,256)==PR_U7F_1010_TRANSFER);
    CHECK(c.cs==0 && c.ip==0 && c.sp==0xEFF4);
    c=setup(m,1,0x5000,0x8000);m[physical(0x4100,0x48)]=0;
    CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,1024,256)==PR_U7F_1010_CALLEE);
    CHECK(c.cs==0x3000 && c.ip==0xFDE);
    c=setup(m,0,0x5000,0x8000);before=c;memcpy(copy,m,PR_U7F_1010_RAM_SIZE);
    CHECK(pr_u7f_caller_1010(NULL,PR_U7F_1010_RAM_SIZE,&c,0x4000,1024,256)==PR_U7F_1010_ARGUMENT);
    CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,NULL,0x4000,1024,256)==PR_U7F_1010_ARGUMENT);
    CHECK(pr_u7f_caller_1010(m,1,&c,0x4000,1024,256)==PR_U7F_1010_ARGUMENT);
    CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,0,256)==PR_U7F_1010_ARGUMENT);
    CHECK(memcmp(&before,&c,sizeof(c))==0 && memcmp(copy,m,PR_U7F_1010_RAM_SIZE)==0);
    for(i=0;i<3u;++i){c=before;if(i==0u)c.sp=0x3F;if(i==1u)c.sp=0xFFF7;if(i==2u)c.ip=0x1011;
        {pr_u7f_1010_cpu saved=c;
        CHECK(pr_u7f_caller_1010(m,PR_U7F_1010_RAM_SIZE,&c,0x4000,1024,256)==PR_U7F_1010_ARGUMENT);
        CHECK(memcmp(&saved,&c,sizeof(c))==0 && memcmp(copy,m,PR_U7F_1010_RAM_SIZE)==0);}}
    free(copy);free(m);printf("%u semantic checks passed\n",checks);return 0;
}

#include "poolrad/recovered/overlay_u7f_clear_0508.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while (0)
static size_t physical(unsigned seg,unsigned off) { return ((size_t)seg*16u+(off&65535u))&1048575u; }
static void put(uint8_t *m,unsigned seg,unsigned off,unsigned value)
{ size_t a=physical(seg,off);m[a]=(uint8_t)value;m[(a+1u)&1048575u]=(uint8_t)(value>>8u); }
static unsigned get(const uint8_t *m,unsigned seg,unsigned off)
{ size_t a=physical(seg,off);return m[a]+256u*m[(a+1u)&1048575u]; }
static pr_u7f_0508_cpu setup(uint8_t *m,unsigned kind,unsigned seg,unsigned off)
{
    pr_u7f_0508_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                      0x7777,0xF000,0x3000,0x508,0x5000,0x1111,0x2000,0x602};
    unsigned i;
    memset(m,0xA5,PR_U7F_0020_RAM_SIZE);
    put(m,c.ss,c.sp,0x9000);put(m,c.ss,c.sp+2u,0xA000);
    put(m,c.ds,0x5D96,off);put(m,c.ds,0x5D98,seg);
    m[physical(seg,off+0x84u)]=(uint8_t)kind;
    put(m,seg,off+0x102u,100);put(m,seg,off+0x104u,0);put(m,seg,off+0x106u,0);
    for(i=0;i<7u;++i) {
        put(m,seg,off+0x88u+2u*i,i+1u);
        put(m,c.ds,0x67F4u+4u*i,0xFFFF);put(m,c.ds,0x67F6u+4u*i,0xFFFF);
    }
    return c;
}
int main(void)
{
    uint8_t *m=malloc(PR_U7F_0020_RAM_SIZE),*copy=malloc(PR_U7F_0020_RAM_SIZE);
    pr_u7f_0508_cpu c,before;unsigned i,k;
    CHECK(m!=NULL && copy!=NULL);
    for(k=0;k<3u;++k) {
        c=setup(m,k==0u?0u:k==1u?0xB3u:1u,0x5000,0x1000);
        CHECK(pr_u7f_clear_0508(m,PR_U7F_0020_RAM_SIZE,&c,0x4000,32,256)==PR_U7F_0508_OK);
        CHECK(c.ip==0x9000 && c.cs==0xA000 && c.sp==0xF004 && c.bp==0x7777);
        CHECK(c.si==0x9ABC && c.ds==0x5000 && c.ss==0x2000);
        CHECK(m[physical(c.ds,0x6DC0)]==1u);
        CHECK(get(m,0x5000,0x1102)==(k==2u?100u:72u));
        for(i=0;i<7u;++i) {
            CHECK(get(m,0x5000,0x1088+2u*i)==(k==2u?i+1u:0u));
            CHECK(get(m,c.ds,0x67F4+4u*i)==(k==2u?0xFFFFu:i));
            CHECK(get(m,c.ds,0x67F6+4u*i)==(k==2u?0xFFFFu:0u));
        }
        CHECK(c.flags==(k==2u?0x646u:0x246u));
    }
    c=setup(m,1,0x5000,0x1000);
    put(m,c.ds,0x1104,0x1000);put(m,c.ds,0x1106,c.ds);
    CHECK(pr_u7f_clear_0508(m,PR_U7F_0020_RAM_SIZE,&c,0x4000,3,256)==PR_U7F_0508_NODE_LIMIT);
    CHECK(c.ip==0x520 && c.cs==0x3000 && c.sp==0xEFF8);
    CHECK(get(m,c.ds,0x1102)==100u);
    c=setup(m,0,0x5000,0x1000);
    CHECK(pr_u7f_clear_0508(m,PR_U7F_0020_RAM_SIZE,&c,0x4000,32,1)==PR_U7F_0508_ITEM_LIMIT);
    CHECK(c.ip==0x54D && c.cs==0x3000 && c.sp==0xEFF8);
    CHECK(get(m,c.ds,0x1102)==99u && get(m,c.ds,0x1088)==1u);
    CHECK(m[physical(c.ss,0xEFF9)]==1u);
    /* The first subtract changes the actual nested return IP. */
    c=setup(m,0,0x2000,0xEFEE - 0x102);
    CHECK(pr_u7f_clear_0508(m,PR_U7F_0020_RAM_SIZE,&c,0x4000,32,256)==PR_U7F_0508_TRANSFER);
    CHECK(c.cs==0x3000 && c.ip==0x5AA && c.sp==0xEFF8);
    c=setup(m,0,0x5000,0x1000);before=c;memcpy(copy,m,PR_U7F_0020_RAM_SIZE);
    CHECK(pr_u7f_clear_0508(NULL,PR_U7F_0020_RAM_SIZE,&c,0x4000,32,256)==PR_U7F_0508_ARGUMENT);
    CHECK(pr_u7f_clear_0508(m,PR_U7F_0020_RAM_SIZE,NULL,0x4000,32,256)==PR_U7F_0508_ARGUMENT);
    CHECK(pr_u7f_clear_0508(m,1,&c,0x4000,32,256)==PR_U7F_0508_ARGUMENT);
    CHECK(pr_u7f_clear_0508(m,PR_U7F_0020_RAM_SIZE,&c,0x4000,0,256)==PR_U7F_0508_ARGUMENT);
    CHECK(pr_u7f_clear_0508(m,PR_U7F_0020_RAM_SIZE,&c,0x4000,32,0)==PR_U7F_0508_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof(c))==0 && memcmp(m,copy,PR_U7F_0020_RAM_SIZE)==0);
    for(i=0;i<3u;++i) {
        c=setup(m,0,0x5000,0x1000);if(i==0u)c.sp=0x13;if(i==1u)c.sp=0xFFFD;if(i==2u)c.ip=0x509;
        before=c;
        CHECK(pr_u7f_clear_0508(m,PR_U7F_0020_RAM_SIZE,&c,0x4000,32,256)==PR_U7F_0508_ARGUMENT);
        CHECK(memcmp(&c,&before,sizeof(c))==0 && memcmp(m,copy,PR_U7F_0020_RAM_SIZE)==0);
    }
    free(copy);free(m);printf("%u semantic checks passed\n",checks);return 0;
}

#include "poolrad/recovered/overlay_u3c6_rebuild_03a0.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned seg,unsigned off) { return ((size_t)seg*16u+(off&65535u))&1048575u; }
static void put(uint8_t *m,unsigned seg,unsigned off,unsigned v) { size_t a=p(seg,off);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8u); }
static pr_u3c6_03a0_cpu setup(uint8_t *m,unsigned count,unsigned sp,unsigned ss)
{
    pr_u3c6_03a0_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,(uint16_t)sp,0x3000,0x3A0,0x5000,0x1111,(uint16_t)ss,0x602};
    static const unsigned shape[8]={0,0,1,0,0,1,255,0};unsigned i,j;
    memset(m,0xA5,PR_U3C6_03A0_RAM_SIZE);
    for(i=0;i<256u;++i)for(j=0;j<8u;++j)m[p(c.ds,0x2844u+8u*i+j)]=(uint8_t)shape[j];
    m[p(c.ds,0x5F2Au)]=(uint8_t)count;
    for(i=1;i<=count;++i){m[p(c.ds,0x5F27u+4u*i)]=(uint8_t)(8u+2u*i);m[p(c.ds,0x5F28u+4u*i)]=(uint8_t)(8u+2u*i);m[p(c.ds,0x5F2Au+4u*i)]=1;}
    put(m,c.ds,0x6716,0x8000);put(m,c.ds,0x6718,c.ds);
    m[p(c.ds,0x8002)]=3;m[p(c.ds,0x8003)]=4;
    put(m,ss,sp,0x9000);put(m,ss,sp+2u,0xA000);return c;
}
static int run(uint8_t *m,pr_u3c6_03a0_cpu *c,size_t slots,size_t cells)
{ return (int)pr_u3c6_rebuild_03a0(m,PR_U3C6_03A0_RAM_SIZE,c,0x6000,slots,cells); }
static int clear_map(const uint8_t *m)
{ unsigned i;for(i=0;i<1250u;++i)if(m[p(0x5000,0x60DBu+i)]!=0u)return 0;return 1; }
int main(void)
{
    uint8_t *m=malloc(PR_U3C6_03A0_RAM_SIZE),*copy=malloc(PR_U3C6_03A0_RAM_SIZE);
    pr_u3c6_03a0_cpu c,before;unsigned i;
    CHECK(m!=NULL && copy!=NULL);
    c=setup(m,2,0xF000,0x2000);CHECK(run(m,&c,512,4096)==0);
    CHECK(m[p(c.ds,0x62D9)]==1 && m[p(c.ds,0x62DA)]==1 && m[p(c.ds,0x630B)]==1);
    CHECK(m[p(c.ds,0x633F)]==2 && m[p(c.ds,0x6340)]==2 && m[p(c.ds,0x6371)]==2);
    CHECK(m[p(c.ds,0x62D8)]==0 && m[p(c.ds,0x633E)]==0);
    CHECK(m[p(c.ds,0x604B)]==7 && m[p(c.ds,0x604C)]==9);
    CHECK(m[p(c.ds,0x6093)]==6 && m[p(c.ds,0x6094)]==8);
    CHECK(m[p(c.ds,0x60DA)]==0xA5 && m[p(c.ds,0x65BD)]==0xA5 && m[p(c.ds,0x65BC)]==0);
    CHECK(c.cs==0xA000 && c.ip==0x9000 && c.sp==0xF004 && c.bp==0x7777);
    CHECK(c.ds==0x5000 && c.es==0x5000 && c.ss==0x2000 && c.flags==0x246);
    CHECK(c.ax==2 && c.bx==0xEF02 && c.cx==3 && c.dx==8 && c.si==0x9ABC && c.di==2);
    c=setup(m,0,0xF000,0x2000);CHECK(run(m,&c,512,4096)==0);
    CHECK(clear_map(m) && m[p(c.ds,0x604B)]==0xA5 && m[p(c.ds,0x6093)]==0xA5);
    CHECK(c.ax==0x401 && c.bx==0xEFEC && c.cx==0 && c.di==0x65BD && c.flags==0x202);
    c=setup(m,1,0xF000,0x2000);m[p(c.ds,0x5F2E)]=0;CHECK(run(m,&c,512,4096)==0);
    CHECK(clear_map(m) && m[p(c.ds,0x604B)]==0xA5 && c.di==4 && c.flags==0x246);
    c=setup(m,1,0xF000,0x2000);
    for(i=0;i<4u;++i)m[p(c.ds,0x284Cu+2u*i)]=255;
    CHECK(run(m,&c,512,4096)==0 && clear_map(m));
    CHECK(m[p(c.ds,0x604B)]==7 && m[p(c.ds,0x6093)]==6);
    c=setup(m,2,0xF000,0x2000);m[p(c.ds,0x5F2F)]=10;m[p(c.ds,0x5F30)]=10;
    CHECK(run(m,&c,512,4096)==0 && m[p(c.ds,0x62D9)]==2 && m[p(c.ds,0x630B)]==2);
    c=setup(m,1,0xF000,0x2000);m[p(c.ds,0x5F2C)]=128;
    CHECK(run(m,&c,512,4096)==0);
    CHECK(m[p(c.ds,0x47E5)]==1 && m[p(c.ds,0x47E6)]==1 && m[p(c.ds,0x4817)]==1);
    CHECK(m[p(c.ds,0x6093)]==124); /* signed subtraction, byte result */
    c=setup(m,2,0xF000,0x2000);CHECK(run(m,&c,1,4096)==PR_U3C6_03A0_SLOT_LIMIT);
    CHECK(c.cs==0x3000 && c.ip==0x3CF && c.bp==0xEFFE && c.sp==0xEFF8 && c.flags==0x203);
    CHECK(m[p(c.ss,0xEFFD)]==2 && m[p(c.ds,0x633F)]==0 && m[p(c.ds,0x604C)]==0xA5);
    c=setup(m,2,0xF000,0x2000);CHECK(run(m,&c,512,1)==PR_U3C6_03A0_CELL_LIMIT);
    CHECK(c.cs==0x3000 && c.ip==0x3ED && c.bp==0xEFFE && c.sp==0xEFF8 && c.flags==0x203);
    CHECK(m[p(c.ss,0xEFFC)]==1 && m[p(c.ds,0x62D9)]==1 && m[p(c.ds,0x62DA)]==0);
    c=setup(m,2,0x60E0,0x5000);CHECK(run(m,&c,512,4096)==0);
    CHECK(c.cs==0 && c.ip==0 && c.bp==0 && c.sp==0x60E4);
    c=setup(m,2,0x65B9,0x5000);CHECK(run(m,&c,512,4096)==PR_U3C6_03A0_TRANSFER);
    CHECK(c.cs==0 && c.ip==0 && c.sp==0x65B1 && c.bp==0x65B7);
    CHECK(c.ax==0x400 && c.cx==0 && c.di==0x65BD && c.flags==0x206);
    for(i=0;i<4u;++i){
        static const unsigned sp[4]={0x1C,0x1D,0xFFFB,0xFFFC};
        c=setup(m,2,sp[i],0x2000);CHECK(run(m,&c,512,4096)==0);
        CHECK(c.sp==(uint16_t)(sp[i]+4u) && c.cs==0xA000 && c.ip==0x9000);
    }
    c=setup(m,2,0xF000,0x2000);before=c;memcpy(copy,m,PR_U3C6_03A0_RAM_SIZE);
    CHECK(pr_u3c6_rebuild_03a0(NULL,PR_U3C6_03A0_RAM_SIZE,&c,0x6000,1,1)==PR_U3C6_03A0_ARGUMENT);
    CHECK(pr_u3c6_rebuild_03a0(m,PR_U3C6_03A0_RAM_SIZE,NULL,0x6000,1,1)==PR_U3C6_03A0_ARGUMENT);
    CHECK(pr_u3c6_rebuild_03a0(m,1,&c,0x6000,1,1)==PR_U3C6_03A0_ARGUMENT);
    CHECK(run(m,&c,0,1)==PR_U3C6_03A0_ARGUMENT && run(m,&c,1,0)==PR_U3C6_03A0_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof(c))==0 && memcmp(m,copy,PR_U3C6_03A0_RAM_SIZE)==0);
    for(i=0;i<3u;++i){
        c=before;if(i==0)c.sp=0x1B;if(i==1)c.sp=0xFFFD;if(i==2)c.ip=0x3A1;
        {pr_u3c6_03a0_cpu rejected=c;CHECK(run(m,&c,1,1)==PR_U3C6_03A0_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof(c))==0 && memcmp(m,copy,PR_U3C6_03A0_RAM_SIZE)==0);}
    }
    free(copy);free(m);printf("%u semantic checks passed\n",checks);return 0;
}

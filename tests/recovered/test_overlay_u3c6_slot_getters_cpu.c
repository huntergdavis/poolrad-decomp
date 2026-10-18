#include "poolrad/recovered/overlay_u3c6_slot_getters_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static void put(uint8_t *m,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8u); }
static pr_u3c6_getter_cpu setup(uint8_t *m,unsigned entry,unsigned count,unsigned selected,unsigned sp,unsigned ss)
{
    pr_u3c6_getter_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,(uint16_t)sp,0x8800,(uint16_t)entry,0x1800,0x1111,(uint16_t)ss,0x602};
    unsigned i;memset(m,0xA5,PR_U3C6_GETTER_RAM_SIZE);
    for(i=0;i<256u;++i) {
        put(m,c.ds,0x65B9u+4u*i,0x1000u+32u*i);put(m,c.ds,0x65BBu+4u*i,0x5000);
        m[p(c.ds,0x5F27u+4u*i)]=(uint8_t)(17u*i+0x27u);
        m[p(c.ds,0x5F28u+4u*i)]=(uint8_t)(17u*i+0x28u);
        m[p(c.ds,0x5F2Au+4u*i)]=(uint8_t)(17u*i+0x2Au);
    }
    m[p(c.ds,0x5F2A)]=(uint8_t)count;
    put(m,ss,sp,0x7000);put(m,ss,sp+2u,0x9001);
    put(m,ss,sp+4u,0x1000u+32u*selected);put(m,ss,sp+6u,0x5000);
    return c;
}
static int run(uint8_t *m,pr_u3c6_getter_cpu *c,size_t visits) { return pr_u3c6_slot_getter_cpu(m,PR_U3C6_GETTER_RAM_SIZE,c,visits); }
int main(void)
{
    const unsigned entries[]={0xBE7,0xC0F,0xC37,0xC5F},counts[]={0,1,2,5,127,128,254,255};
    const unsigned bases[]={0x5F27,0x5F28,0x5F2A};
    uint8_t *m=malloc(PR_U3C6_GETTER_RAM_SIZE),*copy=malloc(PR_U3C6_GETTER_RAM_SIZE);
    pr_u3c6_getter_cpu c,before;unsigned e,a,b,i,want,index;
    CHECK(m!=NULL && copy!=NULL);
    for(e=0;e<4u;++e)for(a=0;a<8u;++a)for(b=0;b<8u;++b) {
        c=setup(m,entries[e],counts[a],counts[b],0x8000,0x2800);
        index=counts[a]==0u || (counts[b]!=0u && counts[b]<=counts[a])?counts[b]:0u;
        want=e==3u?index:m[p(c.ds,bases[e]+4u*index)];
        CHECK(run(m,&c,256)==PR_U3C6_GETTER_OK);
        CHECK((c.ax&255u)==want);
        if(e!=3u)CHECK(c.ax==want && c.di==4u*index && (c.flags&0x10u)!=0u);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8008 && c.bp==0x7777);
        CHECK(c.bx==0x3456 && c.cx==0x5678 && c.si==0x9ABC);
        CHECK(c.ds==0x1800 && c.es==0x1111 && c.ss==0x2800 && (c.flags&0x600u)==0x600u);
    }
    for(e=0;e<4u;++e)for(i=0;i<5u;++i) {
        static const unsigned sp[]={0x20,0x21,0x8001,0xFFF7,0xFFF8};
        c=setup(m,entries[e],5,3,sp[i],0xFFFF);
        CHECK(run(m,&c,3)==PR_U3C6_GETTER_OK);
        CHECK(c.sp==(uint16_t)(sp[i]+8u) && c.cs==0x9001 && c.ip==0x7000);
    }
    for(e=0;e<4u;++e) {
        c=setup(m,entries[e],5,3,0x8000,0x1800);
        CHECK(run(m,&c,3)==PR_U3C6_GETTER_OK);
        c=setup(m,entries[e],5,3,0x8000,0x2800);
        CHECK(run(m,&c,0)==PR_U3C6_GETTER_LIMIT && c.ip==0xC71);
        CHECK(c.sp==(e==3u?0x7FFAu:0x7FEEu) && c.bp==(uint16_t)(c.sp+4u));
        CHECK(m[p(c.ss,c.bp-3u)]==0 && m[p(c.ss,c.bp-2u)]==0 && m[p(c.ss,c.bp-1u)]==0);
        c=setup(m,entries[e],5,3,0x8000,0x2800);
        CHECK(run(m,&c,1)==PR_U3C6_GETTER_LIMIT && c.ip==0xC71);
        CHECK(m[p(c.ss,c.bp-3u)]==1 && (c.ax&255u)==1);
        c=setup(m,entries[e],5,1,0x8000,0x2800);
        CHECK(run(m,&c,1)==PR_U3C6_GETTER_OK);
    }
    c=setup(m,0xBE7,5,3,0x8000,0x2800);before=c;memcpy(copy,m,PR_U3C6_GETTER_RAM_SIZE);
    CHECK(pr_u3c6_slot_getter_cpu(NULL,PR_U3C6_GETTER_RAM_SIZE,&c,256)==PR_U3C6_GETTER_ARGUMENT);
    CHECK(pr_u3c6_slot_getter_cpu(m,PR_U3C6_GETTER_RAM_SIZE,NULL,256)==PR_U3C6_GETTER_ARGUMENT);
    CHECK(pr_u3c6_slot_getter_cpu(m,1,&c,256)==PR_U3C6_GETTER_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof(c))==0 && memcmp(m,copy,PR_U3C6_GETTER_RAM_SIZE)==0);
    for(i=0;i<3u;++i) {
        c=before;if(i==0)c.sp=0x1F;if(i==1)c.sp=0xFFF9;if(i==2)c.ip=0xBE8;
        {pr_u3c6_getter_cpu rejected=c;CHECK(run(m,&c,256)==PR_U3C6_GETTER_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof(c))==0 && memcmp(m,copy,PR_U3C6_GETTER_RAM_SIZE)==0);}
    }
    free(copy);free(m);printf("%u semantic checks passed\n",checks);return 0;
}

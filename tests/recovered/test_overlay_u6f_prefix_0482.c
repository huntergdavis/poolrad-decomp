#include "poolrad/recovered/overlay_u6f_prefix_0482.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static size_t p(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void word(uint8_t *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t a=p(s,o);m[a]=(uint8_t)v;m[(a+1u)&0xfffffu]=(uint8_t)(v>>8);
}
static uint16_t get(const uint8_t *m,uint16_t s,uint16_t o) {
    size_t a=p(s,o);return (uint16_t)(m[a]|((uint16_t)m[(a+1u)&0xfffffu]<<8));
}
static void pointer(uint8_t *m,uint16_t s,uint16_t o,uint16_t seg,uint16_t off) {
    word(m,s,o,off);word(m,s,(uint16_t)(o+2u),seg);
}
static void setup(uint8_t *m,pr_u6f_0482_cpu *c,unsigned len,uint16_t ds,uint16_t sp) {
    static const uint16_t sites[]={0x48c,0x4a6,0x4c2,0x4c7,0x4d8};
    static const uint16_t ips[]={0x94a,0xb0b,0xc3b,0xb98,0x34};
    memset(m,0,PR_U6F_0482_RAM_SIZE);
    *c=(pr_u6f_0482_cpu){0xabcd,0x1234,0x5678,0x9abc,0xdef0,0x3333,
                         0x9999,sp,0x8000,0x482,ds,0x3456,0x5000,0x7603};
    m[p(c->cs,0x476)]=(uint8_t)len;
    for(unsigned i=0;i<len;++i)m[p(c->cs,(uint16_t)(0x477u+i))]=(uint8_t)(65u+i%26u);
    for(unsigned i=0;i<5u;++i) {
        word(m,c->cs,(uint16_t)(sites[i]+1u),ips[i]);
        word(m,c->cs,(uint16_t)(sites[i]+3u),(uint16_t)(i==0u?0x1000u:i==4u?0x4000u:0x2000u));
    }
    pointer(m,ds,0x5d92,0x6000,0x1000);
    m[p(0x6000,0x10c0)]=7u;m[p(0x6000,0x10bd)]=4u;
    m[p(ds,0x8cc)]=0x51u;m[p(ds,0x5376)]=0x72u;m[p(ds,0x5377)]=0x39u;
    pointer(m,c->ss,sp,0x9000,0x200);word(m,c->ss,(uint16_t)(sp+4u),0xab07u);
}
static void callback(uint8_t *m) {
    pr_u6f_0482_cpu c;pr_u6f_0482_pending_call call;
    setup(m,&c,3u,0x3000,0x8000);
    assert(pr_u6f_prefix_0482(m,PR_U6F_0482_RAM_SIZE,&c,&call)==PR_U6F_0482_CALLBACK);
    assert(c.cs==0x8000u && c.ip==0x4d8u && c.sp==0x7dccu && c.bp==0x7ffeu);
    assert(c.ax==11u && c.bx==0x7dc8u && c.cx==0u && c.dx==0x3000u);
    assert(c.si==0x7ed7u && c.di==0x1000u && c.es==0x6000u && c.ds==0x3000u);
    assert(c.ss==0x5000u && c.flags==0x7202u);
    assert(call.call_ip==0x4d8u && call.continuation_ip==0x4ddu);
    assert(call.target_cs==0x4000u && call.target_ip==0x34u);
    assert(m[p(c.ds,0x5376)]==3u && m[p(c.ds,0x5377)]==0x72u);
    assert(m[p(c.ss,0x7dd5)]==4u && memcmp(m+p(c.ss,0x7dd6),"ABCQ",4u)==0);
    assert(m[p(c.ss,0x7ed5)]==1u && m[p(c.ss,0x7ed6)]==0x51u);
    assert(get(m,c.ss,c.sp)==11u && get(m,c.ss,(uint16_t)(c.sp+2u))==4u);
    assert(get(m,c.ss,(uint16_t)(c.sp+4u))==0x7dd5u);
    assert(get(m,c.ss,(uint16_t)(c.sp+6u))==c.ss);
    assert(get(m,c.ss,0x7ffe)==0x9999u); /* Parent has not returned. */
}
static void lengths(uint8_t *m) {
    pr_u6f_0482_cpu c;pr_u6f_0482_pending_call call;uint8_t expected[255];
    setup(m,&c,0u,0x3000,0x8000);
    assert(pr_u6f_prefix_0482(m,PR_U6F_0482_RAM_SIZE,&c,&call)==PR_U6F_0482_CALLBACK);
    assert(m[p(c.ss,0x7dd5)]==1u && m[p(c.ss,0x7dd6)]==0x51u);
    setup(m,&c,255u,0x3000,0x8000);memcpy(expected,m+p(c.cs,0x477),255u);
    assert(pr_u6f_prefix_0482(m,PR_U6F_0482_RAM_SIZE,&c,&call)==PR_U6F_0482_CALLBACK);
    assert(m[p(c.ss,0x7dd5)]==255u);
    assert(memcmp(m+p(c.ss,0x7dd6),expected,255u)==0); /* No append past capacity. */
    assert(m[p(c.ss,0x7ed5)]==1u && m[p(c.ss,0x7ed6)]==0x51u);
}
static void changed_return(uint8_t *m) {
    pr_u6f_0482_cpu c;pr_u6f_0482_pending_call call={1u,2u,3u,4u},before=call;
    setup(m,&c,3u,0x5000,0x55a9);
    assert(pr_u6f_prefix_0482(m,PR_U6F_0482_RAM_SIZE,&c,&call)==PR_U6F_0482_CONTROL_STOP);
    /* Selector stores change both its far return and saved parent BP. */
    assert(c.cs==0x8000u && c.ip==0x455u && c.sp==0x537du && c.bp==0x03a7u);
    assert(memcmp(&call,&before,sizeof call)==0);
    assert(m[p(c.ds,0x5377)]==0x55u && m[p(c.ds,0x5376)]==3u);
}
static void wrapped_helpers(uint8_t *m) {
    /* Selector entry SP=parent-232; load/concat=parent-238; char=parent-23A.
     * These parent offsets place each helper RETF at FFFD/FFFE/FFFF. */
    static const uint16_t stacks[]={0x22f,0x230,0x231,0x235,0x236,0x237,0x238,0x239};
    for(unsigned i=0;i<sizeof stacks/sizeof stacks[0];++i) {
        pr_u6f_0482_cpu c;pr_u6f_0482_pending_call call;
        setup(m,&c,3u,0x3000,stacks[i]);
        assert(pr_u6f_prefix_0482(m,PR_U6F_0482_RAM_SIZE,&c,&call)==PR_U6F_0482_CALLBACK);
        assert(c.cs==0x8000u && c.ip==0x4d8u && c.bp==(uint16_t)(stacks[i]-2u));
        assert(c.sp==(uint16_t)(stacks[i]-0x234u));
        assert(call.target_cs==0x4000u && call.target_ip==0x34u);
        /* At parent 022F the destination far operand starts at SS:FFFF.
         * LES reads its segment from the linear extension (zero in setup),
         * even though RETF pops CS using a wrapped stack-word offset. */
        uint16_t output_segment=stacks[i]==0x22fu?0u:c.ss;
        assert(m[p(output_segment,(uint16_t)(c.bp-0x229u))]==4u);
        for(unsigned j=0;j<4u;++j)
            assert(m[p(output_segment,(uint16_t)(c.bp-0x228u+j))]==(uint8_t)(j==3u?'Q':'A'+j));
    }
}
static void invalid(uint8_t *m,uint8_t *copy) {
    pr_u6f_0482_cpu c,before;pr_u6f_0482_pending_call call={1u,2u,3u,4u},out=call;
    setup(m,&c,3u,0x3000,0x8000);before=c;memcpy(copy,m,PR_U6F_0482_RAM_SIZE);
    assert(pr_u6f_prefix_0482(NULL,PR_U6F_0482_RAM_SIZE,&c,&call)==PR_U6F_0482_ARGUMENT);
    assert(pr_u6f_prefix_0482(m,PR_U6F_0482_RAM_SIZE-1u,&c,&call)==PR_U6F_0482_ARGUMENT);
    assert(pr_u6f_prefix_0482(m,PR_U6F_0482_RAM_SIZE,NULL,&call)==PR_U6F_0482_ARGUMENT);
    assert(pr_u6f_prefix_0482(m,PR_U6F_0482_RAM_SIZE,&c,NULL)==PR_U6F_0482_ARGUMENT);
    assert(memcmp(&c,&before,sizeof c)==0 && memcmp(&call,&out,sizeof call)==0);
    c.ip=0x483u;before=c;
    assert(pr_u6f_prefix_0482(m,PR_U6F_0482_RAM_SIZE,&c,&call)==PR_U6F_0482_ARGUMENT);
    assert(memcmp(&c,&before,sizeof c)==0 && memcmp(&call,&out,sizeof call)==0);
    assert(memcmp(m,copy,PR_U6F_0482_RAM_SIZE)==0);
}
int main(void) {
    uint8_t *ram=malloc(PR_U6F_0482_RAM_SIZE),*copy=malloc(PR_U6F_0482_RAM_SIZE);
    assert(ram && copy);callback(ram);lengths(ram);changed_return(ram);wrapped_helpers(ram);invalid(ram,copy);
    free(copy);free(ram);return 0;
}

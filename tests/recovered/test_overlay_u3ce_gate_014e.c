#include "poolrad/recovered/overlay_u3ce_gate_014e.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static size_t a(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&0xfffffu; }
static void word(uint8_t *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=a(s,o);m[p]=(uint8_t)v;m[(p+1u)&0xfffffu]=(uint8_t)(v>>8);
}
static uint16_t readw(const uint8_t *m,uint16_t s,uint16_t o) {
    size_t p=a(s,o);return (uint16_t)(m[p]|((uint16_t)m[(p+1u)&0xfffffu]<<8));
}
static void setup(uint8_t *m,pr_u3ce_014e_cpu *c,int first,int second) {
    memset(m,0,PR_U3CE_014E_RAM_SIZE);
    *c=(pr_u3ce_014e_cpu){0xabcd,0x1234,0x5678,0x9abc,0xdef0,0x3333,
                         0x9999,0x8000,0x8000,0x14e,0x3000,0x3456,0x5000,0x7603};
    word(m,c->ss,0x8000,0x200);word(m,c->ss,0x8002,0x9000);word(m,c->ss,0x8004,0xab07);
    if(first)word(m,c->ds,0x5e92,0x1000);
    if(second)word(m,c->ds,0x5e98,0x7000); /* Nonzero segment with zero offset. */
    for(unsigned i=0;i<2u;++i) {
        uint16_t site=(uint16_t)(i?0x1a5u:0x17au);
        word(m,c->cs,(uint16_t)(site+1u),0x2595);word(m,c->cs,(uint16_t)(site+3u),0x2222);
    }
}
static void common(const pr_u3ce_014e_cpu *c) {
    assert(c->bx==0x1234u && c->cx==0x5603u && c->dx==0x9abcu && c->si==0xdef0u);
    assert(c->ds==0x3000u && c->es==0x3456u && c->ss==0x5000u);
}
static void zero(uint8_t *m) {
    pr_u3ce_014e_cpu c;pr_u3ce_014e_call p={1,2,3,4},before=p;
    setup(m,&c,0,0);
    assert(pr_u3ce_gate_014e(m,PR_U3CE_014E_RAM_SIZE,&c,&p)==PR_U3CE_014E_RETURNED);
    common(&c);assert(c.ax==0u && c.di==0x38u && c.bp==0x9999u && c.sp==0x8006u);
    assert(c.cs==0x9000u && c.ip==0x200u && c.flags==0x7646u);
    assert(memcmp(&p,&before,sizeof p)==0);
}
static void callbacks(uint8_t *m) {
    for(int first=0;first<2;++first) {
        pr_u3ce_014e_cpu c;pr_u3ce_014e_call p;
        setup(m,&c,first,1);
        assert(pr_u3ce_gate_014e(m,PR_U3CE_014E_RAM_SIZE,&c,&p)==PR_U3CE_014E_CALLBACK);
        common(&c);assert(c.ax==7u && c.bp==0x7ffeu && c.sp==0x7ff8u && c.cs==0x8000u);
        assert(c.di==(first?0x5e92u:0x5e96u));
        assert(c.flags==(first?0x7612u:0x7616u));
        assert(c.ip==(first?0x17au:0x1a5u) && p.call_ip==c.ip);
        assert(p.continuation_ip==(first?0x17fu:0x1aau));
        assert(p.target_cs==0x2222u && p.target_ip==0x2595u);
        assert(readw(m,c.ss,c.sp)==c.di && readw(m,c.ss,(uint16_t)(c.sp+2u))==c.ds);
        assert(readw(m,c.ss,0x7ffe)==0x9999u);
    }
}
static void continuation(uint8_t *m) {
    pr_u3ce_014e_cpu c;pr_u3ce_014e_call p;
    setup(m,&c,1,0);c.ip=0x17f;c.bp=0x7ffe;c.sp=0x7ffc;word(m,c.ss,c.bp,0x9999);
    word(m,c.ss,0x8004,0xabff);word(m,c.ds,0x6658,0x7000);
    assert(pr_u3ce_gate_014e(m,PR_U3CE_014E_RAM_SIZE,&c,&p)==PR_U3CE_014E_CALLBACK);
    assert(c.ax==255u && c.di==0x6656u && c.ip==0x1a5u && c.sp==0x7ff8u);
    assert(readw(m,c.ss,c.sp)==0x6656u); /* Rereads the supplied slot byte. */
    setup(m,&c,1,0);c.ip=0x17f;c.bp=0x7ffe;c.sp=0x7000;word(m,c.ss,c.bp,0x9999);
    assert(pr_u3ce_gate_014e(m,PR_U3CE_014E_RAM_SIZE,&c,&p)==PR_U3CE_014E_RETURNED);
    assert(c.cs==0x9000u && c.ip==0x200u && c.sp==0x8006u && c.bp==0x9999u);
}
static void invalid(uint8_t *m,uint8_t *copy) {
    pr_u3ce_014e_cpu c,before;pr_u3ce_014e_call p={1,2,3,4},old=p;
    setup(m,&c,0,0);before=c;memcpy(copy,m,PR_U3CE_014E_RAM_SIZE);
    assert(pr_u3ce_gate_014e(NULL,PR_U3CE_014E_RAM_SIZE,&c,&p)==PR_U3CE_014E_ARGUMENT);
    assert(pr_u3ce_gate_014e(m,PR_U3CE_014E_RAM_SIZE-1u,&c,&p)==PR_U3CE_014E_ARGUMENT);
    assert(pr_u3ce_gate_014e(m,PR_U3CE_014E_RAM_SIZE,NULL,&p)==PR_U3CE_014E_ARGUMENT);
    assert(pr_u3ce_gate_014e(m,PR_U3CE_014E_RAM_SIZE,&c,NULL)==PR_U3CE_014E_ARGUMENT);
    assert(memcmp(&c,&before,sizeof c)==0 && memcmp(&p,&old,sizeof p)==0);
    c.ip=0x17a;before=c;
    assert(pr_u3ce_gate_014e(m,PR_U3CE_014E_RAM_SIZE,&c,&p)==PR_U3CE_014E_ARGUMENT);
    assert(memcmp(&c,&before,sizeof c)==0 && memcmp(&p,&old,sizeof p)==0);
    assert(memcmp(m,copy,PR_U3CE_014E_RAM_SIZE)==0);
}
int main(void) {
    uint8_t *m=malloc(PR_U3CE_014E_RAM_SIZE),*copy=malloc(PR_U3CE_014E_RAM_SIZE);
    assert(m && copy);zero(m);callbacks(m);continuation(m);invalid(m,copy);
    free(copy);free(m);return 0;
}

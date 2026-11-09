#include "poolrad/replay/u3b0_character_059c.h"
#define MASK (PR_R0899_KEY_RAM_SIZE-1u)
static size_t addr(uint16_t s,uint16_t o) { return (((size_t)s<<4u)+o)&MASK; }
static void cmp8(pr_r0899_cpu *c,unsigned a,unsigned b) {
    unsigned r=(a-b)&0xffu,p=r; p^=p>>4u;p^=p>>2u;p^=p>>1u;
    c->flags=(uint16_t)((c->flags & (uint16_t)~0x08d5u) | (a<b ? 1u : 0u) |
        ((p&1u)==0u ? 4u : 0u) | ((a^b^r)&0x10u) | (r==0u ? 0x40u : 0u) |
        (r&0x80u) | (((a^b)&(a^r)&0x80u)<<4u));
}
pr_u3b0_character_status pr_u3b0_character_059c(uint8_t *m,size_t n,pr_r0899_cpu *c) {
    uint8_t key,gate;
    if(!m || !c || n!=PR_R0899_KEY_RAM_SIZE || c->ip!=0x059cu || c->bp<0x008fu)
        return PR_U3B0_CHARACTER_ARGUMENT;
    key=m[addr(c->ss,(uint16_t)(c->bp-0x60u))];
    c->ax=(uint16_t)((c->ax&0xff00u)|key);cmp8(c,key,0x1bu);
    if(key==0x1bu) {
        m[addr(c->ss,(uint16_t)(c->bp-0x63u))]=1u;
        m[addr(c->ss,(uint16_t)(c->bp-0x60u))]=0u;
        c->ip=0x06cfu;return PR_U3B0_CHARACTER_ESCAPE;
    }
    cmp8(c,key,0x0du);
    if(key!=0x0du) { c->ip=0x05c4u;return PR_U3B0_CHARACTER_OTHER; }
    gate=m[addr(c->ss,(uint16_t)(c->bp-0x8fu))];cmp8(c,gate,0u);
    if(gate==0u) { c->ip=0x06cfu;return PR_U3B0_CHARACTER_ENTER_DISABLED; }
    m[addr(c->ss,(uint16_t)(c->bp-0x60u))]=0x0du;
    m[addr(c->ss,(uint16_t)(c->bp-0x63u))]=1u;
    c->ip=0x06cfu;return PR_U3B0_CHARACTER_ENTER_ENABLED;
}

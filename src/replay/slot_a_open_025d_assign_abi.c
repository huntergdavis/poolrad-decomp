#include "poolrad/replay/slot_a_open_025d_assign_abi.h"
static uint8_t *span(pr_slot_a_directory_state *s, uint16_t seg, uint16_t off, size_t width)
{
    size_t start=((size_t)seg << 4)+off;
    if ((size_t)off+width > 0x10000u || start > s->ram_size || width > s->ram_size-start) return NULL;
    return s->ram+start;
}
static uint16_t word(const uint8_t *p)
{ return (uint16_t)((unsigned)p[0] | ((unsigned)p[1] << 8)); }
static void put(uint8_t *p, uint16_t value)
{ p[0]=(uint8_t)value; p[1]=(uint8_t)(value >> 8); }
static uint16_t sub_flags(uint16_t old, unsigned a, unsigned b, unsigned bits)
{
    unsigned mask=(1u << bits)-1u, sign=1u << (bits-1u), value=(a-b)&mask, p=value&255u;
    uint16_t f=(uint16_t)(old & (uint16_t)~0x8D5u);
    p^=p>>4; p^=p>>2; p^=p>>1;
    if (!(p&1u)) f|=4u;
    if (!value) f|=0x40u;
    if (value&sign) f|=0x80u;
    if (a<b) f|=1u;
    if ((a^b^value)&0x10u) f|=0x10u;
    if ((a^b)&(a^value)&sign) f|=0x800u;
    return f;
}
static pr_slot_a_open_025d_status assign_step(pr_slot_a_directory_state *s);
pr_slot_a_open_025d_status pr_slot_a_open_025d_step(
    pr_slot_a_directory_state *s, pr_slot_a_open_025d_report *report)
{
    pr_slot_a_directory_cpu *c; uint8_t *p; uint16_t value,next,off;
    if (!s || !report) return PR_SLOT_A_OPEN_025D_ARGUMENT;
    *report=(pr_slot_a_open_025d_report){0};
    if (!s->ram || s->ram_size > 0x100000u) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
    c=&s->cpu;
    if (c->cs==0x131Cu && c->ip>=0x13E2u && c->ip<=0x140Du) return assign_step(s);
    if ((c->cs==0x131Cu && c->ip>=0xB25u && c->ip<0xB49u) ||
        (c->cs==0x1026u && c->ip>=0x2EBu && c->ip<=0x31Fu && c->ip!=0x310u)) {
        pr_slot_a_file_probe_status status=pr_slot_a_file_probe_02eb_step(s,report);
        if (status==PR_SLOT_A_FILE_PROBE_STEPPED) return PR_SLOT_A_OPEN_025D_STEPPED;
        if (status==PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        if (status==PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME) return PR_SLOT_A_OPEN_025D_BLOCKED_RESET;
        return PR_SLOT_A_OPEN_025D_BLOCKED_PC;
    }
    if (c->cs==0x2A8Eu && c->ip==0xC26u) {
        p=span(s,c->ss,(uint16_t)(c->sp-4u),4u);
        if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        put(p,0xC2Bu); put(p+2u,c->cs); c->sp=(uint16_t)(c->sp-4u);
        c->cs=0x1026u; c->ip=0x25Du; return PR_SLOT_A_OPEN_025D_STEPPED;
    }
    if (c->cs!=0x1026u) return PR_SLOT_A_OPEN_025D_BLOCKED_PC;
    next=(uint16_t)(c->ip+1u);
    switch (c->ip) {
    case 0x25Du: value=c->bp; goto push;
    case 0x25Eu: c->bp=c->sp; c->ip=0x260u; break;
    case 0x260u:
        c->flags=sub_flags(c->flags,c->sp,0x54u,16u); c->sp=(uint16_t)(c->sp-0x54u); c->ip=0x263u; break;
    case 0x263u: case 0x276u:
        p=span(s,c->ss,(uint16_t)(c->bp+(c->ip==0x263u?0x10u:0xCu)),4u);
        if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        c->di=word(p); c->es=word(p+2u); c->ip=(uint16_t)(c->ip+3u); break;
    case 0x266u: case 0x279u: value=c->es; goto push;
    case 0x267u: case 0x26Cu: case 0x27Au: case 0x27Fu: case 0x28Du: value=c->di; goto push;
    case 0x26Bu: case 0x27Eu: case 0x28Cu: value=c->ss; goto push;
    case 0x268u: case 0x289u: c->di=(uint16_t)(c->bp-0x2Au); c->ip=(uint16_t)(c->ip+3u); break;
    case 0x27Bu: c->di=(uint16_t)(c->bp-0x53u); c->ip=0x27Eu; break;
    case 0x26Du: case 0x280u: c->ax=0x28u; c->ip=(uint16_t)(c->ip+3u); break;
    case 0x270u: case 0x283u: value=c->ax; goto push;
    case 0x28Eu: value=c->cs; goto push;
    case 0x28Fu: value=0x292u; next=0x2EBu; goto push;
    case 0x271u: case 0x284u: case 0x310u:
        p=span(s,c->ss,(uint16_t)(c->sp-4u),4u);
        if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        put(p,(uint16_t)(c->ip+5u)); put(p+2u,c->cs); c->sp=(uint16_t)(c->sp-4u);
        c->ip=c->ip==0x310u?0x13E2u:0xB25u; c->cs=0x131Cu; break;
    default: return PR_SLOT_A_OPEN_025D_BLOCKED_PC;
    }
    return PR_SLOT_A_OPEN_025D_STEPPED;
push:
    off=(uint16_t)(c->sp-2u); p=span(s,c->ss,off,2u);
    if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
    put(p,value); c->sp=off; c->ip=next; return PR_SLOT_A_OPEN_025D_STEPPED;
}
static pr_slot_a_open_025d_status assign_step(pr_slot_a_directory_state *s)
{
    pr_slot_a_directory_cpu *c=&s->cpu; uint8_t *p,*source,*dest;
    uint16_t value,off,ret_ip,ret_cs,count; size_t width,i;
    switch (c->ip) {
    case 0x13E2u: c->bx=c->sp; c->ip=0x13E4u; break;
    case 0x13E4u:
        off=(uint16_t)(c->sp-2u); p=span(s,c->ss,off,2u);
        if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        put(p,c->ds); c->sp=off; c->ip=0x13E5u; break;
    case 0x13E5u: case 0x13E9u:
        p=span(s,c->ss,(uint16_t)(c->bx+(c->ip==0x13E5u?8u:4u)),4u);
        if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        value=word(p); off=word(p+2u);
        if (c->ip==0x13E5u) {c->di=value; c->es=off; c->ip=0x13E9u;}
        else {c->si=value; c->ds=off; c->ip=0x13EDu;} break;
    case 0x13EDu: c->flags&=(uint16_t)~0x400u; c->ip=0x13EEu; break;
    case 0x13EEu: case 0x13F5u:
        c->ax=0u; c->flags=(uint16_t)((c->flags & (uint16_t)~0x8D5u)|0x44u);
        c->ip=(uint16_t)(c->ip+2u); break;
    case 0x13F0u: case 0x13F4u:
        if (c->flags&0x400u) return PR_SLOT_A_OPEN_025D_BLOCKED_PC;
        p=span(s,c->es,c->di,2u); if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        put(p,c->ax); c->di=(uint16_t)(c->di+2u); c->ip=(uint16_t)(c->ip+1u); break;
    case 0x13F1u: c->ax=0xD7B0u; c->ip=0x13F4u; break;
    case 0x13F7u: c->cx=22u; c->ip=0x13FAu; break;
    case 0x13FAu:
        if (c->flags&0x400u) return PR_SLOT_A_OPEN_025D_BLOCKED_PC;
        count=c->cx; width=(size_t)count*2u;
        if (count) {
            p=span(s,c->es,c->di,width); if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
            for (i=0;i<width;i+=2u) put(p+i,c->ax);
        }
        c->di=(uint16_t)(c->di+width); c->cx=0u; c->ip=0x13FCu; break;
    case 0x13FCu:
        if (c->flags&0x400u) return PR_SLOT_A_OPEN_025D_BLOCKED_PC;
        p=span(s,c->ds,c->si,1u); if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        c->ax=(uint16_t)((c->ax&0xFF00u)|*p); ++c->si; c->ip=0x13FDu; break;
    case 0x13FDu: c->flags=sub_flags(c->flags,(uint8_t)c->ax,0x4Fu,8u); c->ip=0x13FFu; break;
    case 0x13FFu: c->ip=c->flags&0x41u?0x1403u:0x1401u; break;
    case 0x1401u: c->ax=(uint16_t)((c->ax&0xFF00u)|0x4Fu); c->ip=0x1403u; break;
    case 0x1403u: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)c->ax); c->ip=0x1405u; break;
    case 0x1405u:
        c->cx&=0xFFu; c->flags=(uint16_t)((c->flags & (uint16_t)~0x8D5u)|0x44u); c->ip=0x1407u; break;
    case 0x1407u:
        if (c->flags&0x400u) return PR_SLOT_A_OPEN_025D_BLOCKED_PC;
        count=c->cx;
        if (count) {
            source=span(s,c->ds,c->si,count); dest=span(s,c->es,c->di,count);
            if (!source || !dest) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
            for (i=0;i<count;++i) dest[i]=source[i];
        }
        c->si=(uint16_t)(c->si+count); c->di=(uint16_t)(c->di+count); c->cx=0u; c->ip=0x1409u; break;
    case 0x1409u:
        c->ax&=0xFF00u; c->flags=(uint16_t)((c->flags & (uint16_t)~0x8D5u)|0x44u); c->ip=0x140Bu; break;
    case 0x140Bu:
        if (c->flags&0x400u) return PR_SLOT_A_OPEN_025D_BLOCKED_PC;
        p=span(s,c->es,c->di,1u); if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        *p=(uint8_t)c->ax; ++c->di; c->ip=0x140Cu; break;
    case 0x140Cu:
        p=span(s,c->ss,c->sp,2u); if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        c->ds=word(p); c->sp=(uint16_t)(c->sp+2u); c->ip=0x140Du; break;
    case 0x140Du:
        p=span(s,c->ss,c->sp,4u); if (!p) return PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY;
        ret_ip=word(p); ret_cs=word(p+2u); c->sp=(uint16_t)(c->sp+12u); c->ip=ret_ip; c->cs=ret_cs; break;
    default: return PR_SLOT_A_OPEN_025D_BLOCKED_PC;
    }
    return PR_SLOT_A_OPEN_025D_STEPPED;
}

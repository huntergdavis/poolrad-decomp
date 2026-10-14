#include "poolrad/replay/system_b98_abi.h"

static uint8_t *span(pr_slot_a_directory_state *s, uint16_t seg,
                     uint16_t off, size_t width)
{
    size_t start = ((size_t)seg << 4) + off;
    if ((size_t)off + width > 0x10000u || start > s->ram_size ||
        width > s->ram_size - start) return NULL;
    return s->ram + start;
}

static uint16_t word(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static uint16_t result_flags(uint16_t old, unsigned result, unsigned bits)
{
    unsigned p = result & 255u, mask = bits == 8u ? 255u : 65535u;
    uint16_t flags = (uint16_t)(old & (uint16_t)~0x08D5u);
    p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    if (!(p & 1u)) flags |= 4u;
    if (!(result & mask)) flags |= 0x40u;
    if (result & (1u << (bits - 1u))) flags |= 0x80u;
    return flags;
}

static uint16_t arithmetic_flags(uint16_t old, unsigned a, unsigned b,
                                  unsigned bits, int subtract)
{
    unsigned mask = bits == 8u ? 255u : 65535u;
    unsigned sign = 1u << (bits - 1u);
    unsigned value = subtract ? (a - b) & mask : (a + b) & mask;
    uint16_t f = result_flags(old, value, bits);
    if (subtract ? a < b : a + b > mask) f |= 1u;
    if ((a ^ b ^ value) & 0x10u) f |= 0x10u;
    if ((subtract ? (a ^ b) : ~(a ^ b)) & (a ^ value) & sign) f |= 0x800u;
    return f;
}

pr_system_b98_status pr_system_b98_step(pr_slot_a_directory_state *s)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p,*source,*destination;
    uint16_t next,value,saved_cf,n;
    size_t i;
    if (!s) return PR_SYSTEM_B98_ARGUMENT;
    if (!s->ram || s->ram_size>0x100000u) return PR_SYSTEM_B98_BLOCKED_MEMORY;
    c=&s->cpu;
    if (c->cs!=0x131Cu) return PR_SYSTEM_B98_BLOCKED_PC;
    next=c->ip;
    switch (c->ip) {
    case 0xB98u: c->flags&=(uint16_t)~0x400u; next=0xB99u; break;
    case 0xB99u: c->bx=c->sp; next=0xB9Bu; break;
    case 0xB9Bu: c->dx=c->ds; next=0xB9Du; break;
    case 0xB9Du: case 0xBA1u:
        p=span(s,c->ss,(uint16_t)(c->bx+(c->ip==0xB9Du?8u:4u)),4u);
        if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        if (c->ip==0xB9Du) { c->di=word(p); c->es=word(p+2u); next=0xBA1u; }
        else { c->si=word(p); c->ds=word(p+2u); next=0xBA5u; }
        break;
    case 0xBA5u:
        p=span(s,c->es,c->di,1u); if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        c->cx=(uint16_t)((c->cx&0xFF00u)|*p); next=0xBA8u; break;
    case 0xBA8u:
        c->cx&=0xFFu; c->flags=result_flags(c->flags,0u,8u); next=0xBAAu; break;
    case 0xBAAu:
        if (c->flags&0x400u) return PR_SYSTEM_B98_BLOCKED_PC;
        p=span(s,c->ds,c->si,1u); if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        c->ax=(uint16_t)((c->ax&0xFF00u)|*p); ++c->si; next=0xBABu; break;
    case 0xBABu:
        p=span(s,c->es,c->di,1u); if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        value=(uint8_t)c->ax;
        c->flags=arithmetic_flags(c->flags,*p,value,8u,0);
        *p=(uint8_t)(*p+value); next=0xBAEu; break;
    case 0xBAEu: next=c->flags&1u?0xBB0u:0xBB8u; break;
    case 0xBB0u:
        p=span(s,c->es,c->di,1u); if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        *p=255u; next=0xBB4u; break;
    case 0xBB4u: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)c->cx); next=0xBB6u; break;
    case 0xBB6u: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)~c->ax); next=0xBB8u; break;
    case 0xBB8u:
        c->flags=arithmetic_flags(c->flags,c->di,c->cx,16u,0);
        c->di=(uint16_t)(c->di+c->cx); next=0xBBAu; break;
    case 0xBBAu:
        saved_cf=c->flags&1u;
        c->flags=(uint16_t)((arithmetic_flags(c->flags,c->di,1u,16u,0)&~1u)|saved_cf);
        ++c->di; next=0xBBBu; break;
    case 0xBBBu: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)c->ax); next=0xBBDu; break;
    case 0xBBDu:
        if (c->flags&0x400u) return PR_SYSTEM_B98_BLOCKED_PC;
        n=c->cx;
        if (n) {
            source=span(s,c->ds,c->si,n); destination=span(s,c->es,c->di,n);
            if (!source || !destination) return PR_SYSTEM_B98_BLOCKED_MEMORY;
            for (i=0u;i<n;++i) destination[i]=source[i];
        }
        c->si=(uint16_t)(c->si+n); c->di=(uint16_t)(c->di+n); c->cx=0u;
        next=0xBBFu; break;
    case 0xBBFu: c->ds=c->dx; next=0xBC1u; break;
    case 0xBC1u:
        p=span(s,c->ss,c->sp,4u); if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        next=word(p); c->cs=word(p+2u); c->sp=(uint16_t)(c->sp+8u); break;
    default: return PR_SYSTEM_B98_BLOCKED_PC;
    }
    c->ip=next; return PR_SYSTEM_B98_STEPPED;
}

pr_system_b98_status pr_slot_a_concat_1999_step(
    pr_slot_a_directory_state *s,pr_slot_a_string_0356_report *report)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p;
    uint16_t next,value;
    if (!s || !report) return PR_SYSTEM_B98_ARGUMENT;
    *report=(pr_slot_a_string_0356_report){0};
    if (!s->ram || s->ram_size>0x100000u) return PR_SYSTEM_B98_BLOCKED_MEMORY;
    c=&s->cpu;
    if (c->cs==0x131Cu) return pr_system_b98_step(s);
    if (c->cs!=0x2A8Eu) return PR_SYSTEM_B98_BLOCKED_PC;
    switch (c->ip) {
    case 0x1999u: case 0x19A3u:
        next=c->ip==0x1999u?0x199Eu:0x19A8u;
        p=span(s,c->ss,(uint16_t)(c->sp-4u),4u);
        if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        put_word(p,next); put_word(p+2u,c->cs);
        c->sp=(uint16_t)(c->sp-4u); c->cs=0x131Cu; c->ip=0xB98u; break;
    case 0x199Eu: c->di=0x15E9u; c->ip=0x19A1u; break;
    case 0x19A1u: case 0x19A2u:
        value=c->ip==0x19A1u?c->cs:c->di;
        p=span(s,c->ss,(uint16_t)(c->sp-2u),2u);
        if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        put_word(p,value); c->sp=(uint16_t)(c->sp-2u); ++c->ip; break;
    case 0x19A8u:
        *report=(pr_slot_a_string_0356_report){1u,0x6Fu,0x19A8u,0x0802u,0x02EBu,0x19ADu,1u};
        return PR_SYSTEM_B98_BLOCKED_FILE_PROBE;
    default: return PR_SYSTEM_B98_BLOCKED_PC;
    }
    return PR_SYSTEM_B98_STEPPED;
}

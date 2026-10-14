#include "poolrad/replay/system_cbc_abi.h"

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

pr_system_b98_status pr_system_cbc_step(pr_slot_a_directory_state *s)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p;
    uint16_t next,value,saved_cf;
    if (!s) return PR_SYSTEM_B98_ARGUMENT;
    if (!s->ram || s->ram_size>0x100000u) return PR_SYSTEM_B98_BLOCKED_MEMORY;
    c=&s->cpu;
    if (c->cs==0x1026u && c->ip==0x3FCu) {
        p=span(s,c->ss,(uint16_t)(c->sp-4u),4u);
        if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        put_word(p,0x401u);put_word(p+2u,c->cs);
        c->sp=(uint16_t)(c->sp-4u);c->cs=0x131Cu;c->ip=0xCBCu;
        return PR_SYSTEM_B98_STEPPED;
    }
    if (c->cs!=0x131Cu) return PR_SYSTEM_B98_BLOCKED_PC;
    if (c->ip>=0xB98u && c->ip<0xBC4u) return pr_system_b98_step(s);
    if ((c->ip>=0xB25u && c->ip<0xB49u) || (c->ip>=0xB57u && c->ip<0xB98u)) {
        pr_slot_a_string_0356_report report;
        pr_slot_a_string_0356_status status=pr_slot_a_string_0356_prefix_step(s,&report);
        if (status==PR_SLOT_A_STRING_0356_STEPPED) return PR_SYSTEM_B98_STEPPED;
        if (status==PR_SLOT_A_STRING_0356_BLOCKED_MEMORY) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        return PR_SYSTEM_B98_BLOCKED_PC;
    }
    next=c->ip;
    switch (c->ip) {
    case 0xCBCu:
        p=span(s,c->ss,(uint16_t)(c->sp-2u),2u);if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        put_word(p,c->bp);c->sp=(uint16_t)(c->sp-2u);next=0xCBDu;break;
    case 0xCBDu:c->bp=c->sp;next=0xCBFu;break;
    case 0xCBFu:
        c->flags=arithmetic_flags(c->flags,c->sp,0x200u,16u,1);
        c->sp=(uint16_t)(c->sp-0x200u);next=0xCC3u;break;
    case 0xCC3u:
        p=span(s,c->ss,(uint16_t)(c->bp+6u),2u);if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        c->flags=arithmetic_flags(c->flags,word(p),0u,16u,1);next=0xCC7u;break;
    case 0xCC7u:
        next=((c->flags&0x40u) || (!!(c->flags&0x80u)!=!!(c->flags&0x800u)))?0xD0Cu:0xCC9u;break;
    case 0xCC9u:c->di=(uint16_t)(c->bp-0x100u);next=0xCCDu;break;
    case 0xCE1u:c->di=(uint16_t)(c->bp-0x200u);next=0xCE5u;break;
    case 0xCCFu:case 0xCE7u:case 0xCFFu:
        p=span(s,c->ss,(uint16_t)(c->bp+10u),4u);if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        c->di=word(p);c->es=word(p+2u);
        next=c->ip==0xCCFu?0xCD2u:(c->ip==0xCE7u?0xCEAu:0xD02u);break;
    case 0xCD4u:c->ax=1u;next=0xCD7u;break;
    case 0xCF3u:c->ax=255u;next=0xCF6u;break;
    case 0xD04u:c->ax=255u;next=0xD07u;break;
    case 0xCD8u:case 0xCECu:
        p=span(s,c->ss,(uint16_t)(c->bp+8u),2u);if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        c->ax=word(p);next=c->ip==0xCD8u?0xCDBu:0xCEFu;break;
    case 0xCDBu:
        saved_cf=c->flags&1u;
        c->flags=(uint16_t)((arithmetic_flags(c->flags,c->ax,1u,16u,1)&~1u)|saved_cf);
        --c->ax;next=0xCDCu;break;
    case 0xCEFu:
        p=span(s,c->ss,(uint16_t)(c->bp+6u),2u);if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        value=word(p);c->flags=arithmetic_flags(c->flags,c->ax,value,16u,0);
        c->ax=(uint16_t)(c->ax+value);next=0xCF2u;break;
    case 0xCCDu:case 0xCCEu:case 0xCD2u:case 0xCD3u:case 0xCD7u:case 0xCDCu:
    case 0xCDDu:case 0xCE5u:case 0xCE6u:case 0xCEAu:case 0xCEBu:case 0xCF2u:
    case 0xCF6u:case 0xCF7u:case 0xCFBu:case 0xD02u:case 0xD03u:case 0xD07u:case 0xD08u:
        switch (c->ip) {
        case 0xCCDu:case 0xCE5u:value=c->ss;break;
        case 0xCD2u:case 0xCEAu:case 0xD02u:value=c->es;break;
        case 0xCCEu:case 0xCD3u:case 0xCE6u:case 0xCEBu:case 0xD03u:value=c->di;break;
        case 0xCDDu:case 0xCF7u:case 0xCFBu:case 0xD08u:value=c->cs;break;
        default:value=c->ax;break;
        }
        p=span(s,c->ss,(uint16_t)(c->sp-2u),2u);if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        put_word(p,value);c->sp=(uint16_t)(c->sp-2u);next=(uint16_t)(c->ip+1u);break;
    case 0xCDEu:case 0xCF8u:case 0xCFCu:case 0xD09u:
        next=(uint16_t)(c->ip+3u);p=span(s,c->ss,(uint16_t)(c->sp-2u),2u);
        if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        put_word(p,next);c->sp=(uint16_t)(c->sp-2u);
        next=c->ip==0xD09u?0xB25u:(c->ip==0xCFCu?0xB98u:0xB57u);break;
    case 0xD0Cu:c->sp=c->bp;next=0xD0Eu;break;
    case 0xD0Eu:
        p=span(s,c->ss,c->sp,2u);if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        value=word(p);c->sp=(uint16_t)(c->sp+2u);c->bp=value;next=0xD0Fu;break;
    case 0xD0Fu:
        p=span(s,c->ss,c->sp,4u);if (!p) return PR_SYSTEM_B98_BLOCKED_MEMORY;
        next=word(p);c->cs=word(p+2u);c->sp=(uint16_t)(c->sp+12u);break;
    default:return PR_SYSTEM_B98_BLOCKED_PC;
    }
    c->ip=next;return PR_SYSTEM_B98_STEPPED;
}

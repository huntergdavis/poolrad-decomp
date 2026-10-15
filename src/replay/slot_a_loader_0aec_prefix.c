#include "poolrad/replay/slot_a_loader_0aec_prefix.h"
#include "poolrad/replay/slot_a_saved_copy_1965.h"
static uint8_t *span(pr_slot_a_directory_state *s, uint16_t seg, uint16_t off, size_t width)
{
    size_t start = ((size_t)seg << 4) + off;
    if ((size_t)off + width > 0x10000u || start > s->ram_size || width > s->ram_size-start) return NULL;
    return s->ram + start;
}
static uint16_t word(const uint8_t *p)
{ return (uint16_t)((unsigned)p[0] | ((unsigned)p[1] << 8)); }
static void put(uint8_t *p, uint16_t v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static uint16_t sub_flags(uint16_t old, unsigned a, unsigned b, unsigned bits)
{
    unsigned mask = (1u << bits)-1u, sign = 1u << (bits-1u), r = (a-b) & mask, p = r & 255u;
    uint16_t f = (uint16_t)(old & (uint16_t)~0x08D5u);
    p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    if (!(p & 1u)) f |= 4u;
    if (!r) f |= 0x40u;
    if (r & sign) f |= 0x80u;
    if (a < b) f |= 1u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x10u;
    if ((a ^ b) & (a ^ r) & sign) f |= 0x800u;
    return f;
}
pr_slot_a_loader_0aec_status pr_slot_a_loader_0aec_step(
    pr_slot_a_directory_state *s, pr_slot_a_loader_0aec_report *report)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p;
    uint16_t value, next, off, target_cs;
    int displacement;
    if (!s || !report) return PR_SLOT_A_LOADER_0AEC_ARGUMENT;
    *report = (pr_slot_a_loader_0aec_report){0};
    if (!s->ram || s->ram_size > 0x100000u) return PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs != 0x2A8Eu) {
        if (c->cs == 0x131Cu && c->ip >= 0xB0Bu && c->ip <= 0xB22u) {
            pr_slot_a_saved_copy_report r;
            pr_slot_a_saved_copy_status status = pr_slot_a_saved_copy_1965_step(s, &r);
            if (status == PR_SLOT_A_SAVED_COPY_STEPPED) return PR_SLOT_A_LOADER_0AEC_STEPPED;
            if (status == PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY) return PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY;
            return PR_SLOT_A_LOADER_0AEC_BLOCKED_PC;
        } else {
            pr_slot_a_string_0425_status status = pr_slot_a_string_0425_step(s, report);
            if (status == PR_SLOT_A_STRING_0425_STEPPED) return PR_SLOT_A_LOADER_0AEC_STEPPED;
            if (status == PR_SLOT_A_STRING_0425_BLOCKED_MEMORY) return PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY;
            if (status == PR_SLOT_A_STRING_0425_BLOCKED_DELETE) return PR_SLOT_A_LOADER_0AEC_BLOCKED_DEPENDENCY;
            return PR_SLOT_A_LOADER_0AEC_BLOCKED_PC;
        }
    }
    next = (uint16_t)(c->ip+1u);
    switch (c->ip) {
    case 0x19D7u: value = 0x19DAu; next = 0xAECu; goto push;
    case 0xAECu: value = c->bp; goto push;
    case 0xAEDu: c->bp = c->sp; c->ip = 0xAEFu; break;
    case 0xAEFu:
        c->flags = sub_flags(c->flags, c->sp, 0x3BAu, 16u);
        c->sp = (uint16_t)(c->sp-0x3BAu); c->ip = 0xAF3u; break;
    case 0xAF3u:
        p = span(s, c->ss, (uint16_t)(c->bp+0xEu), 4u);
        if (!p) return PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY;
        c->di = word(p); c->es = word(p+2u); c->ip = 0xAF6u; break;
    case 0xAF6u: value = c->es; goto push;
    case 0xAFCu: case 0xB11u: case 0xB21u: case 0xB27u: case 0xB37u:
    case 0xB46u: case 0xB56u: case 0xB74u: case 0xBDAu: case 0xBFFu:
    case 0xC05u: case 0xC10u: case 0xC1Bu: case 0xC24u: value = c->ss; goto push;
    case 0xAF7u: case 0xAFDu: case 0xB12u: case 0xB17u: case 0xB22u:
    case 0xB28u: case 0xB38u: case 0xB47u: case 0xB4Cu: case 0xB57u:
    case 0xB6Au: case 0xB75u: case 0xBD5u: case 0xBDBu: case 0xC00u:
    case 0xC06u: case 0xC11u: case 0xC1Cu: case 0xC25u: value = c->di; goto push;
    case 0xB01u: case 0xB3Cu: case 0xB5Bu: case 0xB79u: case 0xBDFu:
    case 0xC1Fu: value = c->ax; goto push;
    case 0xB16u: value = c->ds; goto push;
    case 0xB4Bu: case 0xB69u: case 0xBD4u: value = c->cs; goto push;
    case 0xAF8u: case 0xB23u: case 0xB33u: case 0xC01u: displacement = -0x100; goto lea;
    case 0xB0Du: case 0xB42u: displacement = -0x3B9; goto lea;
    case 0xB1Du: case 0xB52u: case 0xBFBu: displacement = -0x2B9; goto lea;
    case 0xB70u: case 0xC17u: displacement = -0x1B8; goto lea;
    case 0xBD6u: case 0xC0Cu: displacement = -0x187; goto lea;
    case 0xC20u: displacement = -0x180; goto lea;
    case 0xAFEu: case 0xB39u: c->ax = 0xFFu; c->ip = (uint16_t)(c->ip+3u); break;
    case 0xB76u: c->ax = 0x28u; c->ip = 0xB79u; break;
    case 0xBDCu: c->ax = 4u; c->ip = 0xBDFu; break;
    case 0xB13u: c->di = 0x537Au; c->ip = 0xB16u; break;
    case 0xB48u: c->di = 0xAC4u; c->ip = 0xB4Bu; break;
    case 0xB66u: c->di = 0xAD6u; c->ip = 0xB69u; break;
    case 0xBD1u: c->di = 0xAD8u; c->ip = 0xBD4u; break;
    case 0xB58u:
        p = span(s, c->ds, 0x537Bu, 1u);
        if (!p) return PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY;
        c->ax = (uint16_t)((c->ax & 0xFF00u) | *p); c->ip = 0xB5Bu; break;
    case 0xC1Du: c->ax &= 0xFF00u; c->ip = 0xC1Fu; break;
    case 0xB07u: case 0xBCBu:
        p = span(s, c->ss, (uint16_t)(c->bp+(c->ip == 0xB07u ? 6u : 8u)), 1u);
        if (!p) return PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY;
        c->flags = sub_flags(c->flags, *p, 0u, 8u); c->ip = (uint16_t)(c->ip+4u); break;
    case 0xB0Bu: c->ip = c->flags & 0x40u ? 0xB81u : 0xB0Du; break;
    case 0xBCFu: c->ip = c->flags & 0x40u ? 0xBE7u : 0xBD1u; break;
    case 0xB7Fu: c->ip = 0xBCBu; break;
    case 0xBE5u: c->ip = 0xBFBu; break;
    case 0xB02u: value = 0xB25u; goto far_call;
    case 0xB18u: value = 0xB0Bu; goto far_call;
    case 0xB29u: value = 0x425u; goto far_call;
    case 0xB2Eu: case 0xB61u: case 0xB6Bu: case 0xC12u: value = 0xB98u; goto far_call;
    case 0xB3Du: case 0xB7Au: case 0xBE0u: value = 0xB25u; goto far_call;
    case 0xB4Du: case 0xC07u: value = 0xB0Bu; goto far_call;
    case 0xB5Cu: value = 0xC3Bu; goto far_call;
    case 0xB81u: case 0xBE7u: return PR_SLOT_A_LOADER_0AEC_BLOCKED_PATH;
    case 0xC26u:
        *report = (pr_slot_a_loader_0aec_report){1u,0x6Fu,0xC26u,0x802u,0x25Du,0xC2Bu,1u};
        return PR_SLOT_A_LOADER_0AEC_BLOCKED_FILE_OPEN;
    default: return PR_SLOT_A_LOADER_0AEC_BLOCKED_PC;
    }
    return PR_SLOT_A_LOADER_0AEC_STEPPED;
lea:
    c->di = (uint16_t)(c->bp+displacement); c->ip = (uint16_t)(c->ip+4u);
    return PR_SLOT_A_LOADER_0AEC_STEPPED;
push:
    off = (uint16_t)(c->sp-2u); p = span(s,c->ss,off,2u);
    if (!p) return PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY;
    put(p,value); c->sp = off; c->ip = next;
    return PR_SLOT_A_LOADER_0AEC_STEPPED;
far_call:
    off = (uint16_t)(c->sp-4u); p = span(s,c->ss,off,4u);
    if (!p) return PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY;
    target_cs = c->ip == 0xB29u ? 0x1026u : 0x131Cu;
    put(p,(uint16_t)(c->ip+5u)); put(p+2u,c->cs);
    c->sp = off; c->cs = target_cs; c->ip = value;
    return PR_SLOT_A_LOADER_0AEC_STEPPED;
}

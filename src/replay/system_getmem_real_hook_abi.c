#include "poolrad/replay/system_getmem_real_hook_abi.h"
static uint8_t *span(pr_slot_a_directory_state *s, uint16_t seg, uint16_t off, size_t width)
{
    size_t start = ((size_t)seg << 4) + off;
    if ((size_t)off + width > 0x10000u || start > s->ram_size || width > s->ram_size - start) return NULL;
    return s->ram + start;
}
static uint16_t word(const uint8_t *p)
{ return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); }
static void put(uint8_t *p, uint16_t value)
{ p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
static uint16_t subtract(pr_slot_a_directory_cpu *c, uint16_t a, uint16_t b)
{
    uint16_t result = (uint16_t)(a - b), flags = result == 0 ? 0x40u : 0u;
    unsigned ones = 0;
    if (a < b) flags |= 1u;
    if (result & 0x8000u) flags |= 0x80u;
    if ((a ^ b ^ result) & 0x10u) flags |= 0x10u;
    if ((a ^ b) & (a ^ result) & 0x8000u) flags |= 0x800u;
    for (unsigned i = 0; i < 8u; ++i) ones += ((unsigned)result >> i) & 1u;
    if (!(ones & 1u)) flags |= 4u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x8D5u) | flags);
    return result;
}
pr_getmem_real_hook_status pr_system_getmem_real_hook_step(
    pr_slot_a_directory_state *s, pr_system_getmem_report *report)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p;
    uint16_t next, value, segment, offset;
    if (!s || !report) return PR_GETMEM_REAL_HOOK_ARGUMENT;
    *report = (pr_system_getmem_report){0};
    if (!s->ram || s->ram_size > 0x100000u) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs == 0x083Au && c->ip == 0x25u) {
        c->cs = 0x398Au; c->ip = 0x42u; return PR_GETMEM_REAL_HOOK_STEPPED;
    }
    if (c->cs == 0x131Cu && (c->ip == 0x3B7u || c->ip == 0x3C3u)) {
        p = span(s, c->ds, 0x4414u, 4u); if (!p) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
        offset = word(p); segment = word(p + 2u); next = (uint16_t)(c->ip + 4u);
        if (segment != 0x083Au || offset != 0x25u) {
            *report = (pr_system_getmem_report){1u, 0u, c->ip, segment, offset, next};
            return PR_GETMEM_REAL_HOOK_BLOCKED_TARGET;
        }
        p = span(s, c->ss, (uint16_t)(c->sp - 4u), 4u);
        if (!p) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
        put(p, next); put(p + 2u, c->cs); c->sp = (uint16_t)(c->sp - 4u);
        c->cs = segment; c->ip = offset; return PR_GETMEM_REAL_HOOK_STEPPED;
    }
    if (c->cs == 0x131Cu && c->ip == 0x248u) return PR_GETMEM_REAL_HOOK_BLOCKED_WRAPPER;
    if (c->cs == 0x131Cu && c->ip == 0x3C7u) return PR_GETMEM_REAL_HOOK_BLOCKED_POSTHOOK;
    if (c->cs == 0x398Au && c->ip == 0x5Fu) {
        *report = (pr_system_getmem_report){1u, 0u, 0x5Fu, 0xAF8u, 0xB0Bu, 0x64u};
        return PR_GETMEM_REAL_HOOK_BLOCKED_STRING_COPY;
    }
    if (c->cs != 0x398Au && c->cs != 0x131Cu) return PR_GETMEM_REAL_HOOK_BLOCKED_PC;
    next = c->ip;
    if (c->cs == 0x131Cu) {
        switch (c->ip) {
        case 0x3BBu: case 0x3BCu: case 0x3BEu:
            p = span(s, c->ss, c->sp, 2u); if (!p) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
            value = word(p); c->sp = (uint16_t)(c->sp + 2u);
            if (c->ip == 0x3BBu) { c->ax = value; next = 0x3BCu; }
            else if (c->ip == 0x3BCu) { c->dx = value; next = 0x3BDu; }
            else next = value;
            break;
        case 0x3BDu: c->flags &= (uint16_t)~1u; next = 0x3BEu; break;
        default: return PR_GETMEM_REAL_HOOK_BLOCKED_PC;
        }
        c->ip = next; return PR_GETMEM_REAL_HOOK_STEPPED;
    }
    switch (c->ip) {
    case 0x42u: case 0x54u: case 0x58u: case 0x59u: case 0x5Du: case 0x5Eu:
        value = c->ip == 0x42u ? c->bp : c->ip == 0x54u ? c->ax :
            c->ip == 0x58u ? c->ss : c->ip == 0x5Du ? c->cs : c->di;
        p = span(s, c->ss, (uint16_t)(c->sp - 2u), 2u);
        if (!p) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
        put(p, value); c->sp = (uint16_t)(c->sp - 2u); next = (uint16_t)(c->ip + 1u); break;
    case 0x43u: c->bp = c->sp; next = 0x45u; break;
    case 0x45u: c->sp = subtract(c, c->sp, 0x28u); next = 0x48u; break;
    case 0x48u:
        p = span(s, c->ss, (uint16_t)(c->bp + 6u), 2u);
        if (!p) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
        (void)subtract(c, word(p), 0u); next = 0x4Cu; break;
    case 0x4Cu: next = c->flags & 0x41u ? 0xA3u : 0x4Eu; break;
    case 0x4Eu:
        p = span(s, c->ss, (uint16_t)(c->bp - 3u), 1u);
        if (!p) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
        *p = 0u; next = 0x52u; break;
    case 0x52u: c->ax = (uint16_t)((c->ax & 0xFF00u) | 0xEu); next = 0x54u; break;
    case 0x55u: c->di = (uint16_t)(c->bp - 0x20u); next = 0x58u; break;
    case 0x5Au: c->di = 0u; next = 0x5Du; break;
    case 0xA3u:
        p = span(s, c->ss, (uint16_t)(c->bp - 2u), 2u);
        if (!p) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
        c->ax = word(p); next = 0xA6u; break;
    case 0xA6u: c->sp = c->bp; next = 0xA8u; break;
    case 0xA8u:
        p = span(s, c->ss, c->sp, 2u); if (!p) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
        c->bp = word(p); c->sp = (uint16_t)(c->sp + 2u); next = 0xA9u; break;
    case 0xA9u:
        p = span(s, c->ss, c->sp, 4u); if (!p) return PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY;
        next = word(p); c->cs = word(p + 2u); c->sp = (uint16_t)(c->sp + 6u); break;
    default: return PR_GETMEM_REAL_HOOK_BLOCKED_PC;
    }
    c->ip = next; return PR_GETMEM_REAL_HOOK_STEPPED;
}

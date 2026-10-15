#include "poolrad/replay/system_getmem_23f_abi.h"
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
static void put_word(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
pr_system_getmem_status pr_system_getmem_23f_step(
    pr_slot_a_directory_state *s, pr_system_getmem_report *report)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p, *q;
    uint16_t value, next;
    if (!s || !report) return PR_SYSTEM_GETMEM_ARGUMENT;
    *report = (pr_system_getmem_report){0};
    if (!s->ram || s->ram_size > 0x100000u) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs == 0x2A8Eu) {
        if (c->ip == 0x19BAu) return PR_SYSTEM_GETMEM_BLOCKED_CALLER;
        if (c->ip != 0x19B5u) return PR_SYSTEM_GETMEM_BLOCKED_PC;
        p = span(s, c->ss, (uint16_t)(c->sp - 4u), 4u);
        if (!p) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        put_word(p, 0x19BAu); put_word(p + 2u, c->cs);
        c->sp = (uint16_t)(c->sp - 4u); c->cs = 0x131Cu; c->ip = 0x23Fu;
        return PR_SYSTEM_GETMEM_STEPPED;
    }
    if (c->cs != 0x131Cu) return PR_SYSTEM_GETMEM_BLOCKED_PC;
    if (c->ip == 0x245u || c->ip == 0x30Fu) {
        *report = (pr_system_getmem_report){1u, 1u, c->ip, 0xAF8u,
            c->ip == 0x245u ? 0x308u : 0x491u, (uint16_t)(c->ip + 3u)};
        return c->ip == 0x245u ? PR_SYSTEM_GETMEM_BLOCKED_CORE : PR_SYSTEM_GETMEM_BLOCKED_NORMALIZER;
    }
    next = c->ip;
    switch (c->ip) {
    case 0x23Fu:
        p = span(s, c->ss, (uint16_t)(c->sp - 2u), 2u);
        if (!p) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        put_word(p, c->bp); c->sp = (uint16_t)(c->sp - 2u); next = 0x240u; break;
    case 0x240u: c->bp = c->sp; next = 0x242u; break;
    case 0x242u:
        p = span(s, c->ss, (uint16_t)(c->bp + 6u), 2u);
        if (!p) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        c->ax = word(p); next = 0x245u; break;
    case 0x30Cu:
        p = span(s, c->ds, 0x8754u, 2u); if (!p) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        put_word(p, c->ax); next = 0x30Fu; break;
    case 0x363u: case 0x366u:
        p = span(s, c->ss, c->sp, 2u);
        q = span(s, c->es, (uint16_t)(c->di + (c->ip == 0x366u ? 2u : 0u)), 2u);
        if (!p || !q) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        value = word(p); put_word(q, value); c->sp = (uint16_t)(c->sp + 2u);
        next = c->ip == 0x363u ? 0x366u : 0x36Au; break;
    case 0x36Au: case 0x36Eu:
        p = span(s, c->es, (uint16_t)(c->di + (c->ip == 0x36Au ? 4u : 6u)), 2u);
        if (!p) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        put_word(p, c->ip == 0x36Au ? c->cx : c->bx);
        next = c->ip == 0x36Au ? 0x36Eu : 0x372u; break;
    case 0x372u: c->cx = c->di; next = 0x374u; break;
    case 0x374u: c->bx = c->es; next = 0x376u; break;
    case 0x376u: case 0x377u: case 0x37Fu: case 0x380u: case 0x248u:
        p = span(s, c->ss, c->sp, 2u); if (!p) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        value = word(p); c->sp = (uint16_t)(c->sp + 2u);
        switch (c->ip) {
        case 0x376u: c->di = value; next = 0x377u; break;
        case 0x377u: c->es = value; next = 0x378u; break;
        case 0x37Fu: c->ax = value; next = 0x380u; break;
        case 0x380u: c->dx = value; next = 0x381u; break;
        default: c->bp = value; next = 0x249u; break;
        }
        break;
    case 0x378u: case 0x37Bu:
        p = span(s, c->es, (uint16_t)(c->di + (c->ip == 0x37Bu ? 2u : 0u)), 2u);
        if (!p) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        put_word(p, c->ip == 0x378u ? c->cx : c->bx);
        next = c->ip == 0x378u ? 0x37Bu : 0x37Fu; break;
    case 0x381u: c->flags &= (uint16_t)~1u; next = 0x382u; break;
    case 0x382u:
        p = span(s, c->ss, c->sp, 2u); if (!p) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        next = word(p); c->sp = (uint16_t)(c->sp + 2u); break;
    case 0x249u: next = c->flags & 1u ? 0x24Eu : 0x24Bu; break;
    case 0x24Bu:
        p = span(s, c->ss, c->sp, 4u); if (!p) return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
        next = word(p); c->cs = word(p + 2u); c->sp = (uint16_t)(c->sp + 6u); break;
    case 0x24Eu: c->ax = 0xCBu; next = 0x251u; break;
    case 0x251u: next = 0xE2u; break;
    case 0xE2u: return PR_SYSTEM_GETMEM_BLOCKED_RUNTIME_ERROR;
    default: return PR_SYSTEM_GETMEM_BLOCKED_PC;
    }
    c->ip = next; return PR_SYSTEM_GETMEM_STEPPED;
}

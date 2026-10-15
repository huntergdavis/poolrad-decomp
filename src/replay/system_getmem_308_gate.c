#include "poolrad/replay/system_getmem_308_gate.h"
static uint8_t *span(pr_slot_a_directory_state *s, uint16_t seg,
                     uint16_t off, size_t width)
{
    size_t start = ((size_t)seg << 4) + off;
    if ((size_t)off + width > 0x10000u || start > s->ram_size ||
        width > s->ram_size - start) return NULL;
    return s->ram + start;
}
static uint16_t word(const uint8_t *p)
{ return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); }
static void put(uint8_t *p, uint16_t value)
{ p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
static uint16_t szp(uint16_t value, unsigned width)
{
    unsigned ones = 0;
    uint16_t flags = value == 0 ? 0x40u : 0u;
    if (value & (width == 8u ? 0x80u : 0x8000u)) flags |= 0x80u;
    for (unsigned i = 0; i < 8u; ++i) ones += ((unsigned)value >> i) & 1u;
    if (!(ones & 1u)) flags |= 4u;
    return flags;
}
static uint16_t logic(pr_slot_a_directory_cpu *c, uint16_t value, unsigned width)
{
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x8D5u) | szp(value, width));
    return value;
}
static uint16_t arithmetic(pr_slot_a_directory_cpu *c, uint16_t a,
                           uint16_t b, unsigned carry, unsigned width, unsigned sub)
{
    uint32_t mask = width == 8u ? 0xFFu : 0xFFFFu;
    uint32_t sign = width == 8u ? 0x80u : 0x8000u;
    uint32_t wide = sub ? (uint32_t)a - b - carry : (uint32_t)a + b + carry;
    uint16_t result = (uint16_t)(wide & mask), flags = szp(result, width);
    if (sub ? (uint32_t)a < (uint32_t)b + carry : wide > mask) flags |= 1u;
    if (((uint32_t)a ^ b ^ result) & 0x10u) flags |= 0x10u;
    if ((sub ? ((uint32_t)a ^ b) : ~((uint32_t)a ^ b)) & ((uint32_t)a ^ result) & sign)
        flags |= 0x800u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x8D5u) | flags);
    return result;
}
pr_getmem_core_status pr_system_getmem_308_step(
    pr_slot_a_directory_state *s, pr_getmem_flags_profile profile,
    pr_system_getmem_report *report)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p;
    uint16_t next, value, old;
    if (!s || !report) return PR_GETMEM_CORE_ARGUMENT;
    *report = (pr_system_getmem_report){0};
    if (profile != PR_GETMEM_FLAGS_DOSBOX_X_59915C1 && profile != PR_GETMEM_FLAGS_UNICORN_2_1_4)
        return PR_GETMEM_CORE_ARGUMENT;
    if (!s->ram || s->ram_size > 0x100000u) return PR_GETMEM_CORE_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs != 0x131Cu) return PR_GETMEM_CORE_BLOCKED_PC;
    if (c->ip == 0x363u || c->ip == 0x376u) return PR_GETMEM_CORE_BLOCKED_TAIL;
    if (c->ip == 0x248u) return PR_GETMEM_CORE_BLOCKED_RETURN;
    if (c->ip == 0x3B7u || c->ip == 0x3C3u) {
        p = span(s, c->ds, 0x4414u, 4u);
        if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
        *report = (pr_system_getmem_report){1u, 0u, c->ip, word(p + 2u), word(p),
            (uint16_t)(c->ip + 4u)};
        return PR_GETMEM_CORE_BLOCKED_HOOK;
    }
    next = c->ip;
    switch (c->ip) {
    case 0x308u: (void)logic(c, c->ax, 16u); next = 0x30Au; break;
    case 0x30Au: next = c->flags & 0x40u ? 0x383u : 0x30Cu; break;
    case 0x30Cu:
        p = span(s, c->ds, 0x8754u, 2u); if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
        put(p, c->ax); next = 0x30Fu; break;
    case 0x30Fu:
        p = span(s, c->ss, (uint16_t)(c->sp - 2u), 2u);
        if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
        put(p, 0x312u); c->sp = (uint16_t)(c->sp - 2u); next = 0x491u; break;
    case 0x312u: c->di = 0x440Cu; next = 0x315u; break;
    case 0x315u: c->cx = c->ds; next = 0x317u; break;
    case 0x317u: c->es = c->cx; next = 0x319u; break;
    case 0x319u: c->cx = c->di; next = 0x31Bu; break;
    case 0x31Bu: c->bx = c->es; next = 0x31Du; break;
    case 0x31Du:
        p = span(s, c->es, c->di, 4u); if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
        value = word(p); c->es = word(p + 2u); c->di = value; next = 0x320u; break;
    case 0x320u: c->si = c->es; next = 0x322u; break;
    case 0x322u: case 0x328u: case 0x330u: case 0x392u: case 0x39Au:
        if (c->ip == 0x322u || c->ip == 0x392u || c->ip == 0x39Au)
            p = span(s, c->ds, c->ip == 0x322u ? 0x4406u : c->ip == 0x392u ? 0x440Au : 0x4408u, 2u);
        else p = span(s, c->es, (uint16_t)(c->di + (c->ip == 0x328u ? 6u : 4u)), 2u);
        if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
        value = c->ip == 0x322u ? c->si : c->ip == 0x328u || c->ip == 0x392u ? c->dx : c->ax;
        (void)arithmetic(c, value, word(p), 0u, 16u, 1u);
        next = (uint16_t)(c->ip + 4u); break;
    case 0x326u: next = c->flags & 0x40u ? 0x387u : 0x328u; break;
    case 0x32Cu: next = !(c->flags & 0x41u) ? 0x319u : 0x32Eu; break;
    case 0x32Eu: next = c->flags & 1u ? 0x336u : 0x330u; break;
    case 0x334u: next = !(c->flags & 0x41u) ? 0x319u : 0x336u; break;
    case 0x341u: next = c->flags & 0x40u ? 0x376u : 0x343u; break;
    case 0x396u: next = c->flags & 1u ? 0x3A0u : 0x398u; break;
    case 0x398u: next = !(c->flags & 0x41u) ? 0x3BFu : 0x39Au; break;
    case 0x39Eu: next = !(c->flags & 0x41u) ? 0x3BFu : 0x3A0u; break;
    case 0x336u: case 0x337u: case 0x338u: case 0x339u:
    case 0x343u: case 0x344u: case 0x3A0u: case 0x3A1u: case 0x3B6u: case 0x3BFu:
        if (c->ip == 0x3BFu) {
            p = span(s, c->ds, 0x8754u, 2u); if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
            value = word(p);
        } else if (c->ip == 0x336u || c->ip == 0x3A0u) value = c->es;
        else if (c->ip == 0x337u || c->ip == 0x3A1u) value = c->di;
        else if (c->ip == 0x338u || c->ip == 0x343u) value = c->bx;
        else value = c->ip == 0x3B6u ? c->ax : c->cx;
        p = span(s, c->ss, (uint16_t)(c->sp - 2u), 2u);
        if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
        put(p, value); c->sp = (uint16_t)(c->sp - 2u);
        next = c->ip == 0x3BFu ? 0x3C3u : (uint16_t)(c->ip + 1u); break;
    case 0x33Au: case 0x33Du: case 0x345u: case 0x349u:
        p = span(s, c->es, (uint16_t)(c->di + (c->ip == 0x33Au ? 0u : c->ip == 0x33Du ? 2u : c->ip == 0x345u ? 4u : 6u)), 2u);
        if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
        if (c->ip == 0x33Au || c->ip == 0x345u) c->cx = word(p); else c->bx = word(p);
        next = (uint16_t)(c->ip + (c->ip == 0x33Au ? 3u : 4u)); break;
    case 0x34Du: c->cx = arithmetic(c, c->cx, c->ax, 0u, 16u, 1u); next = 0x34Fu; break;
    case 0x34Fu: c->bx = arithmetic(c, c->bx, c->dx, c->flags & 1u, 16u, 1u); next = 0x351u; break;
    case 0x351u: c->cx = logic(c, (uint16_t)(c->cx & 0xFu), 16u); next = 0x354u; break;
    case 0x354u: case 0x387u:
        c->ax = arithmetic(c, c->ax, c->di, 0u, 16u, 0u);
        next = c->ip == 0x354u ? 0x356u : 0x389u; break;
    case 0x356u: case 0x389u:
        c->dx = arithmetic(c, c->dx, c->si, 0u, 16u, 0u);
        next = c->ip == 0x356u ? 0x358u : 0x38Bu; break;
    case 0x358u: case 0x38Bu:
        value = arithmetic(c, (uint16_t)(c->ax & 0xFFu), 0xF0u, 0u, 8u, 0u);
        c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
        next = c->ip == 0x358u ? 0x35Au : 0x38Du; break;
    case 0x35Au: case 0x38Du:
        c->dx = arithmetic(c, c->dx, 0u, c->flags & 1u, 16u, 0u);
        next = c->ip == 0x35Au ? 0x35Du : 0x390u; break;
    case 0x35Du: case 0x390u:
        value = logic(c, (uint16_t)(c->ax & 0xFu), 8u);
        c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
        next = c->ip == 0x35Du ? 0x35Fu : 0x392u; break;
    case 0x35Fu: c->di = c->ax; next = 0x361u; break;
    case 0x361u: c->es = c->dx; next = 0x363u; break;
    case 0x383u: c->ax = logic(c, 0u, 16u); next = 0x385u; break;
    case 0x385u: c->dx = c->ax & 0x8000u ? 0xFFFFu : 0u; next = 0x386u; break;
    case 0x3A2u: c->di = c->cx; next = 0x3A4u; break;
    case 0x3A4u: c->es = c->bx; next = 0x3A6u; break;
    case 0x3A6u: case 0x3A9u: case 0x3ADu: case 0x3B0u:
        p = c->ip == 0x3A6u || c->ip == 0x3A9u ?
            span(s, c->ds, c->ip == 0x3A6u ? 0x4404u : 0x4406u, 2u) :
            span(s, c->es, (uint16_t)(c->di + (c->ip == 0x3ADu ? 0u : 2u)), 2u);
        if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
        put(p, c->ip == 0x3A6u || c->ip == 0x3ADu ? c->ax : c->dx);
        next = c->ip == 0x3ADu ? 0x3B0u : c->ip == 0x3B0u ? 0x3B4u : (uint16_t)(c->ip + (c->ip == 0x3A6u ? 3u : 4u)); break;
    case 0x3B4u: c->ax = logic(c, 0u, 16u); next = 0x3B6u; break;
    case 0x491u: c->ax = arithmetic(c, c->ax, 7u, 0u, 16u, 0u); next = 0x494u; break;
    case 0x494u: c->dx = c->ax; next = 0x496u; break;
    case 0x496u:
        old = c->dx; value = (uint16_t)((old >> 1) | ((c->flags & 1u) << 15));
        c->flags = (uint16_t)((c->flags & (uint16_t)~0x801u) | (old & 1u) |
            ((((value >> 15) ^ (value >> 14)) & 1u) << 11));
        c->dx = value; next = 0x498u; break;
    case 0x498u: case 0x49Au: case 0x49Cu:
        old = c->dx; c->dx = (uint16_t)(old >> 1);
        c->flags = (uint16_t)((c->flags & (uint16_t)~0x8D5u) | szp(c->dx, 16u) |
            (old & 1u) | ((old & 0x8000u) >> 4) |
            (profile == PR_GETMEM_FLAGS_DOSBOX_X_59915C1 ? 0x10u : 0u));
        next = (uint16_t)(c->ip + 2u); break;
    case 0x49Eu: c->ax = logic(c, (uint16_t)(c->ax & 8u), 16u); next = 0x4A1u; break;
    case 0x4A1u: case 0x386u:
        p = span(s, c->ss, c->sp, 2u); if (!p) return PR_GETMEM_CORE_BLOCKED_MEMORY;
        next = word(p); c->sp = (uint16_t)(c->sp + 2u); break;
    default: return PR_GETMEM_CORE_BLOCKED_PC;
    }
    c->ip = next; return PR_GETMEM_CORE_STEPPED;
}

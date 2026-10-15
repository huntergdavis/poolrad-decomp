#include "poolrad/replay/slot_a_caller_19ba_setup.h"
static uint8_t *word_span(pr_slot_a_directory_state *s, uint16_t off)
{
    size_t start = ((size_t)s->cpu.ss << 4) + off;
    if ((size_t)off + 2u > 0x10000u || start > s->ram_size ||
        2u > s->ram_size - start) return NULL;
    return s->ram + start;
}
static uint16_t get(const uint8_t *p)
{ return (uint16_t)((unsigned)p[0] | ((unsigned)p[1] << 8)); }
static void put(uint8_t *p, uint16_t value)
{ p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
pr_slot_a_caller_19ba_status pr_slot_a_caller_19ba_step(
    pr_slot_a_directory_state *s, pr_slot_a_caller_19ba_report *report)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p, *dest;
    uint16_t value, next, off;
    if (!s || !report) return PR_SLOT_A_CALLER_19BA_ARGUMENT;
    *report = (pr_slot_a_caller_19ba_report){0};
    if (!s->ram || s->ram_size > 0x100000u) return PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs != 0x2A8Eu) return PR_SLOT_A_CALLER_19BA_BLOCKED_PC;
    switch (c->ip) {
    case 0x19BAu: case 0x19BEu:
        off = (uint16_t)(c->bp - (c->ip == 0x19BAu ? 0x1CDu : 0x1CBu));
        p = word_span(s, off);
        if (!p) return PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY;
        value = c->ip == 0x19BAu ? c->ax : c->dx;
        put(p, value); c->ip = (uint16_t)(c->ip + 4u);
        return PR_SLOT_A_CALLER_19BA_STEPPED;
    case 0x19C2u:
        c->di = (uint16_t)(c->bp - 0x1F6u); c->ip = 0x19C6u;
        return PR_SLOT_A_CALLER_19BA_STEPPED;
    case 0x19D0u: case 0x19D3u:
        c->ax = (uint16_t)((c->ax & 0xFF00u) | 1u);
        c->ip = (uint16_t)(c->ip + 2u);
        return PR_SLOT_A_CALLER_19BA_STEPPED;
    case 0x19C6u: value = c->ss; next = 0x19C7u; break;
    case 0x19C7u: value = c->di; next = 0x19C8u; break;
    case 0x19C8u: case 0x19CCu:
        off = (uint16_t)(c->bp - (c->ip == 0x19C8u ? 0x1CBu : 0x1CDu));
        p = word_span(s, off);
        if (!p) return PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY;
        value = get(p); next = (uint16_t)(c->ip + 4u); break;
    case 0x19D2u: value = c->ax; next = 0x19D3u; break;
    case 0x19D5u: value = c->ax; next = 0x19D6u; break;
    case 0x19D6u: value = c->cs; next = 0x19D7u; break;
    case 0x19D7u:
        *report = (pr_slot_a_caller_19ba_report){1u, 0x6Fu, 0x19D7u, 0x6Fu, 0xAECu, 0x19DAu, 1u};
        return PR_SLOT_A_CALLER_19BA_BLOCKED_LOADER;
    default: return PR_SLOT_A_CALLER_19BA_BLOCKED_PC;
    }
    off = (uint16_t)(c->sp - 2u); dest = word_span(s, off);
    if (!dest) return PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY;
    put(dest, value); c->sp = off; c->ip = next;
    return PR_SLOT_A_CALLER_19BA_STEPPED;
}

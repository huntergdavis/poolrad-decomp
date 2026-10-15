#include "poolrad/replay/slot_a_file_probe_02eb_abi.h"

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
static uint16_t subtract_flags(uint16_t old, unsigned a, unsigned b, unsigned bits)
{
    unsigned mask = bits == 8u ? 255u : 65535u;
    unsigned sign = 1u << (bits - 1u), value = (a - b) & mask;
    unsigned parity = value & 255u;
    uint16_t flags = (uint16_t)(old & (uint16_t)~0x08D5u);
    parity ^= parity >> 4; parity ^= parity >> 2; parity ^= parity >> 1;
    if (!(parity & 1u)) flags |= 4u;
    if (!value) flags |= 0x40u;
    if (value & sign) flags |= 0x80u;
    if (a < b) flags |= 1u;
    if ((a ^ b ^ value) & 0x10u) flags |= 0x10u;
    if ((a ^ b) & (a ^ value) & sign) flags |= 0x800u;
    return flags;
}

pr_slot_a_file_probe_status pr_slot_a_file_probe_02eb_step(
    pr_slot_a_directory_state *s, pr_slot_a_string_0356_report *report)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p;
    uint16_t next, value;
    if (!s || !report) return PR_SLOT_A_FILE_PROBE_ARGUMENT;
    *report = (pr_slot_a_string_0356_report){0};
    if (!s->ram || s->ram_size > 0x100000u) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs == 0x2A8Eu) {
        if (c->ip == 0x19ADu) return PR_SLOT_A_FILE_PROBE_BLOCKED_CALLER;
        if (c->ip != 0x19A8u) return PR_SLOT_A_FILE_PROBE_BLOCKED_PC;
        p = span(s, c->ss, (uint16_t)(c->sp - 4u), 4u);
        if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        put_word(p, 0x19ADu); put_word(p + 2u, c->cs);
        c->sp = (uint16_t)(c->sp - 4u); c->cs = 0x1026u; c->ip = 0x02EBu;
        return PR_SLOT_A_FILE_PROBE_STEPPED;
    }
    if (c->cs == 0x131Cu && c->ip >= 0xB25u && c->ip < 0xB49u) {
        pr_slot_a_string_0356_report child_report;
        pr_slot_a_string_0356_status status = pr_slot_a_string_0356_prefix_step(s, &child_report);
        if (status == PR_SLOT_A_STRING_0356_STEPPED) return PR_SLOT_A_FILE_PROBE_STEPPED;
        if (status == PR_SLOT_A_STRING_0356_BLOCKED_MEMORY) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        return PR_SLOT_A_FILE_PROBE_BLOCKED_PC;
    }
    if (c->cs != 0x1026u) return PR_SLOT_A_FILE_PROBE_BLOCKED_PC;
    if (c->ip == 0x310u || c->ip == 0x31Fu || c->ip == 0x32Au || c->ip == 0x32Fu) {
        value = c->ip == 0x310u ? 0x13E2u : (c->ip == 0x31Fu ? 0x1410u :
                (c->ip == 0x32Au ? 0x1491u : 0x04A2u));
        *report = (pr_slot_a_string_0356_report){
            1u, 0x0802u, c->ip, 0x0AF8u, value, (uint16_t)(c->ip + 5u), 0u};
        return PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME;
    }
    next = c->ip;
    switch (c->ip) {
    case 0x2EBu:
        p = span(s, c->ss, (uint16_t)(c->sp - 2u), 2u);
        if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        put_word(p, c->bp); c->sp = (uint16_t)(c->sp - 2u); next = 0x2ECu; break;
    case 0x2ECu: c->bp = c->sp; next = 0x2EEu; break;
    case 0x2EEu:
        c->flags = subtract_flags(c->flags, c->sp, 0xAAu, 16u);
        c->sp = (uint16_t)(c->sp - 0xAAu); next = 0x2F2u; break;
    case 0x2F2u:
        p = span(s, c->ss, (uint16_t)(c->bp + 6u), 4u);
        if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        c->di = word(p); c->es = word(p + 2u); next = 0x2F5u; break;
    case 0x2F7u: case 0x30Bu: c->di = (uint16_t)(c->bp - 0x2Au); next = (uint16_t)(c->ip + 3u); break;
    case 0x305u: case 0x315u: case 0x324u:
        c->di = (uint16_t)(c->bp - 0xAAu); next = (uint16_t)(c->ip + 4u); break;
    case 0x2FCu: c->ax = 0x28u; next = 0x2FFu; break;
    case 0x31Bu: c->ax = 0x80u; next = 0x31Eu; break;
    case 0x2F5u: case 0x2F6u: case 0x2FAu: case 0x2FBu: case 0x2FFu:
    case 0x309u: case 0x30Au: case 0x30Eu: case 0x30Fu: case 0x319u:
    case 0x31Au: case 0x31Eu: case 0x328u: case 0x329u:
        switch (c->ip) {
        case 0x2F5u: value = c->es; break;
        case 0x2FAu: case 0x309u: case 0x30Eu: case 0x319u: case 0x328u: value = c->ss; break;
        case 0x2FFu: case 0x31Eu: value = c->ax; break;
        default: value = c->di; break;
        }
        p = span(s, c->ss, (uint16_t)(c->sp - 2u), 2u);
        if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        put_word(p, value); c->sp = (uint16_t)(c->sp - 2u); next = (uint16_t)(c->ip + 1u); break;
    case 0x300u:
        p = span(s, c->ss, (uint16_t)(c->sp - 4u), 4u);
        if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        put_word(p, 0x305u); put_word(p + 2u, c->cs);
        c->sp = (uint16_t)(c->sp - 4u); c->cs = 0x131Cu; next = 0xB25u; break;
    case 0x334u:
        p = span(s, c->ds, 0x49CEu, 2u); if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        put_word(p, c->ax); next = 0x337u; break;
    case 0x337u:
        p = span(s, c->ds, 0x49CEu, 2u); if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        c->flags = subtract_flags(c->flags, word(p), 0u, 16u); next = 0x33Cu; break;
    case 0x33Cu: next = c->flags & 0x40u ? 0x33Eu : 0x344u; break;
    case 0x33Eu:
        p = span(s, c->ss, (uint16_t)(c->bp - 0x2Au), 1u); if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        c->flags = subtract_flags(c->flags, *p, 0u, 8u); next = 0x342u; break;
    case 0x342u: next = c->flags & 0x40u ? 0x344u : 0x348u; break;
    case 0x344u: c->ax &= 0xFF00u; next = 0x346u; break;
    case 0x346u: next = 0x34Au; break;
    case 0x348u: c->ax = (uint16_t)((c->ax & 0xFF00u) | 1u); next = 0x34Au; break;
    case 0x34Au:
        p = span(s, c->ss, (uint16_t)(c->bp - 1u), 1u); if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        *p = (uint8_t)c->ax; next = 0x34Du; break;
    case 0x34Du:
        p = span(s, c->ss, (uint16_t)(c->bp - 1u), 1u); if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        c->ax = (uint16_t)((c->ax & 0xFF00u) | *p); next = 0x350u; break;
    case 0x350u: c->sp = c->bp; next = 0x352u; break;
    case 0x352u:
        p = span(s, c->ss, c->sp, 2u); if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        value = word(p); c->sp = (uint16_t)(c->sp + 2u); c->bp = value; next = 0x353u; break;
    case 0x353u:
        p = span(s, c->ss, c->sp, 4u); if (!p) return PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY;
        next = word(p); c->cs = word(p + 2u); c->sp = (uint16_t)(c->sp + 8u); break;
    default: return PR_SLOT_A_FILE_PROBE_BLOCKED_PC;
    }
    c->ip = next; return PR_SLOT_A_FILE_PROBE_STEPPED;
}

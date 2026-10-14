#include "poolrad/replay/slot_a_continuation_1974.h"
#include "poolrad/replay/slot_a_saved_copy_1965.h"

static uint8_t *span(pr_slot_a_directory_state *s, uint16_t off, size_t size)
{
    size_t start = ((size_t)s->cpu.ss << 4) + off;
    if ((size_t)off + size > 0x10000u || start > s->ram_size ||
        size > s->ram_size - start) return NULL;
    return s->ram + start;
}
static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
}

pr_slot_a_continuation_1974_status pr_slot_a_continuation_1974_step(
    pr_slot_a_directory_state *s, pr_slot_a_continuation_1974_report *report)
{
    pr_slot_a_directory_cpu *c;
    uint16_t value, next;
    uint8_t *p;
    if (!s || !report) return PR_SLOT_A_CONTINUATION_1974_ARGUMENT;
    *report = (pr_slot_a_continuation_1974_report){0};
    if (!s->ram || s->ram_size > 0x100000u)
        return PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs == 0x131Cu) {
        if (c->ip >= 0xB25u && c->ip < 0xB49u) {
            pr_slot_a_string_0356_status status = pr_slot_a_string_0356_step(s, report);
            if (status == PR_SLOT_A_STRING_0356_STEPPED) return PR_SLOT_A_CONTINUATION_1974_STEPPED;
            if (status == PR_SLOT_A_STRING_0356_BLOCKED_MEMORY) return PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY;
        } else if (c->ip >= 0xB0Bu && c->ip < 0xB25u) {
            pr_slot_a_saved_copy_report copy_report;
            pr_slot_a_saved_copy_status status = pr_slot_a_saved_copy_1965_step(s, &copy_report);
            if (status == PR_SLOT_A_SAVED_COPY_STEPPED) return PR_SLOT_A_CONTINUATION_1974_STEPPED;
            if (status == PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY) return PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY;
        }
        return PR_SLOT_A_CONTINUATION_1974_BLOCKED_PC;
    }
    if (c->cs != 0x2A8Eu) return PR_SLOT_A_CONTINUATION_1974_BLOCKED_PC;
    switch (c->ip) {
    case 0x1974u: c->di = (uint16_t)(c->bp - 0x1F6u); c->ip = 0x1978u; break;
    case 0x197Au: c->ax = 0x28u; c->ip = 0x197Du; break;
    case 0x1983u: c->di = (uint16_t)(c->bp - 0x312u); c->ip = 0x1987u; break;
    case 0x1989u: c->di = 0x537Au; c->ip = 0x198Cu; break;
    case 0x1993u: c->di = (uint16_t)(c->bp - 0x1F6u); c->ip = 0x1997u; break;
    case 0x1978u: case 0x1987u: case 0x1997u:
    case 0x1979u: case 0x1988u: case 0x198Du: case 0x1998u:
    case 0x197Du: case 0x198Cu:
        switch (c->ip) {
        case 0x1978u: case 0x1987u: case 0x1997u: value = c->ss; break;
        case 0x197Du: value = c->ax; break;
        case 0x198Cu: value = c->ds; break;
        default: value = c->di; break;
        }
        p = span(s, (uint16_t)(c->sp - 2u), 2u);
        if (!p) return PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY;
        put_word(p, value); c->sp = (uint16_t)(c->sp - 2u); ++c->ip; break;
    case 0x197Eu: case 0x198Eu:
        next = c->ip == 0x197Eu ? 0x1983u : 0x1993u;
        value = c->ip == 0x197Eu ? 0xB25u : 0xB0Bu;
        p = span(s, (uint16_t)(c->sp - 4u), 4u);
        if (!p) return PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY;
        put_word(p, next); put_word(p + 2u, c->cs);
        c->sp = (uint16_t)(c->sp - 4u); c->cs = 0x131Cu; c->ip = value; break;
    case 0x1999u:
        *report = (pr_slot_a_continuation_1974_report){
            1u, 0x6Fu, 0x1999u, 0x0AF8u, 0x0B98u, 0x199Eu, 1u};
        return PR_SLOT_A_CONTINUATION_1974_BLOCKED_B98_ABI;
    default: return PR_SLOT_A_CONTINUATION_1974_BLOCKED_PC;
    }
    return PR_SLOT_A_CONTINUATION_1974_STEPPED;
}

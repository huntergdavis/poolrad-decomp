#include "poolrad/replay/u6f_01b8_entry_abi.h"

static uint8_t *stack_word(pr_slot_a_directory_state *s, uint16_t off)
{
    size_t p = ((size_t)s->cpu.ss << 4) + off;
    if (off == 0xffffu || p > s->ram_size || s->ram_size - p < 2u)
        return NULL;
    return s->ram + p;
}

/* Same defined subtraction flags as the reused 02AE ABI. */
static uint16_t sub_flags(uint16_t old, uint16_t a, uint16_t b)
{
    uint16_t value = (uint16_t)(a - b);
    uint16_t f = (uint16_t)(old & (uint16_t)~0x08d5u);
    unsigned parity = value & 255u;
    parity ^= parity >> 4; parity ^= parity >> 2; parity ^= parity >> 1;
    if (!(parity & 1u)) f |= 4u;
    if (!value) f |= 0x40u;
    if (value & 0x8000u) f |= 0x80u;
    if (a < b) f |= 1u;
    if ((a ^ b ^ value) & 0x10u) f |= 0x10u;
    if ((a ^ b) & (a ^ value) & 0x8000u) f |= 0x800u;
    return f;
}

pr_u6f_01b8_entry_status pr_u6f_01b8_entry_step(
    pr_slot_a_directory_state *s,
    pr_slot_a_directory_service service, void *context,
    pr_slot_a_directory_write *journal, size_t capacity)
{
    pr_slot_a_directory_cpu *c;
    pr_u6f_02ae_status result;
    uint8_t *p;
    uint16_t value, next_sp;
    if (!s || (capacity && !journal)) return PR_U6F_01B8_ENTRY_ARGUMENT;
    if (!s->ram || s->ram_size > 0x100000u)
        return PR_U6F_01B8_ENTRY_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs == 0x2a8eu) {
        switch (c->ip) {
        case 0x01c3u: return PR_U6F_01B8_ENTRY_BOUNDARY;
        case 0x01b9u: c->bp = c->sp; c->ip = 0x01bbu; break;
        case 0x01bbu:
            c->flags = sub_flags(c->flags, c->sp, 0x0306u);
            c->sp = (uint16_t)(c->sp - 0x0306u);
            c->ip = 0x01bfu;
            break;
        case 0x01b8u: case 0x01bfu: case 0x01c0u:
            next_sp = (uint16_t)(c->sp - 2u);
            p = stack_word(s, next_sp);
            if (!p) return PR_U6F_01B8_ENTRY_BLOCKED_MEMORY;
            value = c->ip == 0x01b8u ? c->bp :
                    c->ip == 0x01bfu ? c->cs : 0x01c3u;
            p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
            c->sp = next_sp;
            c->ip = c->ip == 0x01b8u ? 0x01b9u :
                    c->ip == 0x01bfu ? 0x01c0u : 0x02aeu;
            break;
        default: goto callee;
        }
        return PR_U6F_01B8_ENTRY_STEPPED;
    }
callee:
    result = pr_u6f_02ae_step(s, service, context, journal, capacity);
    switch (result) {
    case PR_U6F_02AE_STEPPED: return PR_U6F_01B8_ENTRY_STEPPED;
    case PR_U6F_02AE_RETURNED:
        return c->cs == 0x2a8eu && c->ip == 0x01c3u ?
            PR_U6F_01B8_ENTRY_BOUNDARY : PR_U6F_01B8_ENTRY_TRANSFER;
    case PR_U6F_02AE_BLOCKED_MESSAGE: return PR_U6F_01B8_ENTRY_BLOCKED_UI;
    case PR_U6F_02AE_BLOCKED_DOS_SERVICE: return PR_U6F_01B8_ENTRY_BLOCKED_DOS;
    case PR_U6F_02AE_BLOCKED_MEMORY: return PR_U6F_01B8_ENTRY_BLOCKED_MEMORY;
    case PR_U6F_02AE_BLOCKED_PC: return PR_U6F_01B8_ENTRY_BLOCKED_PC;
    case PR_U6F_02AE_BAD_EVENT: return PR_U6F_01B8_ENTRY_BAD_EVENT;
    case PR_U6F_02AE_ARGUMENT: return PR_U6F_01B8_ENTRY_ARGUMENT;
    }
    return PR_U6F_01B8_ENTRY_BLOCKED_PC;
}

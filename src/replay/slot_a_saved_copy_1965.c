#include "poolrad/replay/slot_a_saved_copy_1965.h"

static uint8_t *span(pr_slot_a_directory_state *s, uint16_t seg,
                     uint16_t off, size_t width)
{
    size_t start = ((size_t)seg << 4) + off;
    if ((size_t)off + width > 0x10000u || start > s->ram_size ||
        width > s->ram_size - start)
        return NULL;
    return s->ram + start;
}

static uint16_t word(const uint8_t *at)
{
    return (uint16_t)((uint16_t)at[0] | ((uint16_t)at[1] << 8));
}

static void put_word(uint8_t *at, uint16_t value)
{
    at[0] = (uint8_t)value;
    at[1] = (uint8_t)(value >> 8);
}

pr_slot_a_saved_copy_status pr_slot_a_saved_copy_1965_step(
    pr_slot_a_directory_state *s, pr_slot_a_saved_copy_report *report)
{
    uint8_t *at, *source, *destination;
    uint16_t next_sp, return_ip, return_cs, count;
    size_t i;
    if (!s || !report) return PR_SLOT_A_SAVED_COPY_ARGUMENT;
    *report = (pr_slot_a_saved_copy_report){0};
    if (!s->ram || s->ram_size > 0x100000u)
        return PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY;

    if (s->cpu.cs == 0x2A8Eu) {
        if (s->cpu.ip == 0x196Au) {
            *report = (pr_slot_a_saved_copy_report){
                1u, 0x6Fu, 0x196Au, 0x0802u, 0x0356u, 0x196Fu};
            return PR_SLOT_A_SAVED_COPY_BLOCKED_0356_ABI;
        }
        if (s->cpu.ip != 0x1965u)
            return PR_SLOT_A_SAVED_COPY_BLOCKED_PC;
        /* Far CALL pushes CS then return IP. Preflight both stores. */
        next_sp = (uint16_t)(s->cpu.sp - 4u);
        at = span(s, s->cpu.ss, next_sp, 4u);
        if (!at) return PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY;
        put_word(at + 2u, s->cpu.cs);
        put_word(at, 0x196Au);
        s->cpu.sp = next_sp;
        s->cpu.cs = 0x131Cu;
        s->cpu.ip = 0x0B0Bu;
        return PR_SLOT_A_SAVED_COPY_STEPPED;
    }
    if (s->cpu.cs != 0x131Cu) return PR_SLOT_A_SAVED_COPY_BLOCKED_PC;
    switch (s->cpu.ip) {
    case 0x0B0Bu: /* CLD */
        s->cpu.flags &= (uint16_t)~0x0400u;
        s->cpu.ip = 0x0B0Cu;
        break;
    case 0x0B0Cu: /* MOV BX,SP */
        s->cpu.bx = s->cpu.sp;
        s->cpu.ip = 0x0B0Eu;
        break;
    case 0x0B0Eu: /* MOV DX,DS */
        s->cpu.dx = s->cpu.ds;
        s->cpu.ip = 0x0B10u;
        break;
    case 0x0B10u: /* LES DI,SS:[BX+8] */
        at = span(s, s->cpu.ss, (uint16_t)(s->cpu.bx + 8u), 4u);
        if (!at) return PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY;
        s->cpu.di = word(at);
        s->cpu.es = word(at + 2u);
        s->cpu.ip = 0x0B14u;
        break;
    case 0x0B14u: /* LDS SI,SS:[BX+4] */
        at = span(s, s->cpu.ss, (uint16_t)(s->cpu.bx + 4u), 4u);
        if (!at) return PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY;
        s->cpu.si = word(at);
        s->cpu.ds = word(at + 2u);
        s->cpu.ip = 0x0B18u;
        break;
    case 0x0B18u: /* LODSB, after the reached CLD */
        if (s->cpu.flags & 0x0400u) return PR_SLOT_A_SAVED_COPY_BLOCKED_PC;
        at = span(s, s->cpu.ds, s->cpu.si, 1u);
        if (!at) return PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY;
        s->cpu.ax = (uint16_t)((s->cpu.ax & 0xFF00u) | *at);
        ++s->cpu.si;
        s->cpu.ip = 0x0B19u;
        break;
    case 0x0B19u: /* STOSB */
        if (s->cpu.flags & 0x0400u) return PR_SLOT_A_SAVED_COPY_BLOCKED_PC;
        at = span(s, s->cpu.es, s->cpu.di, 1u);
        if (!at) return PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY;
        *at = (uint8_t)s->cpu.ax;
        ++s->cpu.di;
        s->cpu.ip = 0x0B1Au;
        break;
    case 0x0B1Au: /* MOV CL,AL */
        s->cpu.cx = (uint16_t)((s->cpu.cx & 0xFF00u) | (uint8_t)s->cpu.ax);
        s->cpu.ip = 0x0B1Cu;
        break;
    case 0x0B1Cu: /* XOR CH,CH; AF is undefined, observed zero. */
        s->cpu.cx &= 0x00FFu;
        s->cpu.flags = (uint16_t)((s->cpu.flags & (uint16_t)~0x08D5u) | 0x0044u);
        s->cpu.ip = 0x0B1Eu;
        break;
    case 0x0B1Eu: /* REP MOVSB, forward byte order even for overlap */
        if (s->cpu.flags & 0x0400u) return PR_SLOT_A_SAVED_COPY_BLOCKED_PC;
        count = s->cpu.cx;
        if (count != 0u) {
            source = span(s, s->cpu.ds, s->cpu.si, count);
            destination = span(s, s->cpu.es, s->cpu.di, count);
            if (!source || !destination)
                return PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY;
            for (i = 0u; i < count; ++i) destination[i] = source[i];
        }
        s->cpu.si = (uint16_t)(s->cpu.si + count);
        s->cpu.di = (uint16_t)(s->cpu.di + count);
        s->cpu.cx = 0u;
        s->cpu.ip = 0x0B20u;
        break;
    case 0x0B20u: /* MOV DS,DX */
        s->cpu.ds = s->cpu.dx;
        s->cpu.ip = 0x0B22u;
        break;
    case 0x0B22u: /* RETF 4 consumes return frame and source pointer */
        at = span(s, s->cpu.ss, s->cpu.sp, 4u);
        if (!at) return PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY;
        return_ip = word(at);
        return_cs = word(at + 2u);
        s->cpu.sp = (uint16_t)(s->cpu.sp + 8u);
        s->cpu.ip = return_ip;
        s->cpu.cs = return_cs;
        break;
    default:
        return PR_SLOT_A_SAVED_COPY_BLOCKED_PC;
    }
    return PR_SLOT_A_SAVED_COPY_STEPPED;
}

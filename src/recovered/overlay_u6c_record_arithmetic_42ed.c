/* U6C +42ED with SYSTEM +0A5B/+0A76/+0AFC; evidence documents the boundary. */
#include "poolrad/recovered/overlay_u6c_record_arithmetic_42ed.h"
#define MASK (PR_U6C_42ED_RAM_SIZE - 1u)
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}
static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8u);
}
static size_t frame(const pr_u6c_42ed_cpu *cpu, int offset)
{
    return physical(cpu->ss, (uint16_t)(cpu->bp + offset));
}
static void push(uint8_t *ram, pr_u6c_42ed_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, physical(cpu->ss, cpu->sp), value);
}
static uint16_t pop(const uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    uint16_t value = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}
static uint16_t arithmetic(pr_u6c_42ed_cpu *cpu, uint16_t left, uint16_t right,
                           unsigned carry, unsigned bits, int subtract)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu;
    uint32_t sign = (mask + 1u) >> 1u;
    uint32_t wide = subtract ? (uint32_t)left - right - carry
                             : (uint32_t)left + right + carry;
    uint16_t result = (uint16_t)(wide & mask);
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (subtract ? (uint32_t)left < (uint32_t)right + carry : wide > mask)
        cpu->flags |= 1u;
    if ((parity & 1u) == 0u) cpu->flags |= 4u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & sign) != 0u) cpu->flags |= 0x80u;
    if (((subtract ? left ^ right : (uint16_t)~(left ^ right)) &
         (left ^ result) & sign) != 0u) cpu->flags |= 0x800u;
    return result;
}
static void logical(pr_u6c_42ed_cpu *cpu, uint16_t value)
{
    (void)arithmetic(cpu, value, 0, 0, 16, 0);
    cpu->flags &= (uint16_t)~0x0811u; /* OR/XOR: undefined AF chosen zero. */
}
static uint16_t rcl(pr_u6c_42ed_cpu *cpu, uint16_t value)
{
    uint16_t result = (uint16_t)((value << 1u) | (cpu->flags & 1u));
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x0801u) |
        (value >> 15u) | (((result >> 15u) ^ (value >> 15u)) << 11u));
    return result;
}
static void far_return(const uint8_t *ram, pr_u6c_42ed_cpu *cpu, unsigned cleanup)
{
    size_t at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + cleanup);
}
static void multiply(uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    uint32_t product;
    cpu->si = cpu->ax;
    cpu->di = cpu->dx;
    product = (uint32_t)cpu->ax * cpu->cx;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16u);
    push(ram, cpu, cpu->ax);
    push(ram, cpu, cpu->dx);
    product = (uint32_t)cpu->si * cpu->bx;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16u);
    cpu->bx = cpu->ax;
    product = (uint32_t)cpu->di * cpu->cx;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16u);
    cpu->cx = cpu->ax;
    cpu->dx = pop(ram, cpu);
    cpu->ax = pop(ram, cpu);
    cpu->dx = arithmetic(cpu, cpu->dx, cpu->bx, 0, 16, 0);
    cpu->dx = arithmetic(cpu, cpu->dx, cpu->cx, 0, 16, 0);
    far_return(ram, cpu, 0);
}
static void negate_pair(pr_u6c_42ed_cpu *cpu, uint16_t *low, uint16_t *high)
{
    *low = (uint16_t)~*low;
    *high = (uint16_t)~*high;
    *low = arithmetic(cpu, *low, 1, 0, 16, 0);
    *high = arithmetic(cpu, *high, 0, cpu->flags & 1u, 16, 0);
}
static void absolute(const uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    logical(cpu, cpu->dx);
    if ((cpu->dx & 0x8000u) != 0u) negate_pair(cpu, &cpu->ax, &cpu->dx);
    far_return(ram, cpu, 0);
}
static int divide(uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    push(ram, cpu, cpu->bp);
    cpu->bp = (uint16_t)(cpu->cx | cpu->bx);
    logical(cpu, cpu->bp);
    if (cpu->bp == 0u) {
        cpu->bp = pop(ram, cpu);
        cpu->ax = 0x00C8u;
        cpu->ip = 0x00E2u;
        return 0;
    }
    logical(cpu, cpu->bx);
    push(ram, cpu, cpu->flags);
    if ((cpu->bx & 0x8000u) != 0u) negate_pair(cpu, &cpu->cx, &cpu->bx);
    logical(cpu, cpu->dx);
    push(ram, cpu, cpu->flags);
    if ((cpu->dx & 0x8000u) != 0u) negate_pair(cpu, &cpu->ax, &cpu->dx);
    cpu->si = cpu->cx;
    cpu->di = cpu->bx;
    cpu->cx = cpu->bx = 0;
    logical(cpu, 0);
    cpu->bp = 0x21u;
    do {
        unsigned carry;
        uint16_t saved_carry;
        cpu->cx = rcl(cpu, cpu->cx);
        cpu->bx = rcl(cpu, cpu->bx);
        cpu->cx = arithmetic(cpu, cpu->cx, cpu->si, 0, 16, 1);
        carry = cpu->flags & 1u;
        cpu->bx = arithmetic(cpu, cpu->bx, cpu->di, carry, 16, 1);
        if ((cpu->flags & 1u) != 0u) {
            cpu->cx = arithmetic(cpu, cpu->cx, cpu->si, 0, 16, 0);
            cpu->bx = arithmetic(cpu, cpu->bx, cpu->di, cpu->flags & 1u, 16, 0);
        }
        cpu->flags ^= 1u;
        cpu->ax = rcl(cpu, cpu->ax);
        cpu->dx = rcl(cpu, cpu->dx);
        saved_carry = cpu->flags & 1u;
        cpu->bp = arithmetic(cpu, cpu->bp, 1, 0, 16, 1);
        cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~1u) | saved_carry);
    } while (cpu->bp != 0u);
    cpu->flags = pop(ram, cpu);
    if ((cpu->flags & 0x80u) != 0u) {
        negate_pair(cpu, &cpu->cx, &cpu->bx);
        cpu->flags = pop(ram, cpu);
        if ((cpu->flags & 0x80u) == 0u) negate_pair(cpu, &cpu->ax, &cpu->dx);
    } else {
        cpu->flags = pop(ram, cpu);
        if ((cpu->flags & 0x80u) != 0u) negate_pair(cpu, &cpu->ax, &cpu->dx);
    }
    cpu->bp = pop(ram, cpu);
    far_return(ram, cpu, 0);
    return 1;
}
static pr_u6c_42ed_status runtime(uint8_t *ram, pr_u6c_42ed_cpu *cpu,
                                const pr_u6c_42ed_layout *layout,
                                uint16_t target, uint16_t continuation)
{
    uint16_t caller_cs = cpu->cs;
    push(ram, cpu, cpu->cs);
    push(ram, cpu, continuation);
    cpu->cs = layout->system_cs;
    cpu->ip = target;
    if (target == 0x0A5Bu) multiply(ram, cpu);
    else if (target == 0x0AFCu) absolute(ram, cpu);
    else if (!divide(ram, cpu)) return PR_U6C_42ED_UNSUPPORTED_RUNTIME;
    return cpu->cs == caller_cs && cpu->ip == continuation
        ? PR_U6C_42ED_OK : PR_U6C_42ED_TRANSFER;
}
static uint16_t signed_index(const uint8_t *ram, const pr_u6c_42ed_cpu *cpu)
{
    uint8_t index = ram[frame(cpu, -3)];
    return index < 0x80u ? index : (uint16_t)(0xFF00u | index);
}
static void load_table(const uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    cpu->ax = signed_index(ram, cpu);
    cpu->di = (uint16_t)(cpu->ax << 2u);
    cpu->cx = word_at(ram, physical(cpu->ds, (uint16_t)(cpu->di + 0x0D24u)));
    cpu->bx = word_at(ram, physical(cpu->ds, (uint16_t)(cpu->di + 0x0D26u)));
}
static void load_record(const uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    cpu->ax = (uint16_t)(signed_index(ram, cpu) << 1u);
    cpu->di = word_at(ram, frame(cpu, -7));
    cpu->es = word_at(ram, (frame(cpu, -7) + 2u) & MASK);
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
}
static void load_amount(const uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    cpu->ax = word_at(ram, frame(cpu, 10));
    cpu->dx = word_at(ram, frame(cpu, 12));
}
static void subtract_amount(uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    uint16_t low = arithmetic(cpu, word_at(ram, frame(cpu, 10)), cpu->ax, 0, 16, 1);
    put_word(ram, frame(cpu, 10), low);
    low = arithmetic(cpu, word_at(ram, frame(cpu, 12)), cpu->dx, cpu->flags & 1u, 16, 1);
    put_word(ram, frame(cpu, 12), low);
}
static int positive_amount(const uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    uint16_t high = word_at(ram, frame(cpu, 12));
    (void)arithmetic(cpu, high, 0, 0, 16, 1);
    if ((int16_t)high > 0) return 1;
    if ((int16_t)high < 0) return 0;
    high = word_at(ram, frame(cpu, 10));
    (void)arithmetic(cpu, high, 0, 0, 16, 1);
    return high != 0u;
}
static int negative_amount(const uint8_t *ram, pr_u6c_42ed_cpu *cpu)
{
    uint16_t high = word_at(ram, frame(cpu, 12));
    (void)arithmetic(cpu, high, 0, 0, 16, 1);
    if ((int16_t)high < 0) return 1;
    if ((int16_t)high > 0) return 0;
    high = word_at(ram, frame(cpu, 10));
    (void)arithmetic(cpu, high, 0, 0, 16, 1);
    /* Source CMP unsigned low,0 / JB cannot take its negative branch. */
    return 0;
}
pr_u6c_42ed_status pr_u6c_record_arithmetic_42ed(
    uint8_t *ram, size_t ram_size, pr_u6c_42ed_cpu *cpu,
    const pr_u6c_42ed_layout *layout)
{
    pr_u6c_42ed_status status;
    size_t at;
    uint8_t index;
    if (!ram || !cpu || !layout || ram_size < PR_U6C_42ED_RAM_SIZE)
        return PR_U6C_42ED_ARGUMENT;
    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    load_amount(ram, cpu);
    cpu->cx = 200;
    cpu->bx = 0;
    status = runtime(ram, cpu, layout, 0x0A5Bu, 0x4303u);
    if (status != PR_U6C_42ED_OK) return status;
    put_word(ram, frame(cpu, 10), cpu->ax);
    put_word(ram, frame(cpu, 12), cpu->dx);
    ram[frame(cpu, -3)] = 0;
    cpu->di = word_at(ram, frame(cpu, 6));
    cpu->es = word_at(ram, (frame(cpu, 6) + 2u) & MASK);
    put_word(ram, frame(cpu, -7), cpu->di);
    put_word(ram, frame(cpu, -5), cpu->es);
    while (positive_amount(ram, cpu)) {
        load_table(ram, cpu);
        load_amount(ram, cpu);
        status = runtime(ram, cpu, layout, 0x0A76u, 0x4347u);
        if (status != PR_U6C_42ED_OK) return status;
        cpu->ax = arithmetic(cpu, cpu->ax, 1, 0, 16, 0);
        cpu->dx = arithmetic(cpu, cpu->dx, 0, cpu->flags & 1u, 16, 0);
        put_word(ram, frame(cpu, -2), cpu->ax);
        load_record(ram, cpu);
        cpu->ax = word_at(ram, physical(cpu->es, (uint16_t)(cpu->di + 0x88u)));
        if (cpu->ax < word_at(ram, frame(cpu, -2))) {
            load_record(ram, cpu);
            cpu->ax = word_at(ram, physical(cpu->es, (uint16_t)(cpu->di + 0x88u)));
            put_word(ram, frame(cpu, -2), cpu->ax);
        }
        load_table(ram, cpu);
        cpu->ax = word_at(ram, frame(cpu, -2));
        cpu->dx = 0;
        status = runtime(ram, cpu, layout, 0x0A5Bu, 0x4394u);
        if (status != PR_U6C_42ED_OK) return status;
        subtract_amount(ram, cpu);
        cpu->dx = word_at(ram, frame(cpu, -2));
        load_record(ram, cpu);
        at = physical(cpu->es, (uint16_t)(cpu->di + 0x88u));
        put_word(ram, at, arithmetic(cpu, word_at(ram, at), cpu->dx, 0, 16, 1));
        index = ram[frame(cpu, -3)];
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | (uint8_t)(index + 1u));
        ram[frame(cpu, -3)] = (uint8_t)cpu->ax;
    }
    if (negative_amount(ram, cpu)) {
        load_amount(ram, cpu);
        status = runtime(ram, cpu, layout, 0x0AFCu, 0x43D7u);
        if (status != PR_U6C_42ED_OK) return status;
        put_word(ram, frame(cpu, 10), cpu->ax);
        put_word(ram, frame(cpu, 12), cpu->dx);
        ram[frame(cpu, -3)] = 4;
        while (positive_amount(ram, cpu)) {
            load_table(ram, cpu);
            load_amount(ram, cpu);
            status = runtime(ram, cpu, layout, 0x0A76u, 0x440Cu);
            if (status != PR_U6C_42ED_OK) return status;
            put_word(ram, frame(cpu, -2), cpu->ax);
            load_table(ram, cpu);
            cpu->ax = word_at(ram, frame(cpu, -2));
            cpu->dx = 0;
            status = runtime(ram, cpu, layout, 0x0A5Bu, 0x442Bu);
            if (status != PR_U6C_42ED_OK) return status;
            subtract_amount(ram, cpu);
            cpu->dx = word_at(ram, frame(cpu, -2));
            load_record(ram, cpu);
            at = physical(cpu->es, (uint16_t)(cpu->di + 0x88u));
            put_word(ram, at, arithmetic(cpu, word_at(ram, at), cpu->dx, 0, 16, 0));
            index = ram[frame(cpu, -3)];
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | (uint8_t)(index - 1u));
            ram[frame(cpu, -3)] = (uint8_t)cpu->ax;
        }
    }
    cpu->sp = cpu->bp;
    cpu->bp = pop(ram, cpu);
    far_return(ram, cpu, 8);
    return PR_U6C_42ED_OK;
}

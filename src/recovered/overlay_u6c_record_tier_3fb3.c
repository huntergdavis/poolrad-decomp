/* Unit 6C:+3FB3..+408B; docs/re/U6C_RECORD_TIER_3FB3.md. */
#include "poolrad/recovered/overlay_u6c_record_tier_3fb3.h"

#define MASK (PR_U6C_3FB3_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8u);
}

static void push(uint8_t *ram, pr_u6c_3fb3_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, physical(cpu->ss, cpu->sp), value);
}

static size_t local_at(const pr_u6c_3fb3_cpu *cpu, uint16_t distance)
{
    return physical(cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void load_far(const uint8_t *ram, pr_u6c_3fb3_cpu *cpu, uint16_t offset)
{
    size_t at = physical(cpu->ss, offset);
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
}

static uint16_t sign_byte(uint8_t value)
{
    return value < 0x80u ? value : (uint16_t)(0xFF00u | value);
}

static void arithmetic_flags(pr_u6c_3fb3_cpu *cpu, uint16_t left,
                             uint16_t right, uint16_t result, int subtract)
{
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (subtract ? left < right : (uint32_t)left + right > 0xFFFFu)
        cpu->flags |= 0x0001u;
    if ((parity & 1u) == 0u) cpu->flags |= 0x0004u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) cpu->flags |= 0x0080u;
    if (((subtract ? left ^ right : (uint16_t)~(left ^ right)) & (left ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x0800u;
}

static void add_ax(pr_u6c_3fb3_cpu *cpu, uint16_t value)
{
    uint16_t left = cpu->ax;
    cpu->ax = (uint16_t)(left + value);
    arithmetic_flags(cpu, left, value, cpu->ax, 0);
}

pr_u6c_3fb3_status pr_u6c_record_tier_3fb3(
    uint8_t *ram, size_t ram_size, pr_u6c_3fb3_cpu *cpu)
{
    size_t at;
    uint16_t caller_cs;
    uint8_t value;
    int divide = 0;
    if (!ram || !cpu || ram_size < PR_U6C_3FB3_RAM_SIZE)
        return PR_U6C_3FB3_ARGUMENT;
    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 10u);
    load_far(ram, cpu, (uint16_t)(cpu->bp + 6u));
    put_word(ram, local_at(cpu, 9u), cpu->di);
    put_word(ram, local_at(cpu, 7u), cpu->es);
    ram[local_at(cpu, 2u)] = 0;
    ram[local_at(cpu, 4u)] = 0;
    ram[local_at(cpu, 5u)] = 0;
    for (;;) {
        cpu->ax = sign_byte(ram[local_at(cpu, 5u)]);
        load_far(ram, cpu, (uint16_t)(cpu->bp - 9u));
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        value = ram[physical(cpu->es, (uint16_t)(cpu->di + 0x96u))];
        if (value != 0u && value < 0x80u) {
            cpu->ax = sign_byte(ram[local_at(cpu, 5u)]);
            load_far(ram, cpu, (uint16_t)(cpu->bp - 9u));
            cpu->di = (uint16_t)(cpu->di + cpu->ax);
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                ram[physical(cpu->es, (uint16_t)(cpu->di + 0x96u))]);
            at = local_at(cpu, 4u);
            ram[at] = (uint8_t)(ram[at] + (uint8_t)cpu->ax);
            at = local_at(cpu, 2u);
            ram[at] = (uint8_t)(ram[at] + 1u);
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[local_at(cpu, 5u)]);
            if ((uint8_t)cpu->ax == 4u || (uint8_t)cpu->ax == 7u) {
                at = local_at(cpu, 4u);
                ram[at] = (uint8_t)(ram[at] + 1u);
            }
        }
        if (ram[local_at(cpu, 5u)] == 7u)
            break;
        at = local_at(cpu, 5u);
        ram[at] = (uint8_t)(ram[at] + 1u);
    }
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08D5u) | 0x0044u);
    push(ram, cpu, word_at(ram, physical(cpu->ss, (uint16_t)(cpu->bp + 8u))));
    push(ram, cpu, word_at(ram, physical(cpu->ss, (uint16_t)(cpu->bp + 6u))));
    caller_cs = cpu->cs;
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x401Bu);
    cpu->ip = 0x3F31u;
    (void)pr_u6c_record_table_sum_3f31(ram, ram_size, cpu);
    if (cpu->cs != caller_cs || cpu->ip != 0x401Bu)
        return PR_U6C_3FB3_TRANSFER;
    ram[local_at(cpu, 3u)] = (uint8_t)cpu->ax;
    value = ram[local_at(cpu, 3u)];
    if (value >= 0x80u) {
        cpu->ax = ram[local_at(cpu, 2u)];
        cpu->cx = cpu->ax;
        cpu->ax = sign_byte(ram[local_at(cpu, 3u)]);
        cpu->dx = 0xFFFFu; /* CWD of a negative sign-extended byte. */
        cpu->ax ^= cpu->dx;
        cpu->ax = (uint16_t)(cpu->ax - cpu->dx);
        add_ax(cpu, cpu->cx);
        cpu->dx = cpu->ax;
        cpu->ax = ram[local_at(cpu, 4u)];
        arithmetic_flags(cpu, cpu->ax, cpu->dx, (uint16_t)(cpu->ax - cpu->dx), 1);
        if ((int16_t)cpu->ax > (int16_t)cpu->dx)
            divide = 1;
        else
            ram[local_at(cpu, 4u)] = 1;
    } else {
        divide = 1;
    }
    if (divide) {
        int32_t dividend, divisor, quotient;
        cpu->ax = ram[local_at(cpu, 2u)];
        cpu->cx = cpu->ax;
        cpu->ax = sign_byte(ram[local_at(cpu, 3u)]);
        cpu->dx = cpu->ax;
        cpu->ax = ram[local_at(cpu, 4u)];
        add_ax(cpu, cpu->dx);
        cpu->dx = (cpu->ax & 0x8000u) != 0u ? 0xFFFFu : 0;
        dividend = (int16_t)cpu->ax;
        divisor = (int16_t)cpu->cx;
        quotient = divisor != 0 ? dividend / divisor : 0;
        if (divisor == 0 || quotient < -32768 || quotient > 32767) {
            cpu->ip = value >= 0x80u ? 0x4056u : 0x4078u;
            return PR_U6C_3FB3_DIVIDE_ERROR;
        }
        cpu->ax = (uint16_t)quotient;
        cpu->dx = (uint16_t)(dividend % divisor);
        /* IDIV leaves arithmetic FLAGS undefined; choose preceding ADD's
         * values. The defined non-arithmetic FLAGS remain unchanged. */
        ram[local_at(cpu, 4u)] = (uint8_t)cpu->ax;
    }
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[local_at(cpu, 4u)]);
    ram[local_at(cpu, 1u)] = (uint8_t)cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[local_at(cpu, 1u)]);
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U6C_3FB3_OK;
}

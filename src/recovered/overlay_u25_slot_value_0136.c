/* Complete slot-value composition; see docs/re/U25_SLOT_VALUE_0136.md. */
#include "poolrad/recovered/overlay_u25_slot_value_0136.h"
#include "poolrad/recovered/overlay_u25_load_word_0e0c.h"

#define RAM_MASK (PR_U25_0136_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static uint16_t arithmetic(pr_u25_slot_value_cpu *cpu, uint16_t left,
                           uint16_t right, uint16_t sign, int subtract)
{
    uint32_t wide = subtract ? (uint32_t)left - right : (uint32_t)left + right;
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint16_t result = (uint16_t)(wide & mask);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (subtract ? left < right : wide > mask) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & sign) != 0u) cpu->flags |= 0x80u;
    if (((subtract ? (left ^ right) : (uint16_t)~(left ^ right)) &
         (left ^ result) & sign) != 0u) cpu->flags |= 0x800u;
    return result;
}

static void set_al(pr_u25_slot_value_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint16_t pop16(const uint8_t *ram, pr_u25_slot_value_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void push16(uint8_t *ram, pr_u25_slot_value_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void logical(pr_u25_slot_value_cpu *cpu, uint16_t result, unsigned bits)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & (bits == 8u ? 0x80u : 0x8000u)) cpu->flags |= 0x80u;
}

static uint16_t shift(pr_u25_slot_value_cpu *cpu, uint16_t value, unsigned count, int right)
{
    uint16_t old = value, carry = 0, overflow = cpu->flags & 0x800u;
    unsigned i;
    count &= 31u;
    if (!count) return value;
    for (i = 0; i < count; ++i) {
        carry = (uint16_t)(right ? value & 1u : value >> 15);
        value = (uint16_t)(right ? value >> 1 : (uint32_t)value << 1);
    }
    logical(cpu, value, 16u);
    if (count == 1u)
        overflow = (right ? old & 0x8000u : (old ^ value) & 0x8000u) ? 0x800u : 0u;
    cpu->flags |= (uint16_t)(carry | overflow);
    return value;
}

static void word_06af(uint8_t *ram, pr_u25_slot_value_cpu *cpu)
{
    push16(ram, cpu, cpu->bp); /* +06AF */
    cpu->bp = (uint16_t)(cpu->sp); /* +06B0 */
    cpu->sp = (uint16_t)(arithmetic(cpu, cpu->sp, 0x2u, 0x8000u, 1)); /* +06B2 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +06B5 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +06B8 */
    cpu->dx = (uint16_t)(cpu->ax); /* +06BA */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +06BC */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +06BF */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +06C1 */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, (uint8_t)cpu->cx, 0)); /* +06C3 */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0)); /* +06C5 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint16_t)(cpu->ax)); /* +06C7 */
    cpu->ax = (uint16_t)(read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +06CA */
    cpu->sp = (uint16_t)(cpu->bp); /* +06CD */
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +06CF */
    {
        /* One contiguous far frame; SP wraps only after all four bytes. */
        size_t frame = physical(cpu->ss, cpu->sp);
        cpu->ip = (uint16_t)(ram[frame] |
                            ((uint16_t)ram[(frame + 1u) & RAM_MASK] << 8));
        cpu->cs = (uint16_t)(ram[(frame + 2u) & RAM_MASK] |
                            ((uint16_t)ram[(frame + 3u) & RAM_MASK] << 8));
        cpu->sp = (uint16_t)(cpu->sp + 8u);
    }
    return; /* +06D0 */
}

/* Explicit member copies keep the distinct public CPU types alias-safe. */
#define COPY_CPU(to, from) do { \
    (to).ax = (from).ax; \
    (to).bx = (from).bx; \
    (to).cx = (from).cx; \
    (to).dx = (from).dx; \
    (to).si = (from).si; \
    (to).di = (from).di; \
    (to).bp = (from).bp; \
    (to).sp = (from).sp; \
    (to).cs = (from).cs; \
    (to).ip = (from).ip; \
    (to).ds = (from).ds; \
    (to).es = (from).es; \
    (to).ss = (from).ss; \
    (to).flags = (from).flags; \
} while (0)

static int fixed_callee(void *context, pr_u25_0136_call_kind kind,
                         uint8_t *ram, size_t ram_size, pr_u25_0136_cpu *cpu)
{
    pr_u25_slot_value_status *result = context;
    if (kind == PR_U25_0136_CALL_06AF) {
        word_06af(ram, cpu);
        *result = PR_U25_SLOT_VALUE_OK;
        return 1;
    }
    if (kind == PR_U25_0136_CALL_0E0C) {
        pr_u25_load_word_cpu nested;
        pr_u25_load_word_status status;
        COPY_CPU(nested, *cpu);
        status = pr_u25_load_word_0e0c(ram, ram_size, &nested);
        COPY_CPU(*cpu, nested);
        *result = status == PR_U25_LOAD_WORD_OK ? PR_U25_SLOT_VALUE_OK :
                  status == PR_U25_LOAD_WORD_TRANSFER ? PR_U25_SLOT_VALUE_TRANSFER :
                  PR_U25_SLOT_VALUE_CALLEE;
        return status == PR_U25_LOAD_WORD_OK;
    }
    *result = PR_U25_SLOT_VALUE_CALLEE;
    return 0;
}

pr_u25_slot_value_status pr_u25_slot_value_0136(
    uint8_t *ram, size_t ram_size, pr_u25_slot_value_cpu *cpu)
{
    pr_u25_slot_value_status nested = PR_U25_SLOT_VALUE_OK;
    pr_u25_0136_status status;
    const pr_u25_0136_io io = {fixed_callee, &nested};
    if (ram == NULL || cpu == NULL || ram_size != PR_U25_0136_RAM_SIZE ||
        cpu->ip != 0x0136u)
        return PR_U25_SLOT_VALUE_ARGUMENT;
    status = pr_u25_entry1_0136(ram, ram_size, cpu, &io, NULL);
    if (status == PR_U25_0136_RETURNED) return PR_U25_SLOT_VALUE_OK;
    if (status == PR_U25_0136_TRANSFER) return PR_U25_SLOT_VALUE_TRANSFER;
    if (status == PR_U25_0136_CALLEE_FAILED) return nested;
    return PR_U25_SLOT_VALUE_CALLEE;
}

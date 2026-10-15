/* U6C:+4235, U9E:+0DE3, SYSTEM:+11C0/+120F; see evidence document. */
#include "poolrad/recovered/overlay_u6c_record_roll_sum_4235.h"

#define MASK (PR_U6C_4235_RAM_SIZE - 1u)

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
static void push(uint8_t *ram, pr_u6c_4235_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_word(ram, physical(cpu->ss, cpu->sp), value);
}
static size_t local_at(const pr_u6c_4235_cpu *cpu, uint16_t distance)
{
    return physical(cpu->ss, (uint16_t)(cpu->bp - distance));
}
static uint8_t byte_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}
static void low_ax(pr_u6c_4235_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}
static uint16_t sign_byte(uint8_t value)
{
    return value < 0x80u ? value : (uint16_t)(0xFF00u | value);
}
static void flags(pr_u6c_4235_cpu *cpu, uint16_t left, uint16_t right,
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
}
static void far_return(const uint8_t *ram, pr_u6c_4235_cpu *cpu, uint16_t cleanup)
{
    size_t at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + cleanup);
}
static void epilogue(const uint8_t *ram, pr_u6c_4235_cpu *cpu, uint16_t cleanup)
{
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    far_return(ram, cpu, cleanup);
}
static void far_call(uint8_t *ram, pr_u6c_4235_cpu *cpu,
                     uint16_t segment, uint16_t offset, uint16_t continuation)
{
    push(ram, cpu, cpu->cs);
    push(ram, cpu, continuation);
    cpu->cs = segment;
    cpu->ip = offset;
}
static int returned(const pr_u6c_4235_cpu *cpu, uint16_t segment, uint16_t offset)
{
    return cpu->cs == segment && cpu->ip == offset;
}

/* Instruction-ordered SYSTEM seed: preserve BX/CX scratch values as well
 * as AX/DX. Low and high seed words are separate live writes, not a host
 * seed/result adapter. The final ADC defines all arithmetic FLAGS. */
static void seed(uint8_t *ram, pr_u6c_4235_cpu *cpu)
{
    uint32_t product;
    uint16_t low, high;
    cpu->ax = word_at(ram, physical(cpu->ds, 0x4428u));
    cpu->bx = word_at(ram, physical(cpu->ds, 0x442Au));
    cpu->cx = cpu->ax;
    product = (uint32_t)cpu->ax * word_at(ram, physical(cpu->cs, 0x1245u));
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16u);
    cpu->cx = (uint16_t)(cpu->cx << 3u);
    cpu->cx = (uint16_t)((cpu->cx & 0x00FFu) |
        ((uint16_t)(uint8_t)((cpu->cx >> 8u) + (uint8_t)cpu->cx) << 8u));
    cpu->dx = (uint16_t)(cpu->dx + cpu->cx + cpu->bx);
    cpu->bx = (uint16_t)(cpu->bx << 2u);
    cpu->dx = (uint16_t)(cpu->dx + cpu->bx);
    cpu->dx = (uint16_t)((cpu->dx & 0x00FFu) |
        ((uint16_t)(uint8_t)((cpu->dx >> 8u) + (uint8_t)cpu->bx) << 8u));
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 5u);
    cpu->bx = (uint16_t)(cpu->bx << 5u);
    cpu->dx = (uint16_t)((cpu->dx & 0x00FFu) |
        ((uint16_t)(uint8_t)((cpu->dx >> 8u) + (uint8_t)cpu->bx) << 8u));
    low = cpu->ax;
    cpu->ax = (uint16_t)(low + 1u);
    high = cpu->dx;
    flags(cpu, high, 0, low == 0xFFFFu, 16, 0);
    cpu->dx = (uint16_t)(high + (low == 0xFFFFu));
    put_word(ram, physical(cpu->ds, 0x4428u), cpu->ax);
    put_word(ram, physical(cpu->ds, 0x442Au), cpu->dx);
    cpu->ip = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
}
static int random_word(uint8_t *ram, pr_u6c_4235_cpu *cpu)
{
    uint16_t system_cs = cpu->cs;
    push(ram, cpu, 0x11C3u);
    cpu->ip = 0x120Fu;
    seed(ram, cpu);
    if (!returned(cpu, system_cs, 0x11C3u)) return 0;
    cpu->ax = 0;
    cpu->bx = cpu->sp;
    cpu->bx = word_at(ram, physical(cpu->ss, (uint16_t)(cpu->bx + 4u)));
    /* OR defines CF/PF/ZF/SF/OF; AF is undefined (choose zero). */
    flags(cpu, cpu->bx, 0, 0, 16, 0);
    cpu->flags &= (uint16_t)~0x0811u;
    if (cpu->bx != 0u) {
        uint16_t numerator = cpu->dx;
        cpu->ax = (uint16_t)(numerator % cpu->bx);
        cpu->dx = (uint16_t)(numerator / cpu->bx);
        /* DIV's arithmetic FLAGS are undefined; retain preceding OR. */
    }
    far_return(ram, cpu, 2u);
    return 1;
}
static int dice(uint8_t *ram, pr_u6c_4235_cpu *cpu,
                const pr_u6c_4235_layout *layout)
{
    size_t at;
    uint8_t count;
    uint16_t dice_cs = cpu->cs;
    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    ram[local_at(cpu, 3u)] = 0;
    count = byte_at(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    flags(cpu, count, 0, 0, 8, 1);
    if (count != 0u) {
        low_ax(cpu, byte_at(ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
        ram[local_at(cpu, 4u)] = (uint8_t)cpu->ax;
        low_ax(cpu, 1u);
        flags(cpu, 1u, ram[local_at(cpu, 4u)], 0, 8, 1);
        ram[local_at(cpu, 2u)] = 1u;
        for (;;) {
            cpu->ax = byte_at(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
            push(ram, cpu, cpu->ax);
            far_call(ram, cpu, layout->system_cs, 0x11C0u, 0x0E13u);
            if (!random_word(ram, cpu) || !returned(cpu, dice_cs, 0x0E13u)) return 0;
            cpu->ax = (uint16_t)(cpu->ax + 1u);
            at = local_at(cpu, 3u);
            ram[at] = (uint8_t)(ram[at] + (uint8_t)cpu->ax);
            low_ax(cpu, ram[local_at(cpu, 2u)]);
            flags(cpu, (uint8_t)cpu->ax, ram[local_at(cpu, 4u)], 0, 8, 1);
            if ((uint8_t)cpu->ax == ram[local_at(cpu, 4u)]) break;
            at = local_at(cpu, 2u);
            ram[at] = (uint8_t)(ram[at] + 1u);
        }
    }
    low_ax(cpu, ram[local_at(cpu, 3u)]);
    ram[local_at(cpu, 1u)] = (uint8_t)cpu->ax;
    low_ax(cpu, ram[local_at(cpu, 1u)]);
    epilogue(ram, cpu, 4u);
    return 1;
}
static void record_gate(const uint8_t *ram, pr_u6c_4235_cpu *cpu)
{
    size_t at;
    cpu->ax = sign_byte(ram[local_at(cpu, 4u)]);
    at = physical(cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->di = word_at(ram, at);
    cpu->es = word_at(ram, (at + 2u) & MASK);
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
}
static uint8_t table(const uint8_t *ram, pr_u6c_4235_cpu *cpu, uint16_t offset)
{
    cpu->ax = sign_byte(ram[local_at(cpu, 4u)]);
    cpu->di = cpu->ax;
    return byte_at(ram, cpu->ds, (uint16_t)(cpu->di + offset));
}
static void threshold(const uint8_t *ram, pr_u6c_4235_cpu *cpu)
{
    uint8_t value = table(ram, cpu, 0x08BEu);
    cpu->ax = (uint16_t)((uint16_t)value << 1u);
    cpu->cx = 3;
    cpu->dx = (uint16_t)(cpu->ax % cpu->cx);
    cpu->ax = (uint16_t)(cpu->ax / cpu->cx);
}
pr_u6c_4235_status pr_u6c_record_roll_sum_4235(
    uint8_t *ram, size_t ram_size, pr_u6c_4235_cpu *cpu,
    const pr_u6c_4235_layout *layout)
{
    size_t at;
    uint16_t caller_cs;
    uint8_t value;
    if (!ram || !cpu || !layout || ram_size < PR_U6C_4235_RAM_SIZE)
        return PR_U6C_4235_ARGUMENT;
    caller_cs = cpu->cs;
    push(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    ram[local_at(cpu, 3u)] = 0;
    ram[local_at(cpu, 4u)] = 0;
    for (;;) {
        record_gate(ram, cpu);
        value = byte_at(ram, cpu->es, (uint16_t)(cpu->di + 0x96u));
        if (value != 0u && value < 0x80u) {
            value = table(ram, cpu, 0x05DEu);
            low_ax(cpu, (uint8_t)(value & byte_at(ram, cpu->ss, (uint16_t)(cpu->bp + 6u))));
            if ((uint8_t)cpu->ax != 0u) {
                value = table(ram, cpu, 0x08B6u);
                low_ax(cpu, value);
                push(ram, cpu, cpu->ax);
                value = table(ram, cpu, 0x08BEu);
                low_ax(cpu, value);
                push(ram, cpu, cpu->ax);
                far_call(ram, cpu, layout->dice_cs, 0x0DE3u, 0x4285u);
                if (!dice(ram, cpu, layout) || !returned(cpu, caller_cs, 0x4285u))
                    return PR_U6C_4235_TRANSFER;
                ram[local_at(cpu, 2u)] = (uint8_t)cpu->ax;
                record_gate(ram, cpu);
                if (byte_at(ram, cpu->es, (uint16_t)(cpu->di + 0x96u)) == 1u) {
                    threshold(ram, cpu);
                    cpu->dx = cpu->ax;
                    cpu->ax = ram[local_at(cpu, 2u)];
                    if ((int16_t)cpu->ax < (int16_t)cpu->dx) {
                        threshold(ram, cpu);
                        ram[local_at(cpu, 2u)] = (uint8_t)cpu->ax;
                    }
                }
                low_ax(cpu, ram[local_at(cpu, 2u)]);
                at = local_at(cpu, 3u);
                ram[at] = (uint8_t)(ram[at] + (uint8_t)cpu->ax);
            }
        }
        flags(cpu, ram[local_at(cpu, 4u)], 7u, 0, 8, 1);
        if (ram[local_at(cpu, 4u)] == 7u) break;
        at = local_at(cpu, 4u);
        ram[at] = (uint8_t)(ram[at] + 1u);
    }
    low_ax(cpu, ram[local_at(cpu, 3u)]);
    ram[local_at(cpu, 1u)] = (uint8_t)cpu->ax;
    low_ax(cpu, ram[local_at(cpu, 1u)]);
    epilogue(ram, cpu, 6u);
    return PR_U6C_4235_OK;
}

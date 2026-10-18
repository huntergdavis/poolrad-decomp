/* DOS v1.3 GAME.OVR U38 +2002..+20DE, 221 original bytes. */
#include "poolrad/recovered/overlay_u38_leaf_2002.h"

#define MASK (PR_U38_2002_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t pop16(const uint8_t *ram, pr_u38_2002_cpu *cpu)
{
    uint16_t result = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return result;
}

static void set_al(pr_u38_2002_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint16_t sign_extend(uint8_t value)
{
    return (value & 0x80u) != 0u ? (uint16_t)(0xFF00u | value) : value;
}

pr_u38_2002_status pr_u38_leaf_2002(
    uint8_t *ram, size_t ram_size, pr_u38_2002_cpu *cpu)
{
    uint16_t base, best, index, product;
    size_t pointer;
    uint8_t value, previous;
    int replace;
    if (ram == NULL || cpu == NULL || ram_size < PR_U38_2002_RAM_SIZE ||
        cpu->ip != 0x2002u)
        return PR_U38_2002_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u); /* PUSH BP */
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->ax = 0u;
    base = cpu->di;
    write16(ram, cpu->ss, (uint16_t)(base - 0x31u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(base - 0x2Fu), cpu->ax);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), 0u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0u);

    for (;;) {
        /* LES DI,SS:[base+8]; ADD DI,sign_extend(index). */
        index = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        set_al(cpu, (uint8_t)index);
        cpu->ax = sign_extend((uint8_t)cpu->ax);
        cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
        /* LES consumes four physically contiguous bytes after forming its
         * 16-bit effective offset; only the 20-bit bus address wraps. */
        pointer = physical(cpu->ss, (uint16_t)(cpu->di + 8u));
        cpu->es = (uint16_t)(ram[(pointer + 2u) & MASK] |
                            ((uint16_t)ram[(pointer + 3u) & MASK] << 8));
        cpu->di = (uint16_t)(ram[pointer] |
                            ((uint16_t)ram[(pointer + 1u) & MASK] << 8));
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        value = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x96u));
        set_al(cpu, value);
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), (uint8_t)cpu->ax);

        if ((int8_t)read8(ram, cpu->ss, (uint16_t)(cpu->bp - 4u)) > 1) {
            set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 4u)));
            cpu->ax = sign_extend((uint8_t)cpu->ax);
            cpu->ax = (uint16_t)(cpu->ax << 1);
            cpu->ax = (uint16_t)(cpu->ax << 1);
            cpu->cx = cpu->ax;
            set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
            cpu->ax = sign_extend((uint8_t)cpu->ax);
            cpu->dx = 0x38u;
            product = cpu->ax;
            cpu->ax = (uint16_t)(product * cpu->dx);
            cpu->dx = (uint16_t)(((uint32_t)product * 0x38u) >> 16);
            cpu->di = cpu->ax;
            cpu->di = (uint16_t)(cpu->di + cpu->cx);
            cpu->ax = read16(ram, cpu->ds, (uint16_t)(cpu->di + 0x409Bu));
            cpu->dx = read16(ram, cpu->ds, (uint16_t)(cpu->di + 0x409Du));
            cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
            write16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Du), cpu->ax);
            write16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Bu), cpu->dx);
        } else {
            cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
            cpu->ax = 0u;
            write16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Du), cpu->ax);
            write16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Bu), cpu->ax);
        }

        set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 4u)));
        previous = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u));
        replace = (int8_t)(uint8_t)cpu->ax >= (int8_t)previous;
        if (replace) {
            cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
            cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Du));
            cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Bu));
            cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
            best = read16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Fu));
            if ((int16_t)cpu->dx < (int16_t)best) {
                replace = 0;
            } else if (cpu->dx == best) {
                best = read16(ram, cpu->ss, (uint16_t)(cpu->di - 0x31u));
                replace = cpu->ax >= best;
            }
            if (!replace) {
                cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
                replace = read16(ram, cpu->ss,
                                 (uint16_t)(cpu->di - 0x2Bu)) == 0xFFFFu;
                if (replace)
                    replace = read16(ram, cpu->ss,
                                     (uint16_t)(cpu->di - 0x2Du)) == 0xFFFFu;
            }
        }
        if (replace) {
            cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
            cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Du));
            cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Bu));
            cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
            write16(ram, cpu->ss, (uint16_t)(cpu->di - 0x31u), cpu->ax);
            write16(ram, cpu->ss, (uint16_t)(cpu->di - 0x2Fu), cpu->dx);
            set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
            set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 4u)));
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), (uint8_t)cpu->ax);
        }

        index = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
        if (index == 7u) {
            /* Final CMP byte [BP-2],7: zero and even parity. */
            cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) |
                                    0x0044u);
            break;
        }
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u),
               (uint8_t)(index + 1u));
    }

    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U38_2002_OK;
}

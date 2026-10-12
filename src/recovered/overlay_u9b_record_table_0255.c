/* GAME.OVR unit 0x9B:+0x0255..+0x0328. Evidence: docs/re/U9B_RECORD_TABLE_0255.md. */
#include "poolrad/recovered/overlay_u9b_record_table_0255.h"

#define RAM_MASK (PR_U9B_0255_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
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

static uint16_t read16_at(const uint8_t *ram, size_t address)
{
    return (uint16_t)((uint16_t)ram[address] |
                      ((uint16_t)ram[(address + 1u) & RAM_MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void set_al(pr_u9b_0255_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u9b_0255_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

static void les_arg(uint8_t *ram, pr_u9b_0255_cpu *cpu)
{
    size_t address = physical(cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = read16_at(ram, address);
    cpu->es = read16_at(ram, (address + 2u) & RAM_MASK);
}

/* +0x02AE..+0x02BD and +0x02F1..+0x0300: both source sequences
 * recompute the table offset and leave AX, BX, CX, DX, SI, DI visible. */
static void table_index(uint8_t *ram, pr_u9b_0255_cpu *cpu)
{
    uint32_t product;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    cbw(cpu);
    cpu->dx = 0x2Du;
    product = (uint32_t)cpu->ax * cpu->dx;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    cpu->di = cpu->ax;
    cpu->di = (uint16_t)(cpu->di + cpu->cx);
    cpu->di = (uint16_t)(cpu->di + cpu->bx);
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) |
                         read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x426Eu)));
}

/* +0x0295..+0x02AC and +0x02D8..+0x02EF: reload the argument before
 * reading the indexed signed record byte; do not cache either. */
static void class_times_five(uint8_t *ram, pr_u9b_0255_cpu *cpu)
{
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    cbw(cpu);
    les_arg(ram, cpu);
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x96u)));
    cbw(cpu);
    cpu->si = cpu->ax;
    cpu->ax = (uint16_t)(cpu->ax << 1);
    cpu->ax = (uint16_t)(cpu->ax << 1);
    cpu->ax = (uint16_t)(cpu->ax + cpu->si);
    cpu->cx = cpu->ax;
}

pr_u9b_0255_status pr_u9b_record_table_0255(
    uint8_t *ram, size_t ram_size, pr_u9b_0255_cpu *cpu)
{
    int first_outer = 1;

    if (ram == NULL || ram_size < PR_U9B_0255_RAM_SIZE || cpu == NULL)
        return PR_U9B_0255_ARGUMENT;

    /* +0x0255..+0x025F: PUSH BP; MOV BP,SP; SUB SP,4; local outer=0. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 0u);

    for (;;) {
        int first_inner = 1;

        if (!first_outer) {
            uint16_t local = (uint16_t)(cpu->bp - 1u);
            write8(ram, cpu->ss, local,
                   (uint8_t)(read8(ram, cpu->ss, local) + 1u));
        }
        first_outer = 0;

        /* +0x0264..+0x0272: one default store before eight inner passes. */
        set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
        cbw(cpu);
        les_arg(ram, cpu);
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        write8(ram, cpu->es, (uint16_t)(cpu->di + 0x6Du), 0x14u);
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0u);

        for (;;) {
            if (!first_inner) {
                uint16_t local = (uint16_t)(cpu->bp - 2u);
                write8(ram, cpu->ss, local,
                       (uint8_t)(read8(ram, cpu->ss, local) + 1u));
            }
            first_inner = 0;

            /* +0x027B..+0x028C: signed byte > 0 enters the table path. */
            set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
            cbw(cpu);
            les_arg(ram, cpu);
            cpu->di = (uint16_t)(cpu->di + cpu->ax);
            if ((int8_t)read8(ram, cpu->es,
                              (uint16_t)(cpu->di + 0x96u)) > 0) {
                /* +0x028F..+0x02D0: table byte and unsigned comparison. */
                set_al(cpu, read8(ram, cpu->ss,
                                  (uint16_t)(cpu->bp - 1u)));
                cbw(cpu);
                cpu->bx = cpu->ax;
                class_times_five(ram, cpu);
                table_index(ram, cpu);
                set_al(cpu, read8(ram, cpu->ss,
                                  (uint16_t)(cpu->bp - 1u)));
                cbw(cpu);
                les_arg(ram, cpu);
                cpu->di = (uint16_t)(cpu->di + cpu->ax);
                set_al(cpu, read8(ram, cpu->es,
                                  (uint16_t)(cpu->di + 0x6Du)));
                if ((uint8_t)cpu->ax > (uint8_t)cpu->dx) {
                    /* +0x02D2..+0x030D: recompute instead of caching. */
                    set_al(cpu, read8(ram, cpu->ss,
                                      (uint16_t)(cpu->bp - 1u)));
                    cbw(cpu);
                    cpu->bx = cpu->ax;
                    class_times_five(ram, cpu);
                    table_index(ram, cpu);
                    set_al(cpu, read8(ram, cpu->ss,
                                      (uint16_t)(cpu->bp - 1u)));
                    cbw(cpu);
                    les_arg(ram, cpu);
                    cpu->di = (uint16_t)(cpu->di + cpu->ax);
                    write8(ram, cpu->es,
                           (uint16_t)(cpu->di + 0x6Du), (uint8_t)cpu->dx);
                }
            }

            /* +0x0311: local inner byte is read anew after any store. */
            if (read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)) == 7u)
                break;
        }

        /* +0x031A: the terminating comparison supplies returned flags. */
        if (read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)) == 4u)
            break;
    }

    /* +0x0323..+0x0328: MOV SP,BP; POP BP; RETF 4. */
    cpu->flags = (uint16_t)((cpu->flags & ~ARITH_FLAGS) | 0x0044u);
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t address = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, address);
        cpu->cs = read16_at(ram, (address + 2u) & RAM_MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U9B_0255_OK;
}

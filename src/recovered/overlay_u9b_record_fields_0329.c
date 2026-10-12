/* GAME.OVR unit 0x9B:+0x0329..+0x045D.
 * Evidence: docs/re/U9B_RECORD_FIELDS_0329.md. */
#include "poolrad/recovered/overlay_u9b_record_fields_0329.h"

#define RAM_MASK (PR_U9B_0329_RAM_SIZE - 1u)
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

static void set_al(pr_u9b_0329_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void clear_ah(pr_u9b_0329_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
}

static void cbw(pr_u9b_0329_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

/* LES reads a contiguous four-byte operand, including at offset FFFF. */
static void les(uint8_t *ram, pr_u9b_0329_cpu *cpu, uint16_t offset)
{
    size_t address = physical(cpu->ss, offset);
    cpu->di = read16_at(ram, address);
    cpu->es = read16_at(ram, (address + 2u) & RAM_MASK);
}

static uint8_t local_index(const uint8_t *ram, const pr_u9b_0329_cpu *cpu)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u));
}

/* Each +0341/+035F/+037F/+03B6/+03D4 sequence reloads the local and
 * cached far pointer, then computes signed record[field]*8 + local. */
static void make_index(uint8_t *ram, pr_u9b_0329_cpu *cpu,
                       uint16_t record_field)
{
    set_al(cpu, local_index(ram, cpu));
    clear_ah(cpu);
    cpu->dx = cpu->ax;
    les(ram, cpu, (uint16_t)(cpu->bp - 5u));
    set_al(cpu, read8(ram, cpu->es,
                      (uint16_t)(cpu->di + record_field)));
    cbw(cpu);
    cpu->di = cpu->ax;
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 3u);
    cpu->di = (uint16_t)(cpu->di << 3);
    cpu->di = (uint16_t)(cpu->di + cpu->dx);
}

static void store_field(uint8_t *ram, pr_u9b_0329_cpu *cpu, uint8_t value)
{
    set_al(cpu, local_index(ram, cpu));
    clear_ah(cpu);
    les(ram, cpu, (uint16_t)(cpu->bp - 5u));
    cpu->di = (uint16_t)(cpu->di + cpu->ax);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 0x76u), value);
}

pr_u9b_0329_status pr_u9b_record_fields_0329(
    uint8_t *ram, size_t ram_size, pr_u9b_0329_cpu *cpu)
{
    int first = 1;
    if (ram == NULL || ram_size < PR_U9B_0329_RAM_SIZE || cpu == NULL)
        return PR_U9B_0329_ARGUMENT;

    /* +0329..+033C: frame, cached far pointer, local index=1. */
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 6u);
    les(ram, cpu, (uint16_t)(cpu->bp + 6u));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->di);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), cpu->es);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), 1u);

    for (;;) {
        if (!first) {
            uint16_t address = (uint16_t)(cpu->bp - 1u);
            write8(ram, cpu->ss, address,
                   (uint8_t)(read8(ram, cpu->ss, address) + 1u));
        }
        first = 0;

        make_index(ram, cpu, 0x2Eu);
        if ((int8_t)read8(ram, cpu->ds,
                          (uint16_t)(cpu->di + 0x3D3Bu)) < 0) {
            make_index(ram, cpu, 0x9Cu);
            set_al(cpu, read8(ram, cpu->ds,
                              (uint16_t)(cpu->di + 0x3CF3u)));
            clear_ah(cpu);
            cpu->bx = cpu->ax;

            make_index(ram, cpu, 0x2Eu);
            set_al(cpu, read8(ram, cpu->ds,
                              (uint16_t)(cpu->di + 0x3D3Bu)));
            cbw(cpu);
            cpu->dx = (int16_t)cpu->ax < 0 ? 0xFFFFu : 0u; /* CWD */
            cpu->ax ^= cpu->dx;
            cpu->ax = (uint16_t)(cpu->ax - cpu->dx);
            if ((int16_t)cpu->ax > (int16_t)cpu->bx) {
                store_field(ram, cpu, 0u);
                goto next;
            }
        }

        make_index(ram, cpu, 0x2Eu);
        set_al(cpu, read8(ram, cpu->ds,
                          (uint16_t)(cpu->di + 0x3D3Bu)));
        cbw(cpu);
        cpu->bx = cpu->ax;

        make_index(ram, cpu, 0x9Cu);
        set_al(cpu, read8(ram, cpu->ds,
                          (uint16_t)(cpu->di + 0x3CF3u)));
        clear_ah(cpu);
        cpu->ax = (uint16_t)(cpu->ax + cpu->bx);
        cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)cpu->ax);
        store_field(ram, cpu, (uint8_t)cpu->dx);

        if (local_index(ram, cpu) < 6u) {
            /* +040A..+042B: unsigned record +13 times five plus local,
             * followed by signed DS table byte in DX. */
            set_al(cpu, local_index(ram, cpu));
            clear_ah(cpu);
            cpu->dx = cpu->ax;
            les(ram, cpu, (uint16_t)(cpu->bp - 5u));
            set_al(cpu, read8(ram, cpu->es,
                              (uint16_t)(cpu->di + 0x13u)));
            clear_ah(cpu);
            cpu->di = cpu->ax;
            cpu->si = cpu->di;
            cpu->di = (uint16_t)(cpu->di << 1);
            cpu->di = (uint16_t)(cpu->di << 1);
            cpu->di = (uint16_t)(cpu->di + cpu->si);
            cpu->di = (uint16_t)(cpu->di + cpu->dx);
            set_al(cpu, read8(ram, cpu->ds,
                              (uint16_t)(cpu->di + 0x3D4Eu)));
            cbw(cpu);
            cpu->dx = cpu->ax;

            set_al(cpu, local_index(ram, cpu));
            clear_ah(cpu);
            les(ram, cpu, (uint16_t)(cpu->bp - 5u));
            cpu->di = (uint16_t)(cpu->di + cpu->ax);
            set_al(cpu, read8(ram, cpu->es,
                              (uint16_t)(cpu->di + 0x76u)));
            clear_ah(cpu);
            cpu->ax = (uint16_t)(cpu->ax + cpu->dx);
            cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (uint8_t)cpu->ax);
            store_field(ram, cpu, (uint8_t)cpu->dx);
        }
next:
        if (local_index(ram, cpu) == 8u)
            break;
    }

    /* +044F..+045D: final compare sets flags; restore BP and RETF 4. */
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
    return PR_U9B_0329_OK;
}

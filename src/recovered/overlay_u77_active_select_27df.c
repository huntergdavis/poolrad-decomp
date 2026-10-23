/* GAME.OVR unit 0x77:+27DF..+28BD. See docs/re/U77_ACTIVE_SELECT_27DF.md. */
#include "poolrad/recovered/overlay_u77_active_select_27df.h"

#define RAM_MASK (PR_U77_27DF_RAM_SIZE - 1u)
#define CMP_FLAGS 0x08D5u /* OF SF ZF AF PF CF */
#define LOGIC_FLAGS 0x08C5u /* OF SF ZF PF CF; AF undefined */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
}

static uint16_t read16_at(const uint8_t *ram, size_t address)
{
    return (uint16_t)(ram[address] |
                      ((uint16_t)ram[(address + 1u) & RAM_MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return read16_at(ram, physical(seg, off));
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t address = physical(seg, off);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint16_t local16(const uint8_t *ram, const pr_u77_27df_cpu *cpu,
                        uint16_t distance)
{
    return read16(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local16(uint8_t *ram, const pr_u77_27df_cpu *cpu,
                        uint16_t distance, uint16_t value)
{
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static void les_local(uint8_t *ram, pr_u77_27df_cpu *cpu)
{
    size_t address = physical(cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->di = read16_at(ram, address);
    cpu->es = read16_at(ram, (address + 2u) & RAM_MASK);
}

static void les_active(uint8_t *ram, pr_u77_27df_cpu *cpu)
{
    size_t address = physical(cpu->ds, 0x5D92u);
    cpu->di = read16_at(ram, address);
    cpu->es = read16_at(ram, (address + 2u) & RAM_MASK);
}

static uint16_t next_offset(const uint8_t *ram, const pr_u77_27df_cpu *cpu)
{
    return read16(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
}

static uint16_t next_segment(const uint8_t *ram, const pr_u77_27df_cpu *cpu)
{
    return read16(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
}

static void parity_flags(pr_u77_27df_cpu *cpu, uint8_t result)
{
    uint8_t parity = result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (!(parity & 1u)) cpu->flags |= 0x0004u;
}

static void or16(pr_u77_27df_cpu *cpu, uint16_t operand)
{
    cpu->ax = (uint16_t)(cpu->ax | operand);
    cpu->flags &= (uint16_t)~(LOGIC_FLAGS | 0x10u); /* pinned: OR clears AF */
    parity_flags(cpu, (uint8_t)cpu->ax);
    if (cpu->ax == 0u) cpu->flags |= 0x0040u;
    if (cpu->ax & 0x8000u) cpu->flags |= 0x0080u;
}

static void cmp16(pr_u77_27df_cpu *cpu, uint16_t left, uint16_t right)
{
    uint16_t result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~CMP_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    parity_flags(cpu, (uint8_t)result);
    if (((left ^ right ^ result) & 0x0010u) != 0u)
        cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x0800u;
}

static void cmp8(pr_u77_27df_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~CMP_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    parity_flags(cpu, result);
    if (((left ^ right ^ result) & 0x10u) != 0u)
        cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x0800u;
}

pr_u77_27df_status pr_u77_active_select_27df_run(
    uint8_t *ram, size_t ram_size, pr_u77_27df_cpu *cpu,
    pr_u77_27df_retf_policy retf_policy)
{
    if (ram == NULL || cpu == NULL || ram_size < PR_U77_27DF_RAM_SIZE ||
        (retf_policy != PR_U77_27DF_RETF_WRAPPED_SS &&
         retf_policy != PR_U77_27DF_RETF_CONTIGUOUS_PHYSICAL))
        return PR_U77_27DF_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    cpu->ax = read16(ram, cpu->ds, 0x5D96u);
    cpu->dx = read16(ram, cpu->ds, 0x5D98u);
    set_local16(ram, cpu, 4u, cpu->ax);
    set_local16(ram, cpu, 2u, cpu->dx);

    cpu->ax = local16(ram, cpu, 4u);
    or16(cpu, local16(ram, cpu, 2u));
    if (cpu->ax != 0u) {
        uint8_t key = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | key);
        cmp8(cpu, key, 0x47u);
        if (key == 0x47u) {
            cpu->ax = local16(ram, cpu, 4u);
            cpu->dx = local16(ram, cpu, 2u);
            {
                uint16_t active_segment = read16(ram, cpu->ds, 0x5D94u);
                cmp16(cpu, cpu->dx, active_segment);
                if (cpu->dx == active_segment) {
                    uint16_t active_offset = read16(ram, cpu->ds, 0x5D92u);
                    cmp16(cpu, cpu->ax, active_offset);
                    if (cpu->ax == active_offset) {
                        /* Head is active: select the last linked record. */
                        for (;;) {
                            uint16_t next_seg;
                            les_local(ram, cpu);
                            cpu->ax = next_offset(ram, cpu);
                            next_seg = next_segment(ram, cpu);
                            or16(cpu, next_seg);
                            if (cpu->ax == 0u) break;
                            les_local(ram, cpu);
                            cpu->ax = next_offset(ram, cpu);
                            cpu->dx = next_segment(ram, cpu);
                            set_local16(ram, cpu, 4u, cpu->ax);
                            set_local16(ram, cpu, 2u, cpu->dx);
                        }
                        goto store_active;
                    }
                }
            }

            /* Find the predecessor of active, or the null end if absent. */
            for (;;) {
                uint16_t next_seg;
                cpu->ax = local16(ram, cpu, 4u);
                or16(cpu, local16(ram, cpu, 2u));
                if (cpu->ax == 0u) break;
                les_local(ram, cpu);
                cpu->ax = next_offset(ram, cpu);
                cpu->dx = next_segment(ram, cpu);
                next_seg = read16(ram, cpu->ds, 0x5D94u);
                cmp16(cpu, cpu->dx, next_seg);
                if (cpu->dx == next_seg) {
                    uint16_t next_off = read16(ram, cpu->ds, 0x5D92u);
                    cmp16(cpu, cpu->ax, next_off);
                    if (cpu->ax == next_off) break;
                }
                set_local16(ram, cpu, 4u, cpu->ax);
                set_local16(ram, cpu, 2u, cpu->dx);
            }
        } else {
            cmp8(cpu, key, 0x4Fu);
            if (key == 0x4Fu) {
                uint16_t next_seg;
                les_active(ram, cpu);
                cpu->ax = next_offset(ram, cpu);
                next_seg = next_segment(ram, cpu);
                or16(cpu, next_seg);
                if (cpu->ax != 0u) {
                    les_active(ram, cpu);
                    cpu->ax = next_offset(ram, cpu);
                    cpu->dx = next_segment(ram, cpu);
                    set_local16(ram, cpu, 4u, cpu->ax);
                    set_local16(ram, cpu, 2u, cpu->dx);
                }
            } else {
                cpu->ax = read16(ram, cpu->ds, 0x5D96u);
                cpu->dx = read16(ram, cpu->ds, 0x5D98u);
                set_local16(ram, cpu, 4u, cpu->ax);
                set_local16(ram, cpu, 2u, cpu->dx);
            }
        }
    }

store_active:
    cpu->ax = local16(ram, cpu, 4u);
    cpu->dx = local16(ram, cpu, 2u);
    write16(ram, cpu->ds, 0x5D92u, cpu->ax);
    write16(ram, cpu->ds, 0x5D94u, cpu->dx);
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t address = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, address);
        cpu->cs = retf_policy == PR_U77_27DF_RETF_WRAPPED_SS
            ? read16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u))
            : read16_at(ram, (address + 2u) & RAM_MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_U77_27DF_OK;
}

pr_u77_27df_status pr_u77_active_select_27df(
    uint8_t *ram, size_t ram_size, pr_u77_27df_cpu *cpu)
{
    return pr_u77_active_select_27df_run(ram, ram_size, cpu,
                                       PR_U77_27DF_RETF_CONTIGUOUS_PHYSICAL);
}

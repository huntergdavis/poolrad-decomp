/* GAME.OVR unit 0x1C:+0111..+0125, complete 21-byte GOSUB handler. */
#include "poolrad/recovered/overlay_u1c_gosub_0111.h"

#define RAM_MASK (PR_U1C_0111_RAM_SIZE - 1u)

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u1c_0111_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u1c_0111_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void set_al(pr_u1c_0111_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static int external_call(uint8_t *ram, size_t size, pr_u1c_0111_cpu *cpu,
                         pr_u1c_0111_external external, void *context,
                         pr_u1c_0111_call call, uint16_t site, uint16_t next)
{
    cpu->ip = site;
    if (!external(context, ram, size, cpu, call)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = next;
    return 1;
}

pr_u1c_0111_status pr_u1c_gosub_0111(
    uint8_t *ram, size_t ram_size, pr_u1c_0111_cpu *cpu,
    pr_u1c_0111_external external, void *context)
{
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U1C_0111_RAM_SIZE)
        return PR_U1C_0111_ARGUMENT;
    push16(ram, cpu, cpu->bp);                    /* +0111 */
    cpu->bp = cpu->sp;
    set_al(cpu, 1u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0111_LOADER, 0x0117u, 0x011Cu))
        return PR_U1C_0111_CALL_ABORT;
    set_al(cpu, 1u);
    push16(ram, cpu, cpu->ax);
    if (!external_call(ram, ram_size, cpu, external, context,
                       PR_U1C_0111_PUSH_FRAME, 0x011Fu, 0x0124u))
        return PR_U1C_0111_CALL_ABORT;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    return PR_U1C_0111_OK;
}

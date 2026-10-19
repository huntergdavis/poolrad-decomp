/* U38 entry 109, complete [2E08,2E37); docs/re/U38_CLEAR_CYCLE_2E08_CPU.md.
 * Reuses identity/key order from the unmerged f783c27d prototype; executes
 * the current shared raw +0000 API instead of duplicating that helper. */
#include "poolrad/recovered/overlay_u38_clear_cycle_2e08_cpu.h"
#define MASK (PR_U38C_RAM_SIZE - 1u)
static size_t physical(uint16_t segment, uint16_t offset)
{ return (((size_t)segment << 4) + offset) & MASK; }
static uint16_t word_at(const uint8_t *ram, size_t at)
{ return (uint16_t)(ram[at & MASK] | ((uint16_t)ram[(at + 1u) & MASK] << 8)); }
static void push(uint8_t *ram, pr_u38_2e08_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}
static uint16_t pop(const uint8_t *ram, pr_u38_2e08_cpu *cpu)
{
    uint16_t value = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}
static void compare_zero(pr_u38_2e08_cpu *cpu, uint8_t value)
{
    uint8_t parity = value;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (!(parity & 1u)) cpu->flags |= 4u;
    if (!value) cpu->flags |= 0x40u;
    if (value & 0x80u) cpu->flags |= 0x80u;
}
pr_u38_2e08_status pr_u38_clear_cycle_2e08_cpu(
    uint8_t *ram, size_t size, pr_u38_2e08_cpu *cpu, int *callee_status)
{
    static const uint8_t keys[] = {0x0Bu, 0x35u, 0x34u, 0x37u};
    static const uint16_t sites[] = {0x2E0Fu, 0x2E16u, 0x2E1Du, 0x2E24u};
    uint8_t gate;
    size_t at;
    if (ram == NULL || cpu == NULL || size != PR_U38C_RAM_SIZE ||
        cpu->ip != 0x2E08u || cpu->sp < 0x000Au || cpu->sp > 0xFFFCu)
        return PR_U38_2E08_ARGUMENT;
    if (callee_status != NULL) *callee_status = -1;
    push(ram, cpu, cpu->bp);                              /* 2E08 */
    cpu->bp = cpu->sp;
    for (unsigned i = 0; i < 4u; ++i) {
        uint16_t source_cs = cpu->cs;
        uint16_t next = (uint16_t)(sites[i] + 3u);
        int status;
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | keys[i]);
        push(ram, cpu, cpu->ax);
        push(ram, cpu, cpu->cs);
        cpu->ip = sites[i];
        at = physical(cpu->cs, sites[i]);
        if (ram[at] != 0xE8u ||
            word_at(ram, at + 1u) != (uint16_t)(0u - next))
            return PR_U38_2E08_CODE;
        push(ram, cpu, next);
        cpu->ip = 0;
        status = pr_u38_helper_0000_cpu(ram, size, cpu);
        if (callee_status != NULL) *callee_status = status;
        if (status != PR_U38C_OK) return PR_U38_2E08_CALLEE;
        if (cpu->cs != source_cs || cpu->ip != next)
            return PR_U38_2E08_TRANSFER;
    }
    gate = ram[physical(cpu->ds, 0x682Au)];               /* 2E27 */
    compare_zero(cpu, gate);
    if (!gate) ram[physical(cpu->ds, 0x6816u)] = 0x64u;   /* 2E2E */
    cpu->bp = pop(ram, cpu);                             /* 2E33 */
    at = physical(cpu->ss, cpu->sp);                     /* 2E34 RETF 0Ah */
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, at + 2u);
    cpu->sp = (uint16_t)(cpu->sp + 14u);
    return PR_U38_2E08_RETURNED;
}

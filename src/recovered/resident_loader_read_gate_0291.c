/* Resident cold-loader read setup, loaded +0291..+029E.
 * Evidence: docs/re/RESIDENT_LOADER_READ_GATE_0291.md. */
#include "poolrad/recovered/resident_loader_read_gate_0291.h"

#define RAM_MASK (PR_HANDLER_02E6_RAM_SIZE - 1u)
#define LOGIC_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return (uint16_t)(ram[physical(segment, offset)] |
                      ((uint16_t)ram[physical(segment,
                                              (uint16_t)(offset + 1u))] << 8));
}

pr_loader_read_gate_0291_status pr_resident_loader_read_gate_0291(
    const uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu)
{
    if (ram == NULL || ram_size < PR_HANDLER_02E6_RAM_SIZE || cpu == NULL ||
        cpu->cs != 0x12C6u || cpu->ip != 0x0291u)
        return PR_LOADER_READ_GATE_0291_ARGUMENT;

    cpu->ds = read16(ram, cpu->es, 0x0010u); /* MOV DS,ES:[0010] */
    cpu->dx = 0u;                           /* XOR DX,DX */
    cpu->flags = (uint16_t)((cpu->flags & ~LOGIC_FLAGS) | 0x0044u);
    cpu->cx = read16(ram, cpu->es, 0x0008u); /* MOV CX,ES:[0008] */
    cpu->ax = (uint16_t)((cpu->ax & 0x00FFu) | 0x3F00u); /* MOV AH,3F */
    cpu->ip = 0x029Fu;
    return PR_LOADER_READ_GATE_0291_AT_DOS_READ;
}

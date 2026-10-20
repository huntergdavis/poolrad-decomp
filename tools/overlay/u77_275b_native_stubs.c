/* Declared stub build for the U77 +275B native corroboration.
 *
 * The native capture substitutes the two resident entry points (00BA:0BB8 and
 * 00BA:13AB) with `RETF 4` stubs. To compare like with like, the C model is
 * linked against these equivalent stubs instead of the real resident models,
 * so BOTH sides see the same declared endpoint: no closure executes, registers
 * are unchanged, and the far return pops IP, CS and the two argument words.
 *
 * This file exists only for the native comparison. The differential uses the
 * real credited resident models, and that is where the composition claim lives.
 */
#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/resident_00ba_character_0bb8_cpu.h"

#define MASK (PR_U9E_10E4_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

/* `RETF 4`: pop IP, CS, then discard two argument words. Registers preserved. */
static pr_res00ba_status stub_return(uint8_t *ram, size_t ram_size,
                                     pr_res00ba_cpu *cpu)
{
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_U9E_10E4_RAM_SIZE)
        return PR_RES00BA_ARGUMENT;
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = word_at(ram, at);
    cpu->cs = word_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_RES00BA_OK;
}

pr_res00ba_status pr_res00ba_0bb8_cpu(uint8_t *ram, size_t ram_size,
                                      pr_res00ba_cpu *cpu, size_t *budget)
{
    (void)budget;
    return stub_return(ram, ram_size, cpu);
}

pr_res00ba_status pr_res00ba_helpers_cpu(uint8_t *ram, size_t ram_size,
                                         pr_res00ba_cpu *cpu, size_t *budget)
{
    (void)budget;
    return stub_return(ram, ram_size, cpu);
}

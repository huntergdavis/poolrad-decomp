/* GAME.OVR U9E entry8 +0E05..+0E0D; see loopback evidence note. */
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_loop_0e05.h"

#define MASK (PR_U9E_ENTRY8_LOOP_RAM_SIZE - 1u)
#define LOGIC_FLAGS 0x08C5u /* XOR AH,AH leaves AF undefined. */
#define AF 0x0010u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static int in_code(const pr_u9e_entry8_loop_cpu *cpu, size_t at)
{
    unsigned offset;
    for (offset = 0x0DE3u; offset <= 0x0E2Du; ++offset)
        if (at == physical(cpu->cs, (uint16_t)offset)) return 1;
    return 0;
}

static int writes_code(const pr_u9e_entry8_loop_cpu *cpu)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp - 2u));
    size_t pushed = physical(cpu->ss, (uint16_t)(cpu->sp - 2u));
    return in_code(cpu, at) || in_code(cpu, pushed) ||
           in_code(cpu, (pushed + 1u) & MASK);
}

pr_u9e_entry8_loop_status pr_u9e_entry8_fullstate_loop_0e05(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_loop_cpu *cpu)
{
    size_t at;
    uint8_t old_index, new_index, range;
    uint16_t af;
    if (ram == NULL || cpu == NULL ||
        ram_size != PR_U9E_ENTRY8_LOOP_RAM_SIZE ||
        cpu->ip != 0x0E05u || writes_code(cpu))
        return PR_U9E_ENTRY8_LOOP_ARGUMENT;

    /* +0E05 INC BYTE SS:[BP-2]. Its AF is defined by the byte carry.
     * +0E08 MOV AL,SS:[BP+6] rereads the live caller range. */
    at = physical(cpu->ss, (uint16_t)(cpu->bp - 2u));
    old_index = ram[at];
    new_index = (uint8_t)(old_index + 1u);
    ram[at] = new_index;
    af = (old_index & 0x0Fu) == 0x0Fu ? AF : 0u;
    range = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))];
    cpu->ax = range;

    /* +0E0B XOR AH,AH acts on AH alone: zero sets ZF/PF, clears
     * CF/OF/SF. AF is undefined; retain the prior INC AF convention.
     * +0E0D PUSH AX writes a physically contiguous WORD at SS:SP-2. */
    cpu->flags = (uint16_t)((cpu->flags &
                             (uint16_t)~(LOGIC_FLAGS | AF)) | 0x0044u | af);
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->ax;
    ram[(at + 1u) & MASK] = (uint8_t)(cpu->ax >> 8);
    cpu->ip = 0x0E0Eu;
    return PR_U9E_ENTRY8_LOOP_AT_CALL;
}

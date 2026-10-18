/* START.EXE resident 0709:0000, the EGA register reset (raw CPU, port-bound); docs/re/RES_0709_EGA_RESET_0000_CPU.md. */
#include "poolrad/recovered/resident_0709_ega_reset_0000_cpu.h"

#define MASK (PR_R0709_EGA_RESET_RAM_SIZE - 1u)

/* The eighteen OUT DX,AL in source order: the OUT's offset, DX and AL. Each follows MOV AL,imm8 (2 bytes) and MOV
 * DX,imm16 (3 bytes). */
static const struct {
    uint16_t ip, port;
    uint8_t value;
} outs[18] = {
    {0x0008u, 0x03CEu, 0x00u}, {0x000Eu, 0x03CFu, 0x00u}, {0x0014u, 0x03CEu, 0x01u}, {0x001Au, 0x03CFu, 0x00u},
    {0x0020u, 0x03CEu, 0x02u}, {0x0026u, 0x03CFu, 0x00u}, {0x002Cu, 0x03CEu, 0x03u}, {0x0032u, 0x03CFu, 0x00u},
    {0x0038u, 0x03CEu, 0x04u}, {0x003Eu, 0x03CFu, 0x00u}, {0x0044u, 0x03CEu, 0x05u}, {0x004Au, 0x03CFu, 0x00u},
    {0x0050u, 0x03CEu, 0x07u}, {0x0056u, 0x03CFu, 0x0Fu}, {0x005Cu, 0x03CEu, 0x08u}, {0x0062u, 0x03CFu, 0xFFu},
    {0x0068u, 0x03C4u, 0x02u}, {0x006Eu, 0x03C5u, 0x0Fu}};
#define OUT_COUNT (sizeof outs / sizeof outs[0])

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

pr_r0709_ega_reset_status pr_r0709_ega_reset_0000_cpu(uint8_t *ram, size_t ram_size, pr_r0709_ega_reset_cpu *cpu,
                                                      const pr_port_env *env)
{
    size_t k;
    size_t at;
    if (ram == NULL || cpu == NULL || env == NULL || env->out == NULL || ram_size != PR_R0709_EGA_RESET_RAM_SIZE || cpu->ip != 0u)
        return PR_R0709_EGA_RESET_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);                                   /* 0000 PUSH BP; MOV BP,SP */
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & MASK] = (uint8_t)(cpu->bp >> 8);
    cpu->bp = cpu->sp;
    for (k = 0; k < OUT_COUNT; ++k) {                                     /* MOV AL,imm8; MOV DX,imm16; OUT DX,AL */
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | outs[k].value);
        cpu->dx = outs[k].port;
        cpu->ip = outs[k].ip;
        if (env->out(env->ctx, cpu->dx, (uint8_t)cpu->ax, 1u, cpu->cs, cpu->ip) != 0)
            return PR_R0709_EGA_RESET_PORT;
    }
    at = physical(cpu->ss, cpu->sp);                                      /* 006F POP BP; RETF */
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_R0709_EGA_RESET_OK;
}

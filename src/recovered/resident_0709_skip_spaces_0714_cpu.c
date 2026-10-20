/* START.EXE resident 0709:0714, the nested space skip of 0709:0768 (raw CPU); docs/re/RES_0709_SKIP_SPACES_0714_CPU.md. */
#include "poolrad/recovered/resident_0709_skip_spaces_0714_cpu.h"

#define MASK (PR_R0709_SKIP_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void szp(pr_r0709_skip_cpu *cpu, uint16_t result, uint16_t sign)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((result & (uint16_t)((sign << 1) - 1u)) == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void cmp8(pr_r0709_skip_cpu *cpu, uint8_t a, uint8_t b)
{
    uint8_t result = (uint8_t)(a - b);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & 0x80u) cpu->flags |= 0x0800u;
    szp(cpu, result, 0x80u);
}

static uint16_t add16(pr_r0709_skip_cpu *cpu, uint16_t a, uint16_t b)
{
    uint32_t wide = (uint32_t)a + b;
    uint16_t result = (uint16_t)wide;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (wide > 0xFFFFu) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (~(a ^ b) & (a ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    szp(cpu, result, 0x8000u);
    return result;
}

/* P := SS:[BP+6]. */
static uint16_t parent(const uint8_t *ram, const pr_r0709_skip_cpu *cpu)
{
    return read16_at(ram, physical(cpu->ss, (uint16_t)(cpu->bp + 6u)));
}

pr_r0709_skip_status pr_r0709_skip_spaces_0714_cpu(uint8_t *ram, size_t ram_size, pr_r0709_skip_cpu *cpu, unsigned max_loopbacks)
{
    unsigned loopbacks = 0;
    uint8_t byte, before;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_R0709_SKIP_RAM_SIZE || cpu->ip != 0x0714u)
        return PR_R0709_SKIP_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);                                   /* 0714 PUSH BP; MOV BP,SP */
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & MASK] = (uint8_t)(cpu->bp >> 8);
    cpu->bp = cpu->sp;
    for (;;) {
        cpu->di = parent(ram, cpu);                                       /* 0717 MOV AL,SS:[P-103h]; CMP AL,SS:[P-101h]; JAE */
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[physical(cpu->ss, (uint16_t)(cpu->di - 0x103u))]);
        cpu->di = parent(ram, cpu);
        byte = ram[physical(cpu->ss, (uint16_t)(cpu->di - 0x101u))];
        cmp8(cpu, (uint8_t)cpu->ax, byte);
        if ((uint8_t)cpu->ax >= byte)
            break;
        cpu->di = parent(ram, cpu);                                       /* 0729 MOV AL,SS:[P-103h]; XOR AH,AH */
        cpu->ax = ram[physical(cpu->ss, (uint16_t)(cpu->di - 0x103u))];
        cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);
        cpu->di = parent(ram, cpu);                                       /* 0733 ADD DI,AX; CMP BYTE SS:[DI-100h],20h; JNE */
        cpu->di = add16(cpu, cpu->di, cpu->ax);
        byte = ram[physical(cpu->ss, (uint16_t)(cpu->di - 0x100u))];
        cmp8(cpu, byte, 0x20u);
        if (byte != 0x20u)
            break;
        cpu->di = parent(ram, cpu);                                       /* 0740 INC BYTE SS:[P-103h]; JMP 0717 */
        at = physical(cpu->ss, (uint16_t)(cpu->di - 0x103u));
        before = ram[at];
        ram[at] = (uint8_t)(before + 1u);
        cpu->flags &= (uint16_t)~0x08D4u;
        if ((before & 0x0Fu) == 0x0Fu) cpu->flags |= 0x0010u;
        if (before == 0x7Fu) cpu->flags |= 0x0800u;
        szp(cpu, ram[at], 0x80u);
        if (loopbacks == max_loopbacks) {
            cpu->ip = 0x0717u;
            return PR_R0709_SKIP_LIMIT;
        }
        ++loopbacks;
    }
    at = physical(cpu->ss, cpu->sp);                                      /* 074A POP BP; RETF 2 */
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    /* Each frame word's offset wraps within SS (DOSBox-X normal/386, measured): at SP FFFE
     * the CS word is SS:0000, not the physically next bytes. */
    cpu->cs = read16_at(ram, physical(cpu->ss, (uint16_t)(cpu->sp + 2u)));
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_R0709_SKIP_OK;
}

/* START.EXE resident 0A40:029C, the Turbo Pascal CRT Delay (raw CPU); docs/re/RES_0709_DELAY_0DBE_CPU.md. */
#include "poolrad/recovered/resident_a40_delay_029c_cpu.h"

#define MASK (PR_A40_DELAY_CPU_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t rd8(const uint8_t *ram, uint16_t segment, uint16_t offset) { return ram[physical(segment, offset)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t segment, uint16_t offset) { return read16_at(ram, physical(segment, offset)); }

static void wr16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void szp(pr_a40_delay_cpu *cpu, uint16_t result, uint16_t sign)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((result & (uint16_t)((sign << 1) - 1u)) == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void logic_flags(pr_a40_delay_cpu *cpu, uint16_t result)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    szp(cpu, result, 0x8000u);
}

/* DEC r16: CF kept. */
static uint16_t dec16(pr_a40_delay_cpu *cpu, uint16_t before)
{
    uint16_t result = (uint16_t)(before - 1u);
    cpu->flags &= (uint16_t)~0x08D4u;
    if ((before & 0x0Fu) == 0u) cpu->flags |= 0x0010u;
    if (before == 0x8000u) cpu->flags |= 0x0800u;
    szp(cpu, result, 0x8000u);
    return result;
}

static void cmp8(pr_a40_delay_cpu *cpu, uint8_t a, uint8_t b)
{
    uint8_t result = (uint8_t)(a - b);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & 0x80u) cpu->flags |= 0x0800u;
    szp(cpu, result, 0x80u);
}

pr_a40_delay_cpu_status pr_a40_delay_029c_cpu(uint8_t *ram, size_t ram_size, pr_a40_delay_cpu *cpu, uint64_t max_loopbacks)
{
    uint64_t loopbacks = 0;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_A40_DELAY_CPU_RAM_SIZE || cpu->ip != 0x029Cu)
        return PR_A40_DELAY_CPU_ARGUMENT;
    cpu->bx = cpu->sp;                                                    /* 029C MOV BX,SP; MOV DX,SS:[BX+4] */
    cpu->dx = rd16(ram, cpu->ss, (uint16_t)(cpu->bx + 4u));
    logic_flags(cpu, cpu->dx);                                            /* 02A2 OR DX,DX; JE 02BA */
    if (cpu->dx != 0u) {
        cpu->ax = 0x0040u;                                                /* 02A6 MOV AX,40h; MOV ES,AX; XOR DI,DI */
        cpu->es = cpu->ax;
        cpu->di = 0u;
        logic_flags(cpu, 0u);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | rd8(ram, cpu->es, cpu->di)); /* 02AD MOV AL,ES:[DI] */
        for (;;) {
            cpu->cx = rd16(ram, cpu->ds, 0x84FCu);                        /* 02B0 MOV CX,[84FCh]; CALL 02BD */
            cpu->sp = (uint16_t)(cpu->sp - 2u);
            wr16(ram, cpu->ss, cpu->sp, 0x02B7u);
            for (;;) {
                cpu->bx = 4u;                                             /* 02BD MOV BX,4; DEC BX; JNE 02C0 */
                for (;;) {
                    cpu->bx = dec16(cpu, cpu->bx);
                    if (cpu->bx == 0u)
                        break;
                    if (loopbacks == max_loopbacks) {
                        cpu->ip = 0x02C0u;
                        return PR_A40_DELAY_CPU_LIMIT;
                    }
                    ++loopbacks;
                }
                cmp8(cpu, (uint8_t)cpu->ax, rd8(ram, cpu->es, cpu->di));  /* 02C3 CMP AL,ES:[DI]; LOOPE 02BD */
                cpu->cx = (uint16_t)(cpu->cx - 1u);
                if (cpu->cx == 0u || !(cpu->flags & 0x0040u))
                    break;
                if (loopbacks == max_loopbacks) {
                    cpu->ip = 0x02BDu;
                    return PR_A40_DELAY_CPU_LIMIT;
                }
                ++loopbacks;
            }
            cpu->sp = (uint16_t)(cpu->sp + 2u);                           /* 02C8 RET (to 02B7) */
            cpu->dx = dec16(cpu, cpu->dx);                                /* 02B7 DEC DX; JNE 02B0 */
            if (cpu->dx == 0u)
                break;
            if (loopbacks == max_loopbacks) {
                cpu->ip = 0x02B0u;
                return PR_A40_DELAY_CPU_LIMIT;
            }
            ++loopbacks;
        }
    }
    at = physical(cpu->ss, cpu->sp);                                      /* 02BA RETF 2 */
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_A40_DELAY_CPU_RETURNED;
}

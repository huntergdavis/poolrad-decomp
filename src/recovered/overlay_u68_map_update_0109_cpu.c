/* GAME.OVR unit 0x68 +0109 map update (raw CPU); docs/re/U68_MAP_UPDATE_0109_CPU.md. */
#include "poolrad/recovered/overlay_u68_map_update_0109_cpu.h"

#define MASK (PR_U68_0109_RAM_SIZE - 1u)

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

static void szp(pr_u68_0109_cpu *cpu, uint16_t result, uint16_t sign)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((result & (uint16_t)((sign << 1) - 1u)) == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

/* CMP BYTE a,b. */
static void cmp8(pr_u68_0109_cpu *cpu, uint8_t a, uint8_t b)
{
    uint8_t result = (uint8_t)(a - b);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & 0x80u) cpu->flags |= 0x0800u;
    szp(cpu, result, 0x80u);
}

/* ADD of words: the six arithmetic flags; returns the sum. */
static uint16_t add16(pr_u68_0109_cpu *cpu, uint16_t a, uint16_t b)
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

/* AND/OR on AL: CF = OF = AF = 0. */
static void logic8(pr_u68_0109_cpu *cpu, uint8_t result)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    szp(cpu, result, 0x80u);
}

/* MOV AL,[BP+0Ah]; CBW; MOV DX,AX; MOV AL,[BP+8]; CBW; MOV CL,4; SHL AX,CL; LES DI,[6A5Ch]; ADD DI,AX; ADD DI,DX. */
static void address(const uint8_t *ram, pr_u68_0109_cpu *cpu)
{
    size_t at;
    uint16_t before;
    cpu->ax = (uint16_t)(int16_t)(int8_t)ram[physical(cpu->ss, (uint16_t)(cpu->bp + 0x0Au))];
    cpu->dx = cpu->ax;
    cpu->ax = (uint16_t)(int16_t)(int8_t)ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))];
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    before = cpu->ax;                                                     /* SHL AX,4: its flags never survive ADD */
    cpu->ax = (uint16_t)(before << 4);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (before & 0x1000u) cpu->flags |= 0x0001u;
    szp(cpu, cpu->ax, 0x8000u);
    at = physical(cpu->ds, 0x6A5Cu);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->di = add16(cpu, cpu->di, cpu->ax);
    cpu->di = add16(cpu, cpu->di, cpu->dx);
}

pr_u68_0109_status pr_u68_map_update_0109_cpu(uint8_t *ram, size_t ram_size, pr_u68_0109_cpu *cpu)
{
    static const uint8_t selectors[4] = {6u, 4u, 2u, 0u}, keep[4] = {0x3Fu, 0xCFu, 0xF3u, 0xFCu}, set[4] = {0x40u, 0x10u, 0x04u, 0x01u};
    size_t at;
    uint8_t x, y, byte;
    unsigned k;
    if (ram == NULL || cpu == NULL || ram_size != PR_U68_0109_RAM_SIZE || cpu->ip != 0x0109u)
        return PR_U68_0109_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);                                   /* 0109 PUSH BP; MOV BP,SP */
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)cpu->bp;
    ram[(at + 1u) & MASK] = (uint8_t)(cpu->bp >> 8);
    cpu->bp = cpu->sp;
    x = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 0x0Au))];              /* 010C x in 0..15? (JL; JG) */
    cmp8(cpu, x, 0u);
    if ((int8_t)x < 0)
        goto leave;
    x = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 0x0Au))];
    cmp8(cpu, x, 0x0Fu);
    if ((int8_t)x > 0x0F)
        goto leave;
    y = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))];                 /* 0118 y in 0..15? (JL; JLE) */
    cmp8(cpu, y, 0u);
    if ((int8_t)y < 0)
        goto leave;
    y = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))];
    cmp8(cpu, y, 0x0Fu);
    if ((int8_t)y > 0x0F)
        goto leave;                                                       /* 0124 JMP 0232 */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))]);  /* 0127 MOV AL,[BP+6] */
    for (k = 0; k < 4u; ++k) {                                            /* CMP AL,6/4/2/0; JNE next */
        cmp8(cpu, (uint8_t)cpu->ax, selectors[k]);
        if ((uint8_t)cpu->ax != selectors[k])
            continue;
        address(ram, cpu);                                                /* read ES:[DI+300h]; AND; OR; MOV BL,AL */
        byte = (uint8_t)(ram[physical(cpu->es, (uint16_t)(cpu->di + 0x300u))] & keep[k]);
        logic8(cpu, byte);
        byte = (uint8_t)(byte | set[k]);
        logic8(cpu, byte);
        cpu->bx = (uint16_t)((cpu->bx & 0xFF00u) | byte);
        address(ram, cpu);                                                /* the address again; MOV ES:[DI+300h],BL */
        ram[physical(cpu->es, (uint16_t)(cpu->di + 0x300u))] = (uint8_t)cpu->bx;
        break;
    }
leave:
    at = physical(cpu->ss, cpu->sp);                                      /* 0232 POP BP; RETF 6 */
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 6u);
    return PR_U68_0109_OK;
}

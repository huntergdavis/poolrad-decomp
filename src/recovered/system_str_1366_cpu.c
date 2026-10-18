/* START.EXE SYSTEM Str 0AF8:1366 with its near longint-to-decimal core 1254, raw CPU; docs/re/SYSTEM_STR_1366_RES_1037.md. */
#include "poolrad/recovered/system_str_1366_cpu.h"

#define MASK (PR_SYSTEM_LEAF_RAM_SIZE - 1u)
#define TABLE 0x12A6u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_system_leaf_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_system_leaf_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void stosb(uint8_t *ram, pr_system_leaf_cpu *cpu, uint8_t value)
{
    ram[physical(cpu->es, cpu->di)] = value;
    cpu->di = (uint16_t)(cpu->di + 1u);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* The power CS:[SI+2]:CS:[SI], reread from RAM. */
static uint32_t power(const uint8_t *ram, const pr_system_leaf_cpu *cpu)
{
    return ((uint32_t)read16(ram, cpu->cs, (uint16_t)(cpu->si + 2u)) << 16) | read16(ram, cpu->cs, cpu->si);
}

static int original_table(const uint8_t *ram, uint16_t cs)
{
    uint32_t expected = 1000000000u;
    unsigned i;
    for (i = 0; i < 10u; ++i, expected /= 10u) {
        uint16_t at = (uint16_t)(TABLE + 4u * i);
        if (read16(ram, cs, at) != (uint16_t)expected || read16(ram, cs, (uint16_t)(at + 2u)) != (uint16_t)(expected >> 16))
            return 0;
    }
    return 1;
}

/* 1254: X in DX:AX to decimal at ES:DI; CX := the characters written. */
static void decimal(uint8_t *ram, pr_system_leaf_cpu *cpu)
{
    uint32_t value, step;
    uint8_t digit;
    push16(ram, cpu, cpu->di);                                            /* 1254 PUSH DI; CLD; MOV BX,AX; OR DX,DX; JGE 1269 */
    cpu->flags &= (uint16_t)~0x0400u;
    cpu->bx = cpu->ax;
    if (cpu->dx & 0x8000u) {
        value = 0u - (((uint32_t)cpu->dx << 16) | cpu->bx);               /* 125C NOT BX; NOT DX; ADD BX,1; ADC DX,0 */
        cpu->bx = (uint16_t)value;
        cpu->dx = (uint16_t)(value >> 16);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x2Du);                /* 1266 MOV AL,'-'; STOSB */
        stosb(ram, cpu, 0x2Du);
    }
    cpu->si = TABLE;                                                      /* 1269 MOV SI,12A6h; MOV CL,9 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 9u);
    for (;;) {                                                            /* 126E CMP DX:BX with CS:[SI]: JB next, JA/JAE found */
        if ((((uint32_t)cpu->dx << 16) | cpu->bx) >= power(ram, cpu))
            break;
        cpu->si = (uint16_t)(cpu->si + 4u);                               /* 127B ADD SI,4; DEC CL; JNE 126E */
        cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(cpu->cx - 1u));
        if ((cpu->cx & 0x00FFu) == 0u)
            break;
    }
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(cpu->cx + 1u));  /* 1282 INC CL */
    do {
        digit = 0x2Fu;                                                    /* 1284 MOV AL,'0'-1 */
        for (;;) {                                                        /* 1286 INC AL; SUB BX,CS:[SI]; SBB DX,CS:[SI+2]; JAE 1286 */
            digit = (uint8_t)(digit + 1u);
            value = ((uint32_t)cpu->dx << 16) | cpu->bx;
            step = power(ram, cpu);
            cpu->bx = (uint16_t)(value - step);
            cpu->dx = (uint16_t)((value - step) >> 16);
            if (value < step)
                break;
        }
        value = (((uint32_t)cpu->dx << 16) | cpu->bx) + power(ram, cpu);  /* 1291 ADD BX,CS:[SI]; ADC DX,CS:[SI+2] */
        cpu->bx = (uint16_t)value;
        cpu->dx = (uint16_t)(value >> 16);
        cpu->si = (uint16_t)(cpu->si + 4u);                               /* 1298 ADD SI,4; STOSB; DEC CL; JNE 1284 */
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | digit);
        stosb(ram, cpu, digit);
        cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(cpu->cx - 1u));
    } while ((cpu->cx & 0x00FFu) != 0u);
    cpu->cx = cpu->di;                                                    /* 12A0 MOV CX,DI; POP DI; SUB CX,DI (flags folded) */
    cpu->di = pop16(ram, cpu);
    cpu->cx = (uint16_t)(cpu->cx - cpu->di);
}

pr_system_leaf_status pr_system_str_1366(uint8_t *ram, size_t ram_size, pr_system_leaf_cpu *cpu)
{
    uint16_t pad, result;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_LEAF_RAM_SIZE || cpu->ip != 0x1366u || !original_table(ram, cpu->cs))
        return PR_SYSTEM_LEAF_ARGUMENT;
    push16(ram, cpu, cpu->bp);                                            /* 1366 PUSH BP; MOV BP,SP; SUB SP,20h (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x20u);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));         /* 136C MOV AX,[BP+0Eh]; MOV DX,[BP+10h] */
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x10u));
    cpu->di = (uint16_t)(cpu->bp - 0x20u);                                /* 1372 LEA DI,[BP-20h]; PUSH SS; POP ES */
    push16(ram, cpu, cpu->ss);
    cpu->es = pop16(ram, cpu);
    push16(ram, cpu, 0x137Au);                                            /* 1377 CALL 1254 */
    decimal(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);                                   /* 12A5 RET: the return word +137A lies below the temporary */
    push16(ram, cpu, cpu->ds);                                            /* 137A PUSH DS; MOV SI,DI; PUSH SS; POP DS */
    cpu->si = cpu->di;
    push16(ram, cpu, cpu->ss);
    cpu->ds = pop16(ram, cpu);
    at = physical(cpu->ss, (uint16_t)(cpu->bp + 8u));                    /* 137F LES DI,[BP+8]; MOV DX,[BP+6]; MOV AX,[BP+0Ch] */
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));
    if ((int16_t)cpu->ax > (int16_t)cpu->dx)                              /* 1388 CMP AX,DX; JLE; MOV AX,DX */
        cpu->ax = cpu->dx;
    if ((int16_t)cpu->cx > (int16_t)cpu->dx)                              /* 138E CMP CX,DX; JLE; MOV CX,DX */
        cpu->cx = cpu->dx;
    if ((int16_t)cpu->ax < (int16_t)cpu->cx)                              /* 1394 CMP AX,CX; JGE; MOV AX,CX */
        cpu->ax = cpu->cx;
    cpu->flags &= (uint16_t)~0x0400u;                                     /* 139A CLD; STOSB */
    stosb(ram, cpu, (uint8_t)cpu->ax);
    result = (uint16_t)(cpu->ax - cpu->cx);                               /* 139C SUB AX,CX; JE 13A8 */
    cpu->flags &= (uint16_t)~0x08D5u;
    if (cpu->ax < cpu->cx) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((cpu->ax ^ cpu->cx ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((cpu->ax ^ cpu->cx) & (cpu->ax ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    cpu->ax = result;
    if (result != 0u) {
        push16(ram, cpu, cpu->cx);                                        /* 13A0 PUSH CX; MOV CX,AX; MOV AL,' '; REP STOSB; POP CX */
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x20u);
        for (pad = result; pad != 0u; --pad)
            stosb(ram, cpu, 0x20u);
        cpu->cx = pop16(ram, cpu);
    }
    while (cpu->cx != 0u) {                                               /* 13A8 REP MOVSB */
        stosb(ram, cpu, ram[physical(cpu->ds, cpu->si)]);
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    cpu->ds = pop16(ram, cpu);                                            /* 13AA POP DS; MOV SP,BP; POP BP; RETF 0Ch */
    cpu->sp = cpu->bp;
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 12u);
    return PR_SYSTEM_LEAF_RETURNED;
}

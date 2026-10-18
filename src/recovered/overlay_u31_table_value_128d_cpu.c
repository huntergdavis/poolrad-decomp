/* GAME.OVR unit 0x31 +128D (raw CPU, composing SYSTEM Move); docs/re/U31_TABLE_VALUE_128D.md. */
#include "poolrad/recovered/overlay_u31_table_value_128d_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#include <string.h>

#define MASK (PR_U31_128D_RAM_SIZE - 1u)
#define ENTRY_IP 0x128Du
#define MOVE_ENTRY 0x0A3Fu

_Static_assert(sizeof(pr_u31_128d_cpu) == sizeof(pr_system_leaf_cpu), "SYSTEM leaf CPU layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void push16(uint8_t *ram, pr_u31_128d_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

/* LES DI,seg:[off]: the segment word is the next two physical bytes. */
static void les_di(const uint8_t *ram, pr_u31_128d_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* SUB/CMP (subtract) or ADD flags at 8 or 16 bits. */
static void arith_flags(pr_u31_128d_cpu *cpu, uint16_t left, uint16_t right, unsigned bits, int subtract)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t wide = subtract ? (uint32_t)left - right : (uint32_t)left + right;
    uint32_t result = wide & mask;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (subtract ? left < right : wide > mask) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((subtract ? (left ^ right) : ~(uint32_t)(left ^ right)) & (left ^ result) & sign) cpu->flags |= 0x0800u;
}

/* JLE after CMP: ZF = 1 or SF != OF. */
static int le(const pr_u31_128d_cpu *cpu)
{
    unsigned zf = (cpu->flags >> 6) & 1u, sf = (cpu->flags >> 7) & 1u, of = (cpu->flags >> 11) & 1u;
    return zf || sf != of;
}

static int zf(const pr_u31_128d_cpu *cpu)
{
    return (cpu->flags & 0x0040u) != 0u;
}

static void set_al(pr_u31_128d_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint8_t local8(const uint8_t *ram, const pr_u31_128d_cpu *cpu, unsigned below)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - below));
}

static void set_local8(uint8_t *ram, const pr_u31_128d_cpu *cpu, unsigned below, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - below), value);
}

/* LES DI,[BP+disp] */
static void les_arg(const uint8_t *ram, pr_u31_128d_cpu *cpu, uint16_t disp)
{
    les_di(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + disp));
}

/* MOV AL,[BP-2]; XOR AH,AH; ADD AX,DX; MOV [BP-2],AL (flags folded) */
static void add_to_value(uint8_t *ram, pr_u31_128d_cpu *cpu)
{
    cpu->ax = (uint16_t)(local8(ram, cpu, 2u) + cpu->dx);
    set_local8(ram, cpu, 2u, (uint8_t)cpu->ax);
}

pr_u31_128d_status pr_u31_table_value_128d_cpu(uint8_t *ram, size_t ram_size, pr_u31_128d_cpu *cpu, int *callee_status)
{
    uint16_t entry, at, return_ip, caller_cs;
    size_t frame;
    uint8_t value;
    pr_system_leaf_cpu leaf;
    pr_system_leaf_status status;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U31_128D_RAM_SIZE)
        return PR_U31_128D_ARGUMENT;
    entry = cpu->ip;
    push16(ram, cpu, cpu->bp);                                            /* +128D PUSH BP; MOV BP,SP; SUB SP,12h */
    cpu->bp = cpu->sp;
    arith_flags(cpu, cpu->sp, 0x12u, 16u, 1);
    cpu->sp = (uint16_t)(cpu->sp - 0x12u);
    les_arg(ram, cpu, 6u);                                                /* +1293 LES DI,[BP+6]; MOV AL,ES:[DI+2Eh] */
    cpu->ax = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Eu));           /* +129A XOR AH,AH (flags folded) */
    cpu->di = (uint16_t)(cpu->ax << 4);                                   /* +129C MOV DI,AX; MOV CL,4; SHL DI,CL */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
    arith_flags(cpu, cpu->di, 0x5582u, 16u, 0);                           /* +12A2 ADD DI,5582h */
    cpu->di = (uint16_t)(cpu->di + 0x5582u);
    push16(ram, cpu, cpu->ds);                                            /* +12A6 PUSH DS; PUSH DI */
    push16(ram, cpu, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x12u);                                /* +12A8 LEA DI,[BP-12h]; PUSH SS; PUSH DI */
    push16(ram, cpu, cpu->ss);
    push16(ram, cpu, cpu->di);
    cpu->ax = 0x10u;                                                      /* +12AD MOV AX,10h; PUSH AX */
    push16(ram, cpu, cpu->ax);
    at = (uint16_t)(entry + 0x24u);                                       /* +12B1 LCALL xxxx:0A3F */
    return_ip = (uint16_t)(at + 5u);
    cpu->ip = at;
    if (read8(ram, cpu->cs, at) != 0x9Au || read16(ram, cpu->cs, (uint16_t)(at + 1u)) != MOVE_ENTRY)
        return PR_U31_128D_CODE;
    caller_cs = cpu->cs;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->cs = read16(ram, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = MOVE_ENTRY;
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_system_move_0a3f(ram, ram_size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U31_128D_TRANSFER;
    {                                                                     /* +12B6..+12C4: v := L[9] * L[10] */
        uint32_t product;
        cpu->dx = local8(ram, cpu, 9u);
        cpu->ax = local8(ram, cpu, 8u);
        product = (uint32_t)cpu->ax * cpu->dx;                            /* MUL DX (flags folded) */
        cpu->ax = (uint16_t)product;
        cpu->dx = (uint16_t)(product >> 16);
        set_local8(ram, cpu, 2u, (uint8_t)cpu->ax);
    }
    les_arg(ram, cpu, 6u);                                                /* +12C7 CMP BYTE ES:[DI+32h],0; JLE */
    value = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x32u));
    arith_flags(cpu, value, 0u, 8u, 1);
    if (!le(cpu)) {
        les_arg(ram, cpu, 6u);                                            /* +12D1 MOV AL,ES:[DI+32h]; CBW; MOV CL,3; SHL AX,CL */
        set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x32u)));
        cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
        cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 3u);
        cpu->ax = (uint16_t)(cpu->ax << 3);
        cpu->dx = cpu->ax;                                                /* +12DD MOV DX,AX; ... MOV [BP-2],AL */
        add_to_value(ram, cpu);
    }
    value = local8(ram, cpu, 7u);                                         /* +12E9 CMP BYTE [BP-7],0; JLE */
    arith_flags(cpu, value, 0u, 8u, 1);
    if (!le(cpu)) {
        set_al(cpu, local8(ram, cpu, 7u));                                /* +12EF MOV AL,[BP-7]; CBW; SHL AX,1 */
        cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
        cpu->ax = (uint16_t)(cpu->ax << 1);
        cpu->dx = cpu->ax;
        add_to_value(ram, cpu);
    }
    les_arg(ram, cpu, 6u);                                                /* +1301 CMP BYTE ES:[DI+2Eh],55h; JNE */
    value = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Eu));
    arith_flags(cpu, value, 0x55u, 8u, 1);
    if (zf(cpu)) {
        les_arg(ram, cpu, 0x0Au);                                         /* +130B LES DI,[BP+0Ah]; LES DI,ES:[DI+108h] */
        les_di(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x108u));
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x0Au));      /* +1313 MOV AX,ES:[DI+0Ah]; OR AX,ES:[DI+0Ch]; JE */
        cpu->ax = (uint16_t)(cpu->ax | read16(ram, cpu->es, (uint16_t)(cpu->di + 0x0Cu)));
        cpu->flags &= (uint16_t)~0x08D5u;
        if (even_parity((uint8_t)cpu->ax)) cpu->flags |= 0x0004u;
        if (cpu->ax == 0u) cpu->flags |= 0x0040u;
        if (cpu->ax & 0x8000u) cpu->flags |= 0x0080u;
        if (!zf(cpu)) {
            les_arg(ram, cpu, 0x0Au);                                     /* +131D LES DI,[BP+0Ah]; LES DI,ES:[DI+108h]; LES DI,ES:[DI+0Ah] */
            les_di(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x108u));
            les_di(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x0Au));
            value = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x76u));     /* +1329 CMP BYTE ES:[DI+76h],0; JLE */
            arith_flags(cpu, value, 0u, 8u, 1);
            if (!le(cpu))
                set_local8(ram, cpu, 2u, 8u);                             /* +1330 MOV BYTE [BP-2],8 */
        }
    }
    set_al(cpu, (uint8_t)(local8(ram, cpu, 4u) & 8u));                    /* +1334 MOV AL,[BP-4]; AND AL,8; OR AL,AL; JBE */
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)cpu->ax)) cpu->flags |= 0x0004u;
    if ((uint8_t)cpu->ax == 0u) cpu->flags |= 0x0040u;
    if (!zf(cpu)) {
        cpu->ax = (uint16_t)(local8(ram, cpu, 0x0Du) - 1u);              /* +133D MOV AL,[BP-0Dh]; XOR AH,AH; DEC AX; SHL AX,1 */
        cpu->ax = (uint16_t)(cpu->ax << 1);
        cpu->dx = cpu->ax;
        add_to_value(ram, cpu);
    }
    value = local8(ram, cpu, 0x11u);                                      /* +1351 CMP BYTE [BP-11h],1; JA */
    arith_flags(cpu, value, 1u, 8u, 1);
    if (value <= 1u) {
        cpu->ax = (uint16_t)(local8(ram, cpu, 2u) + 3u);                  /* +1357 MOV AL,[BP-2]; XOR AH,AH; ADD AX,3 */
        set_local8(ram, cpu, 2u, (uint8_t)cpu->ax);
    }
    les_arg(ram, cpu, 0x0Au);                                             /* +1362 MOV AL,ES:[DI+100h]; XOR AH,AH; MOV DX,AX */
    cpu->dx = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x100u));
    cpu->ax = (uint16_t)(local8(ram, cpu, 0x11u) + cpu->dx);              /* +136E MOV AL,[BP-11h]; XOR AH,AH; ADD AX,DX */
    arith_flags(cpu, cpu->ax, 3u, 16u, 1);                                /* +1375 CMP AX,3; JLE */
    if (!le(cpu))
        set_local8(ram, cpu, 2u, 0u);                                     /* +137A MOV BYTE [BP-2],0 */
    les_arg(ram, cpu, 6u);                                                /* +137E CMP BYTE ES:[DI+3Eh],84h; JNE */
    value = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x3Eu));
    arith_flags(cpu, value, 0x84u, 8u, 1);
    if (zf(cpu)) {
        les_arg(ram, cpu, 6u);                                            /* +1388 MOV AL,ES:[DI+3Dh]; AND AL,0Fh */
        set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 0x3Du)) & 0x0Fu));
        les_arg(ram, cpu, 0x0Au);                                         /* +1391 CMP AL,ES:[DI+0A0h]; JE */
        arith_flags(cpu, (uint8_t)cpu->ax, read8(ram, cpu->es, (uint16_t)(cpu->di + 0xA0u)), 8u, 1);
        if (!zf(cpu))
            set_local8(ram, cpu, 2u, 0u);                                 /* +139B MOV BYTE [BP-2],0 */
    }
    les_arg(ram, cpu, 6u);                                                /* +139F CMP BYTE ES:[DI+3Dh],53h; JNE */
    value = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x3Du));
    arith_flags(cpu, value, 0x53u, 8u, 1);
    if (zf(cpu))
        set_local8(ram, cpu, 2u, 0u);                                     /* +13A9 MOV BYTE [BP-2],0 */
    les_arg(ram, cpu, 6u);                                                /* +13AD CMP BYTE ES:[DI+36h],0; JE */
    value = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x36u));
    arith_flags(cpu, value, 0u, 8u, 1);
    if (!zf(cpu))
        set_local8(ram, cpu, 2u, 0u);                                     /* +13B7 MOV BYTE [BP-2],0 */
    set_al(cpu, local8(ram, cpu, 2u));                                    /* +13BB MOV AL,[BP-2]; MOV [BP-1],AL; MOV AL,[BP-1] */
    set_local8(ram, cpu, 1u, (uint8_t)cpu->ax);
    set_al(cpu, local8(ram, cpu, 1u));
    cpu->sp = cpu->bp;                                                    /* +13C4 MOV SP,BP; POP BP; RETF 8 */
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    frame = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, frame);
    cpu->cs = read16_at(ram, (frame + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 8u);
    return PR_U31_128D_OK;
}

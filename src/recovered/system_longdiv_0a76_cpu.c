/* Turbo Pascal SYSTEM 0AF8:0A76 LongInt divide (raw CPU); docs/re/SYSTEM_LONGDIV_0A76.md. */
#include "poolrad/recovered/system_longdiv_0a76_cpu.h"

#define MASK (PR_SYSTEM_LONGDIV_RAM_SIZE - 1u)
#define CANONICAL(f) ((uint16_t)(((f) & 0x7FD7u) | 0x0002u))

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void push16(uint8_t *ram, pr_system_longdiv_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t pop16(const uint8_t *ram, pr_system_longdiv_cpu *cpu)
{
    uint16_t value = read16_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* OR r16,r16 / OR BP,BX: CF = OF = 0, AF clear as both oracles leave it. */
static void logic_flags(pr_system_longdiv_cpu *cpu, uint16_t result)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
}

/* NOT low; NOT high; ADD low,1; ADC high,0, with the ADC's flags (the ADD's are overwritten). */
static void negate32(pr_system_longdiv_cpu *cpu, uint16_t *low, uint16_t *high)
{
    uint16_t lo = (uint16_t)~*low, hi = (uint16_t)~*high, carry, result;
    carry = lo == 0xFFFFu;
    lo = (uint16_t)(lo + 1u);
    result = (uint16_t)(hi + carry);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (carry && hi == 0xFFFFu) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((hi ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (~hi & result & 0x8000u) cpu->flags |= 0x0800u;
    *low = lo;
    *high = result;
}

pr_system_longdiv_status pr_system_longdiv_0a76(uint8_t *ram, size_t ram_size, pr_system_longdiv_cpu *cpu)
{
    uint16_t f1, f2;
    uint32_t dividend, divisor, quotient = 0u, remainder = 0u;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_SYSTEM_LONGDIV_RAM_SIZE)
        return PR_SYSTEM_LONGDIV_ARGUMENT;
    push16(ram, cpu, cpu->bp);                                            /* 0A76 PUSH BP */
    cpu->bp = (uint16_t)(cpu->cx | cpu->bx);                              /* 0A77 MOV BP,CX; OR BP,BX; JE 0ADD */
    logic_flags(cpu, cpu->bp);
    if (cpu->bp == 0u) {
        cpu->bp = pop16(ram, cpu);                                        /* 0ADD POP BP; MOV AX,0C8h; JMP 00E2 */
        cpu->ax = 0x00C8u;
        cpu->ip = 0x00E2u;
        return PR_SYSTEM_LONGDIV_DIVIDE_ERROR;
    }
    logic_flags(cpu, cpu->bx);                                            /* 0A7D OR BX,BX; PUSHF; JNS */
    f1 = CANONICAL(cpu->flags);
    push16(ram, cpu, f1);
    if (cpu->bx & 0x8000u)
        negate32(cpu, &cpu->cx, &cpu->bx);                                /* 0A82 NOT CX; NOT BX; ADD CX,1; ADC BX,0 */
    logic_flags(cpu, cpu->dx);                                            /* 0A8C OR DX,DX; PUSHF; JNS */
    f2 = CANONICAL(cpu->flags);
    push16(ram, cpu, f2);
    if (cpu->dx & 0x8000u)
        negate32(cpu, &cpu->ax, &cpu->dx);                                /* 0A91 NOT AX; NOT DX; ADD AX,1; ADC DX,0 */
    cpu->si = cpu->cx;                                                    /* 0A9B MOV SI,CX; MOV DI,BX; XOR CX,CX; XOR BX,BX */
    cpu->di = cpu->bx;
    divisor = ((uint32_t)cpu->di << 16) | cpu->si;
    dividend = ((uint32_t)cpu->dx << 16) | cpu->ax;
    {                                                                     /* 0AA3 MOV BP,21h; 33 restoring steps (flags folded) */
        unsigned carry = 0u;
        for (unsigned step = 0u; step < 33u; ++step) {
            unsigned bit;
            remainder = (remainder << 1) | carry;                          /* RCL CX,1; RCL BX,1 (bit 32 is dropped) */
            if (remainder >= divisor) {                                    /* SUB CX,SI; SBB BX,DI: no borrow */
                remainder -= divisor;
                bit = 1u;
            } else {
                bit = 0u;                                                 /* borrow: ADD CX,SI; ADC BX,DI restores */
            }
            carry = (unsigned)(dividend >> 31);                            /* CMC; RCL AX,1; RCL DX,1 */
            dividend = (dividend << 1) | bit;
        }
        quotient = dividend;
    }
    cpu->ax = (uint16_t)quotient;
    cpu->dx = (uint16_t)(quotient >> 16);
    cpu->cx = (uint16_t)remainder;
    cpu->bx = (uint16_t)(remainder >> 16);
    cpu->bp = 0u;                                                         /* 0AB9 DEC BP reaches 0 */
    f2 = pop16(ram, cpu);                                                 /* 0ABC POPF; JNS 0ACE */
    cpu->flags = CANONICAL(f2);
    if (cpu->flags & 0x0080u) {
        negate32(cpu, &cpu->cx, &cpu->bx);                                /* 0ABF negate the remainder */
        f1 = pop16(ram, cpu);                                             /* 0AC9 POPF; JNS 0AD1; JMP 0ADB */
        cpu->flags = CANONICAL(f1);
        if (!(cpu->flags & 0x0080u))
            negate32(cpu, &cpu->ax, &cpu->dx);                            /* 0AD1 negate the quotient */
    } else {
        f1 = pop16(ram, cpu);                                             /* 0ACE POPF; JNS 0ADB */
        cpu->flags = CANONICAL(f1);
        if (cpu->flags & 0x0080u)
            negate32(cpu, &cpu->ax, &cpu->dx);                            /* 0AD1 negate the quotient */
    }
    cpu->bp = pop16(ram, cpu);                                            /* 0ADB POP BP; RETF */
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_SYSTEM_LONGDIV_RETURNED;
}

/* Complete U33 [040F,0615); docs/re/U33_GRID_PATTERN_040F.md. */
#include "poolrad/recovered/overlay_u33_grid_pattern_040f.h"
#include "poolrad/recovered/overlay_u33_clipped_writer_0221.h"

#define MASK (PR_U33_01AD_RAM_SIZE - 1u)
#define ARITH 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}
static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}
static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t p = physical(segment, offset);
    return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8));
}
static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}
static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t p = physical(segment, offset);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & MASK] = (uint8_t)(value >> 8);
}
static void push(uint8_t *ram, pr_u33_040f_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}
static uint16_t pop(const uint8_t *ram, pr_u33_040f_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}
static void set_al(pr_u33_040f_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}
static void cbw(pr_u33_040f_cpu *cpu)
{
    uint16_t value = cpu->ax & 0xFFu;
    cpu->ax = (uint16_t)((value & 0x80u) ? value | 0xFF00u : value);
}
static uint16_t arithmetic(pr_u33_040f_cpu *cpu, uint16_t a, uint16_t b,
                           unsigned bits, int subtract)
{
    uint16_t sign = bits == 8u ? 0x80u : 0x8000u;
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint32_t wide = subtract ? (uint32_t)a - b : (uint32_t)a + b;
    uint16_t result = (uint16_t)(wide & mask);
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    cpu->flags &= (uint16_t)~ARITH;
    if (subtract ? a < b : wide > mask) cpu->flags |= 1u;
    if ((parity & 1u) == 0u) cpu->flags |= 4u;
    if (((a ^ b ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & sign) != 0u) cpu->flags |= 0x80u;
    if (((subtract ? a ^ b : (uint16_t)~(a ^ b)) &
         (a ^ result) & sign) != 0u) cpu->flags |= 0x800u;
    return result;
}
static uint16_t increment(pr_u33_040f_cpu *cpu, uint16_t value,
                          unsigned bits, int decrement)
{
    uint16_t carry = cpu->flags & 1u;
    uint16_t result = arithmetic(cpu, value, 1u, bits, decrement);
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~1u) | carry);
    return result;
}
#define COPY_CPU(dst, src) do { \
    (dst)->ax = (src)->ax; \
    (dst)->bx = (src)->bx; \
    (dst)->cx = (src)->cx; \
    (dst)->dx = (src)->dx; \
    (dst)->si = (src)->si; \
    (dst)->di = (src)->di; \
    (dst)->bp = (src)->bp; \
    (dst)->sp = (src)->sp; \
    (dst)->cs = (src)->cs; \
    (dst)->ip = (src)->ip; \
    (dst)->ds = (src)->ds; \
    (dst)->es = (src)->es; \
    (dst)->ss = (src)->ss; \
    (dst)->flags = (src)->flags; \
} while (0)

static pr_u33_040f_status near_call(uint8_t *ram, size_t size,
    pr_u33_040f_cpu *cpu, uint16_t target, uint16_t continuation)
{
    uint16_t caller_cs = cpu->cs;
    push(ram, cpu, continuation); /* Caller has already pushed CS. */
    cpu->ip = target;
    if (target == 0x01ADu) {
        pr_u33_01ad_status status = pr_u33_cell_pair_01ad(ram, size, cpu);
        if (status == PR_U33_01AD_TRANSFER) return PR_U33_040F_TRANSFER;
        if (status != PR_U33_01AD_OK) return PR_U33_040F_CALLEE;
    } else {
        pr_u33_0221_cpu inner;
        pr_u33_0221_status status;
        COPY_CPU(&inner, cpu);
        status = pr_u33_clipped_writer_0221(ram, size, &inner);
        COPY_CPU(cpu, &inner);
        if (status != PR_U33_0221_OK) return PR_U33_040F_CALLEE;
    }
    return cpu->cs == caller_cs && cpu->ip == continuation ?
        PR_U33_040F_OK : PR_U33_040F_TRANSFER;
}


static void logic8(pr_u33_040f_cpu *cpu, uint8_t value)
{
    (void)arithmetic(cpu, value, 0u, 8u, 0);
    cpu->flags &= (uint16_t)~0x0811u;
}

pr_u33_040f_status pr_u33_grid_pattern_040f(
    uint8_t *ram, size_t ram_size, pr_u33_040f_cpu *cpu)
{
    pr_u33_040f_status status;
    unsigned i;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_01AD_RAM_SIZE ||
        cpu->ip != 0x040Fu || cpu->sp < 0x0080u || cpu->sp > 0xFFFAu)
        return PR_U33_040F_ARGUMENT;
    for (i = 1u; i <= 128u; ++i) {
        size_t p = physical(cpu->ss, (uint16_t)(cpu->sp - i));
        size_t relative = (p - physical(cpu->cs, 0u)) & MASK;
        if ((relative >= 0x012Bu && relative < 0x02BAu) ||
            (relative >= 0x040Fu && relative < 0x0615u))
            return PR_U33_040F_CODE_ALIAS;
    }
    push(ram, cpu, cpu->bp); /* +040F */
    cpu->bp = cpu->sp; /* +0410 */
    cpu->sp = arithmetic(cpu, cpu->sp, 0x6u, 16u, 1); /* +0412 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0415 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x3u)))); /* +0418 */
    push(ram, cpu, cpu->ax); /* +041C */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +041D */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x4u)))); /* +0420 */
    cbw(cpu); /* +0424 */
    cpu->ax = increment(cpu, cpu->ax, 16u, 1); /* +0425 */
    push(ram, cpu, cpu->ax); /* +0426 */
    set_al(cpu, (uint8_t)(0x6u)); /* +0427 */
    push(ram, cpu, cpu->ax); /* +0429 */
    push(ram, cpu, cpu->cs); /* +042A */
    status = near_call(ram, ram_size, cpu, 0x01ADu, 0x042Eu); /* +042B */
    if (status != PR_U33_040F_OK) return status;
    logic8(cpu, (uint8_t)cpu->ax); /* +042E */
    if ((cpu->flags & 0x40u) == 0u) goto L0455; /* +0430 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0432 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x3u)))); /* +0435 */
    cbw(cpu); /* +0439 */
    cpu->ax = increment(cpu, cpu->ax, 16u, 1); /* +043A */
    push(ram, cpu, cpu->ax); /* +043B */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +043C */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x4u)))); /* +043F */
    push(ram, cpu, cpu->ax); /* +0443 */
    set_al(cpu, (uint8_t)(0x0u)); /* +0444 */
    push(ram, cpu, cpu->ax); /* +0446 */
    push(ram, cpu, cpu->cs); /* +0447 */
    status = near_call(ram, ram_size, cpu, 0x01ADu, 0x044Bu); /* +0448 */
    if (status != PR_U33_040F_OK) return status;
    logic8(cpu, (uint8_t)cpu->ax); /* +044B */
    if ((cpu->flags & 0x40u) == 0u) goto L0455; /* +044D */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u), (uint8_t)(0x1u)); /* +044F */
    goto L0459; /* +0453 */
L0455:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u), (uint8_t)(0x0u)); /* +0455 */
L0459:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u), (uint8_t)(0x1u)); /* +0459 */
    goto L0462; /* +045D */
L045F:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u)), 8u, 0))); /* +045F */
L0462:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u)))); /* +0462 */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +0465 */
    if ((cpu->flags & 0x40u) == 0u) goto L04DC; /* +0467 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0469 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x7u)))); /* +046C */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +0470 */
    if ((cpu->flags & 0x40u) == 0u) goto L04A5; /* +0472 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0474 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x8u)))); /* +0477 */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +047B */
    if ((cpu->flags & 0x40u) == 0u) goto L0485; /* +047D */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0x16u)); /* +047F */
    goto L04A3; /* +0483 */
L0485:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +0485 */
    if ((cpu->flags & 0x40u) == 0u) goto L048F; /* +0487 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0xDu)); /* +0489 */
    goto L04A3; /* +048D */
L048F:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +048F */
    if ((cpu->flags & 0x40u) == 0u) goto L04A3; /* +0491 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u)), 0x0u, 8u, 1); /* +0493 */
    if ((cpu->flags & 0x40u) != 0u) goto L049F; /* +0497 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0x0u)); /* +0499 */
    goto L04A3; /* +049D */
L049F:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0xDu)); /* +049F */
L04A3:
    goto L04D9; /* +04A3 */
L04A5:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +04A5 */
    if ((cpu->flags & 0x40u) != 0u) goto L04AD; /* +04A7 */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +04A9 */
    if ((cpu->flags & 0x40u) == 0u) goto L04D9; /* +04AB */
L04AD:
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +04AD */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x8u)), 0x0u, 8u, 1); /* +04B0 */
    if ((cpu->flags & 0x40u) == 0u) goto L04C9; /* +04B5 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u)), 0x0u, 8u, 1); /* +04B7 */
    if ((cpu->flags & 0x40u) != 0u) goto L04C3; /* +04BB */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0xFu)); /* +04BD */
    goto L04C7; /* +04C1 */
L04C3:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0x5u)); /* +04C3 */
L04C7:
    goto L04D9; /* +04C7 */
L04C9:
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u)), 0x0u, 8u, 1); /* +04C9 */
    if ((cpu->flags & 0x40u) != 0u) goto L04D5; /* +04CD */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0x12u)); /* +04CF */
    goto L04D9; /* +04D3 */
L04D5:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0x2u)); /* +04D5 */
L04D9:
    goto L05C2; /* +04D9 */
L04DC:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x2u, 8u, 1); /* +04DC */
    if ((cpu->flags & 0x40u) == 0u) goto L0506; /* +04DE */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +04E0 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x7u)))); /* +04E3 */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +04E7 */
    if ((cpu->flags & 0x40u) == 0u) goto L04F1; /* +04E9 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x16u)); /* +04EB */
    goto L0503; /* +04EF */
L04F1:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +04F1 */
    if ((cpu->flags & 0x40u) == 0u) goto L04FB; /* +04F3 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x11u)); /* +04F5 */
    goto L0503; /* +04F9 */
L04FB:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +04FB */
    if ((cpu->flags & 0x40u) == 0u) goto L0503; /* +04FD */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x5u)); /* +04FF */
L0503:
    goto L05C2; /* +0503 */
L0506:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +0506 */
    if ((cpu->flags & 0x40u) == 0u) goto L0563; /* +0508 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +050A */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x8u)))); /* +050D */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +0511 */
    if ((cpu->flags & 0x40u) == 0u) goto L0537; /* +0513 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0515 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x7u)), 0x0u, 8u, 1); /* +0518 */
    if ((cpu->flags & 0x40u) == 0u) goto L0525; /* +051D */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0x16u)); /* +051F */
    goto L0535; /* +0523 */
L0525:
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u)), 0x0u, 8u, 1); /* +0525 */
    if ((cpu->flags & 0x40u) != 0u) goto L0531; /* +0529 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0x10u)); /* +052B */
    goto L0535; /* +052F */
L0531:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0xAu)); /* +0531 */
L0535:
    goto L0561; /* +0535 */
L0537:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +0537 */
    if ((cpu->flags & 0x40u) == 0u) goto L054D; /* +0539 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u)), 0x0u, 8u, 1); /* +053B */
    if ((cpu->flags & 0x40u) != 0u) goto L0547; /* +053F */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0x14u)); /* +0541 */
    goto L054B; /* +0545 */
L0547:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0x7u)); /* +0547 */
L054B:
    goto L0561; /* +054B */
L054D:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +054D */
    if ((cpu->flags & 0x40u) == 0u) goto L0561; /* +054F */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u)), 0x0u, 8u, 1); /* +0551 */
    if ((cpu->flags & 0x40u) != 0u) goto L055D; /* +0555 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0x1u)); /* +0557 */
    goto L0561; /* +055B */
L055D:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0x3u)); /* +055D */
L0561:
    goto L05C2; /* +0561 */
L0563:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x4u, 8u, 1); /* +0563 */
    if ((cpu->flags & 0x40u) == 0u) goto L05C2; /* +0565 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0567 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x8u)))); /* +056A */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +056E */
    if ((cpu->flags & 0x40u) != 0u) goto L0576; /* +0570 */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +0572 */
    if ((cpu->flags & 0x40u) == 0u) goto L059B; /* +0574 */
L0576:
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0576 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x7u)))); /* +0579 */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +057D */
    if ((cpu->flags & 0x40u) == 0u) goto L0587; /* +057F */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0x16u)); /* +0581 */
    goto L0599; /* +0585 */
L0587:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +0587 */
    if ((cpu->flags & 0x40u) == 0u) goto L0591; /* +0589 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0x17u)); /* +058B */
    goto L0599; /* +058F */
L0591:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +0591 */
    if ((cpu->flags & 0x40u) == 0u) goto L0599; /* +0593 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0xAu)); /* +0595 */
L0599:
    goto L05C2; /* +0599 */
L059B:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +059B */
    if ((cpu->flags & 0x40u) == 0u) goto L05C2; /* +059D */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +059F */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x7u)))); /* +05A2 */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +05A6 */
    if ((cpu->flags & 0x40u) == 0u) goto L05B0; /* +05A8 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0xDu)); /* +05AA */
    goto L05C2; /* +05AE */
L05B0:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +05B0 */
    if ((cpu->flags & 0x40u) == 0u) goto L05BA; /* +05B2 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0x15u)); /* +05B4 */
    goto L05C2; /* +05B8 */
L05BA:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +05BA */
    if ((cpu->flags & 0x40u) == 0u) goto L05C2; /* +05BC */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0x6u)); /* +05BE */
L05C2:
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u)), 0x4u, 8u, 1); /* +05C2 */
    if ((cpu->flags & 0x40u) != 0u) goto L05CB; /* +05C6 */
    goto L045F; /* +05C8 */
L05CB:
    set_al(cpu, (uint8_t)(0x1u)); /* +05CB */
    push(ram, cpu, cpu->ax); /* +05CD */
    set_al(cpu, (uint8_t)(0x0u)); /* +05CE */
    push(ram, cpu, cpu->ax); /* +05D0 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u)))); /* +05D1 */
    push(ram, cpu, cpu->ax); /* +05D4 */
    push(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u))); /* +05D5 */
    push(ram, cpu, cpu->cs); /* +05D8 */
    status = near_call(ram, ram_size, cpu, 0x0221u, 0x05DCu); /* +05D9 */
    if (status != PR_U33_040F_OK) return status;
    set_al(cpu, (uint8_t)(0x2u)); /* +05DC */
    push(ram, cpu, cpu->ax); /* +05DE */
    set_al(cpu, (uint8_t)(0x0u)); /* +05DF */
    push(ram, cpu, cpu->ax); /* +05E1 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u)))); /* +05E2 */
    push(ram, cpu, cpu->ax); /* +05E5 */
    push(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u))); /* +05E6 */
    push(ram, cpu, cpu->cs); /* +05E9 */
    status = near_call(ram, ram_size, cpu, 0x0221u, 0x05EDu); /* +05EA */
    if (status != PR_U33_040F_OK) return status;
    set_al(cpu, (uint8_t)(0x1u)); /* +05ED */
    push(ram, cpu, cpu->ax); /* +05EF */
    set_al(cpu, (uint8_t)(0x1u)); /* +05F0 */
    push(ram, cpu, cpu->ax); /* +05F2 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u)))); /* +05F3 */
    push(ram, cpu, cpu->ax); /* +05F6 */
    push(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u))); /* +05F7 */
    push(ram, cpu, cpu->cs); /* +05FA */
    status = near_call(ram, ram_size, cpu, 0x0221u, 0x05FEu); /* +05FB */
    if (status != PR_U33_040F_OK) return status;
    set_al(cpu, (uint8_t)(0x2u)); /* +05FE */
    push(ram, cpu, cpu->ax); /* +0600 */
    set_al(cpu, (uint8_t)(0x1u)); /* +0601 */
    push(ram, cpu, cpu->ax); /* +0603 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u)))); /* +0604 */
    push(ram, cpu, cpu->ax); /* +0607 */
    push(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u))); /* +0608 */
    push(ram, cpu, cpu->cs); /* +060B */
    status = near_call(ram, ram_size, cpu, 0x0221u, 0x060Fu); /* +060C */
    if (status != PR_U33_040F_OK) return status;
    cpu->sp = cpu->bp; /* +060F */
    cpu->bp = pop(ram, cpu); /* +0611 */
    {
        /* Grid aliases can move this frame across FFFE/FFFF; RETF reads
         * four physically contiguous bytes, then wraps the resulting SP. */
        size_t p = physical(cpu->ss, cpu->sp);
        cpu->ip = read16(ram, cpu->ss, cpu->sp);
        cpu->cs = (uint16_t)(ram[(p + 2u) & MASK] |
                            ((uint16_t)ram[(p + 3u) & MASK] << 8));
        cpu->sp = (uint16_t)(cpu->sp + 6u);
    } /* +0612 */
    return PR_U33_040F_OK;
}

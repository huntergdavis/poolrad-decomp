/* GAME.OVR U3C2 entry5 [0422,057B), 345 bytes; callee/parity gates open. */
#include "poolrad/recovered/overlay_u3c2_entry5_0422.h"

#define RAM_MASK (PR_U3C2_0422_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static uint16_t arithmetic(pr_u3c2_0422_cpu *cpu, uint16_t left,
                           uint16_t right, uint16_t sign, int subtract)
{
    uint32_t wide = subtract ? (uint32_t)left - right : (uint32_t)left + right;
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint16_t result = (uint16_t)(wide & mask);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (subtract ? left < right : wide > mask) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & sign) != 0u) cpu->flags |= 0x80u;
    if (((subtract ? (left ^ right) : (uint16_t)~(left ^ right)) &
         (left ^ result) & sign) != 0u) cpu->flags |= 0x800u;
    return result;
}

static void cmp8(pr_u3c2_0422_cpu *cpu, uint8_t left, uint8_t right)
{
    (void)arithmetic(cpu, left, right, 0x80u, 1);
}

static void cmp16(pr_u3c2_0422_cpu *cpu, uint16_t left, uint16_t right)
{
    (void)arithmetic(cpu, left, right, 0x8000u, 1);
}

static void set_al(pr_u3c2_0422_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u3c2_0422_cpu *cpu)
{
    uint16_t low = cpu->ax & 0xFFu;
    cpu->ax = (uint16_t)((low & 0x80u) != 0u ? low | 0xFF00u : low);
}

static int less(const pr_u3c2_0422_cpu *cpu)
{
    return ((cpu->flags >> 7) & 1u) != ((cpu->flags >> 11) & 1u);
}

static int equal(const pr_u3c2_0422_cpu *cpu)
{
    return (cpu->flags & 0x40u) != 0u;
}

static uint16_t pop16(const uint8_t *ram, pr_u3c2_0422_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void push16(uint8_t *ram, pr_u3c2_0422_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void les(const uint8_t *ram, pr_u3c2_0422_cpu *cpu, uint16_t offset)
{
    size_t a = physical(cpu->ss, offset);
    cpu->di = (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
    cpu->es = (uint16_t)(ram[(a + 2u) & RAM_MASK] |
                         ((uint16_t)ram[(a + 3u) & RAM_MASK] << 8));
}

static uint16_t shift1(pr_u3c2_0422_cpu *cpu, uint16_t old)
{
    uint16_t result = (uint16_t)((uint32_t)old << 1);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if ((old & 0x8000u) != 0u) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & 0x8000u) != 0u) cpu->flags |= 0x80u;
    if (((old ^ result) & 0x8000u) != 0u) cpu->flags |= 0x800u;
    /* SHL AF is undefined; choose zero without a native AF claim. */
    return result;
}

static void increment_ax(pr_u3c2_0422_cpu *cpu)
{
    uint16_t carry = cpu->flags & 1u;
    cpu->ax = arithmetic(cpu, cpu->ax, 1u, 0x8000u, 0);
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~1u) | carry);
}

static void multiply_ax(pr_u3c2_0422_cpu *cpu, uint16_t right)
{
    uint32_t product = (uint32_t)cpu->ax * right;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    cpu->flags &= (uint16_t)~0x0801u;
    if (cpu->dx != 0u) cpu->flags |= 0x0801u;
    /* Other MUL status flags are undefined and retained by convention.
     * Later ADD/XOR/CMP replaces them before a callee boundary or return. */
}

static void logical8_flags(pr_u3c2_0422_cpu *cpu, uint8_t value)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity(value)) cpu->flags |= 4u;
    if (value == 0u) cpu->flags |= 0x40u;
    if ((value & 0x80u) != 0u) cpu->flags |= 0x80u;
    /* Logical AF is undefined; model zero, including at hook/return edges. */
}

static int near_call(uint8_t *ram, size_t ram_size, pr_u3c2_0422_cpu *cpu,
                     const pr_u3c2_0422_hooks *hooks, uint16_t target,
                     uint16_t continuation)
{
    uint16_t caller_cs = cpu->cs;
    pr_u3c2_0422_callee callee = NULL;
    if (hooks != NULL)
        callee = target == 0x01A0u ? hooks->entry2_01a0 : hooks->entry3_02cc;
    push16(ram, cpu, continuation);
    cpu->ip = target;
    if (callee == NULL)
        return 0;
    if (callee(hooks->user, ram, ram_size, cpu) != PR_U3C2_0422_CALLEE_RETURNED)
        return 0;
    return cpu->cs == caller_cs && cpu->ip == continuation;
}


pr_u3c2_0422_status pr_u3c2_entry5_0422(
    uint8_t *ram, size_t ram_size, pr_u3c2_0422_cpu *cpu,
    const pr_u3c2_0422_hooks *hooks)
{
    if (ram == NULL || cpu == NULL || ram_size < PR_U3C2_0422_RAM_SIZE ||
        cpu->ip != 0x0422u)
        return PR_U3C2_0422_ARGUMENT;

    push16(ram, cpu, cpu->bp); /* +0422 */
    cpu->bp = cpu->sp; /* +0423 */
    cpu->sp = arithmetic(cpu, cpu->sp, 0x3au, 0x8000u, 1); /* +0425 */
    les(ram, cpu, (uint16_t)(cpu->bp + 0xau)); /* +0428 */
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di)); /* +042B */
    cpu->ax = shift1(cpu, cpu->ax); /* +042E */
    increment_ax(cpu); /* +0430 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x39u), cpu->ax); /* +0431 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x18u)))); /* +0434 */
    cbw(cpu); /* +0437 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1bu), cpu->ax); /* +0438 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x16u)))); /* +043B */
    cbw(cpu); /* +043E */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x19u), cpu->ax); /* +043F */
    les(ram, cpu, (uint16_t)(cpu->bp + 0x12u)); /* +0442 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di)))); /* +0445 */
    cbw(cpu); /* +0448 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x17u), cpu->ax); /* +0449 */
    les(ram, cpu, (uint16_t)(cpu->bp + 0xeu)); /* +044C */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di)))); /* +044F */
    cbw(cpu); /* +0452 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x15u), cpu->ax); /* +0453 */
    cpu->di = (uint16_t)(cpu->bp - 0x1bu); /* +0456 */
    push16(ram, cpu, cpu->ss); /* +0459 */
    push16(ram, cpu, cpu->di); /* +045A */
    push16(ram, cpu, cpu->cs); /* +045B */
    if (!near_call(ram, ram_size, cpu, hooks, 0x1a0u, 0x45fu)) /* +045C */
        return PR_U3C2_0422_CALLEE;
    cpu->ax = 0u; /* +045F */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x35u), cpu->ax); /* +0461 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x18u)))); /* +0464 */
    cbw(cpu); /* +0467 */
    cpu->cx = cpu->ax; /* +0468 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x16u)))); /* +046A */
    cbw(cpu); /* +046D */
    cpu->dx = 0x32u; /* +046E */
    multiply_ax(cpu, cpu->dx); /* +0471 */
    les(ram, cpu, (uint16_t)(cpu->bp + 0x6u)); /* +0473 */
    cpu->di = arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0); /* +0476 */
    cpu->di = arithmetic(cpu, cpu->di, cpu->cx, 0x8000u, 0); /* +0478 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 0x7u)))); /* +047A */
    cpu->ax &= 0x00FFu; /* +047E */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    cpu->di = cpu->ax; /* +0480 */
    cpu->di = shift1(cpu, cpu->di); /* +0482 */
    cpu->di = shift1(cpu, cpu->di); /* +0484 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x2745u)))); /* +0486 */
    cpu->ax &= 0x00FFu; /* +048A */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x33u), cpu->ax); /* +048C */
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x11u)); /* +048F */
    cmp16(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0xfu))); /* +0492 */
    if (less(cpu) || equal(cpu)) goto L049F; /* +0495 */
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x11u)); /* +0497 */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x31u), cpu->ax); /* +049A */
    goto L04A5; /* +049D */
L049F:
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0xfu)); /* +049F */
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x31u), cpu->ax); /* +04A2 */
L04A5:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x18u)))); /* +04A5 */
    cbw(cpu); /* +04A8 */
    cpu->cx = cpu->ax; /* +04A9 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x16u)))); /* +04AB */
    cbw(cpu); /* +04AE */
    cpu->dx = 0x32u; /* +04AF */
    multiply_ax(cpu, cpu->dx); /* +04B2 */
    les(ram, cpu, (uint16_t)(cpu->bp + 0x6u)); /* +04B4 */
    cpu->di = arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0); /* +04B7 */
    cpu->di = arithmetic(cpu, cpu->di, cpu->cx, 0x8000u, 0); /* +04B9 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 0x7u)))); /* +04BB */
    cpu->ax &= 0x00FFu; /* +04BF */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    cpu->di = cpu->ax; /* +04C1 */
    cpu->di = shift1(cpu, cpu->di); /* +04C3 */
    cpu->di = shift1(cpu, cpu->di); /* +04C5 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x2745u)))); /* +04C7 */
    cpu->ax &= 0x00FFu; /* +04CB */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2fu), cpu->ax); /* +04CD */
    cpu->di = (uint16_t)(cpu->bp - 0x35u); /* +04D0 */
    push16(ram, cpu, cpu->ss); /* +04D3 */
    push16(ram, cpu, cpu->di); /* +04D4 */
    push16(ram, cpu, cpu->cs); /* +04D5 */
    if (!near_call(ram, ram_size, cpu, hooks, 0x1a0u, 0x4d9u)) /* +04D6 */
        return PR_U3C2_0422_CALLEE;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x36u), 0x0u); /* +04D9 */
L04DD:
    les(ram, cpu, (uint16_t)(cpu->bp + 0x6u)); /* +04DD */
    cmp8(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x6u)), 0x0u); /* +04E0 */
    if (!equal(cpu)) goto L050E; /* +04E5 */
    cpu->cx = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x7u)); /* +04E7 */
    cpu->ax = 0x32u; /* +04EA */
    multiply_ax(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u))); /* +04ED */
    les(ram, cpu, (uint16_t)(cpu->bp + 0x6u)); /* +04F0 */
    cpu->di = arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0); /* +04F3 */
    cpu->di = arithmetic(cpu, cpu->di, cpu->cx, 0x8000u, 0); /* +04F5 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 0x7u)))); /* +04F7 */
    cpu->ax &= 0x00FFu; /* +04FB */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    cpu->di = cpu->ax; /* +04FD */
    cpu->di = shift1(cpu, cpu->di); /* +04FF */
    cpu->di = shift1(cpu, cpu->di); /* +0501 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x2746u)))); /* +0503 */
    cpu->ax &= 0x00FFu; /* +0507 */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    cmp16(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1fu))); /* +0509 */
    if (!less(cpu) && !equal(cpu)) goto L0518; /* +050C */
L050E:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u)))); /* +050E */
    cpu->ax &= 0x00FFu; /* +0511 */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    cmp16(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x39u))); /* +0513 */
    if ((cpu->flags & 0x41u) != 0u) goto L053B; /* +0516 */
L0518:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x7u)))); /* +0518 */
    les(ram, cpu, (uint16_t)(cpu->bp + 0x12u)); /* +051B */
    write8(ram, cpu->es, (uint16_t)(cpu->di), (uint8_t)cpu->ax); /* +051E */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u)))); /* +0521 */
    les(ram, cpu, (uint16_t)(cpu->bp + 0xeu)); /* +0524 */
    write8(ram, cpu->es, (uint16_t)(cpu->di), (uint8_t)cpu->ax); /* +0527 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u)))); /* +052A */
    cpu->ax &= 0x00FFu; /* +052D */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    les(ram, cpu, (uint16_t)(cpu->bp + 0xau)); /* +052F */
    write16(ram, cpu->es, (uint16_t)(cpu->di), cpu->ax); /* +0532 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u), 0x0u); /* +0535 */
    goto L0572; /* +0539 */
L053B:
    cpu->di = (uint16_t)(cpu->bp - 0x35u); /* +053B */
    push16(ram, cpu, cpu->ss); /* +053E */
    push16(ram, cpu, cpu->di); /* +053F */
    push16(ram, cpu, cpu->cs); /* +0540 */
    if (!near_call(ram, ram_size, cpu, hooks, 0x2ccu, 0x544u)) /* +0541 */
        return PR_U3C2_0422_CALLEE;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x37u), (uint8_t)cpu->ax); /* +0544 */
    cpu->di = (uint16_t)(cpu->bp - 0x1bu); /* +0547 */
    push16(ram, cpu, cpu->ss); /* +054A */
    push16(ram, cpu, cpu->di); /* +054B */
    push16(ram, cpu, cpu->cs); /* +054C */
    if (!near_call(ram, ram_size, cpu, hooks, 0x2ccu, 0x550u)) /* +054D */
        return PR_U3C2_0422_CALLEE;
    logical8_flags(cpu, (uint8_t)cpu->ax); /* +0550 */
    set_al(cpu, (uint8_t)(0x0u)); /* +0552 */
    if (!equal(cpu)) goto L0557; /* +0554 */
    increment_ax(cpu); /* +0556 */
L0557:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x36u), (uint8_t)cpu->ax); /* +0557 */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x36u)), 0x0u); /* +055A */
    if (!equal(cpu)) goto L0563; /* +055E */
    goto L04DD; /* +0560 */
L0563:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u)))); /* +0563 */
    cpu->ax &= 0x00FFu; /* +0566 */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u);
    les(ram, cpu, (uint16_t)(cpu->bp + 0xau)); /* +0568 */
    write16(ram, cpu->es, (uint16_t)(cpu->di), cpu->ax); /* +056B */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u), 0x1u); /* +056E */
L0572:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u)))); /* +0572 */
    cpu->sp = cpu->bp; /* +0575 */
    cpu->bp = pop16(ram, cpu); /* +0577 */
    cpu->ip = pop16(ram, cpu); /* +0578 */
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 20u);
    return PR_U3C2_0422_OK;
}

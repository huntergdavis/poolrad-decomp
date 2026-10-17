/* U3C2 entry 4: exact shared-state branch transcription; see U3C2_ENTRY4_057B.md. */
#include "poolrad/recovered/overlay_u3c2_entry4_057b.h"

#define RAM_MASK (PR_U3C2_057B_RAM_SIZE - 1u)
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

static uint16_t arithmetic(pr_u3c2_057b_cpu *cpu, uint16_t left,
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

static void cmp8(pr_u3c2_057b_cpu *cpu, uint8_t left, uint8_t right)
{
    (void)arithmetic(cpu, left, right, 0x80u, 1);
}

static void cmp16(pr_u3c2_057b_cpu *cpu, uint16_t left, uint16_t right)
{
    (void)arithmetic(cpu, left, right, 0x8000u, 1);
}

static void set_al(pr_u3c2_057b_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u3c2_057b_cpu *cpu)
{
    uint16_t low = cpu->ax & 0xFFu;
    cpu->ax = (uint16_t)((low & 0x80u) != 0u ? low | 0xFF00u : low);
}

static int less(const pr_u3c2_057b_cpu *cpu)
{
    return ((cpu->flags >> 7) & 1u) != ((cpu->flags >> 11) & 1u);
}

static int equal(const pr_u3c2_057b_cpu *cpu)
{
    return (cpu->flags & 0x40u) != 0u;
}

static uint16_t pop16(const uint8_t *ram, pr_u3c2_057b_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void push16(uint8_t *ram, pr_u3c2_057b_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}


pr_u3c2_057b_status pr_u3c2_entry4_057b(
    uint8_t *ram, size_t ram_size, pr_u3c2_057b_cpu *cpu)
{
    if (ram == NULL || cpu == NULL || ram_size < PR_U3C2_057B_RAM_SIZE ||
        cpu->ip != 0x057Bu)
        return PR_U3C2_057B_REJECT;

    push16(ram, cpu, cpu->bp); /* +057B */
    cpu->bp = cpu->sp; /* +057C */
    cpu->sp = arithmetic(cpu, cpu->sp, 4u, 0x8000u, 1); /* +057E */
    /* Bounds precede direction normalization and all displacement reads. */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xe)), 0u); /* +0581 */
    if (less(cpu)) goto L_05b1; /* +0585 */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xe)), 0x31u); /* +0587 */
    if (!less(cpu) && !equal(cpu)) goto L_05b1; /* +058B */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xc)), 0u); /* +058D */
    if (less(cpu)) goto L_05b1; /* +0591 */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xc)), 0x18u); /* +0593 */
    if (!less(cpu) && !equal(cpu)) goto L_05b1; /* +0597 */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa)), 0u); /* +0599 */
    if (less(cpu)) goto L_05b1; /* +059D */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa)), 0x31u); /* +059F */
    if (!less(cpu) && !equal(cpu)) goto L_05b1; /* +05A3 */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)), 0u); /* +05A5 */
    if (less(cpu)) goto L_05b1; /* +05A9 */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)), 0x18u); /* +05AB */
    if (less(cpu) || equal(cpu)) goto L_05b8; /* +05AF */

L_05b1:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), 0u); /* +05B1 */
    goto L_0900; /* +05B5 */

L_05b8:
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0xffu); /* +05B8 */
    if (!equal(cpu)) goto L_05c2; /* +05BC */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp + 6), 8u); /* +05BE */

L_05c2:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6))); /* +05C2 */
    cpu->ax &= 0x00FFu;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u); /* +05C5 */
    cpu->di = cpu->ax; /* +05C7 */
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x2736))); /* +05C9 */
    cbw(cpu); /* +05CD */
    cpu->dx = cpu->ax; /* +05CE */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xe))); /* +05D0 */
    cbw(cpu); /* +05D3 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +05D4 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint8_t)cpu->ax); /* +05D6 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6))); /* +05D9 */
    cpu->ax &= 0x00FFu;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x44u); /* +05DC */
    cpu->di = cpu->ax; /* +05DE */
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + 0x273f))); /* +05E0 */
    cbw(cpu); /* +05E4 */
    cpu->dx = cpu->ax; /* +05E5 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xc))); /* +05E7 */
    cbw(cpu); /* +05EA */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +05EB */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 3), (uint8_t)cpu->ax); /* +05ED */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xe))); /* +05F0 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +05F3 */
    if (!equal(cpu)) goto L_0600; /* +05F6 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xc))); /* +05F8 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +05FB */
    if (equal(cpu)) goto L_0610; /* +05FE */

L_0600:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0600 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +0603 */
    if (!equal(cpu)) goto L_0617; /* +0606 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0608 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +060B */
    if (!equal(cpu)) goto L_0617; /* +060E */

L_0610:
    /* Original or displaced origin equals target. */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), 1u); /* +0610 */
    goto L_0900; /* +0614 */

L_0617:
    /* Direction 0: signed comparisons retain the original arithmetic order. */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6))); /* +0617 */
    cmp8(cpu, (uint8_t)cpu->ax, 0u); /* +061A */
    if (!equal(cpu)) goto L_0676; /* +061C */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +061E */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0621 */
    if (less(cpu)) goto L_0644; /* +0624 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0626 */
    cbw(cpu); /* +0629 */
    cpu->cx = cpu->ax; /* +062A */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +062C */
    cbw(cpu); /* +062F */
    cpu->dx = cpu->ax; /* +0630 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0632 */
    cbw(cpu); /* +0635 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 1); /* +0636 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 0); /* +0638 */
    cpu->dx = cpu->ax; /* +063A */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +063C */
    cbw(cpu); /* +063F */
    cmp16(cpu, cpu->ax, cpu->dx); /* +0640 */
    if (less(cpu) || equal(cpu)) goto L_066e; /* +0642 */

L_0644:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +0644 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0647 */
    if (!less(cpu) && !equal(cpu)) goto L_066a; /* +064A */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +064C */
    cbw(cpu); /* +064F */
    cpu->cx = cpu->ax; /* +0650 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0652 */
    cbw(cpu); /* +0655 */
    cpu->dx = cpu->ax; /* +0656 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +0658 */
    cbw(cpu); /* +065B */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 1); /* +065C */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 0); /* +065E */
    cpu->dx = cpu->ax; /* +0660 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0662 */
    cbw(cpu); /* +0665 */
    cmp16(cpu, cpu->ax, cpu->dx); /* +0666 */
    if (less(cpu) || equal(cpu)) goto L_066e; /* +0668 */

L_066a:
    set_al(cpu, 0u); /* +066A */
    goto L_0670; /* +066C */

L_066e:
    set_al(cpu, 1u); /* +066E */

L_0670:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)cpu->ax); /* +0670 */
    goto L_0900; /* +0673 */

L_0676:
    /* Direction 1: signed comparisons retain the original arithmetic order. */
    cmp8(cpu, (uint8_t)cpu->ax, 1u); /* +0676 */
    if (!equal(cpu)) goto L_06d2; /* +0678 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +067A */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +067D */
    if (less(cpu)) goto L_06a0; /* +0680 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0682 */
    cbw(cpu); /* +0685 */
    cpu->cx = cpu->ax; /* +0686 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +0688 */
    cbw(cpu); /* +068B */
    cpu->dx = cpu->ax; /* +068C */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +068E */
    cbw(cpu); /* +0691 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 1); /* +0692 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 0); /* +0694 */
    cpu->dx = cpu->ax; /* +0696 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0698 */
    cbw(cpu); /* +069B */
    cmp16(cpu, cpu->ax, cpu->dx); /* +069C */
    if (less(cpu) || equal(cpu)) goto L_06ca; /* +069E */

L_06a0:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +06A0 */
    cbw(cpu); /* +06A3 */
    cpu->cx = cpu->ax; /* +06A4 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +06A6 */
    cbw(cpu); /* +06A9 */
    cpu->dx = cpu->ax; /* +06AA */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +06AC */
    cbw(cpu); /* +06AF */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +06B0 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 1); /* +06B2 */
    cpu->dx = cpu->ax; /* +06B4 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +06B6 */
    cbw(cpu); /* +06B9 */
    cmp16(cpu, cpu->ax, cpu->dx); /* +06BA */
    if (less(cpu)) goto L_06c6; /* +06BC */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +06BE */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +06C1 */
    if (less(cpu) || equal(cpu)) goto L_06ca; /* +06C4 */

L_06c6:
    set_al(cpu, 0u); /* +06C6 */
    goto L_06cc; /* +06C8 */

L_06ca:
    set_al(cpu, 1u); /* +06CA */

L_06cc:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)cpu->ax); /* +06CC */
    goto L_0900; /* +06CF */

L_06d2:
    /* Direction 2: signed comparisons retain the original arithmetic order. */
    cmp8(cpu, (uint8_t)cpu->ax, 2u); /* +06D2 */
    if (!equal(cpu)) goto L_072e; /* +06D4 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +06D6 */
    cbw(cpu); /* +06D9 */
    cpu->cx = cpu->ax; /* +06DA */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +06DC */
    cbw(cpu); /* +06DF */
    cpu->dx = cpu->ax; /* +06E0 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +06E2 */
    cbw(cpu); /* +06E5 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +06E6 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 1); /* +06E8 */
    cpu->dx = cpu->ax; /* +06EA */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +06EC */
    cbw(cpu); /* +06EF */
    cmp16(cpu, cpu->ax, cpu->dx); /* +06F0 */
    if (less(cpu)) goto L_06fc; /* +06F2 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +06F4 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +06F7 */
    if (less(cpu) || equal(cpu)) goto L_0726; /* +06FA */

L_06fc:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +06FC */
    cbw(cpu); /* +06FF */
    cpu->cx = cpu->ax; /* +0700 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0702 */
    cbw(cpu); /* +0705 */
    cpu->dx = cpu->ax; /* +0706 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0708 */
    cbw(cpu); /* +070B */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +070C */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 1); /* +070E */
    cpu->dx = cpu->ax; /* +0710 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +0712 */
    cbw(cpu); /* +0715 */
    cmp16(cpu, cpu->ax, cpu->dx); /* +0716 */
    if (less(cpu)) goto L_0722; /* +0718 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +071A */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +071D */
    if (!less(cpu)) goto L_0726; /* +0720 */

L_0722:
    set_al(cpu, 0u); /* +0722 */
    goto L_0728; /* +0724 */

L_0726:
    set_al(cpu, 1u); /* +0726 */

L_0728:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)cpu->ax); /* +0728 */
    goto L_0900; /* +072B */

L_072e:
    /* Direction 3: signed comparisons retain the original arithmetic order. */
    cmp8(cpu, (uint8_t)cpu->ax, 3u); /* +072E */
    if (!equal(cpu)) goto L_078a; /* +0730 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0732 */
    cbw(cpu); /* +0735 */
    cpu->cx = cpu->ax; /* +0736 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0738 */
    cbw(cpu); /* +073B */
    cpu->dx = cpu->ax; /* +073C */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +073E */
    cbw(cpu); /* +0741 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +0742 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 1); /* +0744 */
    cpu->dx = cpu->ax; /* +0746 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +0748 */
    cbw(cpu); /* +074B */
    cmp16(cpu, cpu->ax, cpu->dx); /* +074C */
    if (less(cpu)) goto L_0758; /* +074E */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0750 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0753 */
    if (!less(cpu)) goto L_0782; /* +0756 */

L_0758:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +0758 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +075B */
    if (less(cpu)) goto L_077e; /* +075E */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0760 */
    cbw(cpu); /* +0763 */
    cpu->cx = cpu->ax; /* +0764 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0766 */
    cbw(cpu); /* +0769 */
    cpu->dx = cpu->ax; /* +076A */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +076C */
    cbw(cpu); /* +076F */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 1); /* +0770 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 0); /* +0772 */
    cpu->dx = cpu->ax; /* +0774 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0776 */
    cbw(cpu); /* +0779 */
    cmp16(cpu, cpu->ax, cpu->dx); /* +077A */
    if (!less(cpu)) goto L_0782; /* +077C */

L_077e:
    set_al(cpu, 0u); /* +077E */
    goto L_0784; /* +0780 */

L_0782:
    set_al(cpu, 1u); /* +0782 */

L_0784:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)cpu->ax); /* +0784 */
    goto L_0900; /* +0787 */

L_078a:
    /* Direction 4: signed comparisons retain the original arithmetic order. */
    cmp8(cpu, (uint8_t)cpu->ax, 4u); /* +078A */
    if (!equal(cpu)) goto L_07e6; /* +078C */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +078E */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0791 */
    if (less(cpu)) goto L_07b4; /* +0794 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0796 */
    cbw(cpu); /* +0799 */
    cpu->cx = cpu->ax; /* +079A */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +079C */
    cbw(cpu); /* +079F */
    cpu->dx = cpu->ax; /* +07A0 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +07A2 */
    cbw(cpu); /* +07A5 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 1); /* +07A6 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 0); /* +07A8 */
    cpu->dx = cpu->ax; /* +07AA */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +07AC */
    cbw(cpu); /* +07AF */
    cmp16(cpu, cpu->ax, cpu->dx); /* +07B0 */
    if (!less(cpu)) goto L_07de; /* +07B2 */

L_07b4:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +07B4 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +07B7 */
    if (!less(cpu) && !equal(cpu)) goto L_07da; /* +07BA */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +07BC */
    cbw(cpu); /* +07BF */
    cpu->cx = cpu->ax; /* +07C0 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +07C2 */
    cbw(cpu); /* +07C5 */
    cpu->dx = cpu->ax; /* +07C6 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +07C8 */
    cbw(cpu); /* +07CB */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 1); /* +07CC */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 0); /* +07CE */
    cpu->dx = cpu->ax; /* +07D0 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +07D2 */
    cbw(cpu); /* +07D5 */
    cmp16(cpu, cpu->ax, cpu->dx); /* +07D6 */
    if (!less(cpu)) goto L_07de; /* +07D8 */

L_07da:
    set_al(cpu, 0u); /* +07DA */
    goto L_07e0; /* +07DC */

L_07de:
    set_al(cpu, 1u); /* +07DE */

L_07e0:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)cpu->ax); /* +07E0 */
    goto L_0900; /* +07E3 */

L_07e6:
    /* Direction 5: signed comparisons retain the original arithmetic order. */
    cmp8(cpu, (uint8_t)cpu->ax, 5u); /* +07E6 */
    if (!equal(cpu)) goto L_0842; /* +07E8 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +07EA */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +07ED */
    if (!less(cpu) && !equal(cpu)) goto L_0810; /* +07F0 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +07F2 */
    cbw(cpu); /* +07F5 */
    cpu->cx = cpu->ax; /* +07F6 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +07F8 */
    cbw(cpu); /* +07FB */
    cpu->dx = cpu->ax; /* +07FC */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +07FE */
    cbw(cpu); /* +0801 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 1); /* +0802 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 0); /* +0804 */
    cpu->dx = cpu->ax; /* +0806 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0808 */
    cbw(cpu); /* +080B */
    cmp16(cpu, cpu->ax, cpu->dx); /* +080C */
    if (!less(cpu)) goto L_083a; /* +080E */

L_0810:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0810 */
    cbw(cpu); /* +0813 */
    cpu->cx = cpu->ax; /* +0814 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0816 */
    cbw(cpu); /* +0819 */
    cpu->dx = cpu->ax; /* +081A */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +081C */
    cbw(cpu); /* +081F */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +0820 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 1); /* +0822 */
    cpu->dx = cpu->ax; /* +0824 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +0826 */
    cbw(cpu); /* +0829 */
    cmp16(cpu, cpu->ax, cpu->dx); /* +082A */
    if (!less(cpu) && !equal(cpu)) goto L_0836; /* +082C */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +082E */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0831 */
    if (!less(cpu)) goto L_083a; /* +0834 */

L_0836:
    set_al(cpu, 0u); /* +0836 */
    goto L_083c; /* +0838 */

L_083a:
    set_al(cpu, 1u); /* +083A */

L_083c:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)cpu->ax); /* +083C */
    goto L_0900; /* +083F */

L_0842:
    /* Direction 6: signed comparisons retain the original arithmetic order. */
    cmp8(cpu, (uint8_t)cpu->ax, 6u); /* +0842 */
    if (!equal(cpu)) goto L_089d; /* +0844 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0846 */
    cbw(cpu); /* +0849 */
    cpu->cx = cpu->ax; /* +084A */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +084C */
    cbw(cpu); /* +084F */
    cpu->dx = cpu->ax; /* +0850 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0852 */
    cbw(cpu); /* +0855 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +0856 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 1); /* +0858 */
    cpu->dx = cpu->ax; /* +085A */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +085C */
    cbw(cpu); /* +085F */
    cmp16(cpu, cpu->ax, cpu->dx); /* +0860 */
    if (!less(cpu) && !equal(cpu)) goto L_086c; /* +0862 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0864 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +0867 */
    if (!less(cpu)) goto L_0896; /* +086A */

L_086c:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +086C */
    cbw(cpu); /* +086F */
    cpu->cx = cpu->ax; /* +0870 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +0872 */
    cbw(cpu); /* +0875 */
    cpu->dx = cpu->ax; /* +0876 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +0878 */
    cbw(cpu); /* +087B */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +087C */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 1); /* +087E */
    cpu->dx = cpu->ax; /* +0880 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +0882 */
    cbw(cpu); /* +0885 */
    cmp16(cpu, cpu->ax, cpu->dx); /* +0886 */
    if (!less(cpu) && !equal(cpu)) goto L_0892; /* +0888 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +088A */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +088D */
    if (less(cpu) || equal(cpu)) goto L_0896; /* +0890 */

L_0892:
    set_al(cpu, 0u); /* +0892 */
    goto L_0898; /* +0894 */

L_0896:
    set_al(cpu, 1u); /* +0896 */

L_0898:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)cpu->ax); /* +0898 */
    goto L_0900; /* +089B */

L_089d:
    /* Direction 7: signed comparisons retain the original arithmetic order. */
    cmp8(cpu, (uint8_t)cpu->ax, 7u); /* +089D */
    if (!equal(cpu)) goto L_08f8; /* +089F */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +08A1 */
    cbw(cpu); /* +08A4 */
    cpu->cx = cpu->ax; /* +08A5 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +08A7 */
    cbw(cpu); /* +08AA */
    cpu->dx = cpu->ax; /* +08AB */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +08AD */
    cbw(cpu); /* +08B0 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0); /* +08B1 */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 1); /* +08B3 */
    cpu->dx = cpu->ax; /* +08B5 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +08B7 */
    cbw(cpu); /* +08BA */
    cmp16(cpu, cpu->ax, cpu->dx); /* +08BB */
    if (!less(cpu) && !equal(cpu)) goto L_08c7; /* +08BD */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +08BF */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +08C2 */
    if (less(cpu) || equal(cpu)) goto L_08f1; /* +08C5 */

L_08c7:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +08C7 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +08CA */
    if (!less(cpu) && !equal(cpu)) goto L_08ed; /* +08CD */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3))); /* +08CF */
    cbw(cpu); /* +08D2 */
    cpu->cx = cpu->ax; /* +08D3 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2))); /* +08D5 */
    cbw(cpu); /* +08D8 */
    cpu->dx = cpu->ax; /* +08D9 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0xa))); /* +08DB */
    cbw(cpu); /* +08DE */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 1); /* +08DF */
    cpu->ax = arithmetic(cpu, cpu->ax, cpu->cx, 0x8000u, 0); /* +08E1 */
    cpu->dx = cpu->ax; /* +08E3 */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +08E5 */
    cbw(cpu); /* +08E8 */
    cmp16(cpu, cpu->ax, cpu->dx); /* +08E9 */
    if (less(cpu) || equal(cpu)) goto L_08f1; /* +08EB */

L_08ed:
    set_al(cpu, 0u); /* +08ED */
    goto L_08f3; /* +08EF */

L_08f1:
    set_al(cpu, 1u); /* +08F1 */

L_08f3:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)cpu->ax); /* +08F3 */
    goto L_0900; /* +08F6 */

L_08f8:
    /* Mode 8 succeeds; 9..254 deliberately leave the result local untouched. */
    cmp8(cpu, (uint8_t)cpu->ax, 8u); /* +08F8 */
    if (!equal(cpu)) goto L_0900; /* +08FA */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), 1u); /* +08FC */

L_0900:
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1))); /* +0900 */
    cpu->sp = cpu->bp; /* +0903 */
    cpu->bp = pop16(ram, cpu); /* +0905 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U3C2_057B_OK; /* +0906 */
}

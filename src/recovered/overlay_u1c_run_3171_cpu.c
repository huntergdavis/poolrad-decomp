/* Candidate-scout draft: GAME.OVR unit 0x1C +3171, the ECL run loop.
 * See include/poolrad/recovered/overlay_u1c_run_3171_cpu.h. No credit claimed. */
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"

#define MASK 0xFFFFu
#define FLAG_MASK 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint8_t read8(uint8_t *ram, size_t ram_size, uint16_t segment, uint16_t offset)
{
    (void)ram_size;
    return ram[physical(segment, offset)];
}

static uint16_t read16(uint8_t *ram, size_t ram_size, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    (void)ram_size;
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & 0xFFFFFu] << 8));
}

static void write8(uint8_t *ram, size_t ram_size, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    (void)ram_size;
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, size_t ram_size, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t a = physical(segment, offset);
    (void)ram_size;
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & 0xFFFFFu] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, size_t ram_size, pr_u1c_3171_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, ram_size, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(uint8_t *ram, size_t ram_size, pr_u1c_3171_cpu *cpu)
{
    uint16_t value = read16(ram, ram_size, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint16_t parity_even(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u ? 0x0004u : 0u;
}

static void add16_flags(pr_u1c_3171_cpu *cpu, uint16_t left, uint16_t right)
{
    uint32_t wide = (uint32_t)left + right;
    uint16_t result = (uint16_t)wide;
    uint16_t flags = parity_even((uint8_t)result);
    if (wide > 0xFFFFu) flags |= 0x0001u;
    if (((left ^ right ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (((~(left ^ right)) & (left ^ result) & 0x8000u) != 0u) flags |= 0x0800u;
    /* ADD DI,AX writes DI, not AX; this helper must only publish flags. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~FLAG_MASK) | flags);
}

static void sub8_flags(pr_u1c_3171_cpu *cpu, uint8_t left, uint8_t right)
{
    uint16_t result = (uint16_t)((uint16_t)(left - right) & 0x00FFu);
    uint16_t flags = parity_even((uint8_t)result);
    if (left < right) flags |= 0x0001u;
    if (((left ^ right ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x0080u) != 0u) flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x0080u) != 0u) flags |= 0x0800u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~FLAG_MASK) | flags);
}

int pr_u1c_3171_dispatch_double(uint8_t *ram, size_t ram_size,
                                pr_u1c_3171_cpu *cpu, void *context)
{
    uint8_t opcode;
    (void)context;
    /* Audit observation, not body behaviour: the ADD DI,AX flags are live when the
     * dispatcher is entered (nothing between ADD and the far call changes them). Record
     * them so the differential can see them; the original side records the same word. */
    {
        /* Audit observation, not body behaviour: record the ORDERED per-dispatch
         * arithmetic-FLAGS word in 64 slots (0F00h..0F7Fh) plus the observation count
         * at 0F80h. Both sides run this same double, so a change on any dispatch -- and
         * the sequence length -- is compared directly. A rolling hash was removed: its
         * 16-bit arithmetic admitted a demonstrated modular cancellation. */
        uint16_t observed = (uint16_t)(cpu->flags & 0x08D5u);
        uint16_t n = read16(ram, ram_size, cpu->ds, 0x0F80u);
        if (n >= 64u) {
            return 3;   /* observation overflow: refuse the case, never drop silently */
        }
        write16(ram, ram_size, cpu->ds, (uint16_t)(0x0F00u + 2u * n), observed);
        write16(ram, ram_size, cpu->ds, 0x0F80u, (uint16_t)(n + 1u));
    }
    opcode = read8(ram, ram_size, cpu->ds, 0x6F7Fu);
    if (opcode == 0x00u) {
        write8(ram, ram_size, cpu->ds, 0x442Eu, 1u);
    } else if (opcode == 0xFFu) {
        write8(ram, ram_size, cpu->ds, 0x49FFu, 1u);
    } else {
        uint16_t pc = read16(ram, ram_size, cpu->ds, 0x49EDu);
        write16(ram, ram_size, cpu->ds, 0x49EDu, (uint16_t)(pc + opcode));
    }
    return 0;
}

int pr_u1c_run_3171(uint8_t *ram, size_t ram_size, pr_u1c_3171_cpu *cpu,
                    uint16_t caller_cs, pr_u1c_3171_dispatch_fn dispatch,
                    void *context)
{
    uint16_t body_cs = cpu->cs;
    uint16_t entry;

    push16(ram, ram_size, cpu, cpu->bp);              /* 3171 push bp */
    cpu->bp = cpu->sp;                                /* 3172 mov bp,sp */
    entry = read16(ram, ram_size, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->ax = entry;                                   /* 3174 mov ax,[bp+6] */
    write16(ram, ram_size, cpu->ds, 0x49EDu, entry);   /* 3177 */
    write8(ram, ram_size, cpu->ds, 0x442Eu, 0u);      /* 317A */

    for (;;) {
        uint16_t pc, di, es, before;

        sub8_flags(cpu, read8(ram, ram_size, cpu->ds, 0x442Eu), 0u);   /* 317F cmp */
        if ((cpu->flags & 0x0040u) == 0u) break;                       /* 3184 jne exit */
        sub8_flags(cpu, read8(ram, ram_size, cpu->ds, 0x49FFu), 0u);   /* 3186 cmp */
        if ((cpu->flags & 0x0040u) == 0u) break;                       /* 318B jne exit */

        write8(ram, ram_size, cpu->ds, 0x6F7Eu,
               read8(ram, ram_size, cpu->ds, 0x6F7Fu));                /* 318D..3190 */
        pc = read16(ram, ram_size, cpu->ds, 0x49EDu);                  /* 3193 */
        cpu->ax = pc;                                                  /* 3193 mov ax,[0x49ED] */
        di = read16(ram, ram_size, cpu->ds, 0x49DEu);                  /* 3196 les di */
        es = read16(ram, ram_size, cpu->ds, 0x49E0u);                  /* 3196 les es */
        before = di;
        di = (uint16_t)(di + pc);                                      /* 319A add di,ax */
        add16_flags(cpu, before, pc);
        cpu->es = es;
        cpu->di = di;
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                             read8(ram, ram_size, es, (uint16_t)(di + 0x6700u))); /* 319C */
        write8(ram, ram_size, cpu->ds, 0x6F7Fu, (uint8_t)cpu->ax);     /* 31A1 */

        push16(ram, ram_size, cpu, body_cs);                           /* 31A4 push cs */
        push16(ram, ram_size, cpu, 0x31A8u);                           /* 31A5 call near */
        if (dispatch == NULL) return 1;
        if (dispatch(ram, ram_size, cpu, context) != 0) return 2;
        cpu->sp = (uint16_t)(cpu->sp + 4u);                            /* callee RETF */
        cpu->cs = body_cs;                                             /* 31A8 */
        cpu->ip = 0x31A8u;                                             /* 31A8 jmp loop */
    }

    write8(ram, ram_size, cpu->ds, 0x442Eu, 0u);      /* 31AA */
    cpu->bp = pop16(ram, ram_size, cpu);              /* 31AF pop bp */
    cpu->ip = pop16(ram, ram_size, cpu);              /* 31B0 retf 2 */
    cpu->cs = pop16(ram, ram_size, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    (void)caller_cs;
    return 0;
}

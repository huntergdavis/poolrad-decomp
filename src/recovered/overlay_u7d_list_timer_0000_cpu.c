/* GAME.OVR unit 0x7D +0000 (raw CPU, composing SYSTEM FillChar 16A8; stops at its U9E +0026 call);
 * docs/re/U7D_LIST_TIMER_0000_CPU.md. */
#include "poolrad/recovered/overlay_u7d_list_timer_0000_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#define MASK (PR_U7D_0000_RAM_SIZE - 1u)

#define COPY_CPU(to, from)                                                                                   \
    do {                                                                                                     \
        (to)->ax = (from)->ax; (to)->bx = (from)->bx; (to)->cx = (from)->cx; (to)->dx = (from)->dx;          \
        (to)->si = (from)->si; (to)->di = (from)->di; (to)->bp = (from)->bp; (to)->sp = (from)->sp;          \
        (to)->cs = (from)->cs; (to)->ip = (from)->ip; (to)->ds = (from)->ds; (to)->es = (from)->es;          \
        (to)->ss = (from)->ss; (to)->flags = (from)->flags;                                                  \
    } while (0)

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u7d_0000_cpu *cpu;
    unsigned max_loopbacks, loopbacks;
    pr_u7d_0000_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t rd8(const machine *m, uint16_t segment, uint16_t offset) { return m->ram[physical(segment, offset)]; }
static uint16_t rd16(const machine *m, uint16_t segment, uint16_t offset) { return read16_at(m->ram, physical(segment, offset)); }
static void wr8(machine *m, uint16_t segment, uint16_t offset, uint8_t value) { m->ram[physical(segment, offset)] = value; }

static void wr16(machine *m, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

/* [BP+displacement] in SS. */
static uint16_t local(const machine *m, int displacement) { return (uint16_t)(m->cpu->bp + (uint16_t)displacement); }
static uint8_t lrd8(const machine *m, int d) { return rd8(m, m->cpu->ss, local(m, d)); }
static uint16_t lrd16(const machine *m, int d) { return rd16(m, m->cpu->ss, local(m, d)); }
static void lwr8(machine *m, int d, uint8_t value) { wr8(m, m->cpu->ss, local(m, d), value); }
static void lwr16(machine *m, int d, uint16_t value) { wr16(m, m->cpu->ss, local(m, d), value); }

static void push16(machine *m, uint16_t value)
{
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    wr16(m, m->cpu->ss, m->cpu->sp, value);
}

/* LES DI,segment:[offset], the pair read physically contiguous. */
static void les(machine *m, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    m->cpu->di = read16_at(m->ram, at);
    m->cpu->es = read16_at(m->ram, (at + 2u) & MASK);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void szp(pr_u7d_0000_cpu *cpu, uint16_t result, uint16_t sign)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((result & (uint16_t)((sign << 1) - 1u)) == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

/* CMP/SUB a - b: the six arithmetic flags. */
static void sub_flags(pr_u7d_0000_cpu *cpu, uint16_t a, uint16_t b, uint16_t sign)
{
    uint16_t result = (uint16_t)((a - b) & (uint16_t)((sign << 1) - 1u));
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & sign) cpu->flags |= 0x0800u;
    szp(cpu, result, sign);
}

/* OR/XOR: CF = OF = AF = 0. */
static void logic_flags(pr_u7d_0000_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    szp(cpu, result, sign);
}

/* INC/DEC of a byte: CF kept. */
static void incdec8_flags(pr_u7d_0000_cpu *cpu, uint8_t before, uint8_t result, int increment)
{
    cpu->flags &= (uint16_t)~0x08D4u;
    if (increment ? before == 0x7Fu : before == 0x80u) cpu->flags |= 0x0800u;
    if (increment ? (result & 0x0Fu) == 0u : (before & 0x0Fu) == 0u) cpu->flags |= 0x0010u;
    szp(cpu, result, 0x80u);
}

static uint8_t inc8_local(machine *m, int d)
{
    uint8_t before = lrd8(m, d), result = (uint8_t)(before + 1u);
    lwr8(m, d, result);
    incdec8_flags(m->cpu, before, result, 1);
    return result;
}

/* MOV AL,byte; XOR AH,AH: AX := the byte, XOR's flags on AH = 0. */
static void load_al_zero_ah(machine *m, uint8_t value)
{
    m->cpu->ax = value;
    logic_flags(m->cpu, 0u, 0x80u);
}

static int loopback(machine *m, uint16_t target)
{
    if (m->loopbacks == m->max_loopbacks) {
        m->cpu->ip = target;
        m->stop = PR_U7D_0000_LIMIT;
        return 0;
    }
    ++m->loopbacks;
    return 1;
}

/* AX := [BP+low] | [BP+high] word pair test (MOV AX,..; OR AX,..). */
static uint16_t or_pair(machine *m, int low, int high)
{
    uint16_t value = (uint16_t)(lrd16(m, low) | lrd16(m, high));
    m->cpu->ax = value;
    logic_flags(m->cpu, value, 0x8000u);
    return value;
}

/* LES DI,[BP+pointer]; MOV AX,ES:[DI+low]; MOV DX,ES:[DI+low+2]; MOV [BP+into],AX; MOV [BP+into+2],DX. */
static void copy_link(machine *m, int pointer, uint16_t low, int into)
{
    pr_u7d_0000_cpu *cpu = m->cpu;
    les(m, cpu->ss, local(m, pointer));
    cpu->ax = rd16(m, cpu->es, (uint16_t)(cpu->di + low));
    cpu->dx = rd16(m, cpu->es, (uint16_t)(cpu->di + low + 2u));
    lwr16(m, into, cpu->ax);
    lwr16(m, into + 2, cpu->dx);
}

/* MOV AL,[BP-2]; XOR AH,AH; MOV DI,AX; MOV BYTE [DI+471Dh],value. */
static void set_flag(machine *m, uint8_t value)
{
    load_al_zero_ah(m, lrd8(m, -2));
    m->cpu->di = m->cpu->ax;
    wr8(m, m->cpu->ds, (uint16_t)(m->cpu->di + 0x471Du), value);
}

/* Compare the far pointer DX:AX with [BP+high]:[BP+low] as CMP DX; JNE; CMP AX: returns 1 when equal. */
static int same_pointer(machine *m, int low, int high)
{
    pr_u7d_0000_cpu *cpu = m->cpu;
    uint16_t word = lrd16(m, high);
    sub_flags(cpu, cpu->dx, word, 0x8000u);
    if (cpu->dx != word)
        return 0;
    word = lrd16(m, low);
    sub_flags(cpu, cpu->ax, word, 0x8000u);
    return cpu->ax == word;
}

static int fillchar(machine *m)
{
    pr_u7d_0000_cpu *cpu = m->cpu;
    pr_system_leaf_cpu state;
    uint16_t caller_cs = cpu->cs, segment;
    cpu->ip = 0x0019u;                                                    /* 0019 LCALL xxxx:16A8 */
    if (rd8(m, cpu->cs, 0x0019u) != 0x9Au || rd16(m, cpu->cs, 0x001Au) != 0x16A8u) {
        m->stop = PR_U7D_0000_CODE;
        return 0;
    }
    segment = rd16(m, cpu->cs, 0x001Cu);
    push16(m, cpu->cs);
    push16(m, 0x001Eu);
    cpu->cs = segment;
    cpu->ip = 0x16A8u;
    COPY_CPU(&state, cpu);
    if (pr_system_fillchar_16a8(m->ram, m->size, &state) != PR_SYSTEM_LEAF_RETURNED) {
        m->stop = PR_U7D_0000_CALLEE;
        return 0;
    }
    COPY_CPU(cpu, &state);
    if (cpu->ip != 0x001Eu || cpu->cs != caller_cs) {
        m->stop = PR_U7D_0000_TRANSFER;
        return 0;
    }
    return 1;
}

/* The member at [BP-9]: its list from the head to the tail found first; returns 0 at a stop. */
static int visit_member(machine *m)
{
    pr_u7d_0000_cpu *cpu = m->cpu;
    uint16_t word;
    set_flag(m, 0u);                                                      /* 00D6 the member's flag := 0 */
    copy_link(m, -9, 0x7Fu, -0x1B);                                       /* 00E2 cur := head */
    copy_link(m, -9, 0x7Fu, -0x0F);                                       /* 00F4 tail := head */
    copy_link(m, -9, 0x7Fu, -0x13);                                       /* 0106 x := head */
    copy_link(m, -0x13, 5u, -0x17);                                       /* 0118 next := x->link */
    les(m, cpu->ss, local(m, -9));                                        /* 0129 the head nil? */
    cpu->ax = (uint16_t)(rd16(m, cpu->es, (uint16_t)(cpu->di + 0x7Fu)) | rd16(m, cpu->es, (uint16_t)(cpu->di + 0x81u)));
    logic_flags(cpu, cpu->ax, 0x8000u);
    if (cpu->ax != 0u) {
        for (;;) {                                                        /* 0137 walk the tail to the last link */
            les(m, cpu->ss, local(m, -0x0F));
            cpu->ax = (uint16_t)(rd16(m, cpu->es, (uint16_t)(cpu->di + 5u)) | rd16(m, cpu->es, (uint16_t)(cpu->di + 7u)));
            logic_flags(cpu, cpu->ax, 0x8000u);
            if (cpu->ax == 0u)
                break;
            copy_link(m, -0x0F, 5u, -0x0F);                               /* 0144 tail := tail->link; JMP 0137 */
            if (!loopback(m, 0x0137u))
                return 0;
        }
    }
    lwr8(m, -0x0A, 0u);                                                   /* 0157 done := 0 */
    for (;;) {                                                            /* 015B while cur and not done */
        if (or_pair(m, -0x1B, -0x19) == 0u)
            break;
        word = lrd8(m, -0x0A);                                            /* 0166 CMP BYTE [BP-0Ah],0 */
        sub_flags(cpu, word, 0u, 0x80u);
        if (word != 0u)
            break;
        cpu->ax = lrd16(m, -0x1B);                                        /* 016F cur == tail: done := 1 */
        cpu->dx = lrd16(m, -0x19);
        if (same_pointer(m, -0x0F, -0x0D))
            lwr8(m, -0x0A, 1u);
        les(m, cpu->ss, local(m, -0x1B));                                 /* 0183 CMP WORD ES:[DI+1],0; JNE 01A1 */
        word = rd16(m, cpu->es, (uint16_t)(cpu->di + 1u));
        sub_flags(cpu, word, 0u, 0x8000u);
        if (word == 0u) {
            copy_link(m, -0x1B, 5u, -0x1B);                               /* 018D cur := cur->link */
        } else {
            load_al_zero_ah(m, lrd8(m, -3));                              /* 01A1 CMP step,ES:[DI+1]; JAE 01DA */
            les(m, cpu->ss, local(m, -0x1B));
            word = rd16(m, cpu->es, (uint16_t)(cpu->di + 1u));
            sub_flags(cpu, cpu->ax, word, 0x8000u);
            if (cpu->ax < word) {
                load_al_zero_ah(m, lrd8(m, -3));                          /* 01AF SUB ES:[DI+1],step; the flag := 1 */
                les(m, cpu->ss, local(m, -0x1B));
                word = rd16(m, cpu->es, (uint16_t)(cpu->di + 1u));
                sub_flags(cpu, word, cpu->ax, 0x8000u);
                wr16(m, cpu->es, (uint16_t)(cpu->di + 1u), (uint16_t)(word - cpu->ax));
                set_flag(m, 1u);
                copy_link(m, -0x1B, 5u, -0x1B);                           /* 01C7 cur := cur->link */
            } else {
                copy_link(m, -0x1B, 5u, -0x17);                           /* 01DA next := cur->link */
                push16(m, lrd16(m, -7));                                  /* 01EB PUSH member; PUSH byte +0; PUSH cur */
                push16(m, lrd16(m, -9));
                les(m, cpu->ss, local(m, -0x1B));
                cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | rd8(m, cpu->es, cpu->di));
                push16(m, cpu->ax);
                push16(m, lrd16(m, -0x19));
                push16(m, lrd16(m, -0x1B));
                cpu->ip = 0x01FEu;                                        /* 01FE LCALL 00B0:002A (U9E +0026) */
                m->stop = rd8(m, cpu->cs, 0x01FEu) == 0x9Au && rd16(m, cpu->cs, 0x01FFu) == 0x002Au ? PR_U7D_0000_PENDING : PR_U7D_0000_CODE;
                return 0;
            }
        }
        copy_link(m, -9, 0x7Fu, -0x13);                                   /* 0239 x := head */
        for (;;) {                                                        /* 024B until x is nil, cur, or before cur */
            if (or_pair(m, -0x13, -0x11) == 0u)
                break;
            cpu->ax = lrd16(m, -0x13);
            cpu->dx = lrd16(m, -0x11);
            if (same_pointer(m, -0x1B, -0x19))
                break;
            les(m, cpu->ss, local(m, -0x13));                             /* 0263 x->link == cur? */
            cpu->ax = rd16(m, cpu->es, (uint16_t)(cpu->di + 5u));
            cpu->dx = rd16(m, cpu->es, (uint16_t)(cpu->di + 7u));
            if (same_pointer(m, -0x1B, -0x19))
                break;
            copy_link(m, -0x13, 5u, -0x13);                               /* 0278 x := x->link; JMP 024B */
            if (!loopback(m, 0x024Bu))
                return 0;
        }
        if (!loopback(m, 0x015Bu))                                        /* 028B JMP 015B */
            return 0;
    }
    for (;;) {                                                            /* 028E the remaining nodes */
        if (or_pair(m, -0x1B, -0x19) == 0u)
            break;
        les(m, cpu->ss, local(m, -0x1B));                                 /* 0296 CMP WORD ES:[DI+1],0; JBE 02AC */
        word = rd16(m, cpu->es, (uint16_t)(cpu->di + 1u));
        sub_flags(cpu, word, 0u, 0x8000u);
        if (word != 0u)
            set_flag(m, 1u);                                              /* 02A0 the flag := 1 */
        copy_link(m, -0x1B, 5u, -0x1B);                                   /* 02AC cur := cur->link; JMP 028E */
        if (!loopback(m, 0x028Eu))
            return 0;
    }
    return 1;
}

pr_u7d_0000_status pr_u7d_list_timer_0000_cpu(uint8_t *ram, size_t ram_size, pr_u7d_0000_cpu *cpu, unsigned max_loopbacks)
{
    machine m;
    uint16_t word, product_low;
    uint32_t product;
    uint8_t byte, before;
    size_t at;
    if (ram == NULL || cpu == NULL || ram_size != PR_U7D_0000_RAM_SIZE || cpu->ip != 0x0000u)
        return PR_U7D_0000_ARGUMENT;
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.max_loopbacks = max_loopbacks; m.loopbacks = 0; m.stop = PR_U7D_0000_OK;
    push16(&m, cpu->bp);                                                  /* 0000 PUSH BP; MOV BP,SP; SUB SP,1Ch */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 0x1Cu, 0x8000u);
    cpu->sp = (uint16_t)(cpu->sp - 0x1Cu);
    byte = rd8(&m, cpu->ds, 0x49F3u);                                     /* 0006 CMP BYTE [49F3h],2; JE 0020 */
    sub_flags(cpu, byte, 2u, 0x80u);
    if (byte != 2u) {
        cpu->di = 0x471Eu;                                                /* 000D FillChar(DS:471E, 48h, 1) */
        push16(&m, cpu->ds);
        push16(&m, cpu->di);
        cpu->ax = 0x0048u;
        push16(&m, cpu->ax);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 1u);
        push16(&m, cpu->ax);
        if (!fillchar(&m))
            return m.stop;
    } else {
        lwr8(&m, -0x0B, 0u);                                              /* 0020 found := 0; i := 1 */
        lwr8(&m, -2, 1u);
        for (;;) {
            load_al_zero_ah(&m, lrd8(&m, -2));                            /* 0028 CMP BYTE [DI+471Dh],0 */
            cpu->di = cpu->ax;
            byte = rd8(&m, cpu->ds, (uint16_t)(cpu->di + 0x471Du));
            sub_flags(cpu, byte, 0u, 0x80u);
            if (byte != 0u)
                lwr8(&m, -0x0B, 1u);                                      /* 0036 found := 1 */
            (void)inc8_local(&m, -2);                                     /* 003A INC BYTE [BP-2] */
            byte = lrd8(&m, -0x0B);                                       /* 003D CMP BYTE [BP-0Bh],0; JNE 0053 */
            sub_flags(cpu, byte, 0u, 0x80u);
            if (byte != 0u)
                break;
            load_al_zero_ah(&m, lrd8(&m, -2));                            /* 0043 CMP i,[ [49D6h] + 67Ch ]; JBE 0028 */
            les(&m, cpu->ds, 0x49D6u);
            word = rd16(&m, cpu->es, (uint16_t)(cpu->di + 0x067Cu));
            sub_flags(cpu, cpu->ax, word, 0x8000u);
            if (cpu->ax > word)
                break;
            if (!loopback(&m, 0x0028u))
                return m.stop;
        }
        byte = lrd8(&m, -0x0B);                                           /* 0053 CMP BYTE [BP-0Bh],0; JNE 005C; JMP 02EC */
        sub_flags(cpu, byte, 0u, 0x80u);
        if (byte == 0u)
            goto leave;
    }
    load_al_zero_ah(&m, lrd8(&m, 8));                                     /* 005C amount := n; k := u */
    lwr16(&m, -5, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | lrd8(&m, 6));
    lwr8(&m, -1, (uint8_t)cpu->ax);
    for (;;) {
        byte = lrd8(&m, -1);                                              /* 006A CMP BYTE [BP-1],1; JLE 008E */
        sub_flags(cpu, byte, 1u, 0x80u);
        if ((int8_t)byte <= 1)
            break;
        before = lrd8(&m, -1);                                            /* 0070 MOV AL,[BP-1]; DEC AL; CBW */
        byte = (uint8_t)(before - 1u);
        incdec8_flags(cpu, before, byte, 0);
        cpu->ax = (uint16_t)(int16_t)(int8_t)byte;
        cpu->di = cpu->ax;                                                /* 0076 MOV DI,AX; SHL DI,1 */
        cpu->flags &= (uint16_t)~0x08D5u;
        if (cpu->di & 0x8000u) cpu->flags |= 0x0001u;
        if ((cpu->di ^ (uint16_t)(cpu->di << 1)) & 0x8000u) cpu->flags |= 0x0800u;
        cpu->di = (uint16_t)(cpu->di << 1);
        szp(cpu, cpu->di, 0x8000u);
        cpu->ax = rd16(&m, cpu->ds, (uint16_t)(cpu->di + 0x363Au));       /* 007A MOV AX,[DI+363Ah]; MUL WORD [BP-5] */
        product = (uint32_t)cpu->ax * lrd16(&m, -5);
        product_low = (uint16_t)product;
        cpu->ax = product_low;
        cpu->dx = (uint16_t)(product >> 16);
        cpu->flags &= (uint16_t)~0x0801u;
        if (cpu->dx != 0u) cpu->flags |= 0x0801u;
        lwr16(&m, -5, cpu->ax);                                           /* 0081 MOV [BP-5],AX */
        before = lrd8(&m, -1);                                            /* 0084 MOV AL,[BP-1]; DEC AL; MOV [BP-1],AL */
        byte = (uint8_t)(before - 1u);
        incdec8_flags(cpu, before, byte, 0);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | byte);
        lwr8(&m, -1, byte);
        if (!loopback(&m, 0x006Au))                                       /* 008C JMP 006A */
            return m.stop;
    }
    for (;;) {
        word = lrd16(&m, -5);                                             /* 008E CMP WORD [BP-5],0; JA 0097; JMP 02EC */
        sub_flags(cpu, word, 0u, 0x8000u);
        if (word == 0u)
            goto leave;
        word = lrd16(&m, -5);                                             /* 0097 step := min(amount, 10) */
        sub_flags(cpu, word, 10u, 0x8000u);
        if (word > 10u) {
            lwr8(&m, -3, 10u);
        } else {
            cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | lrd8(&m, -5));    /* 00A3 MOV AL,[BP-5]; MOV [BP-3],AL */
            lwr8(&m, -3, (uint8_t)cpu->ax);
        }
        lwr8(&m, -2, 1u);                                                 /* 00A9 i := 1; member := DS:[5D96h] */
        cpu->ax = rd16(&m, cpu->ds, 0x5D96u);
        cpu->dx = rd16(&m, cpu->ds, 0x5D98u);
        lwr16(&m, -9, cpu->ax);
        lwr16(&m, -7, cpu->dx);
        for (;;) {
            if (or_pair(&m, -9, -7) == 0u)                                /* 00BA member nil? JMP 02D8 */
                break;
            load_al_zero_ah(&m, lrd8(&m, -2));                            /* 00C5 CMP BYTE [DI+471Dh],0; JNE 00D6 */
            cpu->di = cpu->ax;
            byte = rd8(&m, cpu->ds, (uint16_t)(cpu->di + 0x471Du));
            sub_flags(cpu, byte, 0u, 0x80u);
            if (byte != 0u && !visit_member(&m))
                return m.stop;
            (void)inc8_local(&m, -2);                                     /* 02BF i += 1; member := member->next */
            copy_link(&m, -9, 0x104u, -9);
            if (!loopback(&m, 0x00BAu))                                   /* 02D5 JMP 00BA */
                return m.stop;
        }
        word = lrd16(&m, -5);                                             /* 02D8 amount -= 10, or 0 */
        sub_flags(cpu, word, 10u, 0x8000u);
        if (word > 10u) {
            word = lrd16(&m, -5);
            sub_flags(cpu, word, 10u, 0x8000u);
            lwr16(&m, -5, (uint16_t)(word - 10u));
        } else {
            cpu->ax = 0u;                                                 /* 02E4 XOR AX,AX; MOV [BP-5],AX */
            logic_flags(cpu, 0u, 0x8000u);
            lwr16(&m, -5, cpu->ax);
        }
        if (!loopback(&m, 0x008Eu))                                       /* 02E9 JMP 008E */
            return m.stop;
    }
leave:
    cpu->sp = cpu->bp;                                                    /* 02EC MOV SP,BP; POP BP; RETF 4 */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 4u);
    return PR_U7D_0000_OK;
}

/* GAME.OVR unit 0x3C2 +002E (raw CPU, composing SYSTEM Move); docs/re/U3C2_RECORD_SORT_002E.md. */
#include "poolrad/recovered/overlay_u3c2_record_sort_002e_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#include <string.h>

#define MASK (PR_U3C2_002E_RAM_SIZE - 1u)
#define ENTRY_IP 0x002Eu
#define MOVE_ENTRY 0x0A3Fu

_Static_assert(sizeof(pr_u3c2_002e_cpu) == sizeof(pr_system_leaf_cpu), "SYSTEM leaf CPU layout differs");

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u3c2_002e_cpu *cpu;
    uint16_t entry;
    unsigned loopbacks, max_loopbacks;
    int *callee_status;
    pr_u3c2_002e_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t ds8(const machine *m, uint16_t offset)
{
    return m->ram[physical(m->cpu->ds, offset)];
}

static uint8_t local8(const machine *m, unsigned below)
{
    return m->ram[physical(m->cpu->ss, (uint16_t)(m->cpu->bp - below))];
}

static void set_local8(machine *m, unsigned below, uint8_t value)
{
    m->ram[physical(m->cpu->ss, (uint16_t)(m->cpu->bp - below))] = value;
}

static void push16(machine *m, uint16_t value)
{
    size_t at;
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    at = physical(m->cpu->ss, m->cpu->sp);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t here(const machine *m, uint16_t offset)
{
    return (uint16_t)(m->entry + (uint16_t)(offset - ENTRY_IP));
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* SUB/CMP or ADD flags at 8 or 16 bits. */
static void arith_flags(pr_u3c2_002e_cpu *cpu, uint16_t left, uint16_t right, unsigned bits, int subtract)
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

static void set_al(pr_u3c2_002e_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

/* MOV AL,[BP-below]; XOR AH,AH; MOV DI,AX; MOV SI,DI; SHL DI,1; ADD DI,SI (flags folded): DI := 3k, SI := k. */
static void index3(machine *m, unsigned below)
{
    pr_u3c2_002e_cpu *cpu = m->cpu;
    cpu->ax = local8(m, below);
    cpu->di = cpu->ax;
    cpu->si = cpu->di;
    cpu->di = (uint16_t)((cpu->di << 1) + cpu->si);
}

/* index3, then ADD DI,6718h with its flags (live at a CODE stop). */
static void record3(machine *m, unsigned below)
{
    pr_u3c2_002e_cpu *cpu = m->cpu;
    index3(m, below);
    arith_flags(cpu, cpu->di, 0x6718u, 16u, 0);
    cpu->di = (uint16_t)(cpu->di + 0x6718u);
}

/* MOV AX,3; PUSH AX; LCALL xxxx:0A3F at original offset `site` (arguments already pushed). */
static int move3(machine *m, uint16_t site)
{
    pr_u3c2_002e_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs;
    pr_system_leaf_cpu leaf;
    pr_system_leaf_status status;
    cpu->ax = 3u;
    push16(m, cpu->ax);
    cpu->ip = at;
    if (m->ram[physical(cpu->cs, at)] != 0x9Au || read16_at(m->ram, physical(cpu->cs, (uint16_t)(at + 1u))) != MOVE_ENTRY) {
        m->stop = PR_U3C2_002E_CODE;
        return 0;
    }
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->cs = read16_at(m->ram, physical(caller_cs, (uint16_t)(at + 3u)));
    cpu->ip = MOVE_ENTRY;
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_system_move_0a3f(m->ram, m->size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (m->callee_status != NULL)
        *m->callee_status = (int)status;
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U3C2_002E_TRANSFER;
        return 0;
    }
    return 1;
}

/* A taken loop-back JMP to the INC at original offset `inc`. */
static int loop_back(machine *m, uint16_t inc)
{
    if (m->loopbacks++ == m->max_loopbacks) {
        m->cpu->ip = here(m, inc);
        m->stop = PR_U3C2_002E_LIMIT;
        return 0;
    }
    return 1;
}

/* AL := (MOV AL,[BP-below]; XOR AH,AH; CWD; MOV CX,2; IDIV CX; XCHG DX,AX): AX := byte mod 2, DX := byte div 2. */
static void halve(machine *m, unsigned below)
{
    pr_u3c2_002e_cpu *cpu = m->cpu;
    uint16_t value = local8(m, below);
    cpu->cx = 2u;
    cpu->ax = (uint16_t)(value % 2u);
    cpu->dx = (uint16_t)(value / 2u);
}

/* The inner body +0075..+0185: compare R[i] and R[j] and swap them through three Moves. */
static int compare_and_swap(machine *m)
{
    pr_u3c2_002e_cpu *cpu = m->cpu;
    uint8_t low, high;
    index3(m, 1u);                                                        /* +0075 MOV AL,[DI+671Ah]; MOV [BP-4],AL */
    set_al(cpu, ds8(m, (uint16_t)(cpu->di + 0x671Au)));
    set_local8(m, 4u, (uint8_t)cpu->ax);
    index3(m, 2u);                                                        /* +0089 MOV AL,[DI+671Ah]; MOV [BP-3],AL */
    set_al(cpu, ds8(m, (uint16_t)(cpu->di + 0x671Au)));
    set_local8(m, 3u, (uint8_t)cpu->ax);
    index3(m, 1u);                                                        /* +009D MOV DL,[DI+6719h] */
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | ds8(m, (uint16_t)(cpu->di + 0x6719u)));
    index3(m, 2u);                                                        /* +00AE MOV AL,[DI+6719h]; CMP AL,DL; JB +0115 */
    set_al(cpu, ds8(m, (uint16_t)(cpu->di + 0x6719u)));
    arith_flags(cpu, (uint8_t)cpu->ax, (uint8_t)cpu->dx, 8u, 1);
    if ((uint8_t)cpu->ax >= (uint8_t)cpu->dx) {
        index3(m, 1u);                                                    /* +00C3 MOV DL,[DI+6719h] */
        cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | ds8(m, (uint16_t)(cpu->di + 0x6719u)));
        index3(m, 2u);                                                    /* +00D4 MOV AL,[DI+6719h]; CMP AL,DL; JE +00EC */
        set_al(cpu, ds8(m, (uint16_t)(cpu->di + 0x6719u)));
        arith_flags(cpu, (uint8_t)cpu->ax, (uint8_t)cpu->dx, 8u, 1);
        if ((uint8_t)cpu->ax != (uint8_t)cpu->dx)
            return 1;                                                     /* +00E9 JMP +0186 */
        set_al(cpu, local8(m, 3u));                                       /* +00EC MOV AL,[BP-3]; CMP AL,[BP-4]; JB +00F7 */
        low = (uint8_t)cpu->ax;
        high = local8(m, 4u);
        arith_flags(cpu, low, high, 8u, 1);
        if (low >= high)
            return 1;                                                     /* +00F4 JMP +0186 */
        halve(m, 4u);                                                     /* +00F7 ... MOV BX,AX */
        cpu->bx = cpu->ax;
        halve(m, 3u);                                                     /* +0105 ... XCHG DX,AX */
        arith_flags(cpu, cpu->ax, cpu->bx, 16u, 1);                       /* +0111 CMP AX,BX; JG +0186 */
        if ((int16_t)cpu->ax > (int16_t)cpu->bx)
            return 1;
    }
    record3(m, 1u);                                                       /* +0115 R[i] -> SS:BP-7 */
    push16(m, cpu->ds);
    push16(m, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 7u);
    push16(m, cpu->ss);
    push16(m, cpu->di);
    if (!move3(m, 0x0131u))
        return 0;
    record3(m, 2u);                                                       /* +0136 R[j] -> R[i] */
    push16(m, cpu->ds);
    push16(m, cpu->di);
    record3(m, 1u);
    push16(m, cpu->ds);
    push16(m, cpu->di);
    if (!move3(m, 0x0160u))
        return 0;
    cpu->di = (uint16_t)(cpu->bp - 7u);                                   /* +0165 SS:BP-7 -> R[j] */
    push16(m, cpu->ss);
    push16(m, cpu->di);
    record3(m, 2u);
    push16(m, cpu->ds);
    push16(m, cpu->di);
    return move3(m, 0x0181u);
}

pr_u3c2_002e_status pr_u3c2_record_sort_002e_cpu(uint8_t *ram, size_t ram_size, pr_u3c2_002e_cpu *cpu, unsigned max_loopbacks,
                                                 int *callee_status)
{
    machine m;
    uint8_t value, bound;
    size_t frame;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U3C2_002E_RAM_SIZE)
        return PR_U3C2_002E_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.entry = cpu->ip;
    m.max_loopbacks = max_loopbacks; m.callee_status = callee_status; m.stop = PR_U3C2_002E_OK;
    push16(&m, cpu->bp);                                                  /* +002E PUSH BP; MOV BP,SP; SUB SP,0Ah */
    cpu->bp = cpu->sp;
    arith_flags(cpu, cpu->sp, 0x0Au, 16u, 1);
    cpu->sp = (uint16_t)(cpu->sp - 0x0Au);
    value = ds8(&m, 0x671Au);                                             /* +0034 CMP BYTE [671Ah],1; JA +003E */
    arith_flags(cpu, value, 1u, 8u, 1);
    if (value <= 1u)
        goto leave;
    cpu->ax = (uint16_t)(ds8(&m, 0x671Au) - 1u);                          /* +003E MOV AL,[671Ah]; XOR AH,AH; DEC AX; MOV [BP-8],AL */
    set_local8(&m, 8u, (uint8_t)cpu->ax);
    set_al(cpu, 1u);                                                      /* +0047 MOV AL,1; CMP AL,[BP-8]; JBE +0051 */
    bound = local8(&m, 8u);
    arith_flags(cpu, 1u, bound, 8u, 1);
    if (1u > bound)
        goto leave;
    set_local8(&m, 1u, (uint8_t)cpu->ax);                                 /* +0051 MOV [BP-1],AL */
    for (;;) {
        set_al(cpu, ds8(&m, 0x671Au));                                    /* +0059 MOV AL,[671Ah]; MOV [BP-9],AL */
        set_local8(&m, 9u, (uint8_t)cpu->ax);
        cpu->ax = (uint16_t)(local8(&m, 1u) + 1u);                        /* +005F MOV AL,[BP-1]; XOR AH,AH; INC AX */
        bound = local8(&m, 9u);                                           /* +0065 CMP AL,[BP-9]; JBE +006D */
        arith_flags(cpu, (uint8_t)cpu->ax, bound, 8u, 1);
        if ((uint8_t)cpu->ax <= bound) {
            set_local8(&m, 2u, (uint8_t)cpu->ax);                         /* +006D MOV [BP-2],AL */
            for (;;) {
                if (!compare_and_swap(&m))
                    return m.stop;
                set_al(cpu, local8(&m, 2u));                              /* +0186 MOV AL,[BP-2]; CMP AL,[BP-9]; JE +0191 */
                bound = local8(&m, 9u);
                arith_flags(cpu, (uint8_t)cpu->ax, bound, 8u, 1);
                if ((uint8_t)cpu->ax == bound)
                    break;
                if (!loop_back(&m, 0x0072u))                              /* +018E JMP +0072 */
                    return m.stop;
                set_local8(&m, 2u, (uint8_t)(local8(&m, 2u) + 1u));       /* +0072 INC BYTE [BP-2] (flags folded) */
            }
        }
        set_al(cpu, local8(&m, 1u));                                      /* +0191 MOV AL,[BP-1]; CMP AL,[BP-8]; JE +019C */
        bound = local8(&m, 8u);
        arith_flags(cpu, (uint8_t)cpu->ax, bound, 8u, 1);
        if ((uint8_t)cpu->ax == bound)
            break;
        if (!loop_back(&m, 0x0056u))                                      /* +0199 JMP +0056 */
            return m.stop;
        set_local8(&m, 1u, (uint8_t)(local8(&m, 1u) + 1u));               /* +0056 INC BYTE [BP-1] (flags folded) */
    }
leave:
    cpu->sp = cpu->bp;                                                    /* +019C MOV SP,BP; POP BP; RETF */
    cpu->bp = read16_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    frame = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, frame);
    cpu->cs = read16_at(ram, (frame + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U3C2_002E_OK;
}

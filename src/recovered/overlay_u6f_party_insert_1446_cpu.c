/* GAME.OVR unit 0x6F +1446 (raw CPU, composing SYSTEM FillChar and set membership and, through the loaded stub
 * 00AC:0025, U9B +0000); docs/re/U6F_PARTY_INSERT_1446_CPU.md. */
#include "poolrad/recovered/overlay_u6f_party_insert_1446_cpu.h"
#include "poolrad/recovered/overlay_u9b_entry_zero_0000.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#include <string.h>

#define MASK (PR_U6F_INSERT_CPU_RAM_SIZE - 1u)
#define ENTRY_IP 0x1446u
#define STUB_OFFSET 0x0025u
#define U9B_ENTRY 0x0000u
#define HEAD 0x5D96u
#define ACTIVE 0x5D92u
#define WINDOW1 0x49D6u
#define SLOT 0x00BFu
#define NEXT 0x0104u

_Static_assert(sizeof(pr_u6f_insert_cpu) == sizeof(pr_system_leaf_cpu), "SYSTEM leaf CPU layout differs");
_Static_assert(sizeof(pr_u6f_insert_cpu) == sizeof(pr_u9b_0000_cpu), "U9B +0000 CPU layout differs");

typedef enum { FILL, SET_IN } leaf;

static const uint16_t leaf_entry[2] = {0x16A8u, 0x0DC0u};

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u6f_insert_cpu *cpu;
    uint16_t entry;
    unsigned loopbacks, max_loopbacks;
    pr_u6f_insert_cpu_callee *callee;
    int *callee_status;
    pr_u6f_insert_cpu_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t read8(const machine *m, uint16_t segment, uint16_t offset)
{
    return m->ram[physical(segment, offset)];
}

static void write8(machine *m, uint16_t segment, uint16_t offset, uint8_t value)
{
    m->ram[physical(segment, offset)] = value;
}

static uint16_t read16(const machine *m, uint16_t segment, uint16_t offset)
{
    return read16_at(m->ram, physical(segment, offset));
}

static void write16(machine *m, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(machine *m, uint16_t value)
{
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    write16(m, m->cpu->ss, m->cpu->sp, value);
}

static uint16_t here(const machine *m, uint16_t offset)
{
    return (uint16_t)(m->entry + (uint16_t)(offset - ENTRY_IP));
}

/* SS:[BP+disp] as a word, disp taken modulo 64 KiB. */
static uint16_t local16(const machine *m, uint16_t disp)
{
    return read16(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp));
}

static void set_local16(machine *m, uint16_t disp, uint16_t value)
{
    write16(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp), value);
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

/* OR or XOR: CF = OF = AF = 0, ZF/SF/PF from the result whose sign bit is `sign`; the other flags are kept. */
static void logic_flags(pr_u6f_insert_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void note(machine *m, pr_u6f_insert_cpu_callee which, int status)
{
    if (m->callee != NULL)
        *m->callee = which;
    if (m->callee_status != NULL)
        *m->callee_status = status;
}

/* A taken loop-back JMP to `head`; past the cap it stops there with LIMIT. */
static int loop_back(machine *m, uint16_t head)
{
    if (m->loopbacks == m->max_loopbacks) {
        m->cpu->ip = here(m, head);
        m->stop = PR_U6F_INSERT_CPU_LIMIT;
        return 0;
    }
    m->loopbacks++;
    return 1;
}

/* MOV AX,[BP+6]; MOV DX,[BP+8]; MOV [global],AX; MOV [global+2],DX. */
static void store_incoming(machine *m, uint16_t global)
{
    pr_u6f_insert_cpu *cpu = m->cpu;
    cpu->ax = local16(m, 6u);
    cpu->dx = local16(m, 8u);
    write16(m, cpu->ds, global, cpu->ax);
    write16(m, cpu->ds, (uint16_t)(global + 2u), cpu->dx);
}

/* LES DI,[BP-4]; MOV AX,ES:[DI+104h]; MOV DX,ES:[DI+106h]; MOV [BP-4],AX; MOV [BP-2],DX: the cursor follows its link. */
static void follow_link(machine *m)
{
    pr_u6f_insert_cpu *cpu = m->cpu;
    les(m, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->ax = read16(m, cpu->es, (uint16_t)(cpu->di + NEXT));
    cpu->dx = read16(m, cpu->es, (uint16_t)(cpu->di + NEXT + 2u));
    set_local16(m, (uint16_t)-4, cpu->ax);
    set_local16(m, (uint16_t)-2, cpu->dx);
}

/* LCALL at original offset `site` (arguments already pushed) to a SYSTEM leaf; returns 1 when it came back to the next
 * instruction. */
static int system_call(machine *m, uint16_t site, leaf kind)
{
    pr_u6f_insert_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs, segment;
    pr_system_leaf_cpu state;
    int status;
    cpu->ip = at;
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != leaf_entry[kind]) {
        m->stop = PR_U6F_INSERT_CPU_CODE;
        return 0;
    }
    segment = read16(m, cpu->cs, (uint16_t)(at + 3u));
    push16(m, caller_cs);
    push16(m, return_ip);
    cpu->cs = segment;
    cpu->ip = leaf_entry[kind];
    memcpy(&state, cpu, sizeof state);
    status = (int)(kind == FILL ? pr_system_fillchar_16a8(m->ram, m->size, &state) : pr_system_set_in_0dc0(m->ram, m->size, &state));
    memcpy(cpu, &state, sizeof state);
    note(m, kind == FILL ? PR_U6F_INSERT_CPU_FILLCHAR : PR_U6F_INSERT_CPU_SET_IN, status);
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U6F_INSERT_CPU_TRANSFER;
        return 0;
    }
    return 1;
}

/* +1574 LCALL 00AC:0025 (incoming already pushed); the loaded stub's EA jumps to U9B +0000. */
static int u9b_call(machine *m)
{
    pr_u6f_insert_cpu *cpu = m->cpu;
    uint16_t at = here(m, 0x1574u), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs, stub;
    pr_u9b_0000_cpu state;
    int status;
    cpu->ip = at;
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != STUB_OFFSET) {
        m->stop = PR_U6F_INSERT_CPU_CODE;
        return 0;
    }
    stub = read16(m, cpu->cs, (uint16_t)(at + 3u));
    push16(m, caller_cs);
    push16(m, return_ip);
    cpu->cs = stub;
    cpu->ip = STUB_OFFSET;
    if (read8(m, stub, STUB_OFFSET) != 0xEAu || read16(m, stub, STUB_OFFSET + 1u) != U9B_ENTRY) {
        m->stop = PR_U6F_INSERT_CPU_CODE;
        return 0;
    }
    cpu->cs = read16(m, stub, STUB_OFFSET + 3u);
    cpu->ip = U9B_ENTRY;
    memcpy(&state, cpu, sizeof state);
    status = (int)pr_u9b_entry_zero_0000(m->ram, m->size, &state);
    memcpy(cpu, &state, sizeof state);
    note(m, PR_U6F_INSERT_CPU_U9B, status);
    if (status != (int)PR_U9B_0000_OK) {
        m->stop = PR_U6F_INSERT_CPU_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U6F_INSERT_CPU_TRANSFER;
        return 0;
    }
    return 1;
}

pr_u6f_insert_cpu_status pr_u6f_party_insert_1446_cpu(uint8_t *ram, size_t ram_size, pr_u6f_insert_cpu *cpu, unsigned max_loopbacks,
                                                      pr_u6f_insert_cpu_callee *callee, int *callee_status)
{
    machine m;
    size_t at;
    uint16_t counter;
    if (callee != NULL)
        *callee = PR_U6F_INSERT_CPU_NO_CALLEE;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U6F_INSERT_CPU_RAM_SIZE)
        return PR_U6F_INSERT_CPU_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.entry = cpu->ip; m.max_loopbacks = max_loopbacks;
    m.callee = callee; m.callee_status = callee_status; m.stop = PR_U6F_INSERT_CPU_OK;
    push16(&m, cpu->bp);                                                  /* +1446 PUSH BP; MOV BP,SP; SUB SP,10h (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x10u);
    les(&m, cpu->ss, (uint16_t)(cpu->bp + 6u));                           /* +144C LES DI,[BP+6]; MOV BYTE ES:[DI+0BFh],0FFh */
    write8(&m, cpu->es, (uint16_t)(cpu->di + SLOT), 0xFFu);
    cpu->ax = read16(&m, cpu->ds, HEAD);                                  /* +1455 MOV AX,[5D96h]; OR AX,[5D98h]; JNE +147A */
    cpu->ax |= read16(&m, cpu->ds, HEAD + 2u);
    logic_flags(cpu, cpu->ax, 0x8000u);
    if (cpu->ax == 0u) {
        store_incoming(&m, HEAD);                                         /* +145E head := P */
        store_incoming(&m, ACTIVE);                                       /* +146B active := P; JMP +14BE */
    } else {
        cpu->ax = read16(&m, cpu->ds, HEAD);                              /* +147A MOV AX,[5D96h]; MOV DX,[5D98h]; [BP-4] := head */
        cpu->dx = read16(&m, cpu->ds, HEAD + 2u);
        set_local16(&m, (uint16_t)-4, cpu->ax);
        set_local16(&m, (uint16_t)-2, cpu->dx);
        for (;;) {
            les(&m, cpu->ss, (uint16_t)(cpu->bp - 4u));                   /* +1487 LES DI,[BP-4]; MOV AX,ES:[DI+104h]; OR AX,ES:[DI+106h]; JE +14AB */
            cpu->ax = read16(&m, cpu->es, (uint16_t)(cpu->di + NEXT));
            cpu->ax |= read16(&m, cpu->es, (uint16_t)(cpu->di + NEXT + 2u));
            logic_flags(cpu, cpu->ax, 0x8000u);
            if (cpu->ax == 0u)
                break;
            follow_link(&m);                                              /* +1496 */
            if (!loop_back(&m, 0x1487u))                                  /* +14A9 JMP +1487 */
                return m.stop;
        }
        cpu->ax = local16(&m, 6u);                                        /* +14AB MOV AX,[BP+6]; MOV DX,[BP+8]; LES DI,[BP-4]; link := P */
        cpu->dx = local16(&m, 8u);
        les(&m, cpu->ss, (uint16_t)(cpu->bp - 4u));
        write16(&m, cpu->es, (uint16_t)(cpu->di + NEXT), cpu->ax);
        write16(&m, cpu->es, (uint16_t)(cpu->di + NEXT + 2u), cpu->dx);
    }
    cpu->di = (uint16_t)(cpu->bp - 0x0Cu);                                /* +14BE LEA DI,[BP-0Ch]; PUSH SS; PUSH DI */
    push16(&m, cpu->ss);
    push16(&m, cpu->di);
    cpu->ax = 0x0008u;                                                    /* +14C3 MOV AX,8; PUSH AX; MOV AL,0; PUSH AX */
    push16(&m, cpu->ax);
    cpu->ax = 0x0000u;
    push16(&m, cpu->ax);
    if (!system_call(&m, 0x14CAu, FILL))                                  /* +14CA FillChar(SS:BP-0Ch, 8, 0) */
        return m.stop;
    cpu->ax = read16(&m, cpu->ds, HEAD);                                  /* +14CF [BP-4] := head */
    cpu->dx = read16(&m, cpu->ds, HEAD + 2u);
    set_local16(&m, (uint16_t)-4, cpu->ax);
    set_local16(&m, (uint16_t)-2, cpu->dx);
    for (;;) {
        cpu->ax = local16(&m, (uint16_t)-4);                              /* +14DC MOV AX,[BP-4]; OR AX,[BP-2]; JE +151E */
        cpu->ax |= local16(&m, (uint16_t)-2);
        logic_flags(cpu, cpu->ax, 0x8000u);
        if (cpu->ax == 0u)
            break;
        les(&m, cpu->ss, (uint16_t)(cpu->bp - 4u));                       /* +14E4 LES DI,[BP-4]; MOV AL,ES:[DI+0BFh]; PUSH AX */
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read8(&m, cpu->es, (uint16_t)(cpu->di + SLOT)));
        push16(&m, cpu->ax);
        cpu->di = 0x1426u;                                                /* +14ED MOV DI,1426h; PUSH CS; PUSH DI */
        push16(&m, cpu->cs);
        push16(&m, cpu->di);
        if (!system_call(&m, 0x14F2u, SET_IN))                            /* +14F2 the slot in the set at CS:1426h */
            return m.stop;
        if ((cpu->flags & 0x0040u) == 0u) {                               /* +14F7 JE +1509 */
            les(&m, cpu->ss, (uint16_t)(cpu->bp - 4u));                   /* +14F9 LES DI,[BP-4]; MOV AL,ES:[DI+0BFh]; XOR AH,AH; MOV DI,AX */
            cpu->ax = read8(&m, cpu->es, (uint16_t)(cpu->di + SLOT));
            logic_flags(cpu, 0u, 0x0080u);
            cpu->di = cpu->ax;
            write8(&m, cpu->ss, (uint16_t)(cpu->bp + cpu->di - 0x0Cu), 1u); /* +1505 MOV BYTE [BP+DI-0Ch],1 */
        }
        follow_link(&m);                                                  /* +1509 */
        if (!loop_back(&m, 0x14DCu))                                      /* +151C JMP +14DC */
            return m.stop;
    }
    store_incoming(&m, ACTIVE);                                           /* +151E active := P */
    les(&m, cpu->ds, ACTIVE);                                             /* +152B LES DI,[5D92h]; MOV [BP-10h],DI; MOV [BP-0Eh],ES */
    set_local16(&m, (uint16_t)-0x10, cpu->di);
    set_local16(&m, (uint16_t)-0x0E, cpu->es);
    les(&m, cpu->ss, (uint16_t)(cpu->bp - 0x10u));                        /* +1535 LES DI,[BP-10h]; MOV BYTE ES:[DI+0BFh],0 */
    write8(&m, cpu->es, (uint16_t)(cpu->di + SLOT), 0u);
    for (;;) {
        les(&m, cpu->ss, (uint16_t)(cpu->bp - 0x10u));                    /* +153E LES DI,[BP-10h]; CMP BYTE ES:[DI+0BFh],8; JAE +1565 */
        if (read8(&m, cpu->es, (uint16_t)(cpu->di + SLOT)) >= 8u)
            break;
        les(&m, cpu->ss, (uint16_t)(cpu->bp - 0x10u));                    /* +1549 LES DI,[BP-10h]; MOV AL,ES:[DI+0BFh]; XOR AH,AH; MOV DI,AX */
        cpu->ax = read8(&m, cpu->es, (uint16_t)(cpu->di + SLOT));
        cpu->di = cpu->ax;
        if (read8(&m, cpu->ss, (uint16_t)(cpu->bp + cpu->di - 0x0Cu)) == 0u) /* +1555 CMP BYTE [BP+DI-0Ch],0; JE +1565 */
            break;
        les(&m, cpu->ss, (uint16_t)(cpu->bp - 0x10u));                    /* +155B LES DI,[BP-10h]; INC BYTE ES:[DI+0BFh]; JMP +153E */
        write8(&m, cpu->es, (uint16_t)(cpu->di + SLOT), (uint8_t)(read8(&m, cpu->es, (uint16_t)(cpu->di + SLOT)) + 1u));
    }
    les(&m, cpu->ds, WINDOW1);                                            /* +1565 LES DI,[49D6h]; INC WORD ES:[DI+67Ch] */
    counter = (uint16_t)(read16(&m, cpu->es, (uint16_t)(cpu->di + 0x067Cu)) + 1u);
    write16(&m, cpu->es, (uint16_t)(cpu->di + 0x067Cu), counter);
    cpu->flags &= (uint16_t)~0x08D5u;                                     /* loop C's flags are folded: either exit (JAE, or JE */
    if (even_parity((uint8_t)counter)) cpu->flags |= 0x0004u;             /* after CMP with 0) leaves CF clear, and INC keeps CF */
    if ((counter & 0x000Fu) == 0u) cpu->flags |= 0x0010u;
    if (counter == 0u) cpu->flags |= 0x0040u;
    if (counter & 0x8000u) cpu->flags |= 0x0080u;
    if (counter == 0x8000u) cpu->flags |= 0x0800u;
    push16(&m, local16(&m, 8u));                                          /* +156E PUSH WORD [BP+8]; PUSH WORD [BP+6] */
    push16(&m, local16(&m, 6u));
    if (!u9b_call(&m))                                                    /* +1574 */
        return m.stop;
    cpu->sp = cpu->bp;                                                    /* +1579 MOV SP,BP; POP BP; RETF 4 */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 4u);
    return PR_U6F_INSERT_CPU_OK;
}

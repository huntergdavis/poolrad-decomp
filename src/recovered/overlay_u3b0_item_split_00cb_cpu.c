/* GAME.OVR unit 0x3B0 +00CB (raw CPU, composing SYSTEM string store, FillChar and set membership); docs/re/U3B0_ITEM_SPLIT_00CB.md. */
#include "poolrad/recovered/overlay_u3b0_item_split_00cb_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#include <string.h>

#define MASK (PR_U3B0_00CB_RAM_SIZE - 1u)
#define ENTRY_IP 0x00CBu

_Static_assert(sizeof(pr_u3b0_00cb_cpu) == sizeof(pr_system_0b25_cpu), "SYSTEM 0B25 CPU layout differs");
_Static_assert(sizeof(pr_u3b0_00cb_cpu) == sizeof(pr_system_leaf_cpu), "SYSTEM leaf CPU layout differs");

typedef enum { STORE, FILL, SET_IN } callee;

static const uint16_t callee_entry[3] = {0x0B25u, 0x16A8u, 0x0DC0u};

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u3b0_00cb_cpu *cpu;
    uint16_t entry;
    int *callee_status;
    pr_u3b0_00cb_status stop;
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

/* SS:[BP+disp] as a byte, disp taken modulo 64 KiB. */
static uint8_t frame8(const machine *m, uint16_t disp)
{
    return read8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp));
}

static void set_frame8(machine *m, uint16_t disp, uint8_t value)
{
    write8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp), value);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* The six arithmetic flags of left - right (subtract) or left + right in `bits` bits; the other flags are kept. */
static void arith_flags(pr_u3b0_00cb_cpu *cpu, uint32_t left, uint32_t right, unsigned bits, int subtract)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t result = (subtract ? left - right : left + right) & mask;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (subtract ? left < right : left + right > mask) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((subtract ? (left ^ right) & (left ^ result) : (left ^ result) & (right ^ result)) & sign) cpu->flags |= 0x0800u;
}

static void set_al(pr_u3b0_00cb_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void set_dl(pr_u3b0_00cb_cpu *cpu, uint8_t value)
{
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | value);
}

/* MOV AL,[BP-2Ah]; XOR AH,AH; SHL AX,1; MOV DI,[BP+6]; ADD DI,AX (flags folded unless `live`): DI := L + 2k. */
static void pair_index(machine *m, int live)
{
    pr_u3b0_00cb_cpu *cpu = m->cpu;
    uint16_t link;
    cpu->ax = (uint16_t)(frame8(m, (uint16_t)-0x2Au) << 1);
    link = read16(m, cpu->ss, (uint16_t)(cpu->bp + 6u));
    if (live)
        arith_flags(cpu, link, cpu->ax, 16u, 0);
    cpu->di = (uint16_t)(link + cpu->ax);
}

/* LCALL at original offset `site` (arguments already pushed) to the `kind` callee; returns 1 when it came back to the next instruction. */
static int far_call(machine *m, uint16_t site, callee kind)
{
    pr_u3b0_00cb_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs;
    int status;
    cpu->ip = at;
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != callee_entry[kind]) {
        m->stop = PR_U3B0_00CB_CODE;
        return 0;
    }
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->cs = read16(m, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = callee_entry[kind];
    if (kind == STORE) {
        pr_system_0b25_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_system_strstore_0b25(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
    } else {
        pr_system_leaf_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)(kind == FILL ? pr_system_fillchar_16a8(m->ram, m->size, &leaf) : pr_system_set_in_0dc0(m->ram, m->size, &leaf));
        memcpy(cpu, &leaf, sizeof leaf);
    }
    if (m->callee_status != NULL)
        *m->callee_status = status;
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U3B0_00CB_TRANSFER;
        return 0;
    }
    return 1;
}

/* +0132..+018B for a member byte: open the first item, or close item k at i - 2 and open item k + 1 at i (flags folded). */
static void mark_item(machine *m)
{
    pr_u3b0_00cb_cpu *cpu = m->cpu;
    pair_index(m, 0);                                                     /* +0132 */
    if (read8(m, cpu->ss, (uint16_t)(cpu->di - 0x8Fu)) == 0u) {           /* +013E CMP BYTE SS:[DI-8Fh],0; JNE +015C */
        set_dl(cpu, frame8(m, (uint16_t)-0x2Bu));                         /* +0146 MOV DL,[BP-2Bh] */
        pair_index(m, 0);
        write8(m, cpu->ss, (uint16_t)(cpu->di - 0x8Fu), (uint8_t)cpu->dx); /* MOV SS:[DI-8Fh],DL; JMP +018D */
        return;
    }
    cpu->ax = (uint16_t)(frame8(m, (uint16_t)-0x2Bu) - 2u);               /* +015C MOV AL,[BP-2Bh]; XOR AH,AH; DEC AX; DEC AX; MOV DL,AL */
    set_dl(cpu, (uint8_t)cpu->ax);
    pair_index(m, 0);                                                     /* +0165 */
    write8(m, cpu->ss, (uint16_t)(cpu->di - 0x8Eu), (uint8_t)cpu->dx);    /* MOV SS:[DI-8Eh],DL */
    set_frame8(m, (uint16_t)-0x2Au, (uint8_t)(frame8(m, (uint16_t)-0x2Au) + 1u)); /* +0176 INC BYTE [BP-2Ah] */
    set_dl(cpu, frame8(m, (uint16_t)-0x2Bu));                             /* +0179 MOV DL,[BP-2Bh] */
    pair_index(m, 0);
    write8(m, cpu->ss, (uint16_t)(cpu->di - 0x8Fu), (uint8_t)cpu->dx);    /* MOV SS:[DI-8Fh],DL */
}

pr_u3b0_00cb_status pr_u3b0_item_split_00cb_cpu(uint8_t *ram, size_t ram_size, pr_u3b0_00cb_cpu *cpu, int *callee_status)
{
    machine m;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U3B0_00CB_RAM_SIZE)
        return PR_U3B0_00CB_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.entry = cpu->ip; m.callee_status = callee_status; m.stop = PR_U3B0_00CB_OK;
    push16(&m, cpu->bp);                                                  /* +00CB PUSH BP; MOV BP,SP; SUB SP,2Eh */
    cpu->bp = cpu->sp;
    arith_flags(cpu, cpu->sp, 0x2Eu, 16u, 1);
    cpu->sp = (uint16_t)(cpu->sp - 0x2Eu);
    at = physical(cpu->ss, (uint16_t)(cpu->bp + 8u));                     /* +00D1 LES DI,[BP+8]; PUSH ES; PUSH DI */
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    push16(&m, cpu->es);
    push16(&m, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x29u);                                /* +00D6 LEA DI,[BP-29h]; PUSH SS; PUSH DI; MOV AX,28h; PUSH AX */
    push16(&m, cpu->ss);
    push16(&m, cpu->di);
    cpu->ax = 0x0028u;
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x00DFu, STORE))                                    /* +00DF the local copy, at most 40 bytes */
        return m.stop;
    cpu->di = read16(&m, cpu->ss, (uint16_t)(cpu->bp + 6u));              /* +00E4 MOV DI,[BP+6]; ADD DI,0FF73h */
    arith_flags(cpu, cpu->di, 0xFF73u, 16u, 0);
    cpu->di = (uint16_t)(cpu->di + 0xFF73u);
    push16(&m, cpu->ss);                                                  /* +00EB PUSH SS; PUSH DI; MOV AX,28h; PUSH AX; MOV AL,0; PUSH AX */
    push16(&m, cpu->di);
    cpu->ax = 0x0028u;
    push16(&m, cpu->ax);
    cpu->ax = 0x0000u;
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x00F4u, FILL))                                     /* +00F4 FillChar(L[-8Dh], 40, 0) */
        return m.stop;
    set_frame8(&m, (uint16_t)-0x2Au, 1u);                                 /* +00F9 MOV BYTE [BP-2Ah],1 */
    set_al(cpu, frame8(&m, (uint16_t)-0x29u));                            /* +00FD MOV AL,[BP-29h]; MOV [BP-2Ch],AL */
    set_frame8(&m, (uint16_t)-0x2Cu, (uint8_t)cpu->ax);
    set_al(cpu, frame8(&m, (uint16_t)-0x2Cu));                            /* +0103 MOV AL,[BP-2Ch]; MOV [BP-2Eh],AL */
    set_frame8(&m, (uint16_t)-0x2Eu, (uint8_t)cpu->ax);
    set_al(cpu, 1u);                                                      /* +0109 MOV AL,1; CMP AL,[BP-2Eh]; JBE +0113 (flags folded) */
    if (frame8(&m, (uint16_t)-0x2Eu) != 0u) {
        set_frame8(&m, (uint16_t)-0x2Bu, (uint8_t)cpu->ax);               /* +0113 MOV [BP-2Bh],AL; JMP +011B */
        for (;;) {
            cpu->ax = frame8(&m, (uint16_t)-0x2Bu);                       /* +011B MOV AL,[BP-2Bh]; XOR AH,AH; MOV DI,AX */
            cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);   /* XOR's flags are live only at the CODE stop below */
            cpu->di = cpu->ax;
            set_al(cpu, read8(&m, cpu->ss, (uint16_t)(cpu->bp + cpu->di - 0x29u))); /* +0122 MOV AL,[BP+DI-29h]; PUSH AX */
            push16(&m, cpu->ax);
            cpu->di = 0x00ABu;                                            /* +0126 MOV DI,0ABh; PUSH CS; PUSH DI */
            push16(&m, cpu->cs);
            push16(&m, cpu->di);
            if (!far_call(&m, 0x012Bu, SET_IN))                           /* +012B the byte in the set at CS:00AB */
                return m.stop;
            if ((cpu->flags & 0x0040u) == 0u)                             /* +0130 JE +018D */
                mark_item(&m);
            set_al(cpu, frame8(&m, (uint16_t)-0x2Bu));                    /* +018D MOV AL,[BP-2Bh]; CMP AL,[BP-2Eh]; JNE +0118 (flags folded) */
            if ((uint8_t)cpu->ax == frame8(&m, (uint16_t)-0x2Eu))
                break;
            set_frame8(&m, (uint16_t)-0x2Bu, (uint8_t)(frame8(&m, (uint16_t)-0x2Bu) + 1u)); /* +0118 INC BYTE [BP-2Bh] (flags folded) */
        }
    }
    set_dl(cpu, frame8(&m, (uint16_t)-0x2Cu));                            /* +0195 MOV DL,[BP-2Ch] */
    pair_index(&m, 1);                                                    /* +0198..+01A2: the final flags */
    write8(&m, cpu->ss, (uint16_t)(cpu->di - 0x8Eu), (uint8_t)cpu->dx);   /* +01A4 MOV SS:[DI-8Eh],DL */
    set_al(cpu, frame8(&m, (uint16_t)-0x2Au));                            /* +01A9 MOV AL,[BP-2Ah]; MOV DI,[BP+6]; MOV SS:[DI-8Eh],AL */
    cpu->di = read16(&m, cpu->ss, (uint16_t)(cpu->bp + 6u));
    write8(&m, cpu->ss, (uint16_t)(cpu->di - 0x8Eu), (uint8_t)cpu->ax);
    cpu->sp = cpu->bp;                                                    /* +01B4 MOV SP,BP; POP BP; RETF 6 */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 6u);
    return PR_U3B0_00CB_OK;
}

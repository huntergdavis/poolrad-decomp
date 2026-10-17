/* GAME.OVR unit 0x86:+1499..+14C5, complete 45-byte body, full state. */
#include "poolrad/recovered/overlay_u86_status_1499_full.h"

#define RAM_MASK (PR_U86_1499_FULL_RAM_SIZE - 1u)
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
#define ARITH_FLAGS (CF | PF | AF | ZF | SF | OF)

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u86_1499_full_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u86_1499_full_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint16_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) ? 0u : PF;
}

/* SUB 16-bit: a - b, all six arithmetic flags */
static uint16_t sub16(pr_u86_1499_full_cpu *cpu, uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a - b);
    uint16_t bits = parity((uint8_t)r);
    if (a < b) bits |= CF;
    if ((a ^ b ^ r) & 0x10u) bits |= AF;
    if (r == 0u) bits |= ZF;
    if (r & 0x8000u) bits |= SF;
    if ((a ^ b) & (a ^ r) & 0x8000u) bits |= OF;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | bits);
    return r;
}

#define BODY_OFF 0x1499u
#define BODY_LEN 45u                       /* [1499,14C6) */
#define WINDOW_LO 0x24u                    /* lowest model stack byte: SP0 - 0x24 (the lcall frame) */
#define WINDOW_HI 4u                       /* highest: the caller's far return frame [SP0, SP0 + 4) */

/* Body bytes the hooks may change and the model honours: the call operands it
 * fetches live, CS:+14BA..+14BD (lcall) and CS:+14C0..+14C1 (near disp). */
static int live_operand(unsigned k)
{
    return (k >= 0x21u && k <= 0x24u) || k == 0x27u || k == 0x28u;
}

/* nonzero if a byte of the stack window [SP0 - 0x24, SP0 + 4) is physically a body byte */
static int window_hits_body(const pr_u86_1499_full_cpu *cpu)
{
    size_t body = physical(cpu->cs, BODY_OFF);
    unsigned k;
    for (k = 0; k < WINDOW_LO + WINDOW_HI; k++) {
        size_t a = physical(cpu->ss, (uint16_t)(cpu->sp - WINDOW_LO + k));
        if (((a - body) & RAM_MASK) < BODY_LEN) return 1;
    }
    return 0;
}

/* nonzero if a body byte other than a live operand differs from the entry snapshot */
static int body_changed(const uint8_t *ram, uint16_t cs, const uint8_t *snapshot)
{
    unsigned k;
    for (k = 0; k < BODY_LEN; k++)
        if (!live_operand(k) && ram[physical(cs, (uint16_t)(BODY_OFF + k))] != snapshot[k]) return 1;
    return 0;
}

pr_u86_1499_full_status pr_u86_status_1499_full(
    uint8_t *ram, size_t ram_size, pr_u86_1499_full_cpu *cpu,
    pr_u86_1499_full_external external, void *context)
{
    uint16_t source_cs, entry_ss, target_seg, target_off, entry_sp;
    uint8_t snapshot[BODY_LEN];
    unsigned k;
    int i;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U86_1499_FULL_RAM_SIZE)
        return PR_U86_1499_FULL_ARGUMENT;
    /* RETF/stack-word policy: every word the model pushes or pops, including both
     * modelled RETFs' IP and CS words, lies in [SP0 - 0x24, SP0 + 4); a window that
     * would wrap the SS offset (UNMEASURED) fails closed before anything executes */
    if (cpu->sp < WINDOW_LO || cpu->sp > (uint16_t)(0x10000u - WINDOW_HI))
        return PR_U86_1499_FULL_STACK_BOUNDARY;
    /* a stack write onto the body's own code is not modelled (prefetch unmeasured) */
    if (window_hits_body(cpu)) return PR_U86_1499_FULL_CODE_ALIAS;
    for (k = 0; k < BODY_LEN; k++) snapshot[k] = ram[physical(cpu->cs, (uint16_t)(BODY_OFF + k))];

    push16(ram, cpu, cpu->bp);                                    /* +1499 push bp */
    cpu->bp = cpu->sp;                                            /* +149A mov bp,sp */
    cpu->sp = sub16(cpu, cpu->sp, 0x000Cu);                       /* +149C sub sp,0xC */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |                    /* +149F mov al,[0x681B] */
                         ram[physical(cpu->ds, 0x681Bu)]);
    push16(ram, cpu, cpu->ax);                                    /* +14A2 push ax */
    for (i = 0; i < 4; i++) {                                     /* +14A3..+14AE mov al,0; push ax (x4) */
        cpu->ax = (uint16_t)(cpu->ax & 0xFF00u);
        push16(ram, cpu, cpu->ax);
    }
    cpu->di = (uint16_t)(cpu->bp - 0x0Cu);                        /* +14AF lea di,[bp-0xC] */
    push16(ram, cpu, cpu->ss);                                    /* +14B2 push ss */
    push16(ram, cpu, cpu->di);                                    /* +14B3 push di */
    cpu->di = 0x148Du;                                            /* +14B4 mov di,0x148D */
    push16(ram, cpu, cpu->cs);                                    /* +14B7 push cs */
    push16(ram, cpu, cpu->di);                                    /* +14B8 push di */

    /* +14B9 lcall seg:off: the operand words are fetched live from CS:+14BA
     * (offset) and CS:+14BC (segment) after the pushes above (originally
     * 0x0B0B and 0x0AF8, the latter a loader fixup) */
    source_cs = cpu->cs;
    entry_ss = cpu->ss;
    target_off = read16(ram, source_cs, 0x14BAu);
    target_seg = read16(ram, source_cs, 0x14BCu);
    push16(ram, cpu, source_cs);
    push16(ram, cpu, 0x14BEu);
    cpu->cs = target_seg;
    cpu->ip = target_off;
    entry_sp = cpu->sp;
    if (!external(context, ram, ram_size, cpu, PR_U86_1499_FULL_0B0B)) return PR_U86_1499_FULL_CALL_ABORT;
    if (cpu->cs != source_cs || cpu->ip != 0x14BEu) return PR_U86_1499_FULL_TRANSFER;
    /* the callee's retf 4 pops the return frame and the source pointer only (SS kept) */
    if (cpu->ss != entry_ss || cpu->sp != (uint16_t)(entry_sp + 8u)) return PR_U86_1499_FULL_TRANSFER;
    if (body_changed(ram, source_cs, snapshot)) return PR_U86_1499_FULL_CODE_ALIAS;

    push16(ram, cpu, source_cs);                                  /* +14BE push cs */
    /* +14BF call rel16: the displacement is fetched from CS:+14C0 after the push
     * (originally 0xF401, giving +08C3) and added to the return IP +14C2 */
    target_off = (uint16_t)(0x14C2u + read16(ram, source_cs, 0x14C0u));
    push16(ram, cpu, 0x14C2u);
    cpu->ip = target_off;
    entry_sp = cpu->sp;
    if (!external(context, ram, ram_size, cpu, PR_U86_1499_FULL_08C3)) return PR_U86_1499_FULL_CALL_ABORT;
    /* the hook supplies everything but the return, so SS:SP is still at the return IP */
    if (cpu->ss != entry_ss || cpu->sp != entry_sp) return PR_U86_1499_FULL_TRANSFER;
    if (body_changed(ram, source_cs, snapshot)) return PR_U86_1499_FULL_CODE_ALIAS;
    cpu->ip = pop16(ram, cpu);                                    /* the callee's retf 0xE */
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 0x0Eu);
    if (cpu->cs != source_cs || cpu->ip != 0x14C2u) return PR_U86_1499_FULL_TRANSFER;
    /* all 18 pushed bytes are consumed (4 by retf 4, 14 by retf 0xE): SP is back at the
     * local frame, so mov sp,bp cannot hide a callee that broke the stack or BP contract */
    if (cpu->sp != (uint16_t)(cpu->bp - 0x0Cu)) return PR_U86_1499_FULL_TRANSFER;

    cpu->sp = cpu->bp;                                            /* +14C2 mov sp,bp */
    cpu->bp = pop16(ram, cpu);                                    /* +14C4 pop bp */
    cpu->ip = pop16(ram, cpu);                                    /* +14C5 retf */
    cpu->cs = pop16(ram, cpu);
    return PR_U86_1499_FULL_OK;
}

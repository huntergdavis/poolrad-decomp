/* Bounded model of GAME.OVR unit 0x1c +0x271F (ECL opcode 0x3A); see docs/re/U1C_DELAY_271F.md. */
#include "poolrad/recovered/overlay_u1c_delay_271f.h"

#include <string.h>

uint16_t pr_res_0709_0dbe_arg(uint8_t b49e2)
{
    return (uint16_t)((uint32_t)b49e2 * PR_U1C_DELAY_UNIT_MS);          /* +0x0DC1..+0x0DCB: mul dx (dx:ax), push ax; at most 57,375 */
}

pr_u1c_delay_status pr_u1c_op_delay(pr_rec_ecl_loader_state *st, pr_u1c_delay_env *env, pr_u1c_delay_report *report)
{
    pr_u1c_delay_report local;
    pr_u1c_delay_report *r = report ? report : &local;
    int rc;
    memset(r, 0, sizeof *r);
    if (!st || !env)
        return PR_U1C_DELAY_ARGUMENT;
    st->pc = (uint16_t)(st->pc + 1u);                                   /* +0x2722: inc word [0x49ED], before the call */
    r->ms = pr_res_0709_0dbe_arg(env->b49e2);
    if (!env->hooks.delay_a40_029c)
        return PR_U1C_DELAY_HOOK;
    r->called = 1;
    rc = env->hooks.delay_a40_029c(env->ctx, env, r->ms);               /* 0x709:0xDBE -> 0xA40:0x029C (ms) */
    if (rc != 0) {
        r->callback_status = rc;
        return PR_U1C_DELAY_CALLBACK;
    }
    return PR_U1C_DELAY_OK;
}

/* Raw companion: GAME.OVR [271F,272D), exactly 14 bytes / 6 instructions.
 * Trailing Pascal strings are DATA and are excluded from this translation.
 * See docs/re/U1C_DELAY_271F_CPU.md for CPU/RAM, budget, and timing limits. */
#define DELAY_RAM_MASK (PR_R0709_DELAY_CPU_RAM_SIZE - 1u)

static size_t delay_physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & DELAY_RAM_MASK;
}

static uint16_t delay_read16(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & DELAY_RAM_MASK] << 8));
}

static void delay_write16(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & DELAY_RAM_MASK] = (uint8_t)(value >> 8);
}

static void delay_push(uint8_t *ram, pr_u1c_delay_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    delay_write16(ram, delay_physical(cpu->ss, cpu->sp), value);
}

static uint16_t delay_inc(pr_u1c_delay_cpu *cpu, uint16_t before)
{
    uint16_t result = (uint16_t)(before + 1u);
    uint8_t parity = (uint8_t)result;
    cpu->flags &= (uint16_t)~0x08D4u; /* INC preserves CF. */
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (!(parity & 1u)) cpu->flags |= 0x0004u;
    if (!result) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((before & 0x0Fu) == 0x0Fu) cpu->flags |= 0x0010u;
    if (before == 0x7FFFu) cpu->flags |= 0x0800u;
    return result;
}

pr_u1c_delay_cpu_status pr_u1c_delay_271f_cpu(
    uint8_t *ram, size_t ram_size, pr_u1c_delay_cpu *cpu,
    uint64_t max_loopbacks, pr_u1c_delay_cpu_report *report)
{
    pr_u1c_delay_cpu_report local = {-1, -1};
    pr_u1c_delay_cpu_report *r = report ? report : &local;
    pr_r0709_delay_cpu_status status;
    uint16_t caller_cs, segment;
    size_t at;
    r->wrapper_status = r->delay_status = -1;
    if (!ram || !cpu || ram_size != PR_R0709_DELAY_CPU_RAM_SIZE ||
        cpu->ip != 0x271Fu || cpu->sp < 0x0010u || cpu->sp > 0xFFFCu)
        return PR_U1C_DELAY_CPU_ARGUMENT;
    caller_cs = cpu->cs;
    delay_push(ram, cpu, cpu->bp);                               /* 271F */
    cpu->bp = cpu->sp;                                         /* 2720 */
    at = delay_physical(cpu->ds, 0x49EDu);                      /* 2722 */
    delay_write16(ram, at, delay_inc(cpu, delay_read16(ram, at)));
    cpu->ip = 0x2726u;
    at = delay_physical(caller_cs, 0x2726u);
    if (ram[at] != 0x9Au || delay_read16(ram, (at + 1u) & DELAY_RAM_MASK) != 0x0DBEu)
        return PR_U1C_DELAY_CPU_CODE;
    segment = delay_read16(ram, delay_physical(caller_cs, 0x2729u));
    delay_push(ram, cpu, caller_cs);
    delay_push(ram, cpu, 0x272Bu);
    cpu->cs = segment;
    cpu->ip = 0x0DBEu;
    status = pr_r0709_delay_wrapper_0dbe_cpu(ram, ram_size, cpu, max_loopbacks, &r->delay_status);
    r->wrapper_status = (int)status;
    if (status != PR_R0709_DELAY_CPU_OK)
        return PR_U1C_DELAY_CPU_CALLEE;
    if (cpu->cs != caller_cs || cpu->ip != 0x272Bu)
        return PR_U1C_DELAY_CPU_TRANSFER;
    cpu->bp = delay_read16(ram, delay_physical(cpu->ss, cpu->sp)); /* 272B */
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = delay_physical(cpu->ss, cpu->sp);                       /* 272C */
    cpu->ip = delay_read16(ram, at);
    cpu->cs = delay_read16(ram, (at + 2u) & DELAY_RAM_MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U1C_DELAY_CPU_OK;
}

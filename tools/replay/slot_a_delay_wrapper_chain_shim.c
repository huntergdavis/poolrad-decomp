/* Standalone composition of reached U1C DELAY, 0709 wrapper, and A40 body. */
#include "poolrad/recovered/overlay_u1c_delay_271f.h"
#include "poolrad/recovered/resident_0709_delay_wrapper_0dbe.h"
#include "poolrad/recovered/resident_a40_delay_029c.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define RAM_SIZE (1u << 20)

/* Same word order as pr_r0709_delay_cpu; kept flat for the private probe. */
enum { AX, BX, CX, DX, SI, DI, BP, SP, CS, IP, DS, ES, SS, FLAGS,
       CPU_WORDS };

typedef struct {
    uint8_t *ram;
    pr_r0709_delay_cpu *cpu;
    uint32_t external_ticks;
    uint64_t *counts;
    uint16_t *wrapper_cpu;
    pr_rec_ecl_loader_state *state;
    int boundary;
} chain_ctx;

static size_t address(uint16_t segment, uint16_t offset) {
    return ((((size_t)segment << 4) + offset) & (RAM_SIZE - 1u));
}

static uint16_t read_word(const uint8_t *ram, uint16_t segment,
                          uint16_t offset) {
    size_t at = address(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) &
                                             (RAM_SIZE - 1u)] << 8));
}

static void write_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                       uint16_t value) {
    size_t at = address(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void push(uint8_t *ram, pr_r0709_delay_cpu *cpu, uint16_t value) {
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write_word(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop(const uint8_t *ram, pr_r0709_delay_cpu *cpu) {
    uint16_t value = read_word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void store_cpu(uint16_t out[CPU_WORDS], const pr_r0709_delay_cpu *cpu) {
    out[AX] = cpu->ax; out[BX] = cpu->bx; out[CX] = cpu->cx;
    out[DX] = cpu->dx; out[SI] = cpu->si; out[DI] = cpu->di;
    out[BP] = cpu->bp; out[SP] = cpu->sp; out[CS] = cpu->cs;
    out[IP] = cpu->ip; out[DS] = cpu->ds; out[ES] = cpu->es;
    out[SS] = cpu->ss; out[FLAGS] = cpu->flags;
}

static int read_bios(void *context, uint8_t *value) {
    chain_ctx *ctx = context;
    *value = ctx->ram[address(0x0040u, 0u)];
    return 0;
}

static int read_calibration(void *context, uint16_t *value) {
    chain_ctx *ctx = context;
    *value = read_word(ctx->ram, ctx->cpu->ds, 0x84FCu);
    return 0;
}

static void apply_external_ticks(chain_ctx *ctx) {
    uint32_t prior = read_word(ctx->ram, 0x0040u, 0x006Cu) |
                     ((uint32_t)read_word(ctx->ram, 0x0040u, 0x006Eu) << 16);
    uint32_t after = prior + ctx->external_ticks;
    write_word(ctx->ram, 0x0040u, 0x006Cu, (uint16_t)after);
    write_word(ctx->ram, 0x0040u, 0x006Eu, (uint16_t)(after >> 16));
}

static int call_a40(void *context, uint8_t *ram, size_t ram_size,
                    pr_r0709_delay_cpu *cpu, uint16_t argument) {
    chain_ctx *ctx = context;
    pr_a40_delay_env env = {ctx, read_bios, read_calibration, 1000000u};
    pr_a40_delay_report report;
    pr_a40_delay_status status;

    if (ram != ctx->ram || ram_size != RAM_SIZE ||
        cpu != ctx->cpu || cpu->cs != 0x1264u || cpu->ip != 0x029Cu ||
        read_word(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) != argument) {
        ctx->boundary = 1;
        return 1;
    }
    status = pr_a40_delay_029c_run(argument, &env, &report);
    ctx->counts[0] = report.completed_units;
    ctx->counts[1] = report.bios_reads;
    ctx->counts[2] = report.calibration_reads;
    ctx->counts[3] = report.spin_calls;
    if (status != PR_A40_DELAY_OK) {
        ctx->boundary = 2;
        return 1;
    }
    /* All nonzero units call +02BD. Each call overwrites the same word. */
    if (argument != 0u) {
        write_word(ram, cpu->ss, (uint16_t)(cpu->sp - 2u), 0x02B7u);
        cpu->ax = report.last_baseline;
        cpu->bx = 0u;
        cpu->cx = report.cx_after;
        cpu->dx = (uint16_t)(argument - report.completed_units);
        cpu->di = 0u;
        cpu->es = 0x0040u;
    } else {
        /* +029C MOV BX,SP; +029E MOV DX,[SS:BX+4]; +02A2 OR DX,DX. */
        cpu->bx = cpu->sp;
        cpu->dx = 0u;
    }
    apply_external_ticks(ctx);
    return 0;
}

static int call_wrapper(void *context, pr_u1c_delay_env *unused,
                        uint16_t argument) {
    chain_ctx *ctx = context;
    pr_r0709_delay_cpu *cpu = ctx->cpu;
    pr_r0709_delay_report report;
    pr_r0709_delay_status status;
    (void)unused;

    if (argument != (uint16_t)(ctx->ram[address(cpu->ds, 0x49E2u)] * 225u)) {
        ctx->boundary = 3;
        return 1;
    }
    write_word(ctx->ram, cpu->ds, 0x49EDu, ctx->state->pc);
    /* U1C +2726 far CALL: push CS then return IP +272B. */
    push(ctx->ram, cpu, cpu->cs);
    push(ctx->ram, cpu, 0x272Bu);
    cpu->cs = 0x0F2Du;
    cpu->ip = 0x0DBEu;
    status = pr_r0709_delay_wrapper_0dbe(ctx->ram, RAM_SIZE, cpu,
                                           0x1264u, call_a40, ctx, &report);
    if (status != PR_R0709_DELAY_OK || report.argument != argument) {
        if (ctx->boundary == 0) ctx->boundary = 4;
        return 1;
    }
    store_cpu(ctx->wrapper_cpu, cpu);
    return 0;
}

/* Inputs: one-MiB guest RAM, observed U1C entry CPU words, supplied external
 * BIOS tick advance. Outputs: U1C return CPU, wrapper return CPU, C counts.
 * Nonzero status is the first reached boundary; no expected return tuple is
 * loaded into RAM or CPU. */
int pr_slot_a_delay_wrapper_chain_run(uint8_t *ram, size_t ram_size,
                                      uint16_t cpu_words[CPU_WORDS],
                                      uint16_t wrapper_words[CPU_WORDS],
                                      uint32_t external_ticks,
                                      uint64_t counts[4]) {
    pr_r0709_delay_cpu cpu;
    pr_rec_ecl_loader_state state = {0};
    pr_u1c_delay_env env = {0};
    pr_u1c_delay_report report;
    chain_ctx ctx;
    pr_u1c_delay_status status;

    if (ram == NULL || ram_size != RAM_SIZE || cpu_words == NULL ||
        wrapper_words == NULL || counts == NULL) return 1;
    memset(counts, 0, 4u * sizeof *counts);
    memset(wrapper_words, 0, CPU_WORDS * sizeof *wrapper_words);
    cpu.ax=cpu_words[AX]; cpu.bx=cpu_words[BX]; cpu.cx=cpu_words[CX];
    cpu.dx=cpu_words[DX]; cpu.si=cpu_words[SI]; cpu.di=cpu_words[DI];
    cpu.bp=cpu_words[BP]; cpu.sp=cpu_words[SP]; cpu.cs=cpu_words[CS];
    cpu.ip=cpu_words[IP]; cpu.ds=cpu_words[DS]; cpu.es=cpu_words[ES];
    cpu.ss=cpu_words[SS]; cpu.flags=cpu_words[FLAGS];
    if (cpu.cs != 0x3582u || cpu.ip != 0x271Fu) return 2;
    state.pc = read_word(ram, cpu.ds, 0x49EDu);
    env.b49e2 = ram[address(cpu.ds, 0x49E2u)];
    ctx = (chain_ctx){ram, &cpu, external_ticks, counts,
                      wrapper_words, &state, 0};
    env.ctx = &ctx;
    env.hooks.delay_a40_029c = call_wrapper;

    /* U1C +271F PUSH BP; +2720 MOV BP,SP. The C model advances PC. */
    push(ram, &cpu, cpu.bp);
    cpu.bp = cpu.sp;
    status = pr_u1c_op_delay(&state, &env, &report);
    write_word(ram, cpu.ds, 0x49EDu, state.pc);
    if (status != PR_U1C_DELAY_OK) return ctx.boundary ? ctx.boundary : 5;
    if (report.called != 1u || report.ms != counts[0]) {
        /* Zero-unit cases are outside this reached-frame probe. */
        return 6;
    }
    /* U1C +272B POP BP; +272C RETF. */
    cpu.bp = pop(ram, &cpu);
    cpu.ip = pop(ram, &cpu);
    cpu.cs = pop(ram, &cpu);
    store_cpu(cpu_words, &cpu);
    return 0;
}

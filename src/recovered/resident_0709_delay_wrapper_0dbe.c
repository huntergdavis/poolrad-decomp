#include "poolrad/recovered/resident_0709_delay_wrapper_0dbe.h"

#include <string.h>

#define GUEST_RAM_SIZE (1u << 20)

static size_t physical(uint16_t segment, uint16_t offset) {
    return (((size_t)segment << 4) + offset) & (GUEST_RAM_SIZE - 1u);
}

static uint16_t read_word(const uint8_t *ram, uint16_t segment,
                          uint16_t offset) {
    size_t address = physical(segment, offset);
    return (uint16_t)(ram[address] |
                      ((uint16_t)ram[(address + 1u) & (GUEST_RAM_SIZE - 1u)]
                       << 8));
}

static void write_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                       uint16_t value) {
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & (GUEST_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void push_word(uint8_t *ram, pr_r0709_delay_cpu *cpu, uint16_t value) {
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write_word(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop_word(const uint8_t *ram, pr_r0709_delay_cpu *cpu) {
    uint16_t value = read_word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

pr_r0709_delay_status pr_r0709_delay_wrapper_0dbe(
    uint8_t *ram, size_t ram_size, pr_r0709_delay_cpu *cpu,
    uint16_t a40_cs, pr_r0709_delay_a40_call call_a40, void *ctx,
    pr_r0709_delay_report *report) {
    pr_r0709_delay_report local_report;
    uint16_t wrapper_cs, argument, callee_sp;
    int callback_status;

    if (ram == NULL || ram_size != GUEST_RAM_SIZE || cpu == NULL) {
        return PR_R0709_DELAY_ARGUMENT;
    }
    if (report == NULL) report = &local_report;
    memset(report, 0, sizeof *report);

    wrapper_cs = cpu->cs;
    push_word(ram, cpu, cpu->bp);             /* +0DBE PUSH BP */
    cpu->bp = cpu->sp;                       /* +0DBF MOV BP,SP */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         ram[physical(cpu->ds, 0x49E2u)]); /* +0DC1 MOV AL */
    cpu->ax &= 0x00FFu;                     /* +0DC4 XOR AH,AH */
    cpu->dx = 0x00E1u;                      /* +0DC6 MOV DX,00E1 */
    argument = (uint16_t)(cpu->ax * cpu->dx); /* +0DC9 MUL DX */
    cpu->ax = argument;
    cpu->dx = 0u;
    cpu->flags &= (uint16_t)~0x0801u;       /* defined MUL CF/OF = 0 */
    report->argument = argument;

    push_word(ram, cpu, argument);           /* +0DCB PUSH AX */
    push_word(ram, cpu, wrapper_cs);         /* +0DCC far CALL pushes CS */
    push_word(ram, cpu, 0x0DD1u);           /* then next IP */
    callee_sp = cpu->sp;
    report->callee_frame_sp = callee_sp;
    cpu->cs = a40_cs;
    cpu->ip = 0x029Cu;
    if (call_a40 == NULL) return PR_R0709_DELAY_EXTERNAL;

    report->callback_invoked = 1u;
    callback_status = call_a40(ctx, ram, ram_size, cpu, argument);
    if (callback_status != 0) {
        report->callback_status = callback_status;
        return PR_R0709_DELAY_CALLBACK;
    }
    if (cpu->sp != callee_sp) return PR_R0709_DELAY_CALLEE_FRAME;

    /* A40 RETF 2 consumes its far return and argument. */
    cpu->ip = pop_word(ram, cpu);
    cpu->cs = pop_word(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    /* +0DD1 POP BP; +0DD2 RETF. */
    cpu->bp = pop_word(ram, cpu);
    cpu->ip = pop_word(ram, cpu);
    cpu->cs = pop_word(ram, cpu);
    return PR_R0709_DELAY_OK;
}

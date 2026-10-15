#include "resident_1816_overlay_printer_composition.h"
#include "resident_1816_printer_gate.h"

#include <string.h>

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

typedef struct {
    pr_1816_printer_composition_report *report;
    int reached_gate;
    int error;
} composition_state;

static void push16(uint8_t *ram, pr_1816_caller_cpu *cpu, uint16_t value) {
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = (((size_t)cpu->ss << 4) + cpu->sp) & RAM_MASK;
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static int on_route(void *ctx, uint8_t *ram, size_t ram_size,
                    pr_1816_caller_cpu *cpu, unsigned site,
                    pr_1816_caller_event event, uint16_t word,
                    const pr_1816_caller_frame *frame) {
    composition_state *state = ctx;
    pr_1816_printer_composition_report *report = state->report;
    pr_1816_gate_cpu gate;
    (void)site;
    if (report->event_count >= 6u ||
        (unsigned)event != report->event_count) {
        state->error = 1;
        return 1;
    }
    report->order[report->event_count++] = event;
    if (event != PR_1816_CALL_DISPLAY) {
        if (frame != NULL) state->error = 1;
        report->pushed_words[(unsigned)event] = word;
        return state->error;
    }
    if (frame == NULL || word != 0u ||
        sizeof gate != sizeof *cpu) {
        state->error = 1;
        return 1;
    }
    report->frame = *frame;
    if (report->pushed_words[0] != frame->node_segment ||
        report->pushed_words[1] != frame->node_offset ||
        report->pushed_words[2] != frame->column ||
        report->pushed_words[3] != frame->row ||
        report->pushed_words[4] != frame->flag) {
        state->error = 1;
        return 1;
    }

    /* CALL FAR 00BA:1816 stores the overlay return address. The merged
     * callee gate then executes through CALL FAR 0709:0626. Returning 1
     * makes the caller model stop without simulating a callee return. */
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, (uint16_t)(cpu->ip + 5u));
    cpu->cs = 0x00BAu;
    cpu->ip = 0x1816u;
    memcpy(&gate, cpu, sizeof gate);
    state->error = pr_1816_first_printer_gate(ram, ram_size, &gate);
    memcpy(cpu, &gate, sizeof gate);
    state->reached_gate = !state->error;
    return 1;
}

int pr_1816_overlay_to_first_printer(
    unsigned site, uint8_t *ram, size_t ram_size, pr_1816_caller_cpu *cpu,
    pr_1816_printer_composition_report *report) {
    composition_state state;
    pr_1816_caller_status status;
    if (ram == NULL || cpu == NULL || report == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        site >= PR_1816_CALLER_SITE_COUNT) return 1;
    memset(report, 0, sizeof *report);
    state.report = report;
    state.reached_gate = 0;
    state.error = 0;
    status = pr_1816_caller_route_step(site, ram, ram_size, cpu,
                                       on_route, &state);
    return status == PR_1816_CALLER_BOUNDARY && state.reached_gate &&
                   !state.error && report->event_count == 6u ? 0 : 2;
}

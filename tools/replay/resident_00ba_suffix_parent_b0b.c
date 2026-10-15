#include "resident_00ba_suffix_parent_b0b.h"

#include "poolrad/replay/slot_a_saved_copy_1965.h"

static size_t linear(uint16_t seg, uint16_t off) {
    return ((size_t)seg << 4) + off;
}

static int span_ok(uint16_t seg, uint16_t off, size_t width) {
    size_t at = linear(seg, off);
    return (size_t)off + width <= 0x10000u &&
           at + width <= PR_1816_CALLER_RAM_SIZE;
}

static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off) {
    size_t at = linear(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8));
}

int pr_00ba_suffix_parent_b0b_step(
    unsigned site, uint8_t *ram, size_t ram_size,
    pr_1816_caller_cpu *cpu, pr_00ba_suffix_parent_b0b_report *report) {
    pr_slot_a_directory_state state;
    pr_slot_a_saved_copy_report native_report;
    uint16_t ret_ip, ret_cs, src_off, src_seg, dst_off, dst_seg;
    uint8_t length;
    if (site > 1u || ram == NULL || cpu == NULL || report == NULL ||
        ram_size != PR_1816_CALLER_RAM_SIZE) return 1;
    ret_ip = site == 0u ? 0x11F2u : 0x2ADEu;
    if (cpu->cs != 0x0AF8u || cpu->ip < 0x0B0Bu ||
        cpu->ip > 0x0B22u) return 2;
    if (!span_ok(cpu->ss, cpu->sp, 18u)) return 1;
    ret_cs = word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    src_off = word(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    src_seg = word(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
    dst_off = word(ram, cpu->ss, (uint16_t)(cpu->sp + 8u));
    dst_seg = word(ram, cpu->ss, (uint16_t)(cpu->sp + 10u));
    if (word(ram, cpu->ss, cpu->sp) != ret_ip ||
        src_off != (site == 0u ? 0x0EB2u : 0x2994u) ||
        src_seg != ret_cs || dst_seg != cpu->ss ||
        dst_off != (site == 0u ? (uint16_t)(cpu->bp - 0x47u) :
                                (uint16_t)(cpu->bp - 0x12Au)) ||
        !span_ok(src_seg, src_off, 1u)) return 1;
    length = ram[linear(src_seg, src_off)];
    if (!span_ok(src_seg, src_off, (size_t)length + 1u) ||
        !span_ok(dst_seg, dst_off, (size_t)length + 1u)) return 1;

    state = (pr_slot_a_directory_state){0};
    state.ram = ram;
    state.ram_size = ram_size;
    state.cpu.ax = cpu->ax;
    state.cpu.bx = cpu->bx;
    state.cpu.cx = cpu->cx;
    state.cpu.dx = cpu->dx;
    state.cpu.si = cpu->si;
    state.cpu.di = cpu->di;
    state.cpu.bp = cpu->bp;
    state.cpu.sp = cpu->sp;
    state.cpu.cs = 0x131Cu; /* Byte-identical loaded helper label. */
    state.cpu.ip = cpu->ip;
    state.cpu.ds = cpu->ds;
    state.cpu.es = cpu->es;
    state.cpu.ss = cpu->ss;
    state.cpu.flags = cpu->flags;
    if (pr_slot_a_saved_copy_1965_step(&state, &native_report) !=
        PR_SLOT_A_SAVED_COPY_STEPPED) return 2;

    report->before_ip = cpu->ip;
    report->after_ip = state.cpu.ip;
    report->source_length = length;
    report->returned_to_parent = state.cpu.cs != 0x131Cu;
    cpu->ax = state.cpu.ax;
    cpu->bx = state.cpu.bx;
    cpu->cx = state.cpu.cx;
    cpu->dx = state.cpu.dx;
    cpu->si = state.cpu.si;
    cpu->di = state.cpu.di;
    cpu->bp = state.cpu.bp;
    cpu->sp = state.cpu.sp;
    cpu->cs = state.cpu.cs == 0x131Cu ? 0x0AF8u : state.cpu.cs;
    cpu->ip = state.cpu.ip;
    cpu->ds = state.cpu.ds;
    cpu->es = state.cpu.es;
    cpu->ss = state.cpu.ss;
    cpu->flags = state.cpu.flags;
    return 0;
}

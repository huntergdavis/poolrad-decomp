#include "resident_00ba_1887_copy_return.h"

#include "poolrad/replay/slot_a_saved_copy_1965.h"

static size_t linear(uint16_t seg, uint16_t off) {
    return ((size_t)seg << 4) + off;
}

static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off) {
    size_t at = linear(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8));
}

static int span_ok(uint16_t seg, uint16_t off, size_t width) {
    size_t at = linear(seg, off);
    return (size_t)off + width <= 0x10000u &&
           at + width <= PR_1816_CALLER_RAM_SIZE;
}

int pr_1887_display_copy_return(uint8_t *ram, size_t ram_size,
                                pr_1816_caller_cpu *cpu) {
    pr_slot_a_directory_state state;
    pr_slot_a_saved_copy_report report;
    uint16_t src_off, src_seg, dst_off, dst_seg;
    size_t width;
    unsigned step;
    if (ram == NULL || cpu == NULL || ram_size != PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0AF8u || cpu->ip != 0x0B0Bu ||
        !span_ok(cpu->ss, cpu->sp, 12u)) return 1;
    if (word(ram, cpu->ss, cpu->sp) != 0x1887u ||
        word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) != 0x00BAu ||
        word(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) != 0x1813u ||
        word(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) != 0x00BAu ||
        word(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)) !=
            (uint16_t)(cpu->bp - 4u) ||
        word(ram, cpu->ss, (uint16_t)(cpu->sp + 10u)) != cpu->ss)
        return 1;
    src_off = word(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    src_seg = word(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
    dst_off = word(ram, cpu->ss, (uint16_t)(cpu->sp + 8u));
    dst_seg = word(ram, cpu->ss, (uint16_t)(cpu->sp + 10u));
    if (!span_ok(src_seg, src_off, 1u)) return 1;
    width = (size_t)ram[linear(src_seg, src_off)] + 1u;
    if (!span_ok(src_seg, src_off, width) ||
        !span_ok(dst_seg, dst_off, width)) return 1;

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
    /* The saved-game adapter names the byte-identical helper 131C:0B0B.
     * Its instructions never read CS; the stack holds the real 00BA return. */
    state.cpu.cs = 0x131Cu;
    state.cpu.ip = cpu->ip;
    state.cpu.ds = cpu->ds;
    state.cpu.es = cpu->es;
    state.cpu.ss = cpu->ss;
    state.cpu.flags = cpu->flags;
    for (step = 0u; step < 12u; ++step) {
        if (pr_slot_a_saved_copy_1965_step(&state, &report) !=
            PR_SLOT_A_SAVED_COPY_STEPPED) return 2;
    }
    if (state.cpu.cs != 0x00BAu || state.cpu.ip != 0x1887u) return 2;
    cpu->ax = state.cpu.ax;
    cpu->bx = state.cpu.bx;
    cpu->cx = state.cpu.cx;
    cpu->dx = state.cpu.dx;
    cpu->si = state.cpu.si;
    cpu->di = state.cpu.di;
    cpu->bp = state.cpu.bp;
    cpu->sp = state.cpu.sp;
    cpu->cs = state.cpu.cs;
    cpu->ip = state.cpu.ip;
    cpu->ds = state.cpu.ds;
    cpu->es = state.cpu.es;
    cpu->ss = state.cpu.ss;
    cpu->flags = state.cpu.flags;
    return 0;
}

#include "resident_0709_0626_postcopy_control.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

static size_t addr(uint16_t seg, uint16_t off) {
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off) {
    return ram[addr(seg, off)];
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off) {
    size_t at = addr(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t seg, uint16_t off,
                   uint8_t value) {
    ram[addr(seg, off)] = value;
}

static uint16_t cmp8_flags(uint16_t old, uint8_t lhs, uint8_t rhs) {
    uint8_t result = (uint8_t)(lhs - rhs);
    uint8_t parity = result;
    uint16_t bits = 0;
    unsigned shift;
    if (lhs < rhs) bits |= 0x0001u;
    if (((lhs ^ rhs ^ result) & 0x10u) != 0u) bits |= 0x0010u;
    if (result == 0u) bits |= 0x0040u;
    if ((result & 0x80u) != 0u) bits |= 0x0080u;
    if (((lhs ^ rhs) & (lhs ^ result) & 0x80u) != 0u) bits |= 0x0800u;
    for (shift = 0u; shift < 4u; ++shift)
        parity ^= (uint8_t)(parity >> (1u << shift));
    if ((parity & 1u) == 0u) bits |= 0x0004u;
    return (uint16_t)((old & (uint16_t)~0x08D5u) | bits);
}

int pr_0626_runtime_copy_return(uint8_t *ram, size_t ram_size,
                                 pr_1816_caller_cpu *cpu) {
    uint16_t entry_sp, source_off, source_seg, dest_off, dest_seg;
    uint16_t original_ds, limit;
    uint8_t length, copied;
    unsigned i;
    if (ram == NULL || cpu == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0AF8u || cpu->ip != 0x0B25u) return 1;

    /* 0B25..0B46: CLD, load the two far pointers and length cap, then
     * copy the Pascal length and exactly min(source length, cap) bytes. */
    cpu->flags = (uint16_t)(cpu->flags & (uint16_t)~0x0400u);
    entry_sp = cpu->sp;
    cpu->bx = entry_sp;
    original_ds = cpu->ds;
    cpu->dx = original_ds;
    source_off = read16(ram, cpu->ss, (uint16_t)(entry_sp + 0x0Au));
    source_seg = read16(ram, cpu->ss, (uint16_t)(entry_sp + 0x0Cu));
    cpu->si = source_off;
    cpu->ds = source_seg;
    dest_off = read16(ram, cpu->ss, (uint16_t)(entry_sp + 6u));
    dest_seg = read16(ram, cpu->ss, (uint16_t)(entry_sp + 8u));
    cpu->di = dest_off;
    cpu->es = dest_seg;
    limit = read16(ram, cpu->ss, (uint16_t)(entry_sp + 4u));
    cpu->cx = limit;
    length = read8(ram, cpu->ds, cpu->si);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | length);
    cpu->si = (uint16_t)(cpu->si + 1u);
    cpu->flags = cmp8_flags(cpu->flags, length, (uint8_t)limit);
    copied = length <= (uint8_t)limit ? length : (uint8_t)limit;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | copied);
    write8(ram, cpu->es, cpu->di, copied);
    cpu->di = (uint16_t)(cpu->di + 1u);
    cpu->cx = copied;
    /* XOR CH,CH yields zero, clearing CF/OF/SF and setting ZF/PF. AF is
     * architecturally undefined and is not compared by the differential. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08C5u) | 0x0044u);
    for (i = 0u; i < copied; ++i) {
        uint8_t value = read8(ram, cpu->ds, cpu->si);
        write8(ram, cpu->es, cpu->di, value);
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->di = (uint16_t)(cpu->di + 1u);
    }
    cpu->cx = 0u;
    cpu->ds = original_ds;
    cpu->ip = read16(ram, cpu->ss, entry_sp);
    cpu->cs = read16(ram, cpu->ss, (uint16_t)(entry_sp + 2u));
    cpu->sp = (uint16_t)(entry_sp + 0x0Eu);
    if (cpu->cs != 0x0709u || cpu->ip != 0x0641u) return 2;
    return 0;
}

int pr_0626_postcopy_guard(uint8_t *ram, size_t ram_size,
                            pr_1816_caller_cpu *cpu,
                            pr_0626_postcopy_stop *stop) {
    uint8_t col;
    if (ram == NULL || cpu == NULL || stop == NULL ||
        ram_size < PR_1816_CALLER_RAM_SIZE ||
        cpu->cs != 0x0709u || cpu->ip != 0x0641u) return 1;
    *stop = 0;
    /* 0641..064F: two unsigned byte guards; no unsupported glyph effect
     * is reached. Either stop at 064F or execute 06A1..06A7 RETF 10. */
    col = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));
    cpu->flags = cmp8_flags(cpu->flags, col, 0x27u);
    if (col <= 0x27u) {
        uint8_t row = read8(ram, cpu->ss,
                            (uint16_t)(cpu->bp + 0x0Cu));
        cpu->flags = cmp8_flags(cpu->flags, row, 0x27u);
        if (row <= 0x27u) {
            cpu->ip = 0x064Fu;
            *stop = PR_0626_POSTCOPY_VALID_AT_064F;
            return 0;
        }
    }
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->cs = read16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 0x0Eu);
    *stop = PR_0626_POSTCOPY_PRINTER_RETURN;
    return 0;
}

int pr_0626_copy_and_guard(uint8_t *ram, size_t ram_size,
                            pr_1816_caller_cpu *cpu,
                            pr_0626_postcopy_stop *stop) {
    int status = pr_0626_runtime_copy_return(ram, ram_size, cpu);
    return status ? status : pr_0626_postcopy_guard(ram, ram_size,
                                                    cpu, stop);
}

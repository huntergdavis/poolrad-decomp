/* START.EXE resident 00BA:[0973,09C9) raw-CPU model; docs/re/RESIDENT_FIELD_0973.md. */
#include "poolrad/recovered/resident_00ba_0973_cpu.h"
#include "poolrad/recovered/resident_00ba_byte_string_1037_cpu.h"

#define MASK (PR_RES_0973_RAM_SIZE - 1u)

#define COPY_CPU(to, from)                                                                                   \
    do {                                                                                                     \
        (to)->ax = (from)->ax; (to)->bx = (from)->bx; (to)->cx = (from)->cx; (to)->dx = (from)->dx;          \
        (to)->si = (from)->si; (to)->di = (from)->di; (to)->bp = (from)->bp; (to)->sp = (from)->sp;          \
        (to)->cs = (from)->cs; (to)->ip = (from)->ip; (to)->ds = (from)->ds; (to)->es = (from)->es;          \
        (to)->ss = (from)->ss; (to)->flags = (from)->flags;                                                  \
    } while (0)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8_at(const uint8_t *ram, size_t at)
{
    return ram[at];
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void push16(uint8_t *ram, pr_res_0973_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void note(pr_res_0973_diagnostic *d, uint16_t site, int callee, int status)
{
    if (d == NULL) return;
    d->site = site;
    d->callee = callee;
    d->callee_status = status;
}

pr_res_0973_status pr_res_00ba_0973_cpu(pr_bus *bus, size_t ram_size, pr_res_0973_cpu *cpu,
                                        const pr_res_0973_env *env, pr_res_0973_diagnostic *diagnostic)
{
    pr_res_1037_cpu fmt;
    pr_res_1037_callee fmt_callee;
    pr_r0709_printer_cpu printer;
    pr_r0709_printer_cpu_status printer_status;
    pr_res_0973_env defaults = {65536u, 65536u};
    uint16_t node_off, node_seg, style, destination;
    uint8_t first, threshold, number;
    int fmt_status;

    if (ram_size != PR_RES_0973_RAM_SIZE || cpu == NULL || cpu->ip != 0x0973u)
        return PR_RES_0973_ARGUMENT;
    if (bus == NULL || bus->ram == NULL) return PR_RES_0973_ARGUMENT;
    if (diagnostic != NULL) {
        diagnostic->site = 0;
        diagnostic->callee = -1;
        diagnostic->callee_status = -1;
    }
    if (env == NULL) env = &defaults;

    push16(bus->ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x0102u);

    node_off = read16(bus->ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));
    node_seg = read16(bus->ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));
    first = read8_at(bus->ram, physical(node_seg, (uint16_t)(node_off + 0x011Bu)));
    threshold = read8_at(bus->ram, physical(node_seg, (uint16_t)(node_off + 0x0032u)));
    style = first < threshold ? 0x0Eu : 0x0Au;
    if (read8_at(bus->ram, physical(cpu->ss, (uint16_t)(cpu->bp + 6u))) != 0u)
        style = 0x0Du;
    write8(bus->ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)style);

    cpu->ax = read8_at(bus->ram, physical(cpu->ss, (uint16_t)(cpu->bp + 0x0Au)));
    push16(bus->ram, cpu, cpu->ax);
    cpu->ax = read8_at(bus->ram, physical(cpu->ss, (uint16_t)(cpu->bp + 8u)));
    push16(bus->ram, cpu, cpu->ax);
    cpu->ax = style;
    push16(bus->ram, cpu, cpu->ax);
    destination = (uint16_t)(cpu->bp - 0x0102u);
    push16(bus->ram, cpu, cpu->ss);
    push16(bus->ram, cpu, destination);
    number = read8_at(bus->ram, physical(node_seg, (uint16_t)(node_off + 0x011Bu)));
    cpu->ax = number;
    push16(bus->ram, cpu, cpu->ax);
    if (bus->ram[physical(cpu->cs, 0x09BBu)] != 0xE8u || read16(bus->ram, cpu->cs, 0x09BCu) != 0x0679u)
        return PR_RES_0973_CODE;
    push16(bus->ram, cpu, cpu->cs);
    push16(bus->ram, cpu, 0x09BEu);
    fmt.ax = cpu->ax; fmt.bx = cpu->bx; fmt.cx = cpu->cx; fmt.dx = cpu->dx;
    fmt.si = cpu->si; fmt.di = cpu->di; fmt.bp = cpu->bp; fmt.sp = cpu->sp;
    fmt.ds = cpu->ds; fmt.es = cpu->es; fmt.ss = cpu->ss; fmt.flags = cpu->flags;
    fmt.cs = cpu->cs;
    fmt.ip = 0x1037u;
    fmt_status = (int)pr_res_00ba_1037_cpu(bus->ram, ram_size, &fmt, &fmt_callee, NULL);
    cpu->ax = fmt.ax; cpu->bx = fmt.bx; cpu->cx = fmt.cx; cpu->dx = fmt.dx;
    cpu->si = fmt.si; cpu->di = fmt.di; cpu->bp = fmt.bp; cpu->sp = fmt.sp;
    cpu->cs = fmt.cs; cpu->ip = fmt.ip; cpu->ds = fmt.ds; cpu->es = fmt.es;
    cpu->ss = fmt.ss; cpu->flags = fmt.flags;
    note(diagnostic, 0x09BBu, 0, fmt_status);
    if (fmt_status != (int)PR_RES_1037_OK) {
        if (fmt_status == (int)PR_RES_1037_ARGUMENT) return PR_RES_0973_ARGUMENT;
        if (fmt_status == (int)PR_RES_1037_CODE) return PR_RES_0973_CODE;
        if (fmt_status == (int)PR_RES_1037_TRANSFER) return PR_RES_0973_TRANSFER;
        return PR_RES_0973_CALLEE;
    }
    if (cpu->ip != 0x09BEu || cpu->cs != 0x00BAu)
        return PR_RES_0973_TRANSFER;

    cpu->ip = 0x09BEu;
    if (bus->ram[physical(cpu->cs, 0x09BEu)] != 0x9Au || read16(bus->ram, cpu->cs, 0x09BFu) != 0x0626u)
        return PR_RES_0973_CODE;
    if (read16(bus->ram, cpu->cs, 0x09C1u) != 0x0709u)
        return PR_RES_0973_CODE;
    push16(bus->ram, cpu, cpu->cs);
    push16(bus->ram, cpu, 0x09C3u);
    COPY_CPU(&printer, cpu);
    printer.cs = 0x0709u;
    printer.ip = 0x0626u;
    printer_status = pr_r0709_printer_0626_cpu(bus, ram_size, &printer, env->printer_loopbacks,
                                               env->printer_glyph_loopbacks,
                                               diagnostic != NULL ? &diagnostic->printer : NULL);
    COPY_CPU(cpu, &printer);
    note(diagnostic, 0x09BEu, 1, (int)printer_status);
    if (printer_status != PR_R0709_PRINTER_CPU_OK)
        return PR_RES_0973_CALLEE;
    if (cpu->ip != 0x09C3u || cpu->cs != 0x00BAu)
        return PR_RES_0973_TRANSFER;

    cpu->sp = cpu->bp;
    cpu->bp = read16_at(bus->ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16_at(bus->ram, physical(cpu->ss, cpu->sp));
    cpu->cs = read16_at(bus->ram, physical(cpu->ss, (uint16_t)(cpu->sp + 2u)));
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Au);
    return PR_RES_0973_RETURNED;
}

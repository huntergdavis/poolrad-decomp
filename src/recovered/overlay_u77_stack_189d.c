/* GAME.OVR unit 0x77:+189D..+1A76; +1A77..+1A89 is text data. */
#include "poolrad/recovered/overlay_u77_stack_189d.h"

#define RAM_MASK (PR_U77_189D_RAM_SIZE - 1u)
#define LOGIC_FLAGS 0x08C5u /* OF SF ZF PF CF; AF undefined */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
}

static uint16_t read16_at(const uint8_t *ram, size_t address)
{
    return (uint16_t)(ram[address] |
                      ((uint16_t)ram[(address + 1u) & RAM_MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return read16_at(ram, physical(seg, off));
}

static void write8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u77_189d_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u77_189d_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint8_t local8(const uint8_t *ram, const pr_u77_189d_cpu *cpu,
                      uint16_t distance)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local8(uint8_t *ram, const pr_u77_189d_cpu *cpu,
                       uint16_t distance, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static uint16_t local16(const uint8_t *ram, const pr_u77_189d_cpu *cpu,
                        uint16_t distance)
{
    return read16(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local16(uint8_t *ram, const pr_u77_189d_cpu *cpu,
                        uint16_t distance, uint16_t value)
{
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static void les_local(const uint8_t *ram, pr_u77_189d_cpu *cpu,
                      uint16_t distance)
{
    cpu->di = local16(ram, cpu, distance);
    cpu->es = local16(ram, cpu, (uint16_t)(distance - 2u));
}

static void les_active(const uint8_t *ram, pr_u77_189d_cpu *cpu)
{
    size_t a = physical(cpu->ds, 0x5D92u);
    cpu->di = read16_at(ram, a);
    cpu->es = read16_at(ram, (a + 2u) & RAM_MASK);
}

static void set_al(pr_u77_189d_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void set_logic_flags(pr_u77_189d_cpu *cpu, uint16_t value)
{
    uint8_t parity = (uint8_t)value;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    cpu->flags &= (uint16_t)~(LOGIC_FLAGS | 0x10u); /* pinned: OR/AND/XOR clear AF */
    if (value & 0x8000u) cpu->flags |= 0x0080u;
    if (value == 0u) cpu->flags |= 0x0040u;
    if (!(parity & 1u)) cpu->flags |= 0x0004u;
}

/* The compare blocks load current then target on every field, even where
 * their values were previously read. The two self-comparisons pass current
 * as both arguments, exactly as the source does. */
static int compare_byte(const uint8_t *ram, pr_u77_189d_cpu *cpu,
                        uint16_t field, int self)
{
    uint8_t left, right;
    les_local(ram, cpu, 4u);
    left = read8(ram, cpu->es, (uint16_t)(cpu->di + field));
    set_al(cpu, left);
    les_local(ram, cpu, self ? 4u : 12u);
    right = read8(ram, cpu->es, (uint16_t)(cpu->di + field));
    return left == right;
}

static int compare_word(const uint8_t *ram, pr_u77_189d_cpu *cpu,
                        uint16_t field)
{
    uint16_t left, right;
    les_local(ram, cpu, 4u);
    left = read16(ram, cpu->es, (uint16_t)(cpu->di + field));
    cpu->ax = left;
    les_local(ram, cpu, 12u);
    right = read16(ram, cpu->es, (uint16_t)(cpu->di + field));
    return left == right;
}

pr_u77_189d_status pr_u77_stack_189d(
    uint8_t *ram, size_t ram_size, pr_u77_189d_cpu *cpu,
    pr_u77_189d_remove remove_node, void *context, size_t max_nodes)
{
    size_t visited = 0;
    if (ram == NULL || cpu == NULL || remove_node == NULL ||
        ram_size < PR_U77_189D_RAM_SIZE || max_nodes == 0u)
        return PR_U77_189D_ARGUMENT;

    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 14u);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    set_local16(ram, cpu, 12u, cpu->ax);
    set_local16(ram, cpu, 10u, cpu->dx);
    les_local(ram, cpu, 12u);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u)));
    set_local8(ram, cpu, 13u, (uint8_t)cpu->ax);
    les_active(ram, cpu);
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xC8u));
    cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCAu));
    set_local16(ram, cpu, 4u, cpu->ax);
    set_local16(ram, cpu, 2u, cpu->dx);

    for (;;) {
        uint16_t current_off, current_seg;
        uint16_t next_off, next_seg;
        uint8_t current_count;
        uint16_t sum;
        cpu->ax = local16(ram, cpu, 4u);
        cpu->ax = (uint16_t)(cpu->ax | local16(ram, cpu, 2u));
        set_logic_flags(cpu, cpu->ax);
        if (cpu->ax == 0u) break;
        if (visited++ == max_nodes) return PR_U77_189D_LIMIT;
        les_local(ram, cpu, 4u);
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
        set_local16(ram, cpu, 8u, cpu->ax);
        set_local16(ram, cpu, 6u, cpu->dx);
        current_off = local16(ram, cpu, 4u);
        current_seg = local16(ram, cpu, 2u);
        next_off = local16(ram, cpu, 8u);
        next_seg = local16(ram, cpu, 6u);
        cpu->ax = current_off;
        cpu->dx = current_seg;
        if (current_seg == local16(ram, cpu, 10u) &&
            current_off == local16(ram, cpu, 12u)) goto advance;

        les_local(ram, cpu, 4u);
        if (read8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u)) == 0u)
            goto advance;
        if (!compare_byte(ram, cpu, 0x2Fu, 0) ||
            !compare_byte(ram, cpu, 0x30u, 0) ||
            !compare_byte(ram, cpu, 0x31u, 0) ||
            !compare_byte(ram, cpu, 0x2Eu, 0) ||
            !compare_byte(ram, cpu, 0x32u, 0) ||
            !compare_byte(ram, cpu, 0x33u, 0) ||
            !compare_byte(ram, cpu, 0x36u, 0) ||
            !compare_word(ram, cpu, 0x37u) ||
            !compare_byte(ram, cpu, 0x3Cu, 1))
            goto advance;
        les_local(ram, cpu, 4u);
        if (read8(ram, cpu->es, (uint16_t)(cpu->di + 0x3Cu)) >= 2u)
            goto advance;
        if (!compare_byte(ram, cpu, 0x3Du, 1) ||
            !compare_byte(ram, cpu, 0x3Eu, 1))
            goto advance;

        les_local(ram, cpu, 4u);
        current_count = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u));
        cpu->ax = current_count;
        cpu->dx = cpu->ax;
        cpu->ax = local8(ram, cpu, 13u);
        sum = (uint16_t)(cpu->ax + cpu->dx);
        cpu->ax = sum;
        if (sum <= 0xFFu) {
            les_local(ram, cpu, 4u);
            set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u)));
            set_local8(ram, cpu, 13u,
                       (uint8_t)(local8(ram, cpu, 13u) + (uint8_t)cpu->ax));
            push16(ram, cpu, read16(ram, cpu->ds, 0x5D94u));
            push16(ram, cpu, read16(ram, cpu->ds, 0x5D92u));
            push16(ram, cpu, current_seg);
            push16(ram, cpu, current_off);
            if (!remove_node(context, ram, ram_size, cpu, current_off,
                             current_seg, read16(ram, cpu->ds, 0x5D92u),
                             read16(ram, cpu->ds, 0x5D94u)))
                return PR_U77_189D_REMOVE_ABORT;
            cpu->sp = (uint16_t)(cpu->sp + 8u);
        } else {
            uint16_t room, residual;
            les_local(ram, cpu, 12u);
            write8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u), 0xFFu);
            cpu->ax = local8(ram, cpu, 13u);
            cpu->dx = cpu->ax;
            cpu->ax = (uint16_t)(0xFFu - cpu->dx);
            room = cpu->ax;
            cpu->dx = room;
            les_local(ram, cpu, 4u);
            current_count = read8(ram, cpu->es,
                                  (uint16_t)(cpu->di + 0x39u));
            cpu->ax = (uint16_t)(current_count - cpu->dx);
            residual = cpu->ax;
            les_local(ram, cpu, 4u);
            write8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u),
                   (uint8_t)residual);
            les_local(ram, cpu, 4u);
            set_al(cpu, read8(ram, cpu->es,
                              (uint16_t)(cpu->di + 0x39u)));
            set_local8(ram, cpu, 13u, (uint8_t)cpu->ax);
            cpu->ax = current_off;
            cpu->dx = current_seg;
            set_local16(ram, cpu, 12u, cpu->ax);
            set_local16(ram, cpu, 10u, cpu->dx);
        }

advance:
        cpu->ax = next_off;
        cpu->dx = next_seg;
        set_local16(ram, cpu, 4u, cpu->ax);
        set_local16(ram, cpu, 2u, cpu->dx);
    }

    set_al(cpu, local8(ram, cpu, 13u));
    les_local(ram, cpu, 12u);
    write8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u), (uint8_t)cpu->ax);
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U77_189D_OK;
}

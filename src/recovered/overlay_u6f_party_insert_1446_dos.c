#include "poolrad/recovered/overlay_u6f_party_insert_1446_dos.h"

#define PHYSICAL_MASK (PR_U6F_INSERT_DOS_RAM_SIZE - 1u)

static size_t physical(pr_u6f_insert_far p, unsigned displacement)
{
    uint16_t effective = (uint16_t)(p.offset + displacement);
    return (((size_t)p.segment << 4) + effective) & PHYSICAL_MASK;
}

static uint8_t read_byte(const uint8_t *ram, pr_u6f_insert_far p,
                         unsigned displacement)
{
    return ram[physical(p, displacement)];
}

static void write_byte(uint8_t *ram, pr_u6f_insert_far p,
                       unsigned displacement, uint8_t value)
{
    ram[physical(p, displacement)] = value;
}

static uint16_t read_word(const uint8_t *ram, pr_u6f_insert_far p,
                          unsigned displacement)
{
    size_t address = physical(p, displacement);
    /* A word crosses a 16-bit offset boundary at the next physical byte,
     * not at segment:0000. The bus then wraps at the 20-bit boundary. */
    return (uint16_t)((uint16_t)ram[address] |
                      (uint16_t)((uint16_t)ram[(address + 1u) & PHYSICAL_MASK] << 8));
}

static void write_word(uint8_t *ram, pr_u6f_insert_far p,
                       unsigned displacement, uint16_t value)
{
    size_t address = physical(p, displacement);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & PHYSICAL_MASK] = (uint8_t)(value >> 8);
}

static pr_u6f_insert_far read_far(const uint8_t *ram, uint16_t segment,
                                  uint16_t offset)
{
    pr_u6f_insert_far base = {offset, segment};
    pr_u6f_insert_far value;
    value.offset = read_word(ram, base, 0u);
    value.segment = read_word(ram, base, 2u);
    return value;
}

static void write_far(uint8_t *ram, uint16_t segment, uint16_t offset,
                      pr_u6f_insert_far value)
{
    pr_u6f_insert_far base = {offset, segment};
    write_word(ram, base, 0u, value.offset);
    write_word(ram, base, 2u, value.segment);
}

static int is_null(pr_u6f_insert_far p)
{
    return (uint16_t)(p.offset | p.segment) == 0u;
}

static void run_u9b(uint8_t *ram, pr_u6f_insert_far record,
                    pr_u9b_incr_report *report)
{
    static const struct { uint8_t above, counter; } steps[PR_U9B_INCR_STEPS] = {
        {0x0cu, PR_U9B_INCR_B2}, {0x0du, PR_U9B_INCR_B2},
        {0x0eu, PR_U9B_INCR_B3}, {0x0fu, PR_U9B_INCR_B3},
        {0x10u, PR_U9B_INCR_B4}
    };
    unsigned i;
    for (i = 0u; i < PR_U9B_INCR_STEPS; ++i) {
        uint8_t counter;
        if (read_byte(ram, record, PR_U9B_INCR_T) <= steps[i].above)
            continue;
        counter = read_byte(ram, record, steps[i].counter);
        if (counter == 0u)
            continue;
        write_byte(ram, record, steps[i].counter, (uint8_t)(counter + 1u));
        ++report->increments;
        report->steps_taken |= 1u << i;
    }
}

pr_u6f_insert_dos_status pr_u6f_party_insert_1446_dos(
    const pr_u6f_insert_dos_memory *memory, pr_u6f_insert_far incoming,
    pr_u6f_insert_dos_report *report)
{
    uint8_t *ram;
    uint16_t ds;
    pr_u6f_insert_far cursor, next, selected, window;
    pr_u6f_insert_dos_report result = {0};
    uint8_t used[8] = {0};
    uint16_t head_offset, head_segment, count;

    if (!memory || !memory->ram || memory->ram_size < PR_U6F_INSERT_DOS_RAM_SIZE ||
        !report)
        return PR_U6F_INSERT_DOS_ARGUMENT;
    ram = memory->ram;
    ds = memory->ds;

    /* +0x144F precedes the head check. This byte may alias a DS global. */
    write_byte(ram, incoming, 0xbfu, 0xffu);
    head_offset = read_word(ram, (pr_u6f_insert_far){0x5d96u, ds}, 0u);
    head_segment = read_word(ram, (pr_u6f_insert_far){0x5d98u, ds}, 0u);
    if ((uint16_t)(head_offset | head_segment) == 0u) {
        /* +0x1464..+0x1474: four ordered word stores. */
        write_far(ram, ds, 0x5d96u, incoming);
        write_far(ram, ds, 0x5d92u, incoming);
    } else {
        /* +0x147A rereads the head. The +0x148A/+0x148F test reads
         * precede the +0x1499/+0x149E reads used to advance. */
        cursor = read_far(ram, ds, 0x5d96u);
        for (;;) {
            uint16_t link_offset = read_word(ram, cursor, 0x104u);
            uint16_t link_segment = read_word(ram, cursor, 0x106u);
            ++result.append_nodes;
            if ((uint16_t)(link_offset | link_segment) == 0u)
                break;
            next.offset = read_word(ram, cursor, 0x104u);
            next.segment = read_word(ram, cursor, 0x106u);
            cursor = next;
        }
        write_word(ram, cursor, 0x104u, incoming.offset);
        write_word(ram, cursor, 0x106u, incoming.segment);
    }

    /* +0x14CA FillChar writes eight zero bytes into the local used array.
     * +0x14F2 membership tests only the pushed low slot byte, then the
     * source rereads that byte at +0x14FC before marking used[index]. */
    cursor = read_far(ram, ds, 0x5d96u);
    while (!is_null(cursor)) {
        uint8_t index = read_byte(ram, cursor, 0xbfu);
        ++result.mark_nodes;
        if (index < 8u)
            used[read_byte(ram, cursor, 0xbfu)] = 1u;
        next.offset = read_word(ram, cursor, 0x104u);
        next.segment = read_word(ram, cursor, 0x106u);
        cursor = next;
    }

    /* +0x1524/+0x1527 store the active pointer, then +0x152B rereads
     * it into a local far pointer before changing the record slot. */
    write_far(ram, ds, 0x5d92u, incoming);
    selected = read_far(ram, ds, 0x5d92u);
    write_byte(ram, selected, 0xbfu, 0u);
    for (;;) {
        uint8_t index = read_byte(ram, selected, 0xbfu);
        if (index >= 8u || used[index] == 0u)
            break;
        write_byte(ram, selected, 0xbfu, (uint8_t)(index + 1u));
    }
    result.selected_slot = read_byte(ram, selected, 0xbfu);

    /* +0x1565 reads the window pointer after every earlier record/global
     * write. INC word is a read followed by a 20-bit physical word store. */
    window = read_far(ram, ds, 0x49d6u);
    count = read_word(ram, window, 0x67cu);
    result.window_word = (uint16_t)(count + 1u);
    write_word(ram, window, 0x67cu, result.window_word);

    /* +0x1574 dispatches to the merged u9B:+0x01AF. Use the same five
     * conditions and re-reads over physical RAM so aliases remain visible. */
    run_u9b(ram, incoming, &result.callee);
    *report = result;
    return PR_U6F_INSERT_DOS_OK;
}

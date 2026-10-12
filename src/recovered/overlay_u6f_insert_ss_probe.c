#include "poolrad/recovered/overlay_u6f_insert_ss_probe.h"

#define RAM_MASK (PR_U6F_INSERT_DOS_RAM_SIZE - 1u)

static size_t address(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t load8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[address(segment, offset)];
}

static void store8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[address(segment, offset)] = value;
}

static uint16_t load16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = address(segment, offset);
    return (uint16_t)((uint16_t)ram[a] |
                      (uint16_t)((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void store16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t a = address(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static pr_u6f_insert_far load_far(const uint8_t *ram, uint16_t segment,
                                  uint16_t offset)
{
    pr_u6f_insert_far value;
    value.offset = load16(ram, segment, offset);
    value.segment = load16(ram, segment, (uint16_t)(offset + 2u));
    return value;
}

static void store_far(uint8_t *ram, uint16_t segment, uint16_t offset,
                      pr_u6f_insert_far value)
{
    store16(ram, segment, offset, value.offset);
    store16(ram, segment, (uint16_t)(offset + 2u), value.segment);
}

static uint16_t member_offset(uint16_t bp, uint8_t index)
{
    return (uint16_t)(bp - 12u + index);
}

static int is_null(pr_u6f_insert_far value)
{
    return (uint16_t)(value.offset | value.segment) == 0u;
}

pr_u6f_ss_probe_status pr_u6f_insert_ss_two_node_probe(
    const pr_u6f_insert_dos_memory *memory, uint16_t ss, uint16_t bp,
    pr_u6f_ss_probe_report *report)
{
    uint8_t *ram;
    uint16_t ds;
    pr_u6f_insert_far incoming, head, cursor, link, selected;
    pr_u6f_ss_probe_report result = {0};
    unsigned i;

    if (!memory || !memory->ram ||
        memory->ram_size < PR_U6F_INSERT_DOS_RAM_SIZE || !report)
        return PR_U6F_SS_PROBE_ARGUMENT;
    ram = memory->ram;
    ds = memory->ds;

    /* +0x144C/+0x144F: far argument and first record-slot write. */
    incoming = load_far(ram, ss, (uint16_t)(bp + 6u));
    store8(ram, incoming.segment, (uint16_t)(incoming.offset + 0xbfu), 0xffu);

    /* +0x1455..+0x14B9: one existing head, followed by incoming. */
    head = load_far(ram, ds, 0x5d96u);
    if (is_null(head))
        return PR_U6F_SS_PROBE_OTHER_PATH;
    /* +0x147A re-reads the head; +0x1481/+0x1484 save the cursor in SS. */
    cursor = load_far(ram, ds, 0x5d96u);
    store_far(ram, ss, (uint16_t)(bp - 4u), cursor);
    cursor = load_far(ram, ss, (uint16_t)(bp - 4u));
    link = load_far(ram, cursor.segment, (uint16_t)(cursor.offset + 0x104u));
    if (!is_null(link))
        return PR_U6F_SS_PROBE_OTHER_PATH;
    incoming = load_far(ram, ss, (uint16_t)(bp + 6u));
    cursor = load_far(ram, ss, (uint16_t)(bp - 4u));
    store_far(ram, cursor.segment, (uint16_t)(cursor.offset + 0x104u), incoming);

    /* +0x14BE..+0x14CA: the proved FillChar result is in SS, not a C local. */
    for (i = 0u; i < 8u; ++i)
        store8(ram, ss, member_offset(bp, (uint8_t)i), 0u);

    /* +0x14CF..+0x151C: mark the existing node, then the incoming node.
     * Each cursor and slot read is repeated at its original source site. */
    cursor = load_far(ram, ds, 0x5d96u);
    store_far(ram, ss, (uint16_t)(bp - 4u), cursor);
    for (i = 0u; i < 2u; ++i) {
        uint8_t index;
        cursor = load_far(ram, ss, (uint16_t)(bp - 4u));
        if (is_null(cursor))
            return PR_U6F_SS_PROBE_OTHER_PATH;
        cursor = load_far(ram, ss, (uint16_t)(bp - 4u));
        index = load8(ram, cursor.segment,
                      (uint16_t)(cursor.offset + 0xbfu));
        if (index < 8u) {
            cursor = load_far(ram, ss, (uint16_t)(bp - 4u));
            index = load8(ram, cursor.segment,
                          (uint16_t)(cursor.offset + 0xbfu));
            store8(ram, ss, member_offset(bp, index), 1u);
            if (i == 0u)
                result.first_marked = 1u;
            else
                result.incoming_marked = 1u;
        }
        cursor = load_far(ram, ss, (uint16_t)(bp - 4u));
        link = load_far(ram, cursor.segment,
                        (uint16_t)(cursor.offset + 0x104u));
        store_far(ram, ss, (uint16_t)(bp - 4u), link);
        if (i == 0u) {
            if (link.offset != incoming.offset ||
                link.segment != incoming.segment)
                return PR_U6F_SS_PROBE_OTHER_PATH;
        } else if (!is_null(link)) {
            return PR_U6F_SS_PROBE_OTHER_PATH;
        }
        cursor = link;
    }

    /* +0x151E..+0x1538: re-read argument, store and re-read DS active,
     * write the SS selected pointer, then reset the selected record slot. */
    incoming = load_far(ram, ss, (uint16_t)(bp + 6u));
    store_far(ram, ds, 0x5d92u, incoming);
    selected = load_far(ram, ds, 0x5d92u);
    store_far(ram, ss, (uint16_t)(bp - 0x10u), selected);
    selected = load_far(ram, ss, (uint16_t)(bp - 0x10u));
    store8(ram, selected.segment, (uint16_t)(selected.offset + 0xbfu), 0u);

    /* +0x153E..+0x1563: re-read the SS pointer for each LES and test the
     * actual SS membership byte after the reset. The host cap is only a
     * guard for inputs outside the demonstrated terminating path. */
    for (i = 0u; i < 256u; ++i) {
        uint8_t index;
        selected = load_far(ram, ss, (uint16_t)(bp - 0x10u));
        index = load8(ram, selected.segment,
                      (uint16_t)(selected.offset + 0xbfu));
        if (index >= 8u)
            break;
        selected = load_far(ram, ss, (uint16_t)(bp - 0x10u));
        index = load8(ram, selected.segment,
                      (uint16_t)(selected.offset + 0xbfu));
        if (load8(ram, ss, member_offset(bp, index)) == 0u)
            break;
        selected = load_far(ram, ss, (uint16_t)(bp - 0x10u));
        index = load8(ram, selected.segment,
                      (uint16_t)(selected.offset + 0xbfu));
        store8(ram, selected.segment, (uint16_t)(selected.offset + 0xbfu),
               (uint8_t)(index + 1u));
        ++result.increments;
    }
    if (i == 256u)
        return PR_U6F_SS_PROBE_LOOP_LIMIT;
    selected = load_far(ram, ss, (uint16_t)(bp - 0x10u));
    result.selected_slot = load8(ram, selected.segment,
                                 (uint16_t)(selected.offset + 0xbfu));
    *report = result;
    return PR_U6F_SS_PROBE_OK;
}

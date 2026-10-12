#include "poolrad/recovered/overlay_u6f_party_insert_1446_ss.h"

#define RAM_MASK (PR_U6F_INSERT_DOS_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t address = physical(segment, offset);
    return (uint16_t)((uint16_t)ram[address] |
                      (uint16_t)((uint16_t)ram[(address + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[(address + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static pr_u6f_insert_far read_far(const uint8_t *ram, uint16_t segment,
                                  uint16_t offset)
{
    pr_u6f_insert_far value;
    value.offset = read16(ram, segment, offset);
    value.segment = read16(ram, segment, (uint16_t)(offset + 2u));
    return value;
}

static uint16_t at(pr_u6f_insert_far value, unsigned displacement)
{
    return (uint16_t)(value.offset + displacement);
}

static void push16(uint8_t *ram, uint16_t ss, uint16_t *sp, uint16_t value)
{
    *sp = (uint16_t)(*sp - 2u);
    write16(ram, ss, *sp, value);
}

/* +0x14CA: caller pushes SS, DI, 8, 0; CALL FAR pushes CS and return IP.
 * Hash-pinned SYSTEM +0x16A8 reads [SP+8/+6/+4] and REP STOSB, RETF 8. */
static void fill_used(uint8_t *ram, uint16_t cs, uint16_t ss,
                      uint16_t bp, uint16_t *sp)
{
    pr_u6f_insert_far destination;
    uint16_t count, i;
    uint8_t value;
    push16(ram, ss, sp, ss);
    push16(ram, ss, sp, (uint16_t)(bp - 0xcu));
    push16(ram, ss, sp, 8u);
    push16(ram, ss, sp, 0u);
    push16(ram, ss, sp, cs);
    push16(ram, ss, sp, 0x14cfu);
    destination = read_far(ram, ss, (uint16_t)(*sp + 8u));
    count = read16(ram, ss, (uint16_t)(*sp + 6u));
    value = read8(ram, ss, (uint16_t)(*sp + 4u));
    for (i = 0u; i < count; ++i)
        write8(ram, destination.segment, at(destination, i), value);
    *sp = (uint16_t)(*sp + 12u);
}

/* +0x14F2: the caller's three arguments and CALL FAR write five words.
 * SYSTEM +0x0DC0 reads its far set pointer and low index byte, tests the
 * loaded CS set, then RETF 6. Its 33-byte body has no stack stores. */
static int in_set(uint8_t *ram, uint16_t cs, uint16_t ss,
                  uint16_t *sp, uint16_t ax)
{
    pr_u6f_insert_far set;
    uint8_t index, mask, value;
    push16(ram, ss, sp, ax);
    push16(ram, ss, sp, cs);
    push16(ram, ss, sp, 0x1426u);
    push16(ram, ss, sp, cs);
    push16(ram, ss, sp, 0x14f7u);
    set = read_far(ram, ss, (uint16_t)(*sp + 4u));
    index = read8(ram, ss, (uint16_t)(*sp + 8u));
    mask = (uint8_t)(1u << (index & 7u));
    value = read8(ram, set.segment, at(set, index >> 3));
    *sp = (uint16_t)(*sp + 10u);
    return (value & mask) != 0u;
}

pr_u6f_insert_ss_status pr_u6f_party_insert_1446_ss(
    const pr_u6f_insert_dos_memory *memory,
    const pr_u6f_insert_ss_frame *frame,
    pr_u6f_insert_ss_report *report)
{
    uint8_t *ram;
    uint16_t ds, ss, cs, bp, sp, ax, dx;
    pr_u6f_insert_far record, cursor, selected, window;
    pr_u6f_insert_ss_report result = {0};

    if (!memory || !memory->ram ||
        memory->ram_size < PR_U6F_INSERT_DOS_RAM_SIZE || !frame || !report)
        return PR_U6F_INSERT_SS_ARGUMENT;
    ram = memory->ram;
    ds = memory->ds;
    ss = frame->ss;
    cs = frame->cs;
    sp = frame->entry_sp;

    /* +0x1446..+0x1449: PUSH BP, MOV BP,SP, SUB SP,0x10. */
    push16(ram, ss, &sp, frame->caller_bp);
    bp = sp;
    sp = (uint16_t)(sp - 0x10u);
    result.callee_bp = bp;

    /* +0x144C/+0x144F: argument read and first slot write. */
    record = read_far(ram, ss, (uint16_t)(bp + 6u));
    write8(ram, record.segment, at(record, 0xbfu), 0xffu);
    ax = read16(ram, ds, 0x5d96u);
    if ((uint16_t)(ax | read16(ram, ds, 0x5d98u)) == 0u) {
        /* +0x145E..+0x1474: repeat stack-argument reads before each
         * ordered pair of DS word stores. */
        ax = read16(ram, ss, (uint16_t)(bp + 6u));
        dx = read16(ram, ss, (uint16_t)(bp + 8u));
        write16(ram, ds, 0x5d96u, ax);
        write16(ram, ds, 0x5d98u, dx);
        ax = read16(ram, ss, (uint16_t)(bp + 6u));
        dx = read16(ram, ss, (uint16_t)(bp + 8u));
        write16(ram, ds, 0x5d92u, ax);
        write16(ram, ds, 0x5d94u, dx);
    } else {
        /* +0x147A..+0x14B9: cursor is the four SS local bytes. The link
         * test precedes a second link read when a next node exists. */
        ax = read16(ram, ds, 0x5d96u);
        dx = read16(ram, ds, 0x5d98u);
        write16(ram, ss, (uint16_t)(bp - 4u), ax);
        write16(ram, ss, (uint16_t)(bp - 2u), dx);
        for (;;) {
            cursor = read_far(ram, ss, (uint16_t)(bp - 4u));
            ax = read16(ram, cursor.segment, at(cursor, 0x104u));
            ax = (uint16_t)(ax | read16(ram, cursor.segment,
                                        at(cursor, 0x106u)));
            ++result.append_nodes;
            if (ax == 0u)
                break;
            cursor = read_far(ram, ss, (uint16_t)(bp - 4u));
            ax = read16(ram, cursor.segment, at(cursor, 0x104u));
            dx = read16(ram, cursor.segment, at(cursor, 0x106u));
            write16(ram, ss, (uint16_t)(bp - 4u), ax);
            write16(ram, ss, (uint16_t)(bp - 2u), dx);
        }
        ax = read16(ram, ss, (uint16_t)(bp + 6u));
        dx = read16(ram, ss, (uint16_t)(bp + 8u));
        cursor = read_far(ram, ss, (uint16_t)(bp - 4u));
        write16(ram, cursor.segment, at(cursor, 0x104u), ax);
        write16(ram, cursor.segment, at(cursor, 0x106u), dx);
    }

    fill_used(ram, cs, ss, bp, &sp);
    ax = read16(ram, ds, 0x5d96u);
    dx = read16(ram, ds, 0x5d98u);
    write16(ram, ss, (uint16_t)(bp - 4u), ax);
    write16(ram, ss, (uint16_t)(bp - 2u), dx);
    for (;;) {
        uint8_t index;
        ax = read16(ram, ss, (uint16_t)(bp - 4u));
        ax = (uint16_t)(ax | read16(ram, ss, (uint16_t)(bp - 2u)));
        if (ax == 0u)
            break;
        cursor = read_far(ram, ss, (uint16_t)(bp - 4u));
        index = read8(ram, cursor.segment, at(cursor, 0xbfu));
        ax = (uint16_t)((ax & 0xff00u) | index);
        ++result.mark_nodes;
        if (in_set(ram, cs, ss, &sp, ax)) {
            cursor = read_far(ram, ss, (uint16_t)(bp - 4u));
            index = read8(ram, cursor.segment, at(cursor, 0xbfu));
            write8(ram, ss, (uint16_t)(bp - 0xcu + index), 1u);
        }
        cursor = read_far(ram, ss, (uint16_t)(bp - 4u));
        ax = read16(ram, cursor.segment, at(cursor, 0x104u));
        dx = read16(ram, cursor.segment, at(cursor, 0x106u));
        write16(ram, ss, (uint16_t)(bp - 4u), ax);
        write16(ram, ss, (uint16_t)(bp - 2u), dx);
    }

    /* +0x151E..+0x1563: active pointer and selected pointer are separate
     * ordered word writes; every LES rereads the SS local pointer. */
    ax = read16(ram, ss, (uint16_t)(bp + 6u));
    dx = read16(ram, ss, (uint16_t)(bp + 8u));
    write16(ram, ds, 0x5d92u, ax);
    write16(ram, ds, 0x5d94u, dx);
    selected = read_far(ram, ds, 0x5d92u);
    write16(ram, ss, (uint16_t)(bp - 0x10u), selected.offset);
    write16(ram, ss, (uint16_t)(bp - 0xeu), selected.segment);
    selected = read_far(ram, ss, (uint16_t)(bp - 0x10u));
    write8(ram, selected.segment, at(selected, 0xbfu), 0u);
    for (;;) {
        uint8_t index;
        selected = read_far(ram, ss, (uint16_t)(bp - 0x10u));
        if (read8(ram, selected.segment, at(selected, 0xbfu)) >= 8u)
            break;
        selected = read_far(ram, ss, (uint16_t)(bp - 0x10u));
        index = read8(ram, selected.segment, at(selected, 0xbfu));
        if (read8(ram, ss, (uint16_t)(bp - 0xcu + index)) == 0u)
            break;
        selected = read_far(ram, ss, (uint16_t)(bp - 0x10u));
        index = read8(ram, selected.segment, at(selected, 0xbfu));
        write8(ram, selected.segment, at(selected, 0xbfu),
               (uint8_t)(index + 1u));
    }
    selected = read_far(ram, ss, (uint16_t)(bp - 0x10u));
    result.selected_slot = read8(ram, selected.segment, at(selected, 0xbfu));

    /* +0x1565/+0x1569: window pointer is read after prior writes. */
    window = read_far(ram, ds, 0x49d6u);
    ax = read16(ram, window.segment, at(window, 0x67cu));
    result.window_word = (uint16_t)(ax + 1u);
    write16(ram, window.segment, at(window, 0x67cu), result.window_word);

    /* +0x156E/+0x1571/+0x1574: argument pushes and CALL FAR's return
     * words are visible in SS RAM. Transfer into the overlay thunk and
     * u9B frame effects remain the first unresolved boundary. */
    push16(ram, ss, &sp, read16(ram, ss, (uint16_t)(bp + 8u)));
    push16(ram, ss, &sp, read16(ram, ss, (uint16_t)(bp + 6u)));
    push16(ram, ss, &sp, cs);
    push16(ram, ss, &sp, 0x1579u);
    result.transfer_sp = sp;
    *report = result;
    return PR_U6F_INSERT_SS_AT_U9B_TRANSFER;
}

/* GAME.OVR U9E:+018B..+019B, composed with RES_00BA_2199. */
#include "poolrad/recovered/overlay_u9e_sidecar_call_018b.h"
#include "poolrad/recovered/resident_sidecar_lookup.h"

#define RAM_MASK (PR_U9E_018B_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

/* LES consumes four contiguous physical bytes from one effective address.
 * Its segment word can therefore differ from a separate SS:(offset+2)
 * word read when the effective offset crosses FFFF. */
static pr_rec_far_ptr read_les(const uint8_t *ram, uint16_t segment,
                               uint16_t offset)
{
    size_t at = physical(segment, offset);
    pr_rec_far_ptr value;
    value.offset = (uint16_t)(ram[at] |
        ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
    value.segment = (uint16_t)(ram[(at + 2u) & RAM_MASK] |
        ((uint16_t)ram[(at + 3u) & RAM_MASK] << 8));
    return value;
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset,
                   uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u9e_018b_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u9e_018b_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

typedef struct {
    uint8_t *ram;
    pr_rec_far_ptr cell;
    uint8_t key;
    unsigned phase;
} lookup_memory;

/* The merged lookup preserves the resident's read/write order. Its two
 * read_far(cursor) operations that feed LES need a contiguous four-byte
 * read for the segment word, unlike the separate word reads at the loop
 * null test. Phases follow that documented access order. */
enum {
    HEAD_LOW, HEAD_HIGH, FIRST_WRITE_LOW, FIRST_WRITE_HIGH,
    TEST_LOW, TEST_HIGH, DEREF_LOW, DEREF_HIGH, NODE_BYTE,
    NEXT_LOW, NEXT_HIGH, LINK_LOW, LINK_HIGH,
    NEXT_WRITE_LOW, NEXT_WRITE_HIGH
};

static int lookup_read_word(void *context, uint16_t segment, uint16_t offset,
                            uint16_t *value)
{
    lookup_memory *memory = context;
    switch (memory->phase) {
    case HEAD_LOW: memory->phase = HEAD_HIGH; break;
    case HEAD_HIGH: memory->phase = FIRST_WRITE_LOW; break;
    case TEST_LOW: memory->phase = TEST_HIGH; break;
    case TEST_HIGH: memory->phase = DEREF_LOW; break;
    case DEREF_LOW: memory->phase = DEREF_HIGH; break;
    case DEREF_HIGH: memory->phase = NODE_BYTE; break;
    case NEXT_LOW: memory->phase = NEXT_HIGH; break;
    case NEXT_HIGH: memory->phase = LINK_LOW; break;
    case LINK_LOW: memory->phase = LINK_HIGH; break;
    case LINK_HIGH: memory->phase = NEXT_WRITE_LOW; break;
    default: return 0;
    }
    if (memory->phase == NODE_BYTE || memory->phase == LINK_LOW) {
        /* The just-completed high word belongs to LES ES:DI,[cursor]. */
        size_t at = (physical(memory->cell.segment, memory->cell.offset) +
                     2u) & RAM_MASK;
        *value = (uint16_t)(memory->ram[at] |
            ((uint16_t)memory->ram[(at + 1u) & RAM_MASK] << 8));
    } else {
        *value = read16(memory->ram, segment, offset);
    }
    return 1;
}

static int lookup_write_word(void *context, uint16_t segment, uint16_t offset,
                             uint16_t value)
{
    lookup_memory *memory = context;
    switch (memory->phase) {
    case FIRST_WRITE_LOW: memory->phase = FIRST_WRITE_HIGH; break;
    case FIRST_WRITE_HIGH: memory->phase = TEST_LOW; break;
    case NEXT_WRITE_LOW: memory->phase = NEXT_WRITE_HIGH; break;
    case NEXT_WRITE_HIGH: memory->phase = TEST_LOW; break;
    default: return 0;
    }
    write16(memory->ram, segment, offset, value);
    return 1;
}

static int lookup_read_byte(void *context, uint16_t segment, uint16_t offset,
                            uint8_t *value)
{
    lookup_memory *memory = context;
    if (memory->phase != NODE_BYTE) return 0;
    *value = read8(memory->ram, segment, offset);
    memory->phase = (*value == memory->key) ? TEST_LOW : NEXT_LOW;
    return 1;
}

pr_u9e_018b_status pr_u9e_sidecar_call_018b(
    uint8_t *ram, size_t ram_size, pr_u9e_018b_cpu *cpu, size_t max_nodes)
{
    pr_rec_lookup_io io;
    lookup_memory memory;
    pr_rec_far_ptr cell, character;
    uint16_t key, final_offset, final_segment;
    uint8_t found = 0u;
    pr_rec_lookup_status lookup_status;

    if (ram == NULL || cpu == NULL || ram_size < PR_U9E_018B_RAM_SIZE ||
        cpu->ip != 0x018Bu || max_nodes == 0u)
        return PR_U9E_018B_ARGUMENT;

    /* +018B LCALL pushes CS then +0190 IP; resident prologue reserves two
     * local bytes after saving BP. The five arguments remain above them. */
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x0190u);
    cpu->cs = 0x00BAu;
    cpu->ip = 0x2199u;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 0u);
    cell = read_les(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    key = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0xAu));
    character = read_les(ram, cpu->ss, (uint16_t)(cpu->bp + 0xCu));

    memory.ram = ram;
    memory.cell = cell;
    memory.key = (uint8_t)key;
    memory.phase = HEAD_LOW;
    io.context = &memory;
    io.read_word = lookup_read_word;
    io.write_word = lookup_write_word;
    io.read_byte = lookup_read_byte;
    io.max_nodes = max_nodes;
    lookup_status = pr_rec_res_00ba_2199(&io, cell, key, character, &found);
    if (lookup_status != PR_REC_LOOKUP_OK)
        return PR_U9E_018B_LOOKUP_LIMIT;

    /* The resident's final loop test ORs both cursor words into AX, then
     * copies its local match byte into AL. DX is the segment word most
     * recently copied to the cursor. Its last LES targets the cursor cell. */
    final_offset = read16(ram, cell.segment, cell.offset);
    final_segment = read16(ram, cell.segment, (uint16_t)(cell.offset + 2u));
    cpu->ax = (uint16_t)(((final_offset | final_segment) & 0xFF00u) | found);
    cpu->dx = final_segment;
    cpu->di = cell.offset;
    cpu->es = cell.segment;
    if (found != 0u)
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), 1u);
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), found);
    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);

    /* +0190 OR AL,AL; +0192 JE +019C; found path stores one and jumps
     * directly to +02D4. No outside call at either stop. */
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (found == 0u) {
        cpu->flags |= 0x0044u; /* ZF and even parity */
        cpu->ip = 0x019Cu;
        return PR_U9E_018B_ABSENT;
    }
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0xE1u), 1u);
    cpu->ip = 0x02D4u;
    return PR_U9E_018B_FOUND;
}

#include "resident_1816_caller_routes.h"

#define RAM_MASK (PR_1816_CALLER_RAM_SIZE - 1u)

typedef enum { NODE_BP, NODE_DS } node_source;
typedef enum { ROW_IMMEDIATE, ROW_BP } row_source;

typedef struct {
    uint16_t start_ip, call_ip;
    node_source node;
    uint16_t segment_at, offset_at;
    uint8_t column;
    row_source row_kind;
    uint8_t row_at;
    uint8_t flag;
} route;

/* Offsets and operand forms were checked against the pinned GAME.OVR bytes.
 * BP displacements are sign-extended through the uint16_t offset arithmetic. */
static const route routes[PR_1816_CALLER_SITE_COUNT] = {
    {0x0362, 0x0372, NODE_BP, 0x0010, 0x000E, 0x17, ROW_BP, 0xFF, 0},
    {0x2D3D, 0x2D4F, NODE_DS, 0x5D94, 0x5D92, 0x04, ROW_BP, 0xEA, 0},
    {0x0078, 0x0089, NODE_DS, 0x4718, 0x4716, 0x01, ROW_IMMEDIATE, 1, 0},
    {0x0DCF, 0x0DE0, NODE_DS, 0x5D94, 0x5D92, 0x01, ROW_IMMEDIATE, 21, 0},
    {0x11BD, 0x11CC, NODE_BP, 0xFFC3, 0xFFC1, 0x01, ROW_IMMEDIATE, 1, 1},
    {0x2AA5, 0x2AB6, NODE_DS, 0x5D94, 0x5D92, 0x01, ROW_IMMEDIATE, 1, 1},
    {0x1B3F, 0x1B50, NODE_DS, 0x5D94, 0x5D92, 0x01, ROW_IMMEDIATE, 1, 0},
    {0x33B4, 0x33C3, NODE_BP, 0x000E, 0x000C, 0x01, ROW_IMMEDIATE, 19, 0}
};

static size_t physical(uint16_t segment, uint16_t offset) {
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset) {
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset) {
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value) {
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_1816_caller_cpu *cpu, uint16_t value) {
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_1816_caller_cpu *cpu, uint8_t value) {
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint16_t node_word(const route *r, const uint8_t *ram,
                          const pr_1816_caller_cpu *cpu, uint16_t at) {
    if (r->node == NODE_DS) return read16(ram, cpu->ds, at);
    return read16(ram, cpu->ss, (uint16_t)(cpu->bp + at));
}

static int traced_push(uint8_t *ram, size_t ram_size,
                       pr_1816_caller_cpu *cpu, pr_1816_caller_callback callback,
                       void *ctx, unsigned site, pr_1816_caller_event event,
                       uint16_t value) {
    push16(ram, cpu, value);
    return callback(ctx, ram, ram_size, cpu, site, event, value, NULL);
}

pr_1816_caller_status pr_1816_caller_route_step(
    unsigned site, uint8_t *ram, size_t ram_size, pr_1816_caller_cpu *cpu,
    pr_1816_caller_callback callback, void *ctx) {
    const route *r;
    pr_1816_caller_frame frame;
    uint16_t entry_cs, call_sp;
    if (site >= PR_1816_CALLER_SITE_COUNT || ram == NULL || cpu == NULL ||
        callback == NULL || ram_size < PR_1816_CALLER_RAM_SIZE)
        return PR_1816_CALLER_ARGUMENT;
    r = &routes[site];
    if (cpu->ip != r->start_ip) return PR_1816_CALLER_ARGUMENT;

    frame.node_segment = node_word(r, ram, cpu, r->segment_at);
    if (traced_push(ram, ram_size, cpu, callback, ctx, site,
                    PR_1816_PUSH_NODE_SEGMENT, frame.node_segment))
        return PR_1816_CALLER_BOUNDARY;
    frame.node_offset = node_word(r, ram, cpu, r->offset_at);
    if (traced_push(ram, ram_size, cpu, callback, ctx, site,
                    PR_1816_PUSH_NODE_OFFSET, frame.node_offset))
        return PR_1816_CALLER_BOUNDARY;

    set_al(cpu, r->column);
    frame.column = cpu->ax;
    if (traced_push(ram, ram_size, cpu, callback, ctx, site,
                    PR_1816_PUSH_COLUMN, frame.column))
        return PR_1816_CALLER_BOUNDARY;

    set_al(cpu, r->row_kind == ROW_BP ?
           read8(ram, cpu->ss,
                 (uint16_t)(cpu->bp + (int8_t)r->row_at)) : r->row_at);
    frame.row = cpu->ax;
    if (traced_push(ram, ram_size, cpu, callback, ctx, site,
                    PR_1816_PUSH_ROW, frame.row))
        return PR_1816_CALLER_BOUNDARY;

    set_al(cpu, r->flag);
    frame.flag = cpu->ax;
    if (traced_push(ram, ram_size, cpu, callback, ctx, site,
                    PR_1816_PUSH_FLAG, frame.flag))
        return PR_1816_CALLER_BOUNDARY;

    cpu->ip = r->call_ip;
    entry_cs = cpu->cs;
    call_sp = cpu->sp;
    if (callback(ctx, ram, ram_size, cpu, site, PR_1816_CALL_DISPLAY,
                 0, &frame)) return PR_1816_CALLER_BOUNDARY;
    /* The callback represents the far callee, including its register/RAM
     * effects. Only its RETF 10 stack and return-address effect is local. */
    cpu->cs = entry_cs;
    cpu->sp = (uint16_t)(call_sp + 10u);
    cpu->ip = (uint16_t)(r->call_ip + 5u);
    return PR_1816_CALLER_OK;
}

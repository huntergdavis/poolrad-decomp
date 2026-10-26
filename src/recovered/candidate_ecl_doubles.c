/* Candidate-scout batch 3 helper doubles. See include/poolrad/recovered/candidate_ecl_cpu.h.
 * These stand in for the real unit-0x25 runtime routines; no credit is claimed. */
#include "poolrad/recovered/candidate_ecl_cpu.h"

#define RAM_SIZE ((size_t)1u << 20)
#define MASK (RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t rd8(uint8_t *b, uint16_t segment, uint16_t offset)
{
    return b[physical(segment, offset)];
}

static uint16_t rd16(uint8_t *b, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(b[at] | ((uint16_t)b[(at + 1u) & MASK] << 8u));
}

static void wr8(uint8_t *b, uint16_t segment, uint16_t offset, uint8_t value)
{
    b[physical(segment, offset)] = value;
}

static void wr16(uint8_t *b, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    b[at] = (uint8_t)value;
    b[(at + 1u) & MASK] = (uint8_t)(value >> 8u);
}

static int retf(uint8_t *b, pr_candidate_cpu *c, int argument_bytes)
{
    c->ip = rd16(b, c->ss, c->sp);
    c->cs = rd16(b, c->ss, (uint16_t)(c->sp + 2u));
    c->sp = (uint16_t)(c->sp + 4u + argument_bytes);
    return 0;
}

int pr_candidate_load_operands(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx)
{
    uint8_t count = rd8(b, c->ss, (uint16_t)(c->sp + 4u));
    uint16_t base = ctx != NULL ? ctx->operand_base : 0x1000u;
    uint16_t i;
    for (i = 1u; i <= count; i++) {
        uint16_t at = (uint16_t)(base + (uint16_t)((i - 1u) * 3u));
        wr8(b, c->ds, (uint16_t)(0x7005u + i), rd8(b, c->ds, at));
        wr8(b, c->ds, (uint16_t)(0x7045u + i), rd8(b, c->ds, (uint16_t)(at + 1u)));
        wr8(b, c->ds, (uint16_t)(0x7085u + i), rd8(b, c->ds, (uint16_t)(at + 2u)));
    }
    return retf(b, c, 2);
}

int pr_candidate_operand_word(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx)
{
    uint8_t low = rd8(b, c->ss, (uint16_t)(c->sp + 4u));
    uint8_t high = rd8(b, c->ss, (uint16_t)(c->sp + 6u));
    (void)ctx;
    c->ax = (uint16_t)(((uint16_t)high << 8u) | low);
    return retf(b, c, 4);
}

int pr_candidate_slot_value(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx)
{
    uint8_t slot = rd8(b, c->ss, (uint16_t)(c->sp + 4u));
    (void)ctx;
    c->ax = rd8(b, c->ds, (uint16_t)(0x7045u + slot));
    return retf(b, c, 2);
}

int pr_candidate_store_word(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx)
{
    uint16_t value = rd16(b, c->ss, (uint16_t)(c->sp + 4u));
    uint16_t dest = rd16(b, c->ss, (uint16_t)(c->sp + 6u));
    (void)ctx;
    wr16(b, c->ds, dest, value);
    return retf(b, c, 4);
}

int pr_candidate_store_string(uint8_t *b, pr_candidate_cpu *c, pr_candidate_ctx *ctx)
{
    uint16_t source_off = rd16(b, c->ss, (uint16_t)(c->sp + 4u));
    uint16_t source_seg = rd16(b, c->ss, (uint16_t)(c->sp + 6u));
    uint16_t dest = rd16(b, c->ss, (uint16_t)(c->sp + 8u));
    uint8_t length = rd8(b, source_seg, source_off);
    uint8_t i;
    (void)ctx;
    wr8(b, c->ds, dest, length);
    for (i = 0u; i < length; i++) {
        wr8(b, c->ds, (uint16_t)(dest + 1u + i),
            rd8(b, source_seg, (uint16_t)(source_off + 1u + i)));
    }
    return retf(b, c, 6);
}

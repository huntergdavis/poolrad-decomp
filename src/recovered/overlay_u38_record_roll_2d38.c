/* GAME.OVR unit 0x38:+2D38..+2D87; docs/re/U38_RECORD_ROLL_2D38.md. */
#include "poolrad/recovered/overlay_u38_record_roll_2d38.h"
#include "poolrad/recovered/overlay_u9e_entry9_roll_latch_0e2e.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <string.h>

#define MASK (PR_U38_2D38_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define ACTIVE_OFFSET 0x5D92u
#define LATCH_SUM 0x6818u
#define LATCH_FLAGS 0x6819u
#define LATCH_COUNT 0x681Cu
#define CALL_SITE 0x32u     /* +2D6A - +2D38 */
#define STUB_OFFSET 0x4Du   /* unit 0x9E entry 9 */
#define ENTRY9_IP 0x0E2Eu
#define ENTRY8_IP 0x0DE3u

_Static_assert(sizeof(pr_u38_2d38_cpu) == sizeof(pr_u9e_0e2e_cpu),
               "u9E entry-9 CPU state layout differs");
_Static_assert(sizeof(pr_u38_2d38_cpu) == sizeof(pr_u9e_entry8_compose_cpu),
               "u9E entry-8 CPU state layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u38_2d38_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u38_2d38_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void les(const uint8_t *ram, pr_u38_2d38_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u38_2d38_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

static void add8_flags(pr_u38_2d38_cpu *cpu, uint8_t left, uint8_t right)
{
    uint16_t wide = (uint16_t)left + right;
    uint8_t result = (uint8_t)wide;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (wide & 0x100u) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ result) & (right ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

/* OR/AND on words or bytes: CF/OF clear, undefined AF represented as zero. */
static void logic(pr_u38_2d38_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

typedef struct {
    size_t ram_size;
    int status;
} chain_context;

/* Entry 9's PUSH CS / CALL +0DE3 lands here: the merged entry-8 chain
 * executes through its RETF 4. The merged entry-9 model then pops the
 * same return words from RAM and adds four itself, so SP is put back on
 * the frame; a nonzero return means the callee completed. */
static int entry8_callback(void *context, uint8_t *ram, size_t ram_size, pr_u9e_0e2e_cpu *cpu)
{
    chain_context *chain = (chain_context *)context;
    pr_u9e_entry8_compose_cpu state;
    pr_u9e_entry8_compose_status status;
    memcpy(&state, cpu, sizeof state);
    status = pr_u9e_entry8_fullstate_compose_0de3(ram, ram_size, &state, 256u);
    memcpy(cpu, &state, sizeof state);
    chain->status = (int)status;
    if (status != PR_U9E_ENTRY8_COMPOSE_RETURNED)
        return 0;
    cpu->sp = (uint16_t)(cpu->sp - 8u);
    return 1;
}

/* +2D6A: LCALL 00B0:004D through the loaded stub into entry 9 (+0E2E). */
static pr_u38_2d38_status roll(uint8_t *ram, size_t ram_size, pr_u38_2d38_cpu *cpu,
                               uint16_t entry, int *chain_status)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + CALL_SITE + 5u);
    uint16_t stub_segment;
    pr_u9e_0e2e_cpu latch;
    pr_u9e_0e2e_status status;
    chain_context chain;
    if (read8(ram, cpu->cs, (uint16_t)(entry + CALL_SITE)) != 0x9Au ||
        read16(ram, cpu->cs, (uint16_t)(entry + CALL_SITE + 1u)) != STUB_OFFSET)
        return PR_U38_2D38_CODE;
    stub_segment = read16(ram, cpu->cs, (uint16_t)(entry + CALL_SITE + 3u));
    if (read8(ram, stub_segment, STUB_OFFSET) != 0xEAu ||
        read16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY9_IP)
        return PR_U38_2D38_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = ENTRY9_IP;
    cpu->cs = read16(ram, stub_segment, STUB_OFFSET + 3u);
    chain.ram_size = ram_size;
    chain.status = -1;
    memcpy(&latch, cpu, sizeof latch);
    status = pr_u9e_entry9_roll_latch_0e2e(ram, ram_size, &latch, entry8_callback, &chain);
    memcpy(cpu, &latch, sizeof latch);
    if (chain_status != NULL)
        *chain_status = chain.status;
    if (status != PR_U9E_0E2E_OK)
        return status == PR_U9E_0E2E_CALL_ABORT ? PR_U38_2D38_CALLEE : PR_U38_2D38_ARGUMENT;
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U38_2D38_TRANSFER;
    return PR_U38_2D38_OK;
}

pr_u38_2d38_status pr_u38_record_roll_2d38(
    uint8_t *ram, size_t ram_size, pr_u38_2d38_cpu *cpu, int *chain_status)
{
    uint16_t entry;
    uint8_t value;
    if (chain_status != NULL)
        *chain_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U38_2D38_RAM_SIZE)
        return PR_U38_2D38_ARGUMENT;
    entry = cpu->ip;
    /* +2D38..+2D3B */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    /* +2D3E..+2D58: the active record's far pointer +CC/+CE into the locals. */
    les(ram, cpu, cpu->ds, ACTIVE_OFFSET);
    cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCCu));
    cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0xCEu));
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->ax = (uint16_t)(cpu->ax | read16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    logic(cpu, cpu->ax, 0x8000u);
    if (cpu->ax != 0u) {
        /* +2D5A..+2D62: byte +2E of that record must be 56h. */
        les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 4u));
        value = read8(ram, cpu->es, (uint16_t)(cpu->di + 0x2Eu));
        cmp8(cpu, value, 0x56u);
        if (value == 0x56u) {
            pr_u38_2d38_status status;
            /* +2D64..+2D6F: entry 9 with count 3 and range 8, the live AH in both words. */
            set_al(cpu, 3u);
            push16(ram, cpu, cpu->ax);
            set_al(cpu, 8u);
            push16(ram, cpu, cpu->ax);
            status = roll(ram, ram_size, cpu, entry, chain_status);
            if (status != PR_U38_2D38_OK)
                return status;
            write8(ram, cpu->ds, LATCH_SUM, (uint8_t)cpu->ax);
        }
    }
    /* +2D72..+2D7E: bits 0 or 3 of DS:6819 add the latch count to DS:6818. */
    set_al(cpu, read8(ram, cpu->ds, LATCH_FLAGS));
    set_al(cpu, (uint8_t)(cpu->ax & 9u));
    logic(cpu, (uint8_t)cpu->ax, 0x80u);
    logic(cpu, (uint8_t)cpu->ax, 0x80u);
    if ((uint8_t)cpu->ax != 0u) {
        uint8_t sum;
        set_al(cpu, read8(ram, cpu->ds, LATCH_COUNT));
        sum = read8(ram, cpu->ds, LATCH_SUM);
        add8_flags(cpu, sum, (uint8_t)cpu->ax);
        write8(ram, cpu->ds, LATCH_SUM, (uint8_t)(sum + (uint8_t)cpu->ax));
    }
    /* +2D82..+2D85: MOV SP,BP; POP BP; RETF 0Ah. */
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    {
        size_t at = physical(cpu->ss, cpu->sp);
        cpu->ip = read16_at(ram, at);
        cpu->cs = read16_at(ram, (at + 2u) & MASK);
    }
    cpu->sp = (uint16_t)(cpu->sp + 14u);
    return PR_U38_2D38_OK;
}

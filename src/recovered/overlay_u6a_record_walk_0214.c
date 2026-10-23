/* GAME.OVR unit 0x6A:+0214..+025B; docs/re/U6A_RECORD_WALK_0214.md. */
#include "poolrad/recovered/overlay_u6a_record_walk_0214.h"
#include "overlay_loaded_stubs.h"
#include "poolrad/recovered/overlay_u6a_link_high_clear_019c.h"
#include "poolrad/recovered/overlay_u6a_record_clear_high_0159.h"

#include <string.h>

#define MASK (PR_U6A_0214_RAM_SIZE - 1u)
#define LOOP_HEAD 0x13u       /* +0227 - +0214 */
#define BACK_0159 0x00BBu     /* +0214 - +0159 */
#define BACK_019C 0x0078u     /* +0214 - +019C */

_Static_assert(sizeof(pr_u6a_0214_cpu) == sizeof(pr_u6a_0159_cpu), "u6A +0159 CPU layout differs");
_Static_assert(sizeof(pr_u6a_0214_cpu) == sizeof(pr_u6a_019c_cpu), "u6A +019C CPU layout differs");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
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

static void push16(uint8_t *ram, pr_u6a_0214_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t local16(const uint8_t *ram, const pr_u6a_0214_cpu *cpu, uint16_t back)
{
    return read16(ram, cpu->ss, (uint16_t)(cpu->bp - back));
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* OR r16,m16: CF/OF clear, SF/ZF/PF from the result, undefined AF represented as zero. */
static void or16(pr_u6a_0214_cpu *cpu, uint16_t result)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
}

/* PUSH [BP-2]; PUSH [BP-4]; PUSH CS; CALL near: the record pointer, then the callee on the same RAM
 * and CPU words, which must return to the frame after its RETF 4. */
static pr_u6a_0214_status call_record(uint8_t *ram, size_t ram_size, pr_u6a_0214_cpu *cpu, uint16_t entry,
                                      uint16_t return_delta, int which, int *callee_status)
{
    uint16_t caller_cs = cpu->cs, return_ip = (uint16_t)(entry + return_delta);
    int status;
    push16(ram, cpu, local16(ram, cpu, 2u));
    push16(ram, cpu, local16(ram, cpu, 4u));
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    if (which == 0) {
        pr_u6a_0159_cpu leaf;
        cpu->ip = (uint16_t)(entry - BACK_0159);
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_u6a_record_clear_high_0159(ram, ram_size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
        if (status != PR_U6A_0159_OK) {
            if (callee_status != NULL) *callee_status = status;
            return PR_U6A_0214_CALLEE;
        }
    } else {
        pr_u6a_019c_cpu leaf;
        cpu->ip = (uint16_t)(entry - BACK_019C);
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_u6a_link_high_clear_019c(ram, ram_size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
        if (status == PR_U6A_019C_TRANSFER)
            return PR_U6A_0214_TRANSFER;
        if (status != PR_U6A_019C_OK) {
            if (callee_status != NULL) *callee_status = status;
            return PR_U6A_0214_CALLEE;
        }
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U6A_0214_TRANSFER;
    return PR_U6A_0214_OK;
}

pr_u6a_0214_status pr_u6a_record_walk_0214(
    uint8_t *ram, size_t ram_size, pr_u6a_0214_cpu *cpu, unsigned max_records, int *callee_status)
{
    uint16_t entry;
    unsigned records = 0u;
    pr_u6a_0214_status status;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U6A_0214_RAM_SIZE)
        return PR_U6A_0214_ARGUMENT;
    /* Validate the inherited 019C loaded 0xEA binding before either child runs. */
    if (!pr_overlay_stub_target(ram, cpu->cs,
            (uint16_t)(cpu->ip - BACK_019C + 0x27u), 0x003Eu, 0x323Du, NULL))
        return PR_U6A_0214_ARGUMENT;
    entry = cpu->ip;
    /* +0214..+0224: frame, four local bytes, the head pointer DS:5D96/5D98 (SUB's flags are
     * overwritten by the OR before they are observed). */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    cpu->ax = read16(ram, cpu->ds, 0x5D96u);
    cpu->dx = read16(ram, cpu->ds, 0x5D98u);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    for (;;) {
        /* +0227..+022D: MOV AX,[BP-4]; OR AX,[BP-2]; JE +0258. */
        cpu->ax = local16(ram, cpu, 4u);
        cpu->ax = (uint16_t)(cpu->ax | local16(ram, cpu, 2u));
        or16(cpu, cpu->ax);
        if (cpu->ax == 0u)
            break;
        if (records == max_records) {
            cpu->ip = (uint16_t)(entry + LOOP_HEAD);
            return PR_U6A_0214_LIMIT;
        }
        ++records;
        /* +022F..+0240: +0159 and +019C on the record. */
        status = call_record(ram, ram_size, cpu, entry, 0x25u, 0, callee_status);
        if (status != PR_U6A_0214_OK)
            return status;
        status = call_record(ram, ram_size, cpu, entry, 0x2Fu, 1, callee_status);
        if (status != PR_U6A_0214_OK)
            return status;
        /* +0243..+0256: LES DI,[BP-4]; the record's link at +104h/+106h becomes the pointer. */
        at = physical(cpu->ss, (uint16_t)(cpu->bp - 4u));
        cpu->di = read16_at(ram, at);
        cpu->es = read16_at(ram, (at + 2u) & MASK);
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
        write16(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    }
    /* +0258..+025B: MOV SP,BP; POP BP; RETF. */
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U6A_0214_OK;
}

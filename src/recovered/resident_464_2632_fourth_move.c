#include "poolrad/recovered/resident_464_2632_fourth_move.h"

#include <string.h>

enum {
    CF = 0x0001u, PF = 0x0004u, AF = 0x0010u,
    ZF = 0x0040u, SF = 0x0080u, OF = 0x0800u,
    ARITH = CF | PF | AF | ZF | SF | OF
};

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint8_t byte_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void put_byte(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t word_at(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return (uint16_t)(byte_at(ram, segment, offset) |
        ((uint16_t)byte_at(ram, segment, (uint16_t)(offset + 1u)) << 8));
}

static void push(uint8_t *ram, pr_r464_2632_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    put_byte(ram, cpu->ss, cpu->sp, (uint8_t)value);
    put_byte(ram, cpu->ss, (uint16_t)(cpu->sp + 1u), (uint8_t)(value >> 8));
}

static uint8_t even_parity(uint8_t value)
{
    unsigned bits = 0;
    while (value != 0u) {
        bits ^= value & 1u;
        value >>= 1;
    }
    return (uint8_t)(bits == 0u);
}

static uint16_t or_flags16(uint16_t flags, uint16_t result)
{
    flags &= (uint16_t)~ARITH;
    if (even_parity((uint8_t)result)) flags |= PF;
    if (result == 0u) flags |= ZF;
    if ((result & 0x8000u) != 0u) flags |= SF;
    return flags;
}

static void observe(uint16_t ip, const uint8_t *ram, pr_r464_2632_cpu *cpu,
                    pr_r464_2632_observer observer, void *context)
{
    cpu->ip = ip;
    if (observer != NULL) observer(ip, ram, cpu, context);
}

pr_r464_2632_status pr_r464_2632_fourth_move(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context)
{
    static const pr_r464_2632_cpu measured_pre = {
        0x0C88u, 0x2646u, 8u, 0x3E4Eu, 0u, 0x14A0u, 0x3E68u,
        0x3E78u, 0x3E80u, 0x3E52u, 0x14A0u, 0x14A0u, 0x1D16u, 0x7216u
    };
    uint16_t object_off, object_seg, return_ip, return_cs;
    if (ram == NULL || cpu == NULL || ram_size < PR_R464_2632_RAM_BYTES)
        return PR_R464_2632_INVALID_HOST;
    if (memcmp(cpu, &measured_pre, sizeof(*cpu)) != 0 ||
        word_at(ram, cpu->ss, cpu->sp) != 8u ||
        word_at(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) != 0x3E78u ||
        word_at(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) != cpu->ss ||
        word_at(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) != 0x260Au ||
        word_at(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)) != cpu->ds)
        return PR_R464_2632_UNSUPPORTED_PATH;
    object_off = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    object_seg = word_at(ram, cpu->ss, (uint16_t)(cpu->bp + 8u));
    if ((object_off | object_seg) == 0u)
        return PR_R464_2632_UNSUPPORTED_PATH;

    /* +2646 LCALL 131C:0A3F; this is the existing runtime Move contract:
     * forward byte copy, DS restoration, RETF 10, and stack residue. */
    push(ram, cpu, cpu->cs);
    push(ram, cpu, 0x264Bu);
    cpu->cs = 0x131Cu;
    observe(0x0A3Fu, ram, cpu, observer, context);
    cpu->bx = cpu->sp;
    cpu->dx = cpu->ds;
    cpu->si = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 0x0Au));
    cpu->ds = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 0x0Cu));
    cpu->di = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 6u));
    cpu->es = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 8u));
    cpu->cx = word_at(ram, cpu->ss, (uint16_t)(cpu->bx + 4u));
    cpu->flags &= (uint16_t)~0x0400u;
    while (cpu->cx != 0u) {
        put_byte(ram, cpu->es, cpu->di, byte_at(ram, cpu->ds, cpu->si));
        cpu->si = (uint16_t)(cpu->si + 1u);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    cpu->ds = cpu->dx;
    return_ip = word_at(ram, cpu->ss, cpu->sp);
    return_cs = word_at(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 0x0Eu);
    cpu->cs = return_cs;
    observe(return_ip, ram, cpu, observer, context);

    /* +264B MOV AX,[BP+6]; +264E OR AX,[BP+8]; +2651 JNE +2656. */
    cpu->ax = object_off;
    cpu->ax = (uint16_t)(cpu->ax | object_seg);
    cpu->flags = or_flags16(cpu->flags, cpu->ax);
    observe(0x2651u, ram, cpu, observer, context);
    observe(0x2656u, ram, cpu, observer, context);
    return PR_R464_2632_OK;
}

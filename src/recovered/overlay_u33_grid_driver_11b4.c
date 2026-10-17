/* GAME.OVR unit 0x33:+11B4..+122B; docs/re/U33_GRID_DRIVER_11B4.md. */
#include "poolrad/recovered/overlay_u33_grid_driver_11b4.h"
#include "poolrad/recovered/overlay_u33_grid_diagonal_0bde.h"
#include "poolrad/recovered/overlay_u33_grid_pair_0d13.h"
#include "poolrad/recovered/overlay_u33_grid_bands_0e9e.h"
#include "poolrad/recovered/overlay_u33_grid_roll_0000.h"

#include <string.h>

#define MASK (PR_U33_11B4_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u
#define GRID_POINTER 0x6716u
#define WINDOW_POINTER 0x49D2u
#define MODE 0x49FAu
#define TABLE 0x3648u
#define CLASS_BYTE 0x465Au
#define FILL_SITE 0x17u          /* +11CB */
#define FILL_OFFSET 0x16A8u
#define FILL_COUNT 0x04E2u

_Static_assert(sizeof(pr_u33_11b4_cpu) == sizeof(pr_u33_0bde_cpu), "u33 +0BDE CPU layout differs");
_Static_assert(sizeof(pr_u33_11b4_cpu) == sizeof(pr_u33_0d13_cpu), "u33 +0D13 CPU layout differs");
_Static_assert(sizeof(pr_u33_11b4_cpu) == sizeof(pr_u33_0e9e_cpu), "u33 +0E9E CPU layout differs");
_Static_assert(sizeof(pr_u33_11b4_cpu) == sizeof(pr_u33_0000_cpu), "u33 +0000 CPU layout differs");

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

static void push16(uint8_t *ram, pr_u33_11b4_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u33_11b4_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void cbw(pr_u33_11b4_cpu *cpu)
{
    cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax;
}

/* MUL r/m16: CF and OF follow the high word; the 8086 leaves SF, ZF, AF and PF undefined and
 * the Unicorn/QEMU oracle derives them from the low word with AF clear, which this mirrors. */
static void mul16(pr_u33_11b4_cpu *cpu, uint16_t source);

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void mul16(pr_u33_11b4_cpu *cpu, uint16_t source)
{
    uint32_t product = (uint32_t)cpu->ax * source;
    cpu->ax = (uint16_t)product;
    cpu->dx = (uint16_t)(product >> 16);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (cpu->dx != 0u) cpu->flags |= 0x0801u;
    if (cpu->ax == 0u) cpu->flags |= 0x0040u;
    if ((cpu->ax & 0x8000u) != 0u) cpu->flags |= 0x0080u;
    if (even_parity((uint8_t)cpu->ax)) cpu->flags |= 0x0004u;
}

static void cmp8(pr_u33_11b4_cpu *cpu, uint8_t left, uint8_t right)
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

static void add16_flags(pr_u33_11b4_cpu *cpu, uint16_t left, uint16_t source)
{
    uint32_t wide = (uint32_t)left + source;
    uint16_t result = (uint16_t)wide;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (wide & 0x10000u) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ source ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if ((left ^ result) & (source ^ result) & 0x8000u) cpu->flags |= 0x0800u;
}

static void add8_memory(uint8_t *ram, pr_u33_11b4_cpu *cpu, uint16_t segment, uint16_t offset, uint8_t source)
{
    uint8_t left = read8(ram, segment, offset);
    uint16_t wide = (uint16_t)left + source;
    uint8_t result = (uint8_t)wide;
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (wide & 0x100u) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ source ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ result) & (source ^ result) & 0x80u) cpu->flags |= 0x0800u;
    write8(ram, segment, offset, result);
}

/* LCALL 0AF8:16A8 (relocated word read from the site) into the folded
 * SYSTEM FillChar: BX := SP; LES DI,SS:[BX+8]; CX := SS:[BX+6];
 * AL := SS:[BX+4]; CLD; REP STOSB; RETF 8. */
static pr_u33_11b4_status fill_char(uint8_t *ram, pr_u33_11b4_cpu *cpu, uint16_t entry)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + FILL_SITE + 5u);
    uint16_t segment;
    size_t at;
    if (read8(ram, cpu->cs, (uint16_t)(entry + FILL_SITE)) != 0x9Au ||
        read16(ram, cpu->cs, (uint16_t)(entry + FILL_SITE + 1u)) != FILL_OFFSET)
        return PR_U33_11B4_CODE;
    segment = read16(ram, cpu->cs, (uint16_t)(entry + FILL_SITE + 3u));
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->cs = segment;
    cpu->ip = FILL_OFFSET;
    cpu->bx = cpu->sp;
    at = physical(cpu->ss, (uint16_t)(cpu->bx + 8u));
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    cpu->cx = read16(ram, cpu->ss, (uint16_t)(cpu->bx + 6u));
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bx + 4u)));
    cpu->flags &= (uint16_t)~0x0400u;
    while (cpu->cx != 0u) {
        write8(ram, cpu->es, cpu->di, (uint8_t)cpu->ax);
        cpu->di = (uint16_t)(cpu->di + 1u);
        cpu->cx = (uint16_t)(cpu->cx - 1u);
    }
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 12u);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_11B4_TRANSFER;
    return PR_U33_11B4_OK;
}

typedef int (*body_function)(uint8_t *, size_t, void *, int *);

/* PUSH CS; CALL a composed unit 0x33 body at its own load offset. */
static pr_u33_11b4_status call_body(uint8_t *ram, size_t ram_size, pr_u33_11b4_cpu *cpu, uint16_t entry,
                                    uint16_t return_delta, uint16_t back, body_function body, int *chain_status)
{
    uint16_t caller_cs = cpu->cs;
    uint16_t return_ip = (uint16_t)(entry + return_delta);
    int status;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = (uint16_t)(entry - back);
    status = body(ram, ram_size, cpu, chain_status);
    if (status == 1)
        return PR_U33_11B4_ARGUMENT;
    if (status == 2)
        return PR_U33_11B4_CODE;
    if (status == 3)
        return PR_U33_11B4_CALLEE;
    if (status != 0 || cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U33_11B4_TRANSFER;
    return PR_U33_11B4_OK;
}

static int body_0bde(uint8_t *ram, size_t ram_size, void *cpu, int *chain)
{
    return (int)pr_u33_grid_diagonal_0bde(ram, ram_size, (pr_u33_0bde_cpu *)cpu, chain);
}

static int body_0d13(uint8_t *ram, size_t ram_size, void *cpu, int *chain)
{
    return (int)pr_u33_grid_pair_0d13(ram, ram_size, (pr_u33_0d13_cpu *)cpu, chain);
}

static int body_0e9e(uint8_t *ram, size_t ram_size, void *cpu, int *chain)
{
    return (int)pr_u33_grid_bands_0e9e(ram, ram_size, (pr_u33_0e9e_cpu *)cpu, chain);
}

static int body_0000(uint8_t *ram, size_t ram_size, void *cpu, int *chain)
{
    return (int)pr_u33_grid_roll_0000(ram, ram_size, (pr_u33_0000_cpu *)cpu, chain);
}

#define TRY(call) do { pr_u33_11b4_status status_ = (call); if (status_ != PR_U33_11B4_OK) return status_; } while (0)

pr_u33_11b4_status pr_u33_grid_driver_11b4(
    uint8_t *ram, size_t ram_size, pr_u33_11b4_cpu *cpu, int *chain_status)
{
    uint16_t entry;
    size_t at;
    if (chain_status != NULL)
        *chain_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_11B4_RAM_SIZE)
        return PR_U33_11B4_ARGUMENT;
    entry = cpu->ip;
    /* +11B4..+11B7 */
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    /* +11BA..+11CB: FillChar(grid + 7, 4E2h, 17h); the count word keeps the live AH. */
    at = physical(cpu->ds, GRID_POINTER);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    add16_flags(cpu, cpu->di, 7u);
    cpu->di = (uint16_t)(cpu->di + 7u);
    push16(ram, cpu, cpu->es);
    push16(ram, cpu, cpu->di);
    cpu->ax = FILL_COUNT;
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x17u);
    push16(ram, cpu, cpu->ax);
    TRY(fill_char(ram, cpu, entry));
    /* +11D0..+11E5: the two window bytes. */
    at = physical(cpu->ds, WINDOW_POINTER);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x186u)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), (uint8_t)cpu->ax);
    at = physical(cpu->ds, WINDOW_POINTER);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x188u)));
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), (uint8_t)cpu->ax);
    /* +11E8..+11FA: mode 3 adds 0Dh, mode 4 adds 1Ah to the first byte. */
    {
        uint8_t mode = read8(ram, cpu->ds, MODE);
        cmp8(cpu, mode, 3u);
        if (mode == 3u)
            add8_memory(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 1u), 0x0Du);
        mode = read8(ram, cpu->ds, MODE);
        cmp8(cpu, mode, 4u);
        if (mode == 4u)
            add8_memory(ram, cpu, cpu->ss, (uint16_t)(cpu->bp - 1u), 0x1Au);
    }
    /* +11FE..+1215: DS:465A := DS:[3648 + 2Ch * second + first]. */
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cbw(cpu);
    cpu->cx = cpu->ax;
    set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    cbw(cpu);
    cpu->dx = 0x2Cu;
    mul16(cpu, cpu->dx);
    cpu->di = cpu->ax;
    add16_flags(cpu, cpu->di, cpu->cx);
    cpu->di = (uint16_t)(cpu->di + cpu->cx);
    set_al(cpu, read8(ram, cpu->ds, (uint16_t)(cpu->di + TABLE)));
    write8(ram, cpu->ds, CLASS_BYTE, (uint8_t)cpu->ax);
    /* +1218..+1225: the four generators. */
    TRY(call_body(ram, ram_size, cpu, entry, 0x68u, 0x05D6u, body_0bde, chain_status));
    TRY(call_body(ram, ram_size, cpu, entry, 0x6Cu, 0x04A1u, body_0d13, chain_status));
    TRY(call_body(ram, ram_size, cpu, entry, 0x70u, 0x0316u, body_0e9e, chain_status));
    TRY(call_body(ram, ram_size, cpu, entry, 0x74u, 0x11B4u, body_0000, chain_status));
    /* +1228..+122B: MOV SP,BP; POP BP; RETF. */
    cpu->sp = cpu->bp;
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U33_11B4_OK;
}

/* GAME.OVR unit 0x77 +28BE (raw CPU, composing SYSTEM LongInt multiply and divide); docs/re/U77_WEIGHTED_28BE_CPU.md. */
#include "poolrad/recovered/overlay_u77_weighted_28be_cpu.h"
#include "poolrad/recovered/system_longdiv_0a76_cpu.h"
#include "poolrad/recovered/system_longmul_0a5b_cpu.h"

#include <string.h>

#define MASK (PR_U77_28BE_CPU_RAM_SIZE - 1u)
#define ENTRY_IP 0x28BEu

_Static_assert(sizeof(pr_u77_28be_cpu) == sizeof(pr_system_longmul_cpu), "SYSTEM multiply CPU layout differs");
_Static_assert(sizeof(pr_u77_28be_cpu) == sizeof(pr_system_longdiv_cpu), "SYSTEM divide CPU layout differs");

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u77_28be_cpu *cpu;
    uint16_t entry;
    int *callee_status;
    pr_u77_28be_cpu_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const machine *m, uint16_t segment, uint16_t offset)
{
    return read16_at(m->ram, physical(segment, offset));
}

static void write16(machine *m, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(machine *m, uint16_t value)
{
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    write16(m, m->cpu->ss, m->cpu->sp, value);
}

static uint16_t here(const machine *m, uint16_t offset)
{
    return (uint16_t)(m->entry + (uint16_t)(offset - ENTRY_IP));
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void szp(pr_u77_28be_cpu *cpu, uint16_t result)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
}

/* SHL r16,1: CF the bit shifted out, OF = MSB ^ CF, AF clear as Unicorn leaves it. */
static uint16_t shl1(pr_u77_28be_cpu *cpu, uint16_t value)
{
    uint16_t result = (uint16_t)(value << 1);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (value & 0x8000u) cpu->flags |= 0x0001u;
    if (((result >> 15) ^ (value >> 15)) & 1u) cpu->flags |= 0x0800u;
    szp(cpu, result);
    return result;
}

/* ADC left,0 with carry in `carry`: live only when the divide site is a CODE stop. */
static uint16_t adc0(pr_u77_28be_cpu *cpu, uint16_t left, unsigned carry)
{
    uint16_t result = (uint16_t)(left + carry);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (carry && left == 0xFFFFu) cpu->flags |= 0x0001u;
    if ((left ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (~left & result & 0x8000u) cpu->flags |= 0x0800u;
    szp(cpu, result);
    return result;
}

/* LCALL at original offset `site` to the multiply (divide = 0) or divide (divide = 1); returns 1 when it came back. */
static int far_call(machine *m, uint16_t site, int divide)
{
    pr_u77_28be_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs, entry = divide ? 0x0A76u : 0x0A5Bu;
    int status;
    cpu->ip = at;
    if (m->ram[physical(cpu->cs, at)] != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != entry) {
        m->stop = PR_U77_28BE_CPU_CODE;
        return 0;
    }
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->cs = read16(m, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = entry;
    if (divide) {
        pr_system_longdiv_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_system_longdiv_0a76(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
    } else {
        pr_system_longmul_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_system_longmul_0a5b(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
    }
    if (m->callee_status != NULL)
        *m->callee_status = status;
    if (status != 0) {
        m->stop = PR_U77_28BE_CPU_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U77_28BE_CPU_TRANSFER;
        return 0;
    }
    return 1;
}

/* MOV AL,[BP-9]; CBW: AX := j sign-extended. */
static void load_j(machine *m)
{
    uint8_t j = m->ram[physical(m->cpu->ss, (uint16_t)(m->cpu->bp - 9u))];
    m->cpu->ax = (uint16_t)(j & 0x80u ? 0xFF00u | j : j);
}

pr_u77_28be_cpu_status pr_u77_weighted_28be_cpu(uint8_t *ram, size_t ram_size, pr_u77_28be_cpu *cpu, int *callee_status)
{
    machine m;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U77_28BE_CPU_RAM_SIZE)
        return PR_U77_28BE_CPU_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.entry = cpu->ip; m.callee_status = callee_status; m.stop = PR_U77_28BE_CPU_OK;
    push16(&m, cpu->bp);                                                  /* +28BE PUSH BP; MOV BP,SP; SUB SP,0Ah (flags folded) */
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 0x0Au);
    cpu->ax = 0u;                                                         /* +28C4 XOR AX,AX (flags folded); sum := 0; j := 0 */
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->ax);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 9u))] = 0u;
    for (;;) {
        uint16_t old, added;
        load_j(&m);                                                       /* +28D5 SHL AX,1; LES DI,[BP+6]; ADD DI,AX (flags folded) */
        cpu->ax = (uint16_t)(cpu->ax << 1);
        at = physical(cpu->ss, (uint16_t)(cpu->bp + 6u));
        cpu->di = read16_at(ram, at);
        cpu->es = read16_at(ram, (at + 2u) & MASK);
        cpu->di = (uint16_t)(cpu->di + cpu->ax);
        cpu->ax = read16(&m, cpu->es, (uint16_t)(cpu->di + 0x88u));       /* +28E0 MOV AX,ES:[DI+88h]; XOR DX,DX; MOV CX,AX; MOV BX,DX */
        cpu->dx = 0u;
        cpu->cx = cpu->ax;
        cpu->bx = cpu->dx;
        load_j(&m);                                                       /* +28EB MOV DI,AX; SHL DI,1; SHL DI,1 (the second SHL's flags) */
        cpu->di = (uint16_t)(cpu->ax << 1);
        cpu->di = shl1(cpu, cpu->di);
        cpu->ax = read16(&m, cpu->ds, (uint16_t)(cpu->di + 0x0D24u));     /* +28F5 MOV AX,[DI+0D24h]; MOV DX,[DI+0D26h] */
        cpu->dx = read16(&m, cpu->ds, (uint16_t)(cpu->di + 0x0D26u));
        if (!far_call(&m, 0x28FDu, 0))                                    /* +28FD DX:AX := w[j] * value */
            return m.stop;
        old = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 8u));              /* +2902 ADD [BP-8],AX; ADC [BP-6],DX (flags folded) */
        added = (uint16_t)(old + cpu->ax);
        write16(&m, cpu->ss, (uint16_t)(cpu->bp - 8u), added);
        write16(&m, cpu->ss, (uint16_t)(cpu->bp - 6u),
                (uint16_t)(read16(&m, cpu->ss, (uint16_t)(cpu->bp - 6u)) + cpu->dx + (added < old)));
        if (ram[physical(cpu->ss, (uint16_t)(cpu->bp - 9u))] == 4u)      /* +2908 CMP BYTE [BP-9],4; JNE +28D2 (flags folded) */
            break;
        at = physical(cpu->ss, (uint16_t)(cpu->bp - 9u));                 /* +28D2 INC BYTE [BP-9] (flags folded) */
        ram[at] = (uint8_t)(ram[at] + 1u);
    }
    {
        uint16_t low = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 8u));     /* +290E MOV AX/DX,sum; ADD AX,64h; ADC DX,0 */
        cpu->dx = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 6u));
        cpu->ax = (uint16_t)(low + 0x64u);
        cpu->dx = adc0(cpu, cpu->dx, cpu->ax < low);
    }
    cpu->cx = read16(&m, cpu->ds, 0x0D30u);                               /* +291A MOV CX,[0D30h]; MOV BX,[0D32h] */
    cpu->bx = read16(&m, cpu->ds, 0x0D32u);
    if (!far_call(&m, 0x2922u, 1))                                        /* +2922 DX:AX := (sum + 100) div DS:[0D30h] */
        return m.stop;
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);              /* +2927 MOV [BP-4],AX; MOV [BP-2],DX; reload */
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    cpu->ax = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->dx = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cpu->sp = cpu->bp;                                                    /* +2933 MOV SP,BP; POP BP; RETF 4 */
    cpu->bp = read16(&m, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 4u);
    return PR_U77_28BE_CPU_OK;
}

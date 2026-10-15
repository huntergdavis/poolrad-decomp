#include "poolrad/recovered/overlay_u3c2_caller_windows_0909.h"

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6C_3F31_RAM_SIZE - 1u);
}

static uint8_t read_local(const uint8_t *ram, const pr_u6c_3f31_cpu *cpu,
                          int offset)
{
    return ram[physical(cpu->ss, (uint16_t)(cpu->bp + offset))];
}

static void push(uint8_t *ram, pr_u6c_3f31_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U6C_3F31_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}

pr_u3c2_caller_windows_status pr_u3c2_caller_windows_0909(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu)
{
    unsigned i;
    int first, second, last;
    uint16_t return_ip;
    if (!ram || !cpu || ram_size < PR_U6C_3F31_RAM_SIZE ||
        (cpu->ip != 0x0A71u && cpu->ip != 0x0B73u))
        return PR_U3C2_CALLER_WINDOWS_ARGUMENT;
    first = cpu->ip == 0x0A71u ? -3 : -5;
    second = first - 1;
    last = cpu->ip == 0x0A71u ? 12 : -2;
    return_ip = cpu->ip == 0x0A71u ? 0x0AA5u : 0x0BA7u;
    for (i = 0; i < 4u; ++i) {
        /* MOV AL; XOR AH,AH; MOV DI,AX; MOV AL,[BP+DI-disp]; PUSH AX.
         * Read each local anew: earlier pushes may alias later reads. */
        cpu->ax = read_local(ram, cpu, i < 2u ? first : second);
        cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08D5u) | 0x0044u);
        cpu->di = cpu->ax;
        cpu->ax = read_local(ram, cpu, (int)cpu->di - 15 - (int)i * 4);
        push(ram, cpu, cpu->ax);
    }
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | read_local(ram, cpu, last));
    push(ram, cpu, cpu->ax);
    push(ram, cpu, cpu->cs);
    push(ram, cpu, return_ip);
    cpu->ip = 0x057Bu;
    return PR_U3C2_CALLER_WINDOWS_OK;
}

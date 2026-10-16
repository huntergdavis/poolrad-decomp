#include "poolrad/recovered/overlay_u3c2_entry5_0422.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(pr_u3c2_0422_cpu) == 28u, "14 CPU words");

static size_t address(uint16_t seg, unsigned off)
{
    return ((size_t)seg * 16u + off % 65536u) % PR_U3C2_0422_RAM_SIZE;
}

static void word(uint8_t *ram, uint16_t seg, unsigned off, uint16_t value)
{
    size_t a = address(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) % PR_U3C2_0422_RAM_SIZE] = (uint8_t)(value / 256u);
}

static void far_pointer(uint8_t *ram, uint16_t seg, unsigned off, uint16_t ptr)
{
    size_t a = address(seg, off);
    const uint8_t bytes[4] = {(uint8_t)ptr, (uint8_t)(ptr / 256u), 0u, 0x50u};
    for (unsigned i = 0; i < 4u; ++i)
        ram[(a + i) % PR_U3C2_0422_RAM_SIZE] = bytes[i];
}

static pr_u3c2_0422_cpu initial(uint16_t ss, uint16_t sp)
{
    pr_u3c2_0422_cpu cpu = {0xABCDu, 0x1357u, 0x2468u, 0x3579u,
        0x468Au, 0x579Bu, 0x68ACu, sp, 0xF000u, 0x0422u,
        0x6000u, 0x8000u, ss, 0xFFFFu};
    return cpu;
}

static void setup(uint8_t *ram, const pr_u3c2_0422_cpu *cpu)
{
    unsigned sp = cpu->sp;
    memset(ram, 0xA5, PR_U3C2_0422_RAM_SIZE);
    word(ram, cpu->ss, sp, 0x1234u); word(ram, cpu->ss, sp + 2u, 0xD000u);
    far_pointer(ram, cpu->ss, sp + 4u, 0x1000u);
    far_pointer(ram, cpu->ss, sp + 8u, 0x2000u);
    far_pointer(ram, cpu->ss, sp + 12u, 0x4000u);
    far_pointer(ram, cpu->ss, sp + 16u, 0x3000u);
    word(ram, cpu->ss, sp + 20u, 0xA503u);
    word(ram, cpu->ss, sp + 22u, 0xA502u);
    word(ram, 0x5000u, 0x2000u, 1u);
    ram[address(0x5000u, 0x3000u)] = 5u;
    ram[address(0x5000u, 0x4000u)] = 7u;
}

/* Literal first-callee entry specification. No callback/body executes. */
static void first_entry(uint8_t *ram, pr_u3c2_0422_cpu *cpu)
{
    unsigned sp = cpu->sp;
    word(ram, cpu->ss, sp + 65534u, cpu->bp);
    word(ram, cpu->ss, sp + 65477u, 3u); /* threshold=(2*1)+1 */
    word(ram, cpu->ss, sp + 65507u, 2u);
    word(ram, cpu->ss, sp + 65509u, 3u);
    word(ram, cpu->ss, sp + 65511u, 5u);
    word(ram, cpu->ss, sp + 65513u, 7u);
    word(ram, cpu->ss, sp + 65474u, cpu->ss);
    word(ram, cpu->ss, sp + 65472u, (uint16_t)(sp + 65507u));
    word(ram, cpu->ss, sp + 65470u, cpu->cs);
    word(ram, cpu->ss, sp + 65468u, 0x045Fu);
    cpu->ax = 7u; cpu->di = (uint16_t)(sp + 65507u); cpu->es = 0x5000u;
    cpu->bp = (uint16_t)(sp + 65534u); cpu->sp = (uint16_t)(sp + 65468u);
    cpu->ip = 0x01A0u; cpu->flags = 0xF72Eu;
}

static void missing_hook(uint8_t *ram, uint8_t *expected, uint16_t ss, uint16_t sp)
{
    pr_u3c2_0422_cpu cpu = initial(ss, sp), want;
    setup(ram, &cpu); want = cpu;
    memcpy(expected, ram, PR_U3C2_0422_RAM_SIZE); first_entry(expected, &want);
    assert(pr_u3c2_entry5_0422(ram, PR_U3C2_0422_RAM_SIZE, &cpu, NULL) == PR_U3C2_0422_CALLEE);
    assert(memcmp(&cpu, &want, sizeof(cpu)) == 0);
    assert(memcmp(ram, expected, PR_U3C2_0422_RAM_SIZE) == 0);
}

typedef struct {
    unsigned mode, calls;
    uint8_t *entry_ram, *stopped_ram;
    pr_u3c2_0422_cpu entry_cpu, stopped_cpu;
} stop_context;

static pr_u3c2_0422_callee_status stop_hook(
    void *user, uint8_t *ram, size_t size, pr_u3c2_0422_cpu *cpu)
{
    stop_context *context = user;
    ++context->calls;
    assert(size == PR_U3C2_0422_RAM_SIZE);
    assert(memcmp(cpu, &context->entry_cpu, sizeof(*cpu)) == 0);
    assert(memcmp(ram, context->entry_ram, size) == 0);
    *cpu = (pr_u3c2_0422_cpu){0xA55Au, 0xBEEFu, 0xC0DEu, 0xD00Du,
        0x1111u, 0x2222u, 0x3333u, 0x4444u,
        context->mode == 2u ? 0x1234u : 0xF000u,
        context->mode == 1u ? 0xDEADu : context->mode == 2u ? 0x045Fu : 0x01A0u,
        0x5555u, 0x6666u, 0x7777u, 0x0AD7u};
    ram[0x43210u] = 0xCEu;
    context->stopped_cpu = *cpu;
    memcpy(context->stopped_ram, ram, size);
    return context->mode == 0u ? PR_U3C2_0422_CALLEE_UNRESOLVED :
                                 PR_U3C2_0422_CALLEE_RETURNED;
}

static void hook_stops(uint8_t *ram, uint8_t *expected, uint8_t *stopped)
{
    for (unsigned mode = 0u; mode < 3u; ++mode) {
        pr_u3c2_0422_cpu cpu = initial(0x7000u, 0x8000u);
        stop_context context = {0};
        pr_u3c2_0422_hooks hooks = {stop_hook, NULL, &context};
        setup(ram, &cpu); context.mode = mode;
        context.entry_cpu = cpu; context.entry_ram = expected; context.stopped_ram = stopped;
        memcpy(expected, ram, PR_U3C2_0422_RAM_SIZE);
        first_entry(expected, &context.entry_cpu);
        assert(pr_u3c2_entry5_0422(ram, PR_U3C2_0422_RAM_SIZE, &cpu, &hooks) == PR_U3C2_0422_CALLEE);
        assert(context.calls == 1u);
        assert(memcmp(&cpu, &context.stopped_cpu, sizeof(cpu)) == 0);
        assert(memcmp(ram, stopped, PR_U3C2_0422_RAM_SIZE) == 0);
    }
}

static void guards(uint8_t *ram, uint8_t *expected)
{
    pr_u3c2_0422_cpu cpu = initial(0x7000u, 0x8000u), before = cpu;
    memcpy(expected, ram, PR_U3C2_0422_RAM_SIZE);
    assert(pr_u3c2_entry5_0422(NULL, PR_U3C2_0422_RAM_SIZE, &cpu, NULL) == PR_U3C2_0422_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    assert(pr_u3c2_entry5_0422(ram, PR_U3C2_0422_RAM_SIZE, NULL, NULL) == PR_U3C2_0422_ARGUMENT);
    assert(pr_u3c2_entry5_0422(ram, 0u, &cpu, NULL) == PR_U3C2_0422_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    assert(pr_u3c2_entry5_0422(ram, PR_U3C2_0422_RAM_SIZE - 1u, &cpu, NULL) == PR_U3C2_0422_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    cpu.ip = 0u; before = cpu;
    assert(pr_u3c2_entry5_0422(ram, PR_U3C2_0422_RAM_SIZE, &cpu, NULL) == PR_U3C2_0422_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof(cpu)) == 0);
    assert(memcmp(ram, expected, PR_U3C2_0422_RAM_SIZE) == 0);
}

int main(void)
{
    const uint16_t segments[] = {0x7000u, 0x7000u, 0x7000u, 0x7000u,
                                0x7000u, 0x7000u, 0x7000u, 0xFFFFu};
    const uint16_t stacks[] = {0x8000u, 0u, 1u, 0x003Eu, 0x003Fu, 0xFFFEu, 0xFFFFu, 15u};
    uint8_t *ram = malloc(PR_U3C2_0422_RAM_SIZE), *expected = malloc(PR_U3C2_0422_RAM_SIZE);
    uint8_t *stopped = malloc(PR_U3C2_0422_RAM_SIZE);
    assert(ram && expected && stopped);
    for (unsigned i = 0u; i < sizeof(stacks) / sizeof(stacks[0]); ++i)
        missing_hook(ram, expected, segments[i], stacks[i]);
    hook_stops(ram, expected, stopped); guards(ram, expected);
    free(stopped); free(expected); free(ram);
    puts("U3C2 entry5 boundary-only synthetic: 11 fullRAM/all14 cases; 5 guards PASS");
    return 0;
}

/* Focused synthetic test for the raw-CPU resident 0709:0000 EGA register reset (docs/re/RES_0709_EGA_RESET_0000_CPU.md)
 * and its port environment. No game bytes. */
#include "poolrad/recovered/resident_0709_ega_reset_0000_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "ega reset line %d: %s\n", __LINE__, #x);             \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, SS0 = 0x2000, SP0 = 0x8000 };

typedef struct {
    unsigned count, refuse;
    uint16_t port[32], value[32], ip[32], cs[32];
} log_t;

static uint8_t *ram;
static unsigned checks;

static int record(void *ctx, uint16_t port, uint16_t value, unsigned size, uint16_t cs, uint16_t ip)
{
    log_t *log = ctx;
    if (log->count == log->refuse || size != 1u)
        return 1;
    log->port[log->count] = port; log->value[log->count] = value; log->ip[log->count] = ip; log->cs[log->count] = cs;
    ++log->count;
    return 0;
}

static size_t at(uint16_t segment, uint16_t offset) { return (((size_t)segment << 4) + offset) & (RAM - 1u); }

static pr_r0709_ega_reset_cpu frame(void)
{
    pr_r0709_ega_reset_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, 0x0709u, 0x0000u, 0x1000u, 0x1111u, SS0, 0x7AD7u};
    memset(ram, 0xA5, RAM);
    ram[at(SS0, SP0)] = 0x00; ram[at(SS0, SP0 + 1u)] = 0x70;
    ram[at(SS0, SP0 + 2u)] = 0x01; ram[at(SS0, SP0 + 3u)] = 0x70;
    return cpu;
}

int main(void)
{
    static const uint16_t ports[18] = {0x3CE, 0x3CF, 0x3CE, 0x3CF, 0x3CE, 0x3CF, 0x3CE, 0x3CF, 0x3CE, 0x3CF, 0x3CE, 0x3CF,
                                       0x3CE, 0x3CF, 0x3CE, 0x3CF, 0x3C4, 0x3C5};
    static const uint16_t values[18] = {0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 7, 0x0F, 8, 0xFF, 2, 0x0F};
    pr_r0709_ega_reset_cpu cpu;
    log_t log;
    pr_port_env env;
    unsigned k;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;
    env.ctx = &log; env.out = record; env.in = NULL;

    /* All eighteen OUTs in order, each at its own IP 0008h + 6k; AL = 0Fh with AH kept, DX = 03C5h, FLAGS untouched. */
    memset(&log, 0, sizeof log); log.refuse = 99;
    cpu = frame();
    CHECK(pr_r0709_ega_reset_0000_cpu(ram, RAM, &cpu, &env) == PR_R0709_EGA_RESET_OK && log.count == 18);
    for (k = 0; k < 18; ++k)
        CHECK(log.port[k] == ports[k] && log.value[k] == values[k] && log.ip[k] == 0x0008u + 6u * k && log.cs[k] == 0x0709u);
    CHECK(cpu.ax == 0x120Fu && cpu.dx == 0x03C5u && cpu.flags == 0x7AD7u && cpu.bp == 0x7777u && cpu.bx == 0x3456u);
    CHECK(cpu.cs == 0x7001u && cpu.ip == 0x7000u && cpu.sp == SP0 + 4u && ram[at(SS0, SP0 - 2u)] == 0x77 && ram[at(SS0, SP0 - 1u)] == 0x77);

    /* The environment refuses the fourth OUT (GC data for index 1): PORT at 001Ah with AL and DX loaded, BP framed. */
    memset(&log, 0, sizeof log); log.refuse = 3;
    cpu = frame();
    CHECK(pr_r0709_ega_reset_0000_cpu(ram, RAM, &cpu, &env) == PR_R0709_EGA_RESET_PORT && log.count == 3);
    CHECK(cpu.ip == 0x001Au && cpu.ax == 0x1200u && cpu.dx == 0x03CFu && cpu.sp == SP0 - 2u && cpu.bp == SP0 - 2u);

    /* Guards change nothing. */
    cpu = frame();
    CHECK(pr_r0709_ega_reset_0000_cpu(ram, RAM, &cpu, NULL) == PR_R0709_EGA_RESET_ARGUMENT && cpu.sp == SP0);
    env.out = NULL;
    CHECK(pr_r0709_ega_reset_0000_cpu(ram, RAM, &cpu, &env) == PR_R0709_EGA_RESET_ARGUMENT);
    env.out = record;
    cpu.ip = 0x0001u;
    CHECK(pr_r0709_ega_reset_0000_cpu(ram, RAM, &cpu, &env) == PR_R0709_EGA_RESET_ARGUMENT && cpu.sp == SP0);
    cpu.ip = 0;
    CHECK(pr_r0709_ega_reset_0000_cpu(ram, RAM - 1u, &cpu, &env) == PR_R0709_EGA_RESET_ARGUMENT);

    free(ram);
    printf("ega reset 0000: %u checks passed\n", checks);
    return 0;
}

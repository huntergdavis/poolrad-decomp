/* Focused test for the memory/port bus and the bus-aware SYSTEM FillChar (docs/re/RES_0709_RECT_FILL_00B7_DESIGN.md). No
 * game bytes. */
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/system_fillchar_bus_16a8_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "bus line %d: %s\n", __LINE__, #x);                   \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, SS0 = 0x2000, SP0 = 0x8000 };

static unsigned checks;
static unsigned logged, refuse;
static uint16_t log_offset[8];
static uint8_t log_value[8];

static void on_write(void *ctx, uint16_t offset, uint8_t value)
{
    (void)ctx;
    if (logged < 8u) { log_offset[logged] = offset; log_value[logged] = value; }
    ++logged;
}

static int out(void *ctx, uint16_t port, uint16_t value, unsigned size, uint16_t cs, uint16_t ip)
{
    (void)ctx; (void)port; (void)value; (void)size; (void)cs; (void)ip;
    return refuse != 0u;
}

int main(void)
{
    uint8_t *ram = malloc(RAM);
    pr_ega *ega = malloc(sizeof *ega);
    pr_port_env env = {NULL, out, NULL};
    pr_bus bus;
    pr_system_leaf_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, 0x0709u, 0x0145u, 0x1000u, 0x1111u, SS0, 0x7602u};
    static const uint16_t frame[6] = {0x0145u, 0x0709u, 0x000Au, 3u, 0x0010u, 0xA000u};
    unsigned i;
    if (ram == NULL || ega == NULL)
        return 1;
    memset(ram, 0xA5, RAM);
    pr_ega_reset(ega);
    bus.ram = ram; bus.ega = ega; bus.ports = &env; bus.on_vram_write = on_write; bus.ctx = NULL;

    /* A000 writes and reads go through the EGA (write mode 0 replaces all planes); RAM there is untouched. */
    pr_bus_write8(&bus, 0xA0010u, 0x3Cu);
    CHECK(ega->plane[0][0x10] == 0x3C && ega->plane[3][0x10] == 0x3C && ram[0xA0010] == 0xA5 && logged == 1 && log_offset[0] == 0x10);
    CHECK(pr_bus_read8(&bus, 0xA0010u) == 0x3C && ega->latch[2] == 0x3C);
    pr_bus_write8(&bus, 0x12345u, 0x77u);
    CHECK(ram[0x12345] == 0x77 && pr_bus_read8(&bus, 0x12345u) == 0x77 && logged == 1);

    /* OUTs reach the EGA unless the environment refuses them. */
    CHECK(pr_bus_out(&bus, 0x3CEu, 0x0205u, 2u, 0x0709u, 0x0108u) == 0 && ega->gc[5] == 2);
    refuse = 1;
    CHECK(pr_bus_out(&bus, 0x3CEu, 0x0005u, 2u, 0x0709u, 0x0156u) == 1 && ega->gc[5] == 2);
    refuse = 0;

    /* FillChar(A000:0010, 3, 0Ah) in write mode 2: planes 1 and 3 become FFh, 0 and 2 become 00h; DF cleared. */
    logged = 0;
    for (i = 0; i < 6; ++i) {
        ram[(((size_t)SS0 << 4) + SP0 + 2u * i)] = (uint8_t)frame[i];
        ram[(((size_t)SS0 << 4) + SP0 + 2u * i + 1u)] = (uint8_t)(frame[i] >> 8);
    }
    CHECK(pr_system_fillchar_bus_16a8(&bus, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(ega->plane[1][0x10] == 0xFF && ega->plane[3][0x12] == 0xFF && ega->plane[0][0x11] == 0x00 && ega->plane[2][0x12] == 0x00);
    CHECK(logged == 3 && log_offset[2] == 0x12 && log_value[1] == 0x0A && ram[0xA0011] == 0xA5);
    CHECK(cpu.bx == SP0 && cpu.cx == 0 && cpu.di == 0x0013u && cpu.es == 0xA000u && cpu.ax == 0x120Au && cpu.flags == 0x7202u);
    CHECK(cpu.cs == 0x0709u && cpu.ip == 0x0145u && cpu.sp == SP0 + 12u);

    /* Without an EGA, A000 is plain RAM. */
    bus.ega = NULL;
    cpu.sp = SP0;
    CHECK(pr_system_fillchar_bus_16a8(&bus, &cpu) == PR_SYSTEM_LEAF_RETURNED && ram[0xA0010] == 0x0A && ram[0xA0012] == 0x0A && ram[0xA0013] == 0xA5);

    /* Guards. */
    CHECK(pr_system_fillchar_bus_16a8(NULL, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);
    bus.ram = NULL;
    CHECK(pr_system_fillchar_bus_16a8(&bus, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);

    free(ega);
    free(ram);
    printf("memory bus: %u checks passed\n", checks);
    return 0;
}

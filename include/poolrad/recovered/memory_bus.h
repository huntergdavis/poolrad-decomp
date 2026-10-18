#ifndef POOLRAD_RECOVERED_MEMORY_BUS_H
#define POOLRAD_RECOVERED_MEMORY_BUS_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/ega_hw.h"
#include "poolrad/recovered/port_env.h"

#define PR_BUS_RAM_SIZE (1u << 20)

/* Memory and port bus for raw-CPU models of the port-bound RES 00BA:16EC closure (docs/re/RES_0709_RECT_FILL_00B7_DESIGN.md).
 * With an EGA attached, byte accesses to physical A0000h..AFFFFh go through pr_ega (reads load the latches, writes go
 * through the write mode) instead of RAM; everything else is the 1 MiB RAM. OUTs go first to ports->out (when set: an
 * audit's trace or a refusal), then to the EGA. on_vram_write (may be NULL) sees every A000 write in order. */
typedef struct {
    uint8_t *ram;
    pr_ega *ega;
    const pr_port_env *ports;
    void (*on_vram_write)(void *ctx, uint16_t offset, uint8_t value);
    void *ctx;
} pr_bus;

uint8_t pr_bus_read8(pr_bus *bus, size_t physical);
void pr_bus_write8(pr_bus *bus, size_t physical, uint8_t value);
/* Nonzero when ports->out refused the OUT; the EGA is then unchanged. */
int pr_bus_out(pr_bus *bus, uint16_t port, uint16_t value, unsigned size, uint16_t cs, uint16_t ip);

#endif

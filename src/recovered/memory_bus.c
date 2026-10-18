/* Memory and port bus over RAM and the EGA model; docs/re/RES_0709_RECT_FILL_00B7_DESIGN.md. */
#include "poolrad/recovered/memory_bus.h"

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define VRAM 0xA0000u

static int vram(const pr_bus *bus, size_t physical)
{
    return bus->ega != NULL && (physical & MASK) >= VRAM && (physical & MASK) < VRAM + 0x10000u;
}

uint8_t pr_bus_read8(pr_bus *bus, size_t physical)
{
    physical &= MASK;
    if (vram(bus, physical))
        return pr_ega_read(bus->ega, (uint16_t)(physical - VRAM));
    return bus->ram[physical];
}

void pr_bus_write8(pr_bus *bus, size_t physical, uint8_t value)
{
    physical &= MASK;
    if (vram(bus, physical)) {
        if (bus->on_vram_write != NULL)
            bus->on_vram_write(bus->ctx, (uint16_t)(physical - VRAM), value);
        (void)pr_ega_write(bus->ega, (uint16_t)(physical - VRAM), value);
        return;
    }
    bus->ram[physical] = value;
}

int pr_bus_out(pr_bus *bus, uint16_t port, uint16_t value, unsigned size, uint16_t cs, uint16_t ip)
{
    if (bus->ports != NULL && bus->ports->out != NULL && bus->ports->out(bus->ports->ctx, port, value, size, cs, ip) != 0)
        return 1;
    if (bus->ega != NULL)
        (void)pr_ega_out(bus->ega, port, value, size);
    return 0;
}

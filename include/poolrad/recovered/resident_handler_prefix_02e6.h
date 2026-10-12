#ifndef POOLRAD_RECOVERED_RESIDENT_HANDLER_PREFIX_02E6_H
#define POOLRAD_RECOVERED_RESIDENT_HANDLER_PREFIX_02E6_H

#include <stddef.h>
#include <stdint.h>

#define PR_HANDLER_02E6_RAM_SIZE (1u << 20)

/* State immediately after INT 3Fh enters the loaded resident handler.
 * The interrupt frame is already present in SS RAM. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_handler_02e6_cpu;

typedef enum {
    PR_HANDLER_02E6_AT_LOADER = 0,
    PR_HANDLER_02E6_ARGUMENT
} pr_handler_02e6_status;

/* Translate +02E6 up to, but excluding, the first CALL +0338.
 * AT_LOADER leaves IP at +0303 or +0314 with all prior RAM/register
 * effects applied. The caller must execute the loader and handler tail. */
pr_handler_02e6_status pr_resident_handler_prefix_02e6(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu);

#endif

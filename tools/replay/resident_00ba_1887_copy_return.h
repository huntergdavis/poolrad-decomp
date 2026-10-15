#ifndef POOLRAD_RESIDENT_00BA_1887_COPY_RETURN_H
#define POOLRAD_RESIDENT_00BA_1887_COPY_RETURN_H

#include "resident_1816_caller_routes.h"

/* At the nonzero 1816 suffix-copy call entry, execute the original SYSTEM
 * 0AF8:0B0B helper through its RETF 4. Stop at 00BA:1887, before the
 * second printer call. The caller installs the pinned original helper and
 * private source literal in flat 20-bit RAM. Returns zero on success. */
int pr_1887_display_copy_return(uint8_t *ram, size_t ram_size,
                                pr_1816_caller_cpu *cpu);

#endif

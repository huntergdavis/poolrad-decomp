#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_CHARACTER_0BB8_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_CHARACTER_0BB8_CPU_H
#include "poolrad/recovered/overlay_u9e_entry16_10e4.h"
typedef pr_u9e_10e4_cpu pr_res00ba_cpu;
typedef enum {
    PR_RES00BA_OK = 0,
    PR_RES00BA_ARGUMENT,
    PR_RES00BA_LIMIT,
    PR_RES00BA_TRANSFER,
    PR_RES00BA_FAULT
} pr_res00ba_status;
/* Raw post-CALL CPU14 and exactly 1 MiB A20-off RAM. Code is immutable;
 * CPU and budget storage must be distinct from RAM. Each executed resident
 * instruction consumes one budget unit; LIMIT leaves the unexecuted IP and
 * all earlier effects. Word/LES operands are physically consecutive after
 * 16-bit effective-address wrapping; RETF pops IP and CS separately.
 * Full execution profile and tested boundaries are documented with the chain.
 */
pr_res00ba_status pr_res00ba_0bb8_cpu(uint8_t *ram, size_t ram_size,
                                    pr_res00ba_cpu *cpu, size_t *budget);
/* Same contract; entry IP is 0000, 01FB, 0282, 03A0 or 0BB8. */
pr_res00ba_status pr_res00ba_core_cpu(uint8_t *ram, size_t ram_size,
                                   pr_res00ba_cpu *cpu, size_t *budget);
/* Same contract; entry IP is 10E6, 1168, 11EA, 1282, 1325 or 13AB. */
pr_res00ba_status pr_res00ba_helpers_cpu(uint8_t *ram, size_t ram_size,
                                      pr_res00ba_cpu *cpu, size_t *budget);
#endif

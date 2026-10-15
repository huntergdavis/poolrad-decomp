#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_RECORD_CLEAR_HIGH_0159_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_RECORD_CLEAR_HIGH_0159_H

#include <stddef.h>
#include <stdint.h>

#define PR_U6A_0159_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6a_0159_cpu;

typedef enum {
    PR_U6A_0159_OK = 0,
    PR_U6A_0159_ARGUMENT
} pr_u6a_0159_status;

/* Complete +0159..+019B, entered after CALL with a far record pointer
 * at SS:SP+4. Clears bytes >7F at record+17..2B, then clears record+2C.
 * Argument/index reloads, frame aliases, all CPU words and defined FLAGS
 * follow the original. RETF 4 advances entry SP by eight bytes.
 * Effective offsets/SP wrap at 16 bits; multi-byte operands are contiguous
 * physical bytes with A20 off. Code must remain immutable.
 */
pr_u6a_0159_status pr_u6a_record_clear_high_0159(
    uint8_t *ram, size_t ram_size, pr_u6a_0159_cpu *cpu);

#endif

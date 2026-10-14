#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_RECORD_REMAINING_025C_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_RECORD_REMAINING_025C_H

#include <stddef.h>
#include <stdint.h>

#define PR_U6A_025C_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6a_025c_cpu;

typedef enum {
    PR_U6A_025C_OK = 0,
    PR_U6A_025C_ARGUMENT
} pr_u6a_025c_status;

/* Complete unit 0x6A:+025C..+0301, entered after CALL. SS:SP has the
 * return IP/CS, followed by two word arguments (only low bytes used).
 * Scans 21 bytes at DS:[5D92]+17h, counts nonzero entries whose masked
 * descriptor matches both arguments, and subtracts the count from
 * record[B1h + 3*signed(arg0) + signed(arg1)]. AX retains the 16-bit
 * difference, including borrow in AH; AL is the byte result. RETF 4.
 * Models shared A20-off RAM, stack remnants, aliases and returned flags.
 * Invalid host arguments are rejected without mutation. */
pr_u6a_025c_status pr_u6a_record_remaining_025c(
    uint8_t *ram, size_t ram_size, pr_u6a_025c_cpu *cpu);

#endif

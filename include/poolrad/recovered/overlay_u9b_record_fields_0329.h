#ifndef POOLRAD_RECOVERED_OVERLAY_U9B_RECORD_FIELDS_0329_H
#define POOLRAD_RECOVERED_OVERLAY_U9B_RECORD_FIELDS_0329_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9B_0329_RAM_SIZE (1u << 20)

/* State at entry after the caller's near CALL. SS RAM already contains
 * return IP, caller CS, and the far record argument. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9b_0329_cpu;

typedef enum {
    PR_U9B_0329_OK = 0,
    PR_U9B_0329_ARGUMENT
} pr_u9b_0329_status;

/* Complete terminating paths of GAME.OVR unit 0x9B:+0x0329..+0x045D.
 * All SS/DS/ES accesses share A20-off 1 MiB RAM. Aliased locals, cached
 * pointer words, record fields, and table entries are reread in DOS order.
 * A self-sustaining alias cycle loops as in the original. */
pr_u9b_0329_status pr_u9b_record_fields_0329(
    uint8_t *ram, size_t ram_size, pr_u9b_0329_cpu *cpu);

#endif

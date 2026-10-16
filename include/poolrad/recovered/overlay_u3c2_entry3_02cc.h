#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY3_02CC_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY3_02CC_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3C2_02CC_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c2_02cc_cpu;

typedef enum {
    PR_U3C2_02CC_OK = 0,
    PR_U3C2_02CC_ARGUMENT
} pr_u3c2_02cc_status;

/* Execute GAME.OVR unit 0x3C2 public entry 3, +02CC..+0421.
 * SS:SP holds far return IP/CS, then one far record pointer; RETF 4.
 * Requires at least 1 MiB RAM, CPU storage distinct from RAM, IP=02CC,
 * immutable code and synchronous raw RAM. Word/LES bytes are physically
 * contiguous with 20-bit wrap; each stack operation wraps its 16-bit offset.
 * Record pointers are reloaded live from stack locals before each access.
 * All register/FLAGS outputs and memory writes follow the complete body.
 * Invalid arguments reject atomically. No natural game reach is implied. */
pr_u3c2_02cc_status pr_u3c2_entry3_02cc(
    uint8_t *ram, size_t ram_size, pr_u3c2_02cc_cpu *cpu);

#endif

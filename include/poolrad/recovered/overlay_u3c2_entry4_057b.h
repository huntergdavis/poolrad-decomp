#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY4_057B_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY4_057B_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3C2_057B_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c2_057b_cpu;

typedef enum {
    PR_U3C2_057B_OK = 0,
    PR_U3C2_057B_REJECT
} pr_u3c2_057b_status;

/* U3C2 entry 4 [057B,0909), including its far return and ten argument bytes.
 * Enter with IP=057B and a far-return frame followed by five word arguments:
 * direction, target Y, target X, origin Y, origin X (only low bytes are used).
 * Requires >=1MiB synchronous RAM distinct from CPU, and immutable code.
 * DS:2736/273F are caller-supplied displacement bytes, not host constants.
 * Offsets wrap16; word bytes are contiguous physical20 (A20 off).
 * Modes 9..254 may leave SS:[BP-1] unwritten: its existing byte is returned.
 * FF rewrites the direction argument to 8. All CPU/RAM effects are retained.
 * RETF uses separately wrapped word pops; native boundary proof is pending.
 * Null/short RAM, null CPU or wrong IP reject without changes.
 * OK means model completion, not native-route or strict-credit certification.
 */
pr_u3c2_057b_status pr_u3c2_entry4_057b(
    uint8_t *ram, size_t ram_size, pr_u3c2_057b_cpu *cpu);

#endif

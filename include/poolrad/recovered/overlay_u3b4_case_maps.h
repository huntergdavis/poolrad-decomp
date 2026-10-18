#ifndef POOLRAD_RECOVERED_OVERLAY_U3B4_CASE_MAPS_H
#define POOLRAD_RECOVERED_OVERLAY_U3B4_CASE_MAPS_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B4_CASE_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b4_case_cpu;

typedef enum {
    PR_U3B4_CASE_OK = 0,
    PR_U3B4_CASE_ARGUMENT   /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
} pr_u3b4_case_status;

/* Complete GAME.OVR unit 0x3B4 nested functions +00C7..+00FA (52 bytes,
 * 21 instructions), +00FB..+012E (52 bytes, 21 instructions) and
 * +012F..+016C (62 bytes, 25 instructions); no calls. Each is entered
 * after the far CALL (in the game PUSH CS; CALL from +016D) with SS:[SP]
 * return IP, [SP+2] return CS, [SP+4] the caller's frame link, which is
 * never read, and [SP+6] a word whose low byte b is the argument. Each
 * pushes BP, reserves two bytes, compares b with constants in order,
 * stores the selected byte in [BP-1] and returns it in AL with AH
 * unchanged:
 *   +00C7: 08h -> 00h, 41h -> 00h, E8h -> 0Fh, otherwise 0Fh;
 *   +00FB: 08h -> 08h, 41h -> 07h, E8h -> 07h, otherwise 07h;
 *   +012F: 0Bh -> 08h, 09h -> 06h, DBh -> 06h, 06h -> 01h, otherwise 06h.
 * The final flags are the last CMP AL,imm8's. Only the saved BP and
 * [BP-1] are written; [BP-2] is reserved but never written. RETF 4 leaves
 * SP eight bytes above entry. Offsets wrap at 16 bits and physical
 * addresses at 1 MiB; the RETF frame is read physically contiguous, as
 * the Unicorn oracle reads it. */
pr_u3b4_case_status pr_u3b4_case_map_00c7(uint8_t *ram, size_t ram_size, pr_u3b4_case_cpu *cpu);
pr_u3b4_case_status pr_u3b4_case_map_00fb(uint8_t *ram, size_t ram_size, pr_u3b4_case_cpu *cpu);
pr_u3b4_case_status pr_u3b4_case_map_012f(uint8_t *ram, size_t ram_size, pr_u3b4_case_cpu *cpu);

#endif

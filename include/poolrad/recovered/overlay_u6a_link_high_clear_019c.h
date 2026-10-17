#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_LINK_HIGH_CLEAR_019C_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_LINK_HIGH_CLEAR_019C_H

#include <stddef.h>
#include <stdint.h>

#define PR_U6A_019C_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the unit 0x86 +323D predicate model. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6a_019c_cpu;

typedef enum {
    PR_U6A_019C_OK = 0,
    PR_U6A_019C_ARGUMENT,   /* null RAM/CPU or RAM below 1 MiB; nothing changed */
    PR_U6A_019C_TRANSFER    /* the callee's RETF frame no longer returned to +01C8 */
} pr_u6a_019c_status;

/* Complete GAME.OVR unit 0x6A:+019C..+0213 (120 bytes, 42 instructions),
 * entered after the far CALL: SS:[SP] return IP, [SP+2] return CS,
 * [SP+4] record offset, [SP+6] record segment; RETF 4.
 * It walks the far-linked list headed at record +C8/+CA, following each
 * node's +2A/+2C link until both pointer words are zero. For every node
 * the far call at +01C3 (stub 0096:003E, unit 0x86 public entry 6 +323D,
 * the merged pr_u86_record_predicate_323d) decides whether node bytes
 * +3C, +3D and +3E have bit 7 cleared. Every operand is reread from RAM
 * in source order, so aliased records, nodes, stack locals and frames
 * behave as in DOS. A cyclic list does not return, as in DOS. The far
 * call is modelled as the loaded-state transfer: return CS:IP pushed,
 * callee executed on the same RAM/CPU, RETF frame checked. The entry
 * cpu->ip is the body's load offset (0x019C in the game); the far call
 * pushes the return offset relative to it. */
pr_u6a_019c_status pr_u6a_link_high_clear_019c(
    uint8_t *ram, size_t ram_size, pr_u6a_019c_cpu *cpu);

#endif

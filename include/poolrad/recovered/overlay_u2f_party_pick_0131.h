#ifndef POOLRAD_RECOVERED_OVERLAY_U2F_PARTY_PICK_0131_H
#define POOLRAD_RECOVERED_OVERLAY_U2F_PARTY_PICK_0131_H

#include <stddef.h>
#include <stdint.h>

#define PR_U2F_0131_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the merged unit 0x9E entry-8 chain. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u2f_0131_cpu;

typedef enum {
    PR_U2F_0131_OK = 0,
    PR_U2F_0131_ARGUMENT,   /* null RAM/CPU or RAM not exactly 1 MiB; nothing changed */
    PR_U2F_0131_CODE,       /* the far-call site or its stub is not the loaded-state original */
    PR_U2F_0131_CALLEE,     /* the merged entry-8 chain stopped (its status is in *chain_status) */
    PR_U2F_0131_TRANSFER    /* the callee's RETF frame no longer returned to +0162 */
} pr_u2f_0131_status;

/* Complete GAME.OVR unit 0x2F:+0131..+01F5 (197 bytes, 68 instructions),
 * entered after the far CALL: SS:[SP] return IP, [SP+2] return CS, [SP+4]
 * and [SP+6] a far pointer to the result; RETF 4. It walks the party list
 * headed at DS:5D96/5D98 through node links +104/+106. For every node it
 * rolls once through unit 0x9E public entry 8 (+0DE3, stub 00B0:0048,
 * count 1, range 64h), reads the signed byte +3 of the record that the
 * node's far pointer +108 names, and keeps the node whose byte is highest,
 * a tie going to the higher roll; the kept node is stored through the
 * result pointer as it is found. If no byte was above zero the result is
 * cleared to 0:0. The far call is modelled as the loaded-state transfer:
 * the relocated segment word is read from the call site at cs:(ip+2C),
 * the five-byte far jump at that paragraph's offset 48 supplies the callee
 * CS:IP, the merged pr_u9e_entry8_fullstate_compose_0de3 runs on the same
 * RAM and CPU words (SYSTEM at 0AF8:11C0, seed at DS:4428), and the RETF
 * frame is checked. cpu->ip is the body's load offset (0x0131 in the game).
 * Every operand is reread from RAM; a cyclic list does not return. */
pr_u2f_0131_status pr_u2f_party_pick_0131(
    uint8_t *ram, size_t ram_size, pr_u2f_0131_cpu *cpu, int *chain_status);

#endif

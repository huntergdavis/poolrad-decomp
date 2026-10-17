#ifndef POOLRAD_RECOVERED_OVERLAY_U33_GRID_DRIVER_11B4_H
#define POOLRAD_RECOVERED_OVERLAY_U33_GRID_DRIVER_11B4_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_11B4_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the composed unit 0x33 models. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_11b4_cpu;

typedef enum {
    PR_U33_11B4_OK = 0,
    PR_U33_11B4_ARGUMENT,   /* null RAM/CPU or RAM not exactly 1 MiB; nothing changed */
    PR_U33_11B4_CODE,       /* a far-call site or its stub is not the loaded-state original */
    PR_U33_11B4_CALLEE,     /* a composed body stopped (its status is in *chain_status) */
    PR_U33_11B4_TRANSFER    /* a RETF frame no longer returned to its call site */
} pr_u33_11b4_status;

/* Complete GAME.OVR unit 0x33:+11B4..+122B (120 bytes, 63 instructions),
 * entered after the far CALL with no arguments; RETF. It fills the
 * 4E2h bytes from the far grid DS:6716/6718 plus 7 with 17h through the
 * SYSTEM FillChar at 0AF8:16A8 (folded here from its pinned twenty
 * bytes; the relocated segment word is read from the call site), takes
 * the window bytes +186 and +188 of DS:49D2/49D4 (adding 0Dh or 1Ah to
 * the first when DS:49FA is 3 or 4), stores DS:3648 + 2Ch * second +
 * first at DS:465A, and then runs +0BDE, +0D13, +0E9E and +0000 in that
 * order through PUSH CS/CALL (the composed pr_u33_grid_diagonal_0bde,
 * pr_u33_grid_pair_0d13, pr_u33_grid_bands_0e9e and pr_u33_grid_roll_0000,
 * entered at ip - 5D6, ip - 4A1, ip - 316 and ip - 11B4). cpu->ip is the
 * body's load offset (0x11B4 in the game). Every operand is reread from
 * RAM in source order. */
pr_u33_11b4_status pr_u33_grid_driver_11b4(
    uint8_t *ram, size_t ram_size, pr_u33_11b4_cpu *cpu, int *chain_status);

#endif

#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_CLEARMONSTERS_125D_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_CLEARMONSTERS_125D_H

/* Bounded model of DOS v1.3 GAME.OVR unit 0x1c +0x125D..+0x12C5 (file 0x1FA3..0x200B, exactly 105 bytes,
 * 36 instructions, `retf` last byte, six bytes of locals, no operands, no result): the handler of ECL opcode
 * 0x1C (CLEARMONSTERS, reference name). The next routine is +0x12C6. Evidence: docs/re/U1C_CLEARMONSTERS_125D.md.
 *
 * In the original's order:
 *   [0x49ED] := [0x49ED] + 1  (the PC skips the opcode byte)             (+0x1263)
 *   [0x4435] := 0; [0x84E7] := 0; [0x6F83] := 8                         (+0x1267..+0x1271)
 *   FillChar(DS:0x67F4, 0x1C, 0)  (0xAF8:0x16A8: `rep stosb`; the 28 bytes end at 0x680F, so the list
 *       head words at 0x6810..0x6813 are not part of the block)          (+0x1276..+0x1282)
 *   while far [0x6810]:[0x6812] <> nil (both words ORed):                (+0x1287..+0x12C0)
 *       link := the two words at node+0x2A (offset) and node+0x2C (segment), read before the free
 *       FreeMem(head, 0x3F)  (0xAF8:0x0254, the Pascal heap manager at 0xAF8:0x03D3; a failed free
 *           is runtime error 204)
 *       head := link
 *   return
 *
 * FillChar is translated exactly (a zero fill of the 28-byte DS block, supplied as host state). The monster
 * list lives in the Pascal heap: the nodes are explicit host state (a view of the node's segment from offset 0,
 * the far offset and the far next link at +0x2A/+0x2C, as in the other party-list models) and the heap manager is
 * the explicit host boundary `free_af8_0254(node, 0x3F)`: a fallible hook (missing: HOOK status; nonzero return:
 * CALLBACK status; the head is not advanced past a node whose free did not succeed). No heap parity is claimed. The
 * list has no cycle check in the original; the model's cap is a host guard.
 *
 * Classification: bounded model (the heap manager is an explicit boundary). Not a strict translation. No
 * game-function credit is claimed. */

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/ecl_helpers.h"

#define PR_U1C_CMON_BLOCK_SIZE 0x1Cu           /* mov ax, 0x1C */
#define PR_U1C_CMON_NODE_SIZE 0x3Fu            /* mov ax, 0x3F : FreeMem size */
#define PR_U1C_CMON_LINK_OFFSET 0x2Au          /* es:[di + 0x2A] offset word, +0x2C segment word */
#define PR_U1C_CMON_LIST_CAP 65536u            /* host guard */
#define PR_U1C_CMON_B6F83 8u                   /* mov byte [0x6F83], 8 */

typedef struct pr_u1c_cmon_node {
    const uint8_t *segment;                    /* host bytes of the node's ES segment, from offset 0 */
    size_t segment_size;                       /* at most 0x10000 */
    uint16_t offset;                           /* the far pointer's offset word */
    const struct pr_u1c_cmon_node *next;       /* the far pointer at +0x2A/+0x2C (NULL = both words zero) */
} pr_u1c_cmon_node;

typedef struct pr_u1c_cmon_env pr_u1c_cmon_env;

typedef struct {
    /* 0xAF8:0x0254 FreeMem(far head, 0x3F). Nonzero return stops the handler with PR_U1C_CMON_CALLBACK. */
    int (*free_af8_0254)(void *ctx, pr_u1c_cmon_env *env, const pr_u1c_cmon_node *node, uint16_t size);
} pr_u1c_cmon_hooks;

struct pr_u1c_cmon_env {
    uint8_t b4435, b84e7, b6f83;               /* DS bytes written */
    uint8_t block_67f4[PR_U1C_CMON_BLOCK_SIZE];   /* DS:0x67F4..0x680F */
    const pr_u1c_cmon_node *head;              /* far [0x6810]:[0x6812] (NULL = both words zero) */
    pr_u1c_cmon_hooks hooks;
    void *ctx;
};

typedef enum {
    PR_U1C_CMON_OK = 0,
    PR_U1C_CMON_ARGUMENT,   /* null state or env */
    PR_U1C_CMON_LIST,       /* host guard: a node has a null or too small view for its four link bytes, or the list exceeds the
                               cap; the earlier frees stand and the PC has advanced */
    PR_U1C_CMON_HOOK,       /* a reached free hook is NULL; the head stays at that node */
    PR_U1C_CMON_CALLBACK    /* the free hook returned nonzero (report->callback_status); the head stays at that node */
} pr_u1c_cmon_status;

typedef struct {
    unsigned freed;                            /* successful frees */
    int callback_status;
} pr_u1c_cmon_report;

pr_u1c_cmon_status pr_u1c_op_clearmonsters(pr_rec_ecl_loader_state *state, pr_u1c_cmon_env *env,
                                           pr_u1c_cmon_report *report);

#endif

#ifndef POOLRAD_RECOVERED_RESIDENT_LOAD_PIECES_0767_H
#define POOLRAD_RECOVERED_RESIDENT_LOAD_PIECES_0767_H

#include <stddef.h>
#include <stdint.h>

/* Bounded local-control model of RES_00BA:0767 at loaded image 0x01307.
 * Pointers are DOS far identities, not host addresses. */
typedef struct {
    uint16_t segment;
    uint16_t offset;
} pr_rec_0767_far;

/* Each event stands at a decoded call boundary. args are the low bytes of
 * the scalar pushed words, in push order; any far pointer is in node instead.
 * The callee inspection confirms these are the bytes consumed. TITLE_A/B
 * identify private Pascal literals
 * by call site; their contents are not part of this public model. node is
 * used by ACTIVE_TEXT, OTHER_TEXT, FIELD_111 and FIELD_11B. */
typedef enum {
    PR_REC_0767_TITLE_A,
    PR_REC_0767_TITLE_B,
    PR_REC_0767_FRAME,
    PR_REC_0767_ACTIVE_TEXT,
    PR_REC_0767_OTHER_TEXT,
    PR_REC_0767_FIELD_111,
    PR_REC_0767_FIELD_11B
} pr_rec_0767_event_kind;

typedef struct {
    pr_rec_0767_event_kind kind;
    pr_rec_0767_far node;
    uint8_t args[4];
    uint8_t arg_count;
} pr_rec_0767_event;

typedef struct {
    void *ctx;
    /* These reads occur at their decoded points, after prior effect calls.
     * read_head obtains DS:0x5D96; read_byte sees offsets 0x111/0x11B;
     * read_next obtains the far pointer at node +0x104. */
    int (*read_head)(void *ctx, pr_rec_0767_far *head);
    int (*read_byte)(void *ctx, pr_rec_0767_far node,
                     uint16_t displacement, uint8_t *value);
    int (*read_next)(void *ctx, pr_rec_0767_far node,
                     pr_rec_0767_far *next);
    /* Stands at the complete called routine. The callback must supply AL;
     * only AL from the final FRAME event is returned to this caller. */
    int (*effect)(void *ctx, const pr_rec_0767_event *event, uint8_t *al);
} pr_rec_0767_io;

typedef enum {
    PR_REC_0767_OK = 0,
    PR_REC_0767_ARGUMENT,
    PR_REC_0767_BOUNDARY,
    PR_REC_0767_NODE_LIMIT
} pr_rec_0767_status;

/* DOS frame: active far pointer at [BP+6], RETF 4. The mode byte is the
 * value read at DS:0x49F3; the head is read through read_head later in the
 * body. max_nodes is a host cycle bound, not a DOS exit. On host failure or
 * node limit, *al is unchanged and earlier callback effects are retained.
 * A missing callback fails only when its call site is reached. Only io and
 * al are required before the first title effect. */
pr_rec_0767_status pr_rec_res_00ba_0767(
    const pr_rec_0767_io *io, pr_rec_0767_far active,
    uint8_t mode_49f3, size_t max_nodes, uint8_t *al);

#endif

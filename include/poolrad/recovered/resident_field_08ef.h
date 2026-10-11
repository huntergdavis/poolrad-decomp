#ifndef POOLRAD_RECOVERED_RESIDENT_FIELD_08EF_H
#define POOLRAD_RECOVERED_RESIDENT_FIELD_08EF_H

#include <stdint.h>

/* Bounded local-control model of RES_00BA:08EF, loaded image 0x0148F.
 * DOS far pointers retain segment:offset identity; they are not host pointers. */
typedef struct {
    uint16_t segment;
    uint16_t offset;
} pr_rec_08ef_far;

/* Each event is one decoded call. FORMAT writes the first stack string;
 * COPY_FIRST copies it to the second with a 255-byte cap. On the >0x3C
 * branch, COPY_LITERAL replaces the first with a private Pascal literal,
 * APPEND_NUMBER appends the second string to that literal in the first,
 * and COPY_FINAL copies the first into the second with the same cap. DISPLAY
 * uses the second string. The model
 * does not expose either string or the literal's contents. */
typedef enum {
    PR_REC_08EF_FORMAT,
    PR_REC_08EF_COPY_FIRST,
    PR_REC_08EF_COPY_LITERAL,
    PR_REC_08EF_APPEND_NUMBER,
    PR_REC_08EF_COPY_FINAL,
    PR_REC_08EF_DISPLAY
} pr_rec_08ef_event_kind;

typedef struct {
    pr_rec_08ef_event_kind kind;
    pr_rec_08ef_far node;
    uint16_t number; /* FORMAT: absolute distance from 0x3C, 0..195. */
    uint16_t limit;  /* COPY_FIRST/FINAL: 0xFF. */
    uint8_t x, y, style; /* DISPLAY: caller bytes and style 0x0A. */
} pr_rec_08ef_event;

typedef struct {
    void *ctx;
    /* Reads node +0x111 twice: before FORMAT, then after COPY_FIRST. */
    int (*read_byte)(void *ctx, pr_rec_08ef_far node,
                     uint16_t displacement, uint8_t *value);
    /* Called at each formatter/string/display call. Only the final DISPLAY
     * result AL becomes the function result. */
    int (*effect)(void *ctx, const pr_rec_08ef_event *event, uint8_t *al);
} pr_rec_08ef_io;

typedef enum {
    PR_REC_08EF_OK = 0,
    PR_REC_08EF_ARGUMENT,
    PR_REC_08EF_BOUNDARY
} pr_rec_08ef_status;

/* DOS frame: node far pointer at [BP+0xA], x at [BP+8], y at [BP+6];
 * RETF 8. Only io and al are required up front; a missing or failed callback
 * reports BOUNDARY when reached. On BOUNDARY, output AL remains unchanged and
 * earlier callback effects remain visible. */
pr_rec_08ef_status pr_rec_res_00ba_08ef(const pr_rec_08ef_io *io,
                                        pr_rec_08ef_far node, uint8_t x,
                                        uint8_t y, uint8_t *al);

#endif

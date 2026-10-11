#ifndef POOLRAD_RECOVERED_RESIDENT_FIELD_0973_H
#define POOLRAD_RECOVERED_RESIDENT_FIELD_0973_H

#include <stdint.h>

/* Bounded local-control model of RES_00BA:0973 at loaded image 0x01513.
 * A DOS far pointer is an identity, not a host address. */
typedef struct {
    uint16_t segment;
    uint16_t offset;
} pr_rec_0973_far;

/* FORMAT_NUMBER stands at the local 0x01BD7 call that fills a stack Pascal
 * string. DISPLAY stands at 0x0709:0626 and consumes that string. Their
 * formatter, string, and display effects remain callback responsibilities. */
typedef enum {
    PR_REC_0973_FORMAT_NUMBER,
    PR_REC_0973_DISPLAY
} pr_rec_0973_event_kind;

typedef struct {
    pr_rec_0973_event_kind kind;
    pr_rec_0973_far node;
    uint8_t number; /* FORMAT_NUMBER: second read of node +0x11B. */
    uint8_t x, y, style; /* DISPLAY: caller bytes and selected raw style. */
} pr_rec_0973_event;

typedef struct {
    void *ctx;
    /* Read order: node +0x11B, node +0x32, then node +0x11B again. */
    int (*read_byte)(void *ctx, pr_rec_0973_far node,
                     uint16_t displacement, uint8_t *value);
    /* Only AL from the final DISPLAY event becomes the function result. */
    int (*effect)(void *ctx, const pr_rec_0973_event *event, uint8_t *al);
} pr_rec_0973_io;

typedef enum {
    PR_REC_0973_OK = 0,
    PR_REC_0973_ARGUMENT,
    PR_REC_0973_BOUNDARY
} pr_rec_0973_status;

/* DOS frame: node offset/segment [BP+0xC]/[BP+0xE], x [BP+0xA],
 * y [BP+8], override flag [BP+6], then RETF 0x0A. Only io and al are
 * required up front. Missing or failing callbacks stop at their reached
 * site, keep earlier callback effects, and leave output AL unchanged. */
pr_rec_0973_status pr_rec_res_00ba_0973(const pr_rec_0973_io *io,
                                        pr_rec_0973_far node, uint8_t x,
                                        uint8_t y, uint8_t override_flag,
                                        uint8_t *al);

#endif

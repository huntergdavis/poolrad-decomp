#ifndef POOLRAD_RECOVERED_RESIDENT_UNEQUAL_DISPLAY_1816_H
#define POOLRAD_RECOVERED_RESIDENT_UNEQUAL_DISPLAY_1816_H

#include <stdint.h>

/* Local-control model of RES_00BA:1816. Far pointers are DOS identities. */
typedef struct {
    uint16_t segment;
    uint16_t offset;
} pr_rec_1816_far;

typedef enum {
    PR_REC_1816_PRINT_NODE,
    PR_REC_1816_COPY_SUFFIX,
    PR_REC_1816_PRINT_SUFFIX
} pr_rec_1816_effect_kind;

typedef struct {
    pr_rec_1816_effect_kind kind;
    pr_rec_1816_far node;
    uint8_t column;
    uint8_t row;
    uint8_t color;
} pr_rec_1816_effect;

typedef struct {
    void *ctx;
    /* Displacements 0x10D, conditionally 0x10E, and conditionally 0x000
     * are read at their decoded points. The last read follows PRINT_NODE. */
    int (*read_byte)(void *ctx, pr_rec_1816_far node,
                     uint16_t displacement, uint8_t *value);
    /* COPY_SUFFIX stands for 0x0AF8:0B0B copying the private two-character
     * Pascal literal at CS:1813 into the local stack buffer. PRINT_SUFFIX
     * stands for 0x0709:0626 consuming that buffer. The literal's content
     * stays outside this public model. Effects may mutate later reads. */
    int (*effect)(void *ctx, const pr_rec_1816_effect *event, uint8_t *al);
} pr_rec_1816_io;

typedef enum {
    PR_REC_1816_OK = 0,
    PR_REC_1816_ARGUMENT,
    PR_REC_1816_BOUNDARY
} pr_rec_1816_status;

/* DOS frame: flag word [BP+6], row word [BP+8], column word [BP+10],
 * node far pointer [BP+12]; RETF 10. Only low bytes of scalar arguments
 * are consumed by the reached printer. On boundary failure *al is unchanged
 * and prior effects remain. The last successful printer supplies return AL. */
pr_rec_1816_status pr_rec_res_00ba_1816(
    const pr_rec_1816_io *io, pr_rec_1816_far node,
    uint8_t flag, uint8_t row, uint8_t column, uint8_t *al);

#endif

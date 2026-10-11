#ifndef POOLRAD_RECOVERED_RESIDENT_LOAD_ROUTE_27D9_H
#define POOLRAD_RECOVERED_RESIDENT_LOAD_ROUTE_27D9_H

#include <stdint.h>

/* Bounded model of RES_00BA:27D9, loaded at image 0x03379. The external
 * resident and overlay calls remain callback boundaries. */
typedef struct {
    uint8_t length;
    uint8_t bytes[3];
} pr_rec_27d9_string;

typedef enum {
    PR_REC_27D9_RES_03F1_02E8,
    PR_REC_27D9_LOCAL_32CA,
    PR_REC_27D9_OVL_03D0_003E,
    PR_REC_27D9_OVL_03D0_0039,
    PR_REC_27D9_OVL_03D0_002A,
    PR_REC_27D9_OVL_03CA_0025,
    PR_REC_27D9_RES_00BA_0767,
    PR_REC_27D9_RES_00BA_290B
} pr_rec_27d9_target;

typedef struct {
    /* Low bytes of AX words pushed in source order; their high bytes are
     * not established by this body. word_args are full pushed words. */
    uint8_t byte_args[6];
    uint8_t byte_count;
    uint16_t word_args[2];
    uint8_t word_count;
    /* The only DS far pointer this body passes is DS:0x6ABE. */
    uint8_t has_ds_pointer;
    uint16_t ds_offset;
    /* Valid only during call(). No game literal bytes are stored here. */
    const pr_rec_27d9_string *string;
} pr_rec_27d9_args;

typedef struct {
    void *context;
    /* Zero means a host callback completed. This is not a DOS error code. */
    int (*read_ds_byte)(void *context, uint16_t offset, uint8_t *value);
    int (*read_ds_word)(void *context, uint16_t offset, uint16_t *value);
    int (*read_cs_bytes)(void *context, uint16_t offset,
                         uint8_t count, uint8_t *bytes);
    int (*call)(void *context, pr_rec_27d9_target target,
                const pr_rec_27d9_args *args);
} pr_rec_27d9_io;

typedef enum {
    PR_REC_27D9_OK = 0,
    PR_REC_27D9_ARGUMENT,
    PR_REC_27D9_BOUNDARY
} pr_rec_27d9_status;

/* No consumed argument frame: the body reads DS:0x49F3 and ends in RETF.
 * Modes 0, 1, 2, 3, 4, and 6 have decoded call paths; all other values
 * return after the selector comparisons. Register return values are left
 * unspecified because the body does not assign a final AX result.
 * A missing or failing callback stops when first reached, preserving prior
 * effects. Unused callbacks may be null. */
pr_rec_27d9_status pr_rec_res_00ba_27d9(const pr_rec_27d9_io *io);

#endif

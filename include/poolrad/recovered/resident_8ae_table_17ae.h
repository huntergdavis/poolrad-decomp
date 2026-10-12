#ifndef POOLRAD_RECOVERED_RESIDENT_8AE_TABLE_17AE_H
#define POOLRAD_RECOVERED_RESIDENT_8AE_TABLE_17AE_H

#include <stddef.h>
#include <stdint.h>

#define PR_REC_8AE_SEGMENT_BYTES 65536u

typedef enum {
    PR_REC_8AE_TABLE_OK = 0,
    PR_REC_8AE_TABLE_ARGUMENT
} pr_rec_8ae_table_status;

/* RES_08AE:+17AE..+1819. segment is the complete writable DS=0x8AE
 * 64 KiB view. A short/null view is rejected without side effects.
 * This models the call-free procedure's memory effects, including 16-bit
 * effective-offset wrap and aliasing between source and output areas.
 * It makes no claim that a saved one-Up run reaches this leaf. */
pr_rec_8ae_table_status pr_rec_res_08ae_17ae(uint8_t *segment,
                                               size_t segment_size,
                                               uint16_t word_argument);

#endif

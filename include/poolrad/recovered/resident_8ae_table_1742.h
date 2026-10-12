#ifndef POOLRAD_RECOVERED_RESIDENT_8AE_TABLE_1742_H
#define POOLRAD_RECOVERED_RESIDENT_8AE_TABLE_1742_H

#include "poolrad/recovered/resident_8ae_table_17ae.h"

/* RES_08AE:+1742..+17AD. segment is a complete 64 KiB writable
 * DS=0x8AE view. A null or short view is rejected without effects.
 * This models the call-free procedure's memory effects; saved one-Up
 * selection of this leaf remains unproved. */
pr_rec_8ae_table_status pr_rec_res_08ae_1742(uint8_t *segment,
                                               size_t segment_size,
                                               uint16_t word_argument);

#endif

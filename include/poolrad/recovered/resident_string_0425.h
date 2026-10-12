#ifndef POOLRAD_RECOVERED_RESIDENT_STRING_0425_H
#define POOLRAD_RECOVERED_RESIDENT_STRING_0425_H

#include <stddef.h>
#include <stdint.h>

/* Resident 0x0802:0x0425, loaded 0x08445..0x0852E. The caller supplies
 * the ten live DS bytes read at DS:0x0D1A..0x0D23. Source and output are
 * Pascal strings (length byte followed by that many bytes). */
typedef enum {
    PR_REC_0802_0425_OK = 0,
    PR_REC_0802_0425_ARGUMENT,
    PR_REC_0802_0425_BOUNDS
} pr_rec_0802_0425_status;

/* The DOS frame receives source at [BP+6], destination at [BP+0xA], and
 * returns with RETF 4. The C host bounds failures leave output untouched.
 * Source, table, and output may alias. */
pr_rec_0802_0425_status pr_rec_res_0802_0425(
    const uint8_t *source, size_t source_size,
    const uint8_t *table, size_t table_size,
    uint8_t *output, size_t output_capacity);

#endif

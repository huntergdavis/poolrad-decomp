#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_FILENAME_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_FILENAME_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u6f_party_string_chain.h"
#include "poolrad/recovered/resident_dax_selector_02eb.h"

/* The filename local begins at caller [BP-0x312]. Its 254-byte bounded
 * region ends before the neighboring [BP-0x214] local. This model rejects
 * inputs that would write outside that region in the DOS frame. */
#define PR_U6F_PARTY_FILENAME_LOCAL_SIZE 254u

typedef struct {
    uint8_t filename[PR_U6F_PARTY_FILENAME_LOCAL_SIZE];
    uint16_t ax;       /* returned by RES_0802:02EB on a completed probe */
    uint16_t next_pc;  /* +0x19A8, +0x19B1, or +0x19EA */
} pr_u6f_party_filename_result;

typedef enum {
    PR_U6F_PARTY_FILENAME_OK = 0,
    PR_U6F_PARTY_FILENAME_ARGUMENT,
    PR_U6F_PARTY_FILENAME_PRIOR,
    PR_U6F_PARTY_FILENAME_BOUNDS,
    PR_U6F_PARTY_FILENAME_NEED_IO,
    PR_U6F_PARTY_FILENAME_IO_BOUNDARY
} pr_u6f_party_filename_status;

/* The prior frame must be the completed +0x1965..+0x1982 first-slot result.
 * 'prefix' models live DS:0x537A; 'suffix' models compiled CS:0x15E9.
 * Both are caller-supplied Pascal strings. A NULL or incomplete io stops
 * at +0x19A8 with the constructed filename. A completed probe selects
 * +0x19B1 (AL nonzero) or +0x19EA (AL zero); neither branch is executed.
 * A callback failure retains earlier callback effects and the filename. */
pr_u6f_party_filename_status pr_u6f_party_filename_first(
    const pr_u6f_party_string_frame *prior,
    const uint8_t *prefix, size_t prefix_size,
    const uint8_t *suffix, size_t suffix_size,
    const pr_rec_dax_selector_02eb_io *io,
    pr_u6f_party_filename_result *out);

#endif

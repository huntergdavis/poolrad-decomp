#ifndef POOLRAD_RECOVERED_RESIDENT_STRING_0356_H
#define POOLRAD_RECOVERED_RESIDENT_STRING_0356_H

/* START.EXE resident 0x0802:0x0356, loaded 0x08376..0x08444.
 * See docs/re/RESIDENT_STRING_0356.md. Inputs and output are Pascal strings:
 * byte 0 is length, followed by exactly that many data bytes. The slot-A
 * caller supplies the private one-character needle; no game text is embedded.
 */
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_REC_0802_0356_OK = 0,
    PR_REC_0802_0356_ARGUMENT,
    PR_REC_0802_0356_BOUNDS
} pr_rec_0802_0356_status;

/* Models all output paths of the 207-byte body. The source is [BP+0xA],
 * needle [BP+6], and output [BP+0xE]; DOS RETF 8 retains the output pointer.
 * A host bounds failure leaves output untouched. Input/output may alias. */
pr_rec_0802_0356_status pr_rec_res_0802_0356(
    const uint8_t *source, size_t source_size,
    const uint8_t *needle, size_t needle_size,
    uint8_t *output, size_t output_capacity);

#endif

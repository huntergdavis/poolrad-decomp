#ifndef POOLRAD_FORMATS_ECL_H
#define POOLRAD_FORMATS_ECL_H

#include <stddef.h>
#include <stdint.h>

#define PR_ECL_MAX_RECORD 65535u
#define PR_ECL_HEADER_SIZE 22u
#define PR_ECL_ENTRY_COUNT 5u
#define PR_ECL_ADDRESS_BASE 0x9900u
#define PR_ECL_DOS_PREFIX 0x1388u

typedef enum {
    PR_ECL_OK = 0,
    PR_ECL_ARGUMENT,
    PR_ECL_SIZE,
    PR_ECL_HEADER,
    PR_ECL_ADDRESS,
    PR_ECL_TEXT_FRAME,
    PR_ECL_OUTPUT_SIZE
} pr_ecl_status;

/* Borrows one already unpacked DAX block. Keep data alive and unchanged. */
typedef struct {
    const uint8_t *data;
    size_t size;
    uint16_t prefix;
    uint16_t entry_address[PR_ECL_ENTRY_COUNT];
    size_t entry_offset[PR_ECL_ENTRY_COUNT];
} pr_ecl_record;

/* All offsets are relative to the start of the unpacked block, including
 * its two-byte prefix. No text bytes are copied into this descriptor. */
typedef struct {
    size_t source_offset;
    size_t payload_offset;
    size_t packed_size;
    size_t end_offset;
} pr_ecl_text;

pr_ecl_status pr_ecl_open(const uint8_t *data, size_t size, pr_ecl_record *out);
pr_ecl_status pr_ecl_address_to_offset(const pr_ecl_record *record,
                                       uint16_t address, size_t *offset);
pr_ecl_status pr_ecl_offset_to_address(const pr_ecl_record *record,
                                       size_t offset, uint16_t *address);
/* Parse only at a caller-provided operand position. Does not scan opcodes. */
pr_ecl_status pr_ecl_text_at(const pr_ecl_record *record, size_t source_offset,
                             pr_ecl_text *out);
/* Produces ASCII plus NUL. written excludes NUL; OUTPUT_SIZE reports the
 * character count. Output must not overlap record data. */
pr_ecl_status pr_ecl_text_decode(const pr_ecl_record *record,
                                 const pr_ecl_text *text, char *out,
                                 size_t capacity, size_t *written);
const char *pr_ecl_status_string(pr_ecl_status status);

#endif

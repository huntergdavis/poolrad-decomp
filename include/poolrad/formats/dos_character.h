#ifndef POOLRAD_FORMATS_DOS_CHARACTER_H
#define POOLRAD_FORMATS_DOS_CHARACTER_H

#include <stddef.h>
#include <stdint.h>

#define PR_DOS_CHARACTER_SIZE 285u

typedef enum {
    PR_DOS_CHARACTER_OK = 0,
    PR_DOS_CHARACTER_ARGUMENT,
    PR_DOS_CHARACTER_BAD_SIZE,
    PR_DOS_CHARACTER_RANGE
} pr_dos_character_status;

/* Borrows one CHRDAT*.SAV record. Keep data alive and unchanged. Byte meanings
 * are deliberately unspecified until verified from DOS evidence. */
typedef struct {
    const uint8_t *data;
    size_t size;
} pr_dos_character_record;

/* Raw block-relative range with an exclusive end. No bytes are copied. */
typedef struct {
    size_t source_offset;
    size_t end_offset;
} pr_dos_character_span;

pr_dos_character_status pr_dos_character_open(const uint8_t *data, size_t size,
                                                pr_dos_character_record *out);
pr_dos_character_status pr_dos_character_raw_span(
    const pr_dos_character_record *record, size_t source_offset,
    size_t length, pr_dos_character_span *out);
const char *pr_dos_character_status_string(pr_dos_character_status status);

#endif

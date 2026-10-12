#ifndef POOLRAD_FORMATS_DOS_SAVGAM_H
#define POOLRAD_FORMATS_DOS_SAVGAM_H

#include <stddef.h>
#include <stdint.h>

#define PR_DOS_SAVGAM_SIZE 13137u
#define PR_DOS_SAVGAM_BLOCK_COUNT 10u

typedef enum {
    PR_DOS_SAVGAM_OK = 0,
    PR_DOS_SAVGAM_ARGUMENT,
    PR_DOS_SAVGAM_BAD_SIZE,
    PR_DOS_SAVGAM_RANGE
} pr_dos_savgam_status;

/* Borrows one complete SAVGAM*.DAT file. Keep data alive and unchanged. */
typedef struct {
    const uint8_t *data;
    size_t size;
} pr_dos_savgam_record;

typedef struct {
    size_t source_offset;
    size_t end_offset;
} pr_dos_savgam_span;

/* Only byte locations proven by the DOS loader and the gameplay loop are
 * exposed. The file's first ECL byte is overwritten after load by the byte
 * at window 1 +0x624, so both values are retained here. */
typedef struct {
    uint8_t saved_ecl_file;
    uint8_t restored_ecl_file;
    uint8_t map_id;
    uint8_t x;
    uint8_t y;
    uint8_t facing;
    uint8_t mode;
    uint8_t character_count;
} pr_dos_savgam_core;

pr_dos_savgam_status pr_dos_savgam_open(const uint8_t *data, size_t size,
                                        pr_dos_savgam_record *out);
/* Block indices follow the ten original DOS file transfers. The returned
 * offsets have an exclusive end; no bytes are copied. */
pr_dos_savgam_status pr_dos_savgam_block(const pr_dos_savgam_record *record,
                                         size_t index, pr_dos_savgam_span *out);
pr_dos_savgam_status pr_dos_savgam_read_core(const pr_dos_savgam_record *record,
                                             pr_dos_savgam_core *out);
const char *pr_dos_savgam_status_string(pr_dos_savgam_status status);

#endif

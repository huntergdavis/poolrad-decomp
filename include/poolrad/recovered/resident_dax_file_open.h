#ifndef POOLRAD_RECOVERED_RESIDENT_DAX_FILE_OPEN_H
#define POOLRAD_RECOVERED_RESIDENT_DAX_FILE_OPEN_H

#include <stdbool.h>
#include <stdint.h>

/* A Turbo Pascal length-prefixed string, represented without game text. */
typedef struct {
    uint8_t length;
    uint8_t bytes[255];
} pr_rec_dax_open_string;

typedef struct {
    void *context;
    bool (*read_ds_byte)(void *context, uint16_t offset, uint8_t *value);
    bool (*read_table_string)(void *context, uint16_t offset,
                              pr_rec_dax_open_string *value);
    /* The Pascal literal at CS:0x05CB has length one. Read its data byte. */
    bool (*read_code_byte)(void *context, uint16_t offset, uint8_t *value);
    /* Boundary at resident 0x0802:0x025D. AL may be any nonzero success
     * value. This callback owns the first file record's effects. */
    bool (*open_configured)(void *context,
                            const pr_rec_dax_open_string *name_ext,
                            const pr_rec_dax_open_string *configured_path,
                            uint8_t mode, uint8_t *result_al);
    /* Unresolved 0xAF8 runtime calls on the second and first file records. */
    bool (*assign_second)(void *context,
                          const pr_rec_dax_open_string *name_ext);
    bool (*reset_second)(void *context, uint16_t record_size);
    bool (*io_result)(void *context, uint16_t *result);
    bool (*read_second)(void *context, uint16_t *header_word,
                        uint16_t requested_bytes, uint16_t *actual_bytes);
    bool (*seek_first)(void *context, uint32_t absolute_offset);
} pr_rec_dax_open_io;

typedef enum {
    PR_REC_DAX_OPEN_OK = 0,
    PR_REC_DAX_OPEN_ARGUMENT,
    PR_REC_DAX_OPEN_BOUNDARY
} pr_rec_dax_open_status;

/* Path-complete model of START.EXE RES_0802:05CD. base_path is the reader's
 * already assembled DS:0x547C plus caller-name string; extension is its
 * second explicit string argument. On a completed call AL
 * becomes zero after the local open boundary reports failure, or one after
 * the second record's read and the first record's seek. The header word is
 * written by read_second, then incremented by two with 16-bit wrap.
 * Callback failure is a host error: AL is unchanged and prior effects stay. */
pr_rec_dax_open_status pr_rec_res_0802_05cd(
    const pr_rec_dax_open_io *io,
    const pr_rec_dax_open_string *base_path,
    const pr_rec_dax_open_string *extension,
    uint16_t *header_word, uint8_t *al);

#endif

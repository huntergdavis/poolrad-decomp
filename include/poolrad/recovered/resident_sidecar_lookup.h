#ifndef POOLRAD_RECOVERED_RESIDENT_SIDECAR_LOOKUP_H
#define POOLRAD_RECOVERED_RESIDENT_SIDECAR_LOOKUP_H

#include "poolrad/recovered/resident_sidecar_remove.h"

#include <stddef.h>
#include <stdint.h>

/* START.EXE RES_00BA_2199 searches the character's 9-byte sidecar list.
 * Host statuses are adapter outcomes; the DOS result is AL=0 or AL=1. */
typedef enum {
    PR_REC_LOOKUP_OK = 0,
    PR_REC_LOOKUP_ARGUMENT,
    PR_REC_LOOKUP_MEMORY,
    PR_REC_LOOKUP_LIMIT
} pr_rec_lookup_status;

typedef struct {
    void *context;
    /* Access exact segment:offset locations. A DOS word at 0xFFFF reads
     * its high byte at 0x0000; the adapter owns that physical behavior. */
    int (*read_word)(void *context, uint16_t segment, uint16_t offset,
                     uint16_t *value);
    int (*write_word)(void *context, uint16_t segment, uint16_t offset,
                      uint16_t value);
    int (*read_byte)(void *context, uint16_t segment, uint16_t offset,
                     uint8_t *value);
    /* Host guard for corrupt cyclic links. The DOS routine has no bound. */
    size_t max_nodes;
} pr_rec_lookup_io;

/* cursor_cell = [BP+6,+8], key = low byte of [BP+0xA], character =
 * [BP+0xC,+0xE]. On success, write the current node pointer or all-zero
 * to cursor_cell and set *found to DOS AL (0 or 1). The original RETF 10
 * removes all three arguments. Host failures leave *found unchanged, but
 * earlier far-memory writes are not rolled back. */
pr_rec_lookup_status pr_rec_res_00ba_2199(const pr_rec_lookup_io *io,
                                           pr_rec_far_ptr cursor_cell,
                                           uint16_t key_word,
                                           pr_rec_far_ptr character,
                                           uint8_t *found);

#endif

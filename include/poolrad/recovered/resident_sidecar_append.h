#ifndef POOLRAD_RECOVERED_RESIDENT_SIDECAR_APPEND_H
#define POOLRAD_RECOVERED_RESIDENT_SIDECAR_APPEND_H

#include "poolrad/recovered/resident_sidecar_remove.h"

#include <stddef.h>
#include <stdint.h>

/* START.EXE RES_00BA_15F7 uses one 0x3F-byte sidecar allocation. The
 * statuses below describe host adapter outcomes, not DOS return registers. */
#define PR_REC_APPEND_BLOCK_BYTES 0x003Fu

typedef enum {
    PR_REC_APPEND_OK = 0,
    PR_REC_APPEND_ARGUMENT,
    PR_REC_APPEND_ALLOCATE_ABORT,
    PR_REC_APPEND_COPY_FAILURE,
    PR_REC_APPEND_MEMORY,
    PR_REC_APPEND_LIMIT
} pr_rec_append_status;

typedef struct {
    void *context;
    /* SYSTEM 0xAF8:0x023F GetMem(0x3F), returning DX:AX. A false result
     * models its branch into runtime error dispatch. Heap/error internals
     * remain in this callback. A successful result is non-null. */
    int (*allocate_block)(void *context, uint16_t bytes,
                          pr_rec_far_ptr *block);
    /* SYSTEM 0xAF8:0x1685 Move(source, new_block, 0x3F). The adapter models
     * the runtime's byte copy. A false result stops this host translation
     * before link writes. Valid allocations are distinct from the source. */
    int (*copy_block)(void *context, pr_rec_far_ptr source,
                      pr_rec_far_ptr destination, uint16_t bytes);
    /* Access individual DOS far-memory words with 16-bit offset wrap. */
    int (*read_word)(void *context, uint16_t segment, uint16_t offset,
                     uint16_t *value);
    int (*write_word)(void *context, uint16_t segment, uint16_t offset,
                      uint16_t value);
    /* Host guard against corrupt cyclic lists. The DOS loop has no bound. */
    size_t max_nodes;
} pr_rec_append_io;

/* source = [BP+6,+8], character = [BP+0xA,+0xC]. Allocate/copy a complete
 * sidecar, clear its next far pointer at +0x2A, then append it to the
 * character's list headed at +0xC8. The DOS routine returns with RETF 8. */
pr_rec_append_status pr_rec_res_00ba_15f7(const pr_rec_append_io *io,
                                           pr_rec_far_ptr source,
                                           pr_rec_far_ptr character);

#endif

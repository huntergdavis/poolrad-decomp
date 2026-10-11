#ifndef POOLRAD_RECOVERED_RESIDENT_SIDECAR_REMOVE_H
#define POOLRAD_RECOVERED_RESIDENT_SIDECAR_REMOVE_H

#include <stddef.h>
#include <stdint.h>

/* Full external-effect translation of START.EXE RES_00BA_1509 in the audited
 * DOS v1.3 build. The original accepts two far pointers and returns with
 * RETF 8, without a defined result value. These host statuses describe the
 * observed path and adapter failures; they are not DOS register values. */
typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr_rec_far_ptr;

typedef enum {
    PR_REC_REMOVE_OK = 0,
    PR_REC_REMOVE_REPORTED_NULL,
    PR_REC_REMOVE_REPORTED_MISSING,
    PR_REC_REMOVE_ARGUMENT,
    PR_REC_REMOVE_MEMORY,
    PR_REC_REMOVE_LIMIT,
    PR_REC_REMOVE_RELEASE_ABORT
} pr_rec_remove_status;

/* Literal offsets identify two private in-executable Pascal strings. No
 * game text is included in this interface. */
#define PR_REC_REMOVE_NULL_LITERAL 0x14CEu
#define PR_REC_REMOVE_MISSING_LITERAL 0x14E2u
#define PR_REC_REMOVE_MESSAGE_PARAMETER 0x0Eu
#define PR_REC_REMOVE_BLOCK_BYTES 0x003Fu

typedef struct {
    void *context;
    /* Read or write a word at the exact 16-bit segment:offset supplied.
     * Return 1 on success, 0 when the host cannot model the DOS access. */
    int (*read_word)(void *context, uint16_t segment, uint16_t offset,
                     uint16_t *value);
    int (*write_word)(void *context, uint16_t segment, uint16_t offset,
                      uint16_t value);
    /* The two original error calls are separate: copy the identified Pascal
     * literal to a local string, then present that local string with 0x0E.
     * Callbacks may hold the private text outside the public repository. */
    void (*copy_literal)(void *context, uint16_t resident_segment,
                         uint16_t literal_offset);
    void (*present_message)(void *context, uint8_t parameter);
    /* Model SYSTEM 0xAF8:0x0254 FreeMem(ptr, 0x003F). Return 0 if its
     * terminal failure path is taken; the C function then stops. */
    int (*release_block)(void *context, pr_rec_far_ptr block, uint16_t bytes);
    /* Host guard for corrupt cyclic links. Valid DOS lists terminate before
     * this count; the original routine has no cycle bound. Must be nonzero. */
    size_t max_nodes;
} pr_rec_remove_io;

/* target = [BP+6,+8], character = [BP+0xA,+0xC]. The character's sidecar
 * head is at +0xC8; each 0x3F-byte node's next pointer is at +0x2A.
 * Far-pointer equality compares both words, and null requires both zero.
 * Field address additions use 16-bit offset wraparound as on DOS. */
pr_rec_remove_status pr_rec_res_00ba_1509(const pr_rec_remove_io *io,
                                           pr_rec_far_ptr target,
                                           pr_rec_far_ptr character);

#endif

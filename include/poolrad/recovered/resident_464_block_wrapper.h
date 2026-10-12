#ifndef POOLRAD_RECOVERED_RESIDENT_464_BLOCK_WRAPPER_H
#define POOLRAD_RECOVERED_RESIDENT_464_BLOCK_WRAPPER_H

#include <stddef.h>
#include <stdint.h>

/* Bounded host model of START.EXE RES_0464:0111. See
 * docs/re/RESIDENT_464_BLOCK_WRAPPER.md. DOS far pointers and the heap are
 * represented by host objects; no native memory ownership is inferred. */
typedef struct {
    uint8_t *bytes;
    size_t capacity;
} pr_r464_block_object;

#define PR_R464_BLOCK_DATA_OFFSET 0x11u

typedef enum {
    PR_R464_WRAPPER_OK = 0,
    PR_R464_WRAPPER_ARGUMENT,
    PR_R464_WRAPPER_HOOK,
    PR_R464_WRAPPER_CALLBACK,
    PR_R464_WRAPPER_READER,
    PR_R464_WRAPPER_NAME,
    PR_R464_WRAPPER_BLOCK
} pr_r464_wrapper_status;

typedef enum {
    PR_R464_CALL_RELEASE_PRIOR = 1,
    PR_R464_CALL_READ_BLOCK,
    PR_R464_CALL_MAKE_OBJECT,
    PR_R464_CALL_PROCESS_BLOCK,
    PR_R464_CALL_RELEASE_BLOCK,
    PR_R464_CALL_FLUSH
} pr_r464_wrapper_call;

typedef struct {
    void *ctx;
    const char *extension; /* Host-provided C form of private CS:+010C string. */
    /* Signature matches the merged bounded reader pr_rec_res_0802_0716.
     * The wrapper supplies at most the first 20 name bytes and low(id_word). */
    int (*read_block)(void *ctx, const char *name, const char *extension,
                      uint8_t id, uint16_t *size, const uint8_t **block);
    /* Called only if *destination is non-null. The hook owns its mutation. */
    int (*release_prior)(void *ctx, pr_r464_block_object **destination);
    /* Models RES_0464:248E, including allocation and header initialization.
     * On success, *destination must have writable bytes with capacity >= 17. */
    int (*make_object)(void *ctx, uint16_t word_0, uint16_t word_2,
                       uint8_t byte_8, uint8_t arg_c,
                       pr_r464_block_object **destination);
    /* Models RES_0464:02A5 with the full loaded block and the two caller
     * bytes. The wrapper itself does not interpret the remaining data. */
    int (*process_block)(void *ctx, const uint8_t *block, uint16_t size,
                         uint16_t data_offset, uint8_t arg_c, uint8_t arg_a,
                         pr_r464_block_object *object);
    /* Signature matches pr_rec_res_0802_0716_release. Reached only after
     * process_block succeeds. */
    void (*release_block)(void *ctx, const uint8_t *block, uint16_t size);
    int (*flush_899_0104)(void *ctx);
} pr_r464_wrapper_env;

typedef struct {
    pr_r464_wrapper_call calls[6];
    unsigned call_count;
    pr_r464_wrapper_call stopped_at;
    int callback_status;
    int reader_status;
    uint16_t block_size;
    /* Non-null when a stop precedes the DOS FreeMem site. Caller must use
     * env->release_block to reclaim it, if that callback owns the block. */
    const uint8_t *held_block;
} pr_r464_wrapper_report;

/* DOS frame: far destination slot [BP+6], raw byte [BP+0A], raw byte
 * [BP+0C], low byte of ID word [BP+0E], far Pascal name [BP+0x10],
 * RETF 0x0E. name is a 256-byte Pascal buffer. A missing reached hook or
 * nonzero callback return stops after all preceding effects. A zero returned
 * block size exits after read_block, even when the old object was released. */
pr_r464_wrapper_status pr_r464_block_wrapper(
    const pr_r464_wrapper_env *env, const uint8_t name[256],
    uint16_t id_word, uint8_t arg_a, uint8_t arg_c,
    pr_r464_block_object **destination, pr_r464_wrapper_report *report);

#endif

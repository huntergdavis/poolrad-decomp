#ifndef POOLRAD_RECOVERED_RESIDENT_464_BITMAP_RELEASE_H
#define POOLRAD_RECOVERED_RESIDENT_464_BITMAP_RELEASE_H

#include <stdbool.h>
#include <stdint.h>

/* DOS far pointers are kept as two 16-bit words, including partially null values. */
typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr464_far_ptr;

typedef struct {
    void *context;
    bool (*read8)(void *context, pr464_far_ptr address, uint8_t *value);
    bool (*read16)(void *context, pr464_far_ptr address, uint16_t *value);
    bool (*write16)(void *context, pr464_far_ptr address, uint16_t value);
    bool (*free_block)(void *context, pr464_far_ptr block, uint16_t size);
} pr464_bitmap_memory;

typedef enum {
    PR464_BITMAP_OK = 0,
    PR464_BITMAP_BAD_ARGUMENT,
    PR464_BITMAP_ACCESS_FAILED,
    PR464_BITMAP_FREE_FAILED
} pr464_bitmap_status;

/* Translate START.EXE resident 0x0464:0x2595 for successful DOS callbacks.
 * pointer_slot points to the caller's far-pointer variable. video_mode is the
 * byte at DS:0x4A1E; it matters only when the secondary block exists.
 * Host callback failures stop the translation after any preceding effects. */
pr464_bitmap_status pr464_bitmap_release(const pr464_bitmap_memory *memory,
                                         pr464_far_ptr pointer_slot,
                                         uint8_t video_mode);

#endif

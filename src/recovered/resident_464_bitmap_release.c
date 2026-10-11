#include "poolrad/recovered/resident_464_bitmap_release.h"

static pr464_far_ptr at(pr464_far_ptr base, uint16_t delta)
{
    base.offset = (uint16_t)(base.offset + delta);
    return base;
}

pr464_bitmap_status pr464_bitmap_release(const pr464_bitmap_memory *memory,
                                         pr464_far_ptr pointer_slot,
                                         uint8_t video_mode)
{
    uint16_t offset, segment, stride, aux_offset, aux_segment;
    uint8_t planes;
    uint16_t data_size;
    pr464_far_ptr bitmap;

    if (!memory || !memory->read8 || !memory->read16 || !memory->write16 ||
        !memory->free_block)
        return PR464_BITMAP_BAD_ARGUMENT;

    /* 0x06BDB..0x06BE7: either nonzero word makes the pointer non-null. */
    if (!memory->read16(memory->context, pointer_slot, &offset) ||
        !memory->read16(memory->context, at(pointer_slot, 2), &segment))
        return PR464_BITMAP_ACCESS_FAILED;
    if ((uint16_t)(offset | segment) == 0)
        return PR464_BITMAP_OK;

    /* 0x06BEA..0x06C14: reload the pointer, then read its custom header. */
    if (!memory->read16(memory->context, pointer_slot, &bitmap.offset) ||
        !memory->read16(memory->context, at(pointer_slot, 2), &bitmap.segment) ||
        !memory->read8(memory->context, at(bitmap, 8), &planes) ||
        !memory->read16(memory->context, at(bitmap, 0x11), &stride) ||
        !memory->read16(memory->context, at(bitmap, 0x13), &aux_offset) ||
        !memory->read16(memory->context, at(bitmap, 0x15), &aux_segment))
        return PR464_BITMAP_ACCESS_FAILED;
    data_size = (uint16_t)((uint32_t)planes * stride);

    /* 0x06C16..0x06C4A: the auxiliary allocation uses a quarter size
     * only in video mode 1. The pointer fields are read again before FreeMem. */
    if ((uint16_t)(aux_offset | aux_segment) != 0) {
        pr464_far_ptr auxiliary;
        if (!memory->read16(memory->context, at(bitmap, 0x15),
                            &auxiliary.segment) ||
            !memory->read16(memory->context, at(bitmap, 0x13),
                            &auxiliary.offset))
            return PR464_BITMAP_ACCESS_FAILED;
        if (!memory->free_block(memory->context, auxiliary,
                                video_mode == 1 ? (uint16_t)(data_size >> 2)
                                                : data_size))
            return PR464_BITMAP_FREE_FAILED;
    }

    /* 0x06C4A..0x06C6F: reread the caller's pointer in push order, free
     * data_size + 23 modulo 65536, then clear its offset and segment words. */
    if (!memory->read16(memory->context, at(pointer_slot, 2), &segment) ||
        !memory->read16(memory->context, pointer_slot, &offset))
        return PR464_BITMAP_ACCESS_FAILED;
    bitmap.offset = offset;
    bitmap.segment = segment;
    if (!memory->free_block(memory->context, bitmap,
                            (uint16_t)(data_size + 0x17u)))
        return PR464_BITMAP_FREE_FAILED;
    if (!memory->write16(memory->context, pointer_slot, 0) ||
        !memory->write16(memory->context, at(pointer_slot, 2), 0))
        return PR464_BITMAP_ACCESS_FAILED;
    return PR464_BITMAP_OK;
}

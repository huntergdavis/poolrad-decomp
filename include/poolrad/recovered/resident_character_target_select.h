#ifndef POOLRAD_RECOVERED_RESIDENT_CHARACTER_TARGET_SELECT_H
#define POOLRAD_RECOVERED_RESIDENT_CHARACTER_TARGET_SELECT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr_rec_select_far_ptr;

typedef struct {
    void *context;
    bool (*read_far_word)(void *context, uint16_t segment, uint16_t offset,
                          uint16_t *value);
    bool (*read_far_byte)(void *context, uint16_t segment, uint16_t offset,
                          uint8_t *value);
    bool (*read_global_byte)(void *context, uint16_t offset, uint8_t *value);
    bool (*write_far_word)(void *context, uint16_t segment, uint16_t offset,
                           uint16_t value);
} pr_rec_target_select_io;

typedef enum {
    PR_REC_TARGET_SELECT_OK = 0,
    PR_REC_TARGET_SELECT_ARGUMENT,
    PR_REC_TARGET_SELECT_MEMORY
} pr_rec_target_select_status;

/* START.EXE RES_00BA_2E8B. The four-byte output slot is cleared before
 * inspecting the character link. Each output-word write is observable.
 * On success AX contains the original routine's result, including AH. */
pr_rec_target_select_status pr_rec_res_00ba_2e8b(
    const pr_rec_target_select_io *io, pr_rec_select_far_ptr character,
    pr_rec_select_far_ptr output_slot, uint16_t *ax);


/* Raw CPU composition API; legacy callback API above is retained. */
#include <stddef.h>
#include <stdint.h>
#define PR_TARGET_CPU_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_target_cpu;
typedef enum { PR_TARGET_RETURNED, PR_TARGET_ARGUMENT, PR_TARGET_CODE,
               PR_TARGET_CALLEE, PR_TARGET_TRANSFER } pr_target_status;
typedef struct { int selector_status, resident_status; } pr_target_diagnostic;
/* Complete RES_00BA [2E8B,2F57), 204 bytes, RETF8. Entry SP+4 is a far
 * output cell; SP+8 is a far character pointer. Raw CPU14 and exact 1 MiB
 * A20-off flat RAM, immutable code, IP2E8B, SP0008..FFF4. Live aliases,
 * physically contiguous words/LES operands, wrapping effective offsets.
 * Reached far-return frames must fit in SS:0000..FFFF. No IRQ/MMIO/faults
 * or self-modifying code. CPU storage must not overlap RAM. Invalid host
 * arguments reject atomically. Logical AF=0 (normal/386 measured profile).
 * The existing callback selector API remains unchanged. */
pr_target_status pr_res_target_select_2e8b_cpu(uint8_t *ram,size_t size,pr_target_cpu *cpu);
#endif

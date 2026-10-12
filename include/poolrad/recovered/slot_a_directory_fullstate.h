#ifndef POOLRAD_RECOVERED_SLOT_A_DIRECTORY_FULLSTATE_H
#define POOLRAD_RECOVERED_SLOT_A_DIRECTORY_FULLSTATE_H

#include <stddef.h>
#include <stdint.h>

/* Saved slot-A SYSTEM directory boundary, starting at the already reached
 * INT 21h at 131C:1821. The pathname converter and earlier wrapper setup
 * have completed before this entry. Guest RAM is a linear 20-bit image;
 * only addresses below ram_size are accessible. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp;
    uint16_t cs, ds, es, ss, fs, gs, ip, flags;
} pr_slot_a_directory_cpu;

typedef struct {
    uint32_t linear;
    uint8_t old_byte, new_byte;
} pr_slot_a_directory_write;

typedef enum {
    PR_SLOT_A_DIRECTORY_OK = 0,
    PR_SLOT_A_DIRECTORY_BLOCKED_DOS_SERVICE,
    PR_SLOT_A_DIRECTORY_BOUNDARY,
    PR_SLOT_A_DIRECTORY_BAD_EVENT
} pr_slot_a_directory_status;

/* A successful service callback returns the complete post-INT CPU and an
 * ordered journal. Repeated addresses are allowed when each old_byte matches
 * the preceding write. The stepper validates the whole event before any RAM
 * or CPU mutation. The callback may inspect but must not mutate ram. */
typedef pr_slot_a_directory_status (*pr_slot_a_directory_service)(
    void *context, const pr_slot_a_directory_cpu *before,
    const uint8_t *ram, size_t ram_size,
    pr_slot_a_directory_cpu *after,
    pr_slot_a_directory_write *writes, size_t write_capacity,
    size_t *write_count);

typedef struct {
    pr_slot_a_directory_cpu cpu;
    uint8_t *ram;
    size_t ram_size;
} pr_slot_a_directory_state;

/* One step reaches a comparison stop: 1823, 1826 or 1829, 02D5, 04A2,
 * 04A8, or 02DA. The service is consulted only at 131C:1821. A missing
 * callback stops there with BLOCKED_DOS_SERVICE and preserves the state. */
pr_slot_a_directory_status pr_slot_a_directory_step(
    pr_slot_a_directory_state *state,
    pr_slot_a_directory_service service, void *context,
    pr_slot_a_directory_write *journal, size_t journal_capacity);

#endif

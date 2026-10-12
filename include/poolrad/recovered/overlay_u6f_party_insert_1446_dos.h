#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_INSERT_1446_DOS_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_INSERT_1446_DOS_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/overlay_u6f_party_insert_1446.h"

#define PR_U6F_INSERT_DOS_RAM_SIZE (1u << 20)

/* A20-off, byte-addressable DOS RAM. All DS globals, record pointers, and
 * window pointers share this one view, including segment aliases. The host
 * supplies the entire 1 MiB physical address space. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t ds;
} pr_u6f_insert_dos_memory;

typedef struct {
    size_t append_nodes;
    size_t mark_nodes;
    uint8_t selected_slot;
    uint16_t window_word;
    pr_u9b_incr_report callee;
} pr_u6f_insert_dos_report;

typedef enum {
    PR_U6F_INSERT_DOS_OK = 0,
    PR_U6F_INSERT_DOS_ARGUMENT
} pr_u6f_insert_dos_status;

/* Complete terminating path of unit 0x6F:+0x1446..+0x157E for mapped DOS
 * memory. Incoming is the caller's far-pointer argument. All source reads and
 * writes of DS globals and ES records use the same 20-bit RAM, in instruction
 * order. FillChar and membership use their proved local byte semantics; the
 * final u9B call follows its five proved read/conditional-increment steps.
 * There is no host traversal cap: a cyclic list loops, as the DOS code does.
 * Only an invalid host RAM view is rejected before execution. */
pr_u6f_insert_dos_status pr_u6f_party_insert_1446_dos(
    const pr_u6f_insert_dos_memory *memory, pr_u6f_insert_far incoming,
    pr_u6f_insert_dos_report *report);

#endif

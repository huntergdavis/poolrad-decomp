#ifndef POOLRAD_RECOVERED_OVERLAY_U9B_RECORD_TABLE_0255_H
#define POOLRAD_RECOVERED_OVERLAY_U9B_RECORD_TABLE_0255_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9B_0255_RAM_SIZE (1u << 20)

/* Register state on entry to unit 0x9B:+0x0255, after the caller's CALL.
 * The return IP/CS and far record argument already occupy SS RAM. The model
 * executes PUSH BP through RETF 4 and leaves the returned CPU state here. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9b_0255_cpu;

typedef enum {
    PR_U9B_0255_OK = 0,
    PR_U9B_0255_ARGUMENT
} pr_u9b_0255_status;

/* Complete terminating paths of the 212-byte, call-free DOS v1.3 body.
 * Every SS local, far-pointer argument, ES record, and DS table access uses
 * the same A20-off 1 MiB RAM. Reads are repeated in source order so aliases
 * can alter subsequent indexes and pointers. A cycling alias loops as in DOS.
 * Null/short host RAM or CPU is rejected before any write. */
pr_u9b_0255_status pr_u9b_record_table_0255(
    uint8_t *ram, size_t ram_size, pr_u9b_0255_cpu *cpu);

#endif

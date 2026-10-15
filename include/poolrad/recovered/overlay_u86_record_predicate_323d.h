#ifndef POOLRAD_RECOVERED_OVERLAY_U86_RECORD_PREDICATE_323D_H
#define POOLRAD_RECOVERED_OVERLAY_U86_RECORD_PREDICATE_323D_H

#include <stddef.h>
#include <stdint.h>

#define PR_U86_323D_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u86_323d_cpu;

typedef enum {
    PR_U86_323D_OK = 0,
    PR_U86_323D_ARGUMENT
} pr_u86_323d_status;

/* Complete unit 86:+323D..+3289, entered after far CALL. SS:[SP+4]
 * holds the record offset and SS:[SP+6] its segment; RETF 4 consumes
 * the four return bytes plus four argument bytes. AL=1 iff the non-null
 * record's unsigned byte +2E indexes a DS:5582+16*i byte in 11..13.
 * Preserves raw repeated reads, register/flag effects and aliased frames.
 * Code must stay immutable. Offsets/SP wrap at 16 bits; word/LES/RETF
 * operands use contiguous physical bytes with A20 off (1 MiB wrap).
 * AF is undefined on the null-pointer OR path; all non-null CMP flags
 * are defined, including AF. No other input pointers are rejected.
 */
pr_u86_323d_status pr_u86_record_predicate_323d(
    uint8_t *ram, size_t ram_size, pr_u86_323d_cpu *cpu);

#endif

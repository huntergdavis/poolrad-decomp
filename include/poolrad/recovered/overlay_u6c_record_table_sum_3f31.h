#ifndef POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_TABLE_SUM_3F31_H
#define POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_TABLE_SUM_3F31_H

#include <stddef.h>
#include <stdint.h>

#define PR_U6C_3F31_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6c_3f31_cpu;

typedef enum {
    PR_U6C_3F31_OK = 0,
    PR_U6C_3F31_ARGUMENT
} pr_u6c_3f31_status;

/* Complete +3F31..+3FB2, entered after CALL with far record at SS:SP+4.
 * Eight signed record-byte gates select byte additions from DS:408E+type,
 * with two conditional increments. All local/record/table aliases and
 * defined CPU/FLAGS are retained. AX returns the byte total; RETF 4.
 * Effective offsets/SP wrap at 16 bits; multi-byte operands are contiguous
 * physical bytes with A20 off. Code must remain immutable.
 */
pr_u6c_3f31_status pr_u6c_record_table_sum_3f31(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu);

#endif

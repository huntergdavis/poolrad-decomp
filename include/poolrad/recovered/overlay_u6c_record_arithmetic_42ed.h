#ifndef POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_ARITHMETIC_42ED_H
#define POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_ARITHMETIC_42ED_H

#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U6C_42ED_RAM_SIZE PR_U6C_3F31_RAM_SIZE
typedef pr_u6c_3f31_cpu pr_u6c_42ed_cpu;
typedef struct { uint16_t system_cs; } pr_u6c_42ed_layout;
typedef enum {
    PR_U6C_42ED_OK = 0,
    PR_U6C_42ED_ARGUMENT,
    PR_U6C_42ED_TRANSFER,
    PR_U6C_42ED_UNSUPPORTED_RUNTIME
} pr_u6c_42ed_status;

/* Loaded U6C +42ED..4453 and reached SYSTEM multiply/divide/absolute bodies.
 * Entry: far record at SS:SP+4, signed amount low/high at SS:SP+8/+10;
 * RETF 8. RAM/CPU include nested far frames and saved FLAGS. Code/layout
 * stay immutable, offsets wrap at 16 bits, contiguous bytes at 20 bits.
 * TRANSFER stops after an altered internal far return. Zero divisor stops
 * at SYSTEM +00E2 before its unmodeled error handler, with AX=00C8 and BP
 * already popped. AF after OR is undefined (chosen zero), including saved
 * flag words. No loader, asynchronous interrupts or artificial loop cap.
 */
pr_u6c_42ed_status pr_u6c_record_arithmetic_42ed(
    uint8_t *ram, size_t ram_size, pr_u6c_42ed_cpu *cpu,
    const pr_u6c_42ed_layout *layout);
#endif

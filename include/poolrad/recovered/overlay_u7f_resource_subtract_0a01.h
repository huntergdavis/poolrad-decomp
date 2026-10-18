#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_RESOURCE_SUBTRACT_0A01_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_RESOURCE_SUBTRACT_0A01_H
#include "poolrad/recovered/overlay_u7f_record_subtract_0020.h"
typedef pr_u7f_0020_cpu pr_u7f_0a01_cpu;
typedef enum {
    PR_U7F_0A01_OK = 0,
    PR_U7F_0A01_ARGUMENT,
    PR_U7F_0A01_CONTROL_EXIT
} pr_u7f_0a01_status;
/* Complete public entry 8 (+0A01..+0A7C), entered after CALL at +0A01.
 * SS:SP+4 index low byte, +6 amount, +8 record offset, +10 record segment.
 * Includes the existing +0020 callee, ordered aliases, A20-off 1 MiB RAM,
 * all CPU words, and RETF 8. Original instruction bytes must be immutable.
 * An altered nested return resumes at any recovered instruction boundary;
 * CONTROL_EXIT exposes the exact CPU/RAM state at a transfer outside those
 * boundaries. No external dispatcher is silently simulated.
 * SHL's undefined AF follows the measured DOSBox-X normal-core 386 oracle
 * (set). Other x86 implementations may choose another value for this bit.
 * Scalar words and LES operands use contiguous physical bytes after 16-bit
 * effective addressing; RETF uses separately wrapped 16-bit stack pops.
 * LES operands at FFFD..FFFF have Unicorn evidence only. Hardware faults,
 * interrupts, executing-code aliases, and CPU storage overlapping RAM are
 * outside the profile. Repeated redirected internal returns have no
 * artificial iteration cap; the host must supply an appropriate frame.
 */
pr_u7f_0a01_status pr_u7f_resource_subtract_0a01(
    uint8_t *ram, size_t ram_size, pr_u7f_0a01_cpu *cpu);
#endif

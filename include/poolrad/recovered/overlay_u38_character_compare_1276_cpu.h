#ifndef POOLRAD_RECOVERED_OVERLAY_U38_CHARACTER_COMPARE_1276_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_CHARACTER_COMPARE_1276_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U38_1276_RAM_SIZE (1u << 20)
typedef struct {uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;} pr_u38_1276_cpu;
typedef enum {
    PR_U38_1276_OK=0,PR_U38_1276_ARGUMENT,PR_U38_1276_CODE,
    PR_U38_1276_CALLEE,PR_U38_1276_TRANSFER
} pr_u38_1276_status;
/* Complete U38 [1276,12B2), entry40, RETF0Ah. Five word arguments unused.
 * Active pointer DS:5D92/5D94 supplies byte+9Fh; unless it equals1, return.
 * Compare the active Pascal string against CS:1268 via raw SYSTEM +0C10.
 * On inequality reload the active pointer and compare against CS:1270.
 * Either equality subtracts4 from DS:6822. All reads/writes, stack/data
 * aliases and flags stay live; code strings are read from RAM. The leaf
 * clears DF and returns complete CPU/RAM effects. No result substitution.
 * Exactly1MiB A20-off RAM, IP1276, SP[000E,FFF2], immutable loaded code,
 * constants and call bindings. CPU/status storage distinct from RAM.
 * Effective offsets wrap16 bits; physical addresses20 bits. Word/LES pairs
 * read physically contiguous bytes. Interrupts/faults/arbitrary-PC resume
 * excluded. CODE stops before malformed reached LCALL, four arguments
 * pushed. CALLEE/TRANSFER retain dependency state (defensive guards; the
 * current read-only leaf cannot produce them under this profile).
 * ARGUMENT changes no CPU/RAM. Nullable callee_status starts -1 and keeps
 * the last compare status. No rollback. Both loaded segments may differ.
 */
pr_u38_1276_status pr_u38_character_compare_1276_cpu(uint8_t *ram,size_t size,
    pr_u38_1276_cpu *cpu,int *callee_status);
#endif

#ifndef POOLRAD_RECOVERED_OVERLAY_U38_CHARACTER_LOOP_11F9_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_CHARACTER_LOOP_11F9_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U38_11F9_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u38_11f9_cpu;
typedef enum {
    PR_U38_11F9_OK=0, PR_U38_11F9_ARGUMENT, PR_U38_11F9_CODE,
    PR_U38_11F9_CALLEE, PR_U38_11F9_TRANSFER, PR_U38_11F9_LIMIT
} pr_u38_11f9_status;
/* Complete U38 executable [11F9,1268), entry39, RETF0Ah. Five word
 * arguments; entry SS:[SP+0Ah]/[SP+0Ch] is a far record pointer A.
 * Follow A+108h to B; save B+0Ah's far pointer P at DS:6826/6828.
 * Active character DS:5D92/5D94 must have byte+9Fh=1 and (byte+6Ch&7Fh)=2.
 * If eligible, compare P's Pascal string against DS:(03F2h+16*i), i=1..8;
 * each equality subtracts4 from DS:6822. Raw SYSTEM +0C10 supplies all
 * CPU/RAM effects and clears DF. Every operand, including the byte counter
 * SS:[BP-1] after subtraction, is read live; data/stack aliases are allowed.
 * max_loopbacks caps taken loop-backs at +1236 before INC (normally7).
 * Zero allows the first comparison. LIMIT retains all completed effects.
 * Exactly1MiB A20-off RAM, IP11F9, SP[0010,FFF2], immutable original code
 * and loaded call binding. All operand pairs are physically contiguous;
 * offsets wrap16 bits and physical addresses20 bits. CPU/status storage
 * distinct from RAM. Interrupts, faults and arbitrary-PC resume excluded.
 * CODE stops at +1250 before a malformed LCALL, four arguments pushed;
 * CALLEE/TRANSFER preserve failed or changed dependency returns (defensive
 * guards; the current read-only leaf always returns under this profile).
 * ARGUMENT changes no CPU/RAM. callee_status (nullable) starts -1 and keeps
 * the last comparator status. No rollback. Flags follow the dependency's
 * profile; SHL's undefined flags are overwritten by ADD before any stop.
 */
pr_u38_11f9_status pr_u38_character_loop_11f9_cpu(uint8_t *ram,size_t ram_size,
    pr_u38_11f9_cpu *cpu,unsigned max_loopbacks,int *callee_status);
#endif

#ifndef POOLRAD_RECOVERED_OVERLAY_U86_STRING_NODE_028D_H
#define POOLRAD_RECOVERED_OVERLAY_U86_STRING_NODE_028D_H
#include <stddef.h>
#include <stdint.h>
#define PR_U86_028D_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u86_028d_cpu;
typedef enum { PR_U86_028D_NO_CALLEE=-1,PR_U86_028D_GETMEM=0,PR_U86_028D_STRSTORE,PR_U86_028D_STRLOAD,PR_U86_028D_STRCAT } pr_u86_028d_callee;
typedef struct { pr_u86_028d_callee callee;int status; } pr_u86_028d_diagnostic;
typedef enum { PR_U86_028D_OK=0,PR_U86_028D_ARGUMENT,PR_U86_028D_CODE,PR_U86_028D_CALLEE,PR_U86_028D_TRANSFER,PR_U86_028D_LIST_LIMIT } pr_u86_028d_status;
/* Complete U86 [028D,044E), RETF2; byte index/flag in word at entry SP+4.
 * Append an optional group-heading node and a prefixed item-name node to
 * DS:4766/4768. Nodes are 46 bytes: Pascal string capacity40, heading flag
 * +29h, next pointer +2Ah/+2Ch. The low7 argument bits select the item;
 * bit7 selects the prefix at immutable CS:0287 versus CS:028A. Group bytes
 * are DS:31FB+16*item; group/name strings DS:0A79+41*signed8(group) / 28E9+41*item.
 * Byte count of preceding non-heading nodes starts at1 and wraps; CBW
 * sign-extends it for the DS:476A history reads/writes. No host array cap.
 * All data/heap/stack operands are reread in original order. Compose raw
 * SYSTEM GetMem, StrStore, StrLoad and StrCat using live LCALL segments.
 * Exact1MiB A20-off RAM; entry IP028D and conservative SP[0140,FFFA].
 * CPU/diagnostic distinct from RAM; immutable loaded code and prefix data;
 * writes must not overlap code. Every dependency return frame must fit
 * within SS:0000..FFFF, including alias-induced frame changes. Logical AF
 * follows normal/386 zero; MUL's undefined bits follow accepted U86 +07D6
 * and are overwritten before every observable stop/return here.
 * max_list_visits positive, counted at +02D4 including the terminal node;
 * max_heap_steps is GetMem's per-call walk-edge budget and may be zero.
 * CALLEE retains the raw status/state (including RunError/custom HeapError);
 * CODE stops before a malformed reached LCALL; TRANSFER after an altered
 * direct return; LIST_LIMIT before the next +02D4. No rollback. ARGUMENT
 * leaves CPU/RAM unchanged; diagnostic is initialized to NO_CALLEE/-1.
 * Interrupts, faults and self-modifying code are outside this profile.
 */
pr_u86_028d_status pr_u86_string_node_028d(uint8_t *ram,size_t size,
    pr_u86_028d_cpu *cpu,size_t max_list_visits,unsigned max_heap_steps,
    pr_u86_028d_diagnostic *diagnostic);
#endif

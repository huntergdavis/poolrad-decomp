#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_STRING_NODE_0E37_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_STRING_NODE_0E37_H
#include <stddef.h>
#include <stdint.h>
#define PR_U6A_0E37_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u6a_0e37_cpu;
typedef enum {
    PR_U6A_0E37_OK=0,PR_U6A_0E37_ARGUMENT,PR_U6A_0E37_GETMEM_STOP,
    PR_U6A_0E37_STRSTORE_STOP,PR_U6A_0E37_TRANSFER
} pr_u6a_0e37_status;
/* Complete executable U6A [0E37,0EBE), RETF8. Entry stack: return IP/CS,
 * parent-frame offset, source string offset/segment, attribute WORD.
 * Snapshot a Pascal string into SS:BP-29h (capacity40); GetMem46; append
 * to the tail pointer at SS:parent-0Ch using node link+2Ah/+2Ch; advance
 * the tail, clear its new link, store the snapshot and attribute+29h.
 *
 * Exact 1MiB A20-off RAM, IP0E37, initial SP[0060,FFF4]. Caller/heap
 * writes must avoid code; CPU/diagnostic storage must not alias RAM.
 * Original loaded bodies are immutable; far-call segments are read live. Every
 * reached dependency far-return frame must fit inside SS:0000..FFFF,
 * including after aliases change BP. Data/stack aliases remain live.
 * GetMem's default HeapError is composed; custom HeapError, RunError203,
 * walk limit or changed return retain their live state as GETMEM_STOP.
 * max_heap_steps follows GetMem's taken free-list walk-edge budget.
 * TRANSFER preserves an unexpected direct callee return. ARGUMENT
 * leaves RAM/CPU unchanged; callee_status is -1 then last raw status.
 * No rollback; interrupts, self-modifying code and faults are excluded.
 */
pr_u6a_0e37_status pr_u6a_string_node_0e37(uint8_t *ram,size_t size,
    pr_u6a_0e37_cpu *cpu,unsigned max_heap_steps,int *callee_status);
#endif

#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_LIST_RELEASE_0026_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_LIST_RELEASE_0026_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U9E_0026_RAM_SIZE (1u<<20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u9e_0026_cpu;
typedef struct { unsigned max_list_loopbacks,max_heap_loopbacks; } pr_u9e_0026_limits;
typedef struct {
    int freemem_status; /* -1 until called; otherwise pr_system_freemem_status */
    uint16_t handler_offset,handler_segment; /* live table pointer at PENDING */
} pr_u9e_0026_diagnostic;
typedef enum {
    PR_U9E_0026_OK=0, PR_U9E_0026_ARGUMENT, PR_U9E_0026_CODE,
    PR_U9E_0026_CALLEE, PR_U9E_0026_TRANSFER, PR_U9E_0026_PENDING,
    PR_U9E_0026_LIMIT
} pr_u9e_0026_status;
/* Partial chain: exact U9E [0026,0140), composing raw SYSTEM FreeMem and
 * the U9E +0000 dispatcher prefix. PENDING stops BEFORE +001E's indirect
 * LCALL [DS:6828+4*type], with the dispatcher frame and five handler words
 * already pushed. No callback is invented, skipped, or treated as success.
 * After far CALL: return IP/CS, item off/seg, type word, member off/seg;
 * RETF 10. A nil item searches member+7F by type. A found/supplied node
 * with byte+4 zero is unlinked and freed with size9. A missing predecessor
 * still writes through 0000:0000 exactly as the original does.
 * Exact1MiB A20off RAM; IP0026, SP[0040,FFF2]; CPU/limits/diagnostic must
 * be distinct from RAM and each other; code/bindings immutable, all reached
 * return frames nonwrapping. Live operands preserve stack/data aliases.
 * AF follows DOSBox-X normal/386: logical zero, SHL one. IRQs/faults excluded.
 * CODE checks reached CALL operands; FreeMem's segment is read live.
 * Shared list budget counts taken +0076->0052 / +0113->00E5 edges. LIMIT
 * stops at that edge's target before the next iteration. Zero is valid.
 * Other stops retain partial CPU/RAM; ARGUMENT leaves CPU/RAM unchanged.
 */
pr_u9e_0026_status pr_u9e_list_release_0026_cpu(uint8_t *ram,size_t size,
    pr_u9e_0026_cpu *cpu,const pr_u9e_0026_limits *limits,
    pr_u9e_0026_diagnostic *diagnostic);
#endif

#ifndef POOLRAD_RECOVERED_OVERLAY_U86_SIDECAR_LIST_044E_H
#define POOLRAD_RECOVERED_OVERLAY_U86_SIDECAR_LIST_044E_H
#include <stddef.h>
#include <stdint.h>
#define PR_U86_044E_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u86_044e_cpu;
typedef struct { size_t max_caller_visits,max_list_visits;unsigned max_lookup_loopbacks,max_heap_steps; } pr_u86_044e_limits;
typedef enum { PR_U86_044E_NO_CALLEE=-1,PR_U86_044E_LOOKUP=0,PR_U86_044E_APPEND } pr_u86_044e_callee;
typedef struct { pr_u86_044e_callee callee;int status,nested_callee,nested_status; } pr_u86_044e_diagnostic;
typedef enum { PR_U86_044E_OK=0,PR_U86_044E_ARGUMENT,PR_U86_044E_CODE,PR_U86_044E_CALLEE,PR_U86_044E_TRANSFER,PR_U86_044E_LOOP_LIMIT } pr_u86_044e_status;
/* Complete U86 [044E,051C), RETF2. Low argument byte selects item mode:
 * nonzero accepts slot bytes >80h; zero accepts any nonzero slot byte.
 * Raw RES +2199 lookup(key10h), gate clear/check, then slots1..3 compose
 * U86 +028D and store current object pointers in DS:477F+4*byte[4843].
 * All memory operands are live, including local-index and pointer aliases.
 * Exact1MiB A20-off RAM, entry IP044E, conservative SP[0150,FFFA], immutable
 * code/prefixes/bindings. CPU/limits/diagnostic must be distinct from RAM.
 * Dependencies retain their profiles; all reached return frames must be
 * nonwrapping even after alias changes. Logical AF uses normal/386 zero.
 * Positive caller/list budgets; lookup-loopback and heap-walk budgets may
 * be zero. Caller visits count +04AD, list budget resets per append call.
 * CODE precedes malformed reached LCALL/CALL; CALLEE retains raw partial
 * state/status, APPEND also exposes its nested callee/status. TRANSFER is
 * an altered direct return; LOOP_LIMIT is before the next +04AD. No rollback.
 * ARGUMENT changes no CPU/RAM; diagnostic initializes NO_CALLEE/-1. Null
 * diagnostic allowed. IRQs, faults and self-modifying code are excluded.
 */
pr_u86_044e_status pr_u86_sidecar_list_044e(uint8_t *ram,size_t size,pr_u86_044e_cpu *cpu,
    const pr_u86_044e_limits *limits,pr_u86_044e_diagnostic *diagnostic);
#endif

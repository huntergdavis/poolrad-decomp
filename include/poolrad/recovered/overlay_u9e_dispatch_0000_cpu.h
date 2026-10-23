#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_DISPATCH_0000_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_DISPATCH_0000_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_U9E_0000_RAM_SIZE (1u << 20)

typedef struct { uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags; } pr_u9e_0000_cpu;

typedef enum {
    PR_U9E_0000_RETURNED = 0,
    PR_U9E_0000_ARGUMENT,   /* null/size/IP or entry SP outside the contract; nothing changed */
    PR_U9E_0000_REFUSED,    /* the index byte is outside the declared domain (slot 0, 140..255); nothing changed, no call made */
    PR_U9E_0000_GAP,        /* the slot's handler is a NAMED gap (see diagnostic.slot/handler_id/gap); CPU left at the LCALL (IP 001E) with the five words pushed */
    PR_U9E_0000_CODE,       /* the table word or the loaded stub is not the one the model was built on; IP at the LCALL, nothing called */
    PR_U9E_0000_LIMIT,      /* the host budget ran out at the next unexecuted dispatcher instruction */
    PR_U9E_0000_CALLEE,     /* a composed handler stopped otherwise; diagnostic.callee_status holds its raw status */
    PR_U9E_0000_TRANSFER    /* a return went elsewhere than the dispatcher expected; CPU left at the popped CS:IP */
} pr_u9e_0000_status;

typedef enum {
    PR_U9E_0000_GAP_NONE = 0,
    PR_U9E_0000_GAP_SEMANTIC,    /* credited handler with a semantic model: phase 2 */
    PR_U9E_0000_GAP_UNCREDITED,  /* handler without a strict model */
    PR_U9E_0000_GAP_DEFERRED     /* credited raw model whose world (the 1693 display closure) is not composed here */
} pr_u9e_0000_gap;

/* Host caps for the composed handlers' own limits (they carry loop-back, SYSTEM-call and heap caps, not instruction budgets). */
typedef struct {
    unsigned loopbacks, system_calls, heap_steps;
    size_t links;
} pr_u9e_0000_limits;

typedef struct {
    int slot;                    /* the index byte, -1 before it is read */
    uint32_t handler_id;         /* unit << 16 | code offset of the slot's handler, 0 when there is none */
    pr_u9e_0000_gap gap;
    int callee_status;           /* raw status of the composed handler when it stopped, else -1 */
    uint16_t stub_off, stub_seg; /* the table word this call resolved */
} pr_u9e_0000_diagnostic;

/* Original unit 0x9E +0000 (GAME.OVR, public entry 1, stub 00B0:0025): the effect dispatcher, 38 bytes / 16 instructions, RETF 000Ch (docs/re/OVL_009E_0000_CPU.md).
 * Frame after the far call: SP IP, SP+2 CS, SP+4 five forwarded words' sources ([BP+6] is a byte, [BP+8], [BP+0Ah], [BP+0Ch], [BP+0Eh] are words) and SP+0Eh the INDEX byte.
 * The body forwards the five words, computes DI = 4 * INDEX and executes LCALL [DI+6828h].
 *
 * DOMAIN (declared and ASSERTED): INDEX must be 1..139. Anything else (0 and 140..255) is REFUSED before any instruction runs: CPU, RAM and budget untouched, nothing called.
 * COVERAGE: only the slots whose kind is RAW below are composed with their handler's model (the raw-CPU credited handlers). Every other slot in 1..139 STOPS at the LCALL with GAP
 * and a named gap kind; the dispatcher is NOT modelled for those slots, and no caller may read a RETURNED stop as covering them.
 *
 * Contract: exactly 1 MiB of A20-off RAM, IP 0000, entry SP in [0200h,FFEEh]; the table words at DS:6828h+4*INDEX and the loaded stub (EAh, offset, segment) must be the original's
 * (CODE otherwise). Each dispatcher instruction consumes one unit of budget; the stub JMP and the composed handlers consume none (they carry their own caps in limits). Pinned flags: SHL sets AF.
 * LIMIT leaves the next unexecuted counted instruction in cpu->cs:ip. diagnostic may be NULL. */
pr_u9e_0000_status pr_u9e_dispatch_0000_cpu(uint8_t *ram, size_t ram_size, pr_u9e_0000_cpu *cpu, size_t *budget,
                                            const pr_u9e_0000_limits *limits, pr_u9e_0000_diagnostic *diagnostic);

/* The slot table the model was built on, for the audit and the tests. kind: 0 refused, 1 raw (composed), 2 semantic gap, 3 uncredited gap, 4 deferred gap. */
int pr_u9e_dispatch_0000_slot_kind(unsigned slot);
uint16_t pr_u9e_dispatch_0000_slot_stub_off(unsigned slot);
uint16_t pr_u9e_dispatch_0000_slot_stub_seg(unsigned slot);
uint16_t pr_u9e_dispatch_0000_slot_entry(unsigned slot);
uint32_t pr_u9e_dispatch_0000_slot_id(unsigned slot);
#endif

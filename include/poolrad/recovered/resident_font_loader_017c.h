#ifndef POOLRAD_RECOVERED_RESIDENT_FONT_LOADER_017C_H
#define POOLRAD_RECOVERED_RESIDENT_FONT_LOADER_017C_H
#include <stddef.h>
#include <stdint.h>

#define PR_FONT017C_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_font017c_cpu;

typedef struct {
    unsigned ordinal;
    uint16_t call_ip, target_segment, target_offset, return_cs, return_ip;
} pr_font017c_call;

/* Called after the original far CALL pushes CS then return IP. The hook
 * executes the external callee, including its return and stack cleanup.
 * It may change all live CPU/RAM state. Success must return to return_cs/ip.
 * The two B0B copies leave destination pointers for the subsequent DAX call;
 * a hook must implement that contract, not assume all calls remove all args.
 * A nonzero result stops immediately with preceding effects retained. */
typedef int (*pr_font017c_callee)(void *context, const pr_font017c_call *call,
                                 uint8_t *ram, size_t ram_size,
                                 pr_font017c_cpu *cpu);

typedef enum {
    PR_FONT017C_OK = 0, PR_FONT017C_ARGUMENT,
    PR_FONT017C_CALLEE, PR_FONT017C_RETURN
} pr_font017c_status;

/* Complete RES0709 [017C,01DF), including RETF. cpu->ip must be 017C.
 * load_segment is the loader relocation bias added to original call segments.
 * RAM and CPU storage must be distinct; immutable code, synchronous raw RAM,
 * 20-bit physical wrap, contiguous word/LES operand bytes, independently wrapped
 * RETF pops.
 * Invalid arguments are atomic. Callback failures retain live partial state.
 * External callees are explicit boundaries, not translated by this function. */
pr_font017c_status pr_resident_font_loader_017c(
    uint8_t *ram, size_t ram_size, pr_font017c_cpu *cpu,
    uint16_t load_segment, pr_font017c_callee callee, void *context);
#endif

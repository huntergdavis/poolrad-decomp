#ifndef POOLRAD_RECOVERED_OVERLAY_U25_INIT_01B6_H
#define POOLRAD_RECOVERED_OVERLAY_U25_INIT_01B6_H
#include "poolrad/recovered/overlay_u25_loader_0000.h"
typedef pr_u25_0000_cpu pr_u25_01b6_cpu;
typedef enum {
    PR_U25_01B6_OK = 0,
    PR_U25_01B6_ARGUMENT,
    PR_U25_01B6_LOOP_LIMIT,
    PR_U25_01B6_OPERAND_LIMIT,
    PR_U25_01B6_STRING_LIMIT,
    PR_U25_01B6_CALLEE,
    PR_U25_01B6_TRANSFER
} pr_u25_01b6_status;
/* Complete U25 entry 3 [01B6,0322): script-state initializer. Enter IP=01B6
 * after far CALL, SS:SP = return IP/CS; no arguments, RETF. Calls the full
 * +0000 loader/string chain, +06AF word conversion and SYSTEM FillChar.
 * CPU14 and shared 1 MiB RAM are live throughout; system_cs is the loaded
 * SYSTEM binding. No host script/result hooks or string truncation.
 *
 * Initial SP [0236,FFFC] reserves nested frames without offset wrap.
 * Effective offsets wrap16, word/LES bytes are contiguous physical20.
 * RAM must be synchronous, distinct from CPU, with immutable loaded code
 * and bindings. All writes (including nested calls/stack) must avoid code;
 * this is a precondition, not a dynamically checked code-write guard.
 * Data/stack alias effects are retained in instruction order.
 *
 * Positive max_clear_iterations counts shared +02D7/+02FC loop headers;
 * positive max_operands resets at each +0000 call, and positive
 * max_string_iterations resets at each nested string call. A limit stops
 * before the declined instruction with partial CPU/RAM, never a DOS return.
 * CALLEE preserves a rejected callee's entry state; TRANSFER preserves an
 * unexpected internal return. Invalid initial arguments reject atomically.
 * Logical/shift AF=0 and multi-bit shift OF retained follow the reused
 * loader contract. Native boundary profiles are documented separately.
 */
pr_u25_01b6_status pr_u25_script_init_01b6(
    uint8_t *ram, size_t ram_size, pr_u25_01b6_cpu *cpu,
    uint16_t system_cs, size_t max_clear_iterations,
    size_t max_operands, size_t max_string_iterations);
#endif

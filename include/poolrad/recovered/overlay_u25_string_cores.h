#ifndef POOLRAD_RECOVERED_OVERLAY_U25_STRING_CORES_H
#define POOLRAD_RECOVERED_OVERLAY_U25_STRING_CORES_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_STRINGS_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_strings_cpu;

typedef enum {
    PR_U25_STRINGS_OK = 0,
    PR_U25_STRINGS_ARGUMENT,
    PR_U25_STRINGS_LIMIT,
    PR_U25_STRINGS_TRANSFER
} pr_u25_strings_status;

/* Full-state U25 string cores, composed with local +06D3/+06FB and SYSTEM
 * 0B0B/0B25/0B98/0C3B. Enter after far CALL with the original return frame.
 * 1153 arguments: buffer index word, packed byte count word (low bytes).
 * 12FB arguments: address word, buffer index word (low byte). Both RETF 4.
 *
 * RAM must be exactly 1 MiB, synchronous, distinct from CPU; code and loaded
 * SYSTEM binding must remain immutable. system_cs is its loaded segment.
 * Data pointers and tables live in shared RAM; all aliasing effects follow
 * instruction order. Source/destination writes must not modify code.
 * Initial SP in [0218,FFF8] avoids a wrapped reserved frame. Effective
 * offsets wrap at 16 bits; word bytes are physically contiguous with A20 off.
 *
 * Each reached loop header (1172,1327,13D3,1453,14D2) consumes one budget
 * unit; LIMIT stops before that original instruction with reached CPU/RAM.
 * This is a host boundary, not a DOS return or a termination guarantee.
 * TRANSFER exposes an unexpected callee return, e.g. a clobbered frame.
 * No hooks, host string cap or implicit termination are substituted.
 * Undefined logical/shift AF is zero by convention; multi-bit shift OF
 * is retained. Native boundary profiles and route parity remain separate.
 * Invalid entry arguments reject atomically; successful model execution
 * alone is not strict-credit certification.
 */
pr_u25_strings_status pr_u25_unpack_text_1153(
    uint8_t *ram, size_t ram_size, pr_u25_strings_cpu *cpu,
    uint16_t system_cs, size_t max_iterations);
pr_u25_strings_status pr_u25_string_by_address_12fb(
    uint8_t *ram, size_t ram_size, pr_u25_strings_cpu *cpu,
    uint16_t system_cs, size_t max_iterations);
#endif

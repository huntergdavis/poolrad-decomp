#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_SIDECAR_CALL_018B_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_SIDECAR_CALL_018B_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_018B_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_018b_cpu;

typedef enum {
    PR_U9E_018B_ABSENT = 0, /* CS:019C, before set-membership call */
    PR_U9E_018B_FOUND,      /* CS:02D4, before indirect dispatch setup */
    PR_U9E_018B_ARGUMENT,
    PR_U9E_018B_LOOKUP_LIMIT
} pr_u9e_018b_status;

/* Enter at the original U9E far call, with its five prepared words at
 * SS:SP. Acyclic, mapped sidecar lists are required. max_nodes is a host
 * guard; its failure does not represent a DOS return. */
pr_u9e_018b_status pr_u9e_sidecar_call_018b(
    uint8_t *ram, size_t ram_size, pr_u9e_018b_cpu *cpu, size_t max_nodes);

#endif

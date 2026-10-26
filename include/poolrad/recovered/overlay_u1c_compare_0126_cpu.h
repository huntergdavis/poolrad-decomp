#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_COMPARE_0126_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_COMPARE_0126_CPU_H

#include "poolrad/recovered/overlay_u25_loader_0000.h"

typedef pr_u25_0000_cpu pr_u1c_compare_0126_cpu;
typedef enum {
    PR_U1C_0126_RETURNED = 0,
    PR_U1C_0126_ARGUMENT,
    PR_U1C_0126_CODE,
    PR_U1C_0126_CALLEE,
    PR_U1C_0126_TRANSFER
} pr_u1c_compare_0126_status;
typedef enum {
    PR_U1C_0126_NONE = 0,
    PR_U1C_0126_OPERANDS,
    PR_U1C_0126_STRING_COPY,
    PR_U1C_0126_STRING_COMPARE,
    PR_U1C_0126_SLOT_VALUE,
    PR_U1C_0126_WORD_COMPARE
} pr_u1c_compare_0126_callee;
typedef struct {
    uint16_t system_cs;
    size_t max_operands, max_string_iterations, max_compare_instructions;
} pr_u1c_compare_0126_limits;
typedef struct {
    pr_u1c_compare_0126_callee callee;
    int status;
} pr_u1c_compare_0126_diagnostic;

/* Read-only observation before each instruction of the 0126 body. The callback
 * must not mutate CPU/RAM, reenter the model, or execute guest operations. It
 * observes flags even when a subsequent callee overwrites them. NULL disables it. */
typedef void (*pr_u1c_compare_0126_observer)(const uint8_t *ram, size_t size,
                                          const pr_u1c_compare_0126_cpu *cpu,
                                          void *context);

/* U1C [0126,01A3), ECL COMPARE: no arguments, far return. Composes the existing
 * raw U25 operand loader, slot reader, string/word compares, and SYSTEM StrStore.
 * Exactly 1 MiB A20-off flat RAM, separate from CPU/limits/diagnostic objects;
 * synchronous execution with immutable executed code and already-loaded stubs.
 * Entry IP=0126, SP in [0436,FFFB], positive limits. Effective offsets wrap16;
 * multibyte operands are contiguous physical20. TF/interrupts and hardware
 * apertures are outside this contract. Data/frame aliases follow original order.
 * The operand and string limits retain the callee's documented meaning; the
 * compare limit counts instructions of U25:1815, not its bounded SYSTEM leaves.
 * Slot-value cyclic party lists can fail to terminate; no termination claim.
 * CODE stops before an invalid body call or unloaded/misdirected stub. CALLEE
 * exposes a reached helper refusal/limit with its CPU/RAM and diagnostic.
 * TRANSFER exposes an altered return. ARGUMENT rejects entry without CPU/RAM
 * changes. This candidate API confers no identity, credit or readiness. */
pr_u1c_compare_0126_status pr_u1c_compare_0126(
    uint8_t *ram, size_t size, pr_u1c_compare_0126_cpu *cpu,
    const pr_u1c_compare_0126_limits *limits,
    pr_u1c_compare_0126_diagnostic *diagnostic,
    pr_u1c_compare_0126_observer observe, void *context);

#endif

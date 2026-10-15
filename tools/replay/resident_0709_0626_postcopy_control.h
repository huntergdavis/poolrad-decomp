#ifndef POOLRAD_RESIDENT_0709_0626_POSTCOPY_CONTROL_H
#define POOLRAD_RESIDENT_0709_0626_POSTCOPY_CONTROL_H

#include "resident_1816_caller_routes.h"

typedef enum {
    PR_0626_POSTCOPY_VALID_AT_064F = 1,
    PR_0626_POSTCOPY_PRINTER_RETURN = 2
} pr_0626_postcopy_stop;

/* Start at 0AF8:0B25 and execute its complete 36-byte body through the
 * far return to 0709:0641. Return 2 if the copied return frame was changed. */
int pr_0626_runtime_copy_return(uint8_t *ram, size_t ram_size,
                                 pr_1816_caller_cpu *cpu);

/* Start at 0709:0641. Valid input stops before 064F; invalid row/column
 * executes the printer epilogue and returns to the caller. */
int pr_0626_postcopy_guard(uint8_t *ram, size_t ram_size,
                            pr_1816_caller_cpu *cpu,
                            pr_0626_postcopy_stop *stop);

/* Convenience composition of the two verified local transitions. All
 * memory uses a flat 20-bit real-mode image. */
int pr_0626_copy_and_guard(uint8_t *ram, size_t ram_size,
                            pr_1816_caller_cpu *cpu,
                            pr_0626_postcopy_stop *stop);

#endif

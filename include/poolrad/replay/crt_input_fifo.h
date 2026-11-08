#ifndef POOLRAD_REPLAY_CRT_INPUT_FIFO_H
#define POOLRAD_REPLAY_CRT_INPUT_FIFO_H

#include "poolrad/recovered/crt_keyboard_cpu.h"
#include <stddef.h>
#include <stdint.h>

#define PR_CRT_INPUT_FIFO_CAPACITY 16u

typedef struct {
    uint16_t events[PR_CRT_INPUT_FIFO_CAPACITY]; /* BIOS AX: scan in AH, character in AL */
    size_t head;
    size_t count;
} pr_crt_input_fifo;

typedef enum {
    PR_CRT_INPUT_OK = 0,
    PR_CRT_INPUT_ARGUMENT,
    PR_CRT_INPUT_FULL
} pr_crt_input_status;

/* Initialize before use. The caller owns the queue and serializes access. */
void pr_crt_input_fifo_init(pr_crt_input_fifo *fifo);
/* A full queue is unchanged; events are raw caller-supplied AX words. */
pr_crt_input_status pr_crt_input_fifo_push(pr_crt_input_fifo *fifo, uint16_t ax);

/* pr_crt_int16_service implementation. AH=01 peeks without consuming: ZF
 * indicates empty; when pending, AX is the front event. AH=00 consumes AX;
 * empty reads and unsupported AH decline without changing queue or *after.
 * On success IP advances by 2, and fields other than IP, AX and (only for
 * AH=01) ZF are preserved. No guest RAM is accessed. This is a bounded host
 * policy, not a complete BIOS model. Empty AH=00 makes the current CRT model
 * return PR_CRT_SERVICE at the INT after prior effects; it is not resumable. */
int pr_crt_input_fifo_service(void *context, const pr_crt_cpu *before,
                              pr_crt_cpu *after);

#endif

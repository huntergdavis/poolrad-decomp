#include "poolrad/replay/crt_input_fifo.h"

static int valid(const pr_crt_input_fifo *fifo)
{
    return fifo != NULL && fifo->head < PR_CRT_INPUT_FIFO_CAPACITY &&
           fifo->count <= PR_CRT_INPUT_FIFO_CAPACITY;
}

void pr_crt_input_fifo_init(pr_crt_input_fifo *fifo)
{
    if (fifo != NULL) {
        fifo->head = 0u;
        fifo->count = 0u;
    }
}

pr_crt_input_status pr_crt_input_fifo_push(pr_crt_input_fifo *fifo, uint16_t ax)
{
    size_t tail;
    if (!valid(fifo)) return PR_CRT_INPUT_ARGUMENT;
    if (fifo->count == PR_CRT_INPUT_FIFO_CAPACITY) return PR_CRT_INPUT_FULL;
    tail = (fifo->head + fifo->count) % PR_CRT_INPUT_FIFO_CAPACITY;
    fifo->events[tail] = ax;
    ++fifo->count;
    return PR_CRT_INPUT_OK;
}

int pr_crt_input_fifo_service(void *context, const pr_crt_cpu *before,
                              pr_crt_cpu *after)
{
    pr_crt_input_fifo *fifo = context;
    pr_crt_cpu result;
    unsigned function;
    if (!valid(fifo) || before == NULL || after == NULL) return 1;
    function = before->ax >> 8u;
    if (function > 1u || (function == 0u && fifo->count == 0u)) return 1;
    result = *before;
    result.ip = (uint16_t)(result.ip + 2u);
    if (function == 1u) {
        if (fifo->count == 0u) result.flags |= 0x0040u;
        else {
            result.flags &= (uint16_t)~0x0040u;
            result.ax = fifo->events[fifo->head];
        }
    } else {
        result.ax = fifo->events[fifo->head];
        fifo->head = (fifo->head + 1u) % PR_CRT_INPUT_FIFO_CAPACITY;
        --fifo->count;
    }
    *after = result;
    return 0;
}

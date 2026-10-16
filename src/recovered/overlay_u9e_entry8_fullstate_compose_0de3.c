/* Bounded U9E entry8 source composition; see evidence note. */
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include "poolrad/recovered/overlay_u9e_entry8_fullstate_loop_0e05.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_postcall_0e13.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_tail_0e1f.h"
#include "poolrad/recovered/overlay_u9e_entry8_system_call_0e0e.h"

/* These are distinct C struct types. Copy their fourteen named CPU words;
 * never reinterpret one component's pointer as another's. */
#define COPY_CPU(dst, src) do {         \
    (dst).ax = (src).ax;                 \
    (dst).bx = (src).bx;                 \
    (dst).cx = (src).cx;                 \
    (dst).dx = (src).dx;                 \
    (dst).si = (src).si;                 \
    (dst).di = (src).di;                 \
    (dst).bp = (src).bp;                 \
    (dst).sp = (src).sp;                 \
    (dst).cs = (src).cs;                 \
    (dst).ip = (src).ip;                 \
    (dst).ds = (src).ds;                 \
    (dst).es = (src).es;                 \
    (dst).ss = (src).ss;                 \
    (dst).flags = (src).flags;           \
} while (0)

pr_u9e_entry8_compose_status pr_u9e_entry8_fullstate_compose_0de3(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_compose_cpu *cpu,
    unsigned max_system_calls)
{
    pr_u9e_entry8_prefix_status prefix;
    unsigned calls = 0u;

    if (ram == NULL || cpu == NULL ||
        ram_size != PR_U9E_ENTRY8_PREFIX_RAM_SIZE || cpu->ip != 0x0DE3u)
        return PR_U9E_ENTRY8_COMPOSE_ARGUMENT;

    prefix = pr_u9e_entry8_fullstate_prefix_0de3(ram, ram_size, cpu);
    if (prefix == PR_U9E_ENTRY8_PREFIX_ARGUMENT)
        return PR_U9E_ENTRY8_COMPOSE_PREFIX_REJECTED;

    while (prefix == PR_U9E_ENTRY8_PREFIX_AT_CALL) {
        pr_u9e_entry8_call_cpu call_cpu;
        pr_u9e_entry8_call_status call;
        pr_u9e_entry8_postcall_cpu post_cpu;
        pr_u9e_entry8_postcall_status post;

        /* This host cap is checked before the next SYSTEM call slice.
         * The already executed prefix or loopback remains in live state. */
        if (calls >= max_system_calls)
            return PR_U9E_ENTRY8_COMPOSE_LIMIT;

        COPY_CPU(call_cpu, *cpu);
        call = pr_u9e_entry8_system_call_0e0e(ram, ram_size, &call_cpu);
        COPY_CPU(*cpu, call_cpu);
        if (call == PR_U9E_ENTRY8_CALL_ARGUMENT)
            return PR_U9E_ENTRY8_COMPOSE_CALL_ARGUMENT;
        if (call == PR_U9E_ENTRY8_CALL_SYSTEM_ARGUMENT)
            return PR_U9E_ENTRY8_COMPOSE_SYSTEM_ARGUMENT;
        if (call == PR_U9E_ENTRY8_CALL_SYSTEM_FAILED)
            return PR_U9E_ENTRY8_COMPOSE_SYSTEM_FAILED;
        if (call == PR_U9E_ENTRY8_CALL_SYSTEM_TRANSFER)
            return PR_U9E_ENTRY8_COMPOSE_SYSTEM_TRANSFER;
        if (call == PR_U9E_ENTRY8_CALL_RETURN_TRANSFER)
            return PR_U9E_ENTRY8_COMPOSE_RETURN_TRANSFER;
        ++calls;

        COPY_CPU(post_cpu, *cpu);
        post = pr_u9e_entry8_fullstate_postcall_0e13(
            ram, ram_size, &post_cpu);
        COPY_CPU(*cpu, post_cpu);
        if (post == PR_U9E_ENTRY8_POSTCALL_ARGUMENT)
            return PR_U9E_ENTRY8_COMPOSE_POSTCALL_REJECTED;
        if (post == PR_U9E_ENTRY8_POSTCALL_AT_TAIL)
            break;

        {
            pr_u9e_entry8_loop_cpu loop_cpu;
            pr_u9e_entry8_loop_status loop;
            COPY_CPU(loop_cpu, *cpu);
            loop = pr_u9e_entry8_fullstate_loop_0e05(
                ram, ram_size, &loop_cpu);
            COPY_CPU(*cpu, loop_cpu);
            if (loop != PR_U9E_ENTRY8_LOOP_AT_CALL)
                return PR_U9E_ENTRY8_COMPOSE_LOOP_REJECTED;
        }
    }

    {
        pr_u9e_entry8_tail_cpu tail_cpu;
        pr_u9e_entry8_tail_status tail;
        COPY_CPU(tail_cpu, *cpu);
        tail = pr_u9e_entry8_fullstate_tail_0e1f(
            ram, ram_size, &tail_cpu);
        COPY_CPU(*cpu, tail_cpu);
        if (tail != PR_U9E_ENTRY8_TAIL_RETURNED)
            return PR_U9E_ENTRY8_COMPOSE_TAIL_REJECTED;
    }
    return PR_U9E_ENTRY8_COMPOSE_RETURNED;
}

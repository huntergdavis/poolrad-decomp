/* Bounded composition of GAME.OVR U7F +0FD2..+100F. */
#include "poolrad/recovered/overlay_u7f_roll_composed_0fd2.h"
#include "poolrad/recovered/overlay_u7f_roll_prefix_0fd2.h"
#include "poolrad/recovered/overlay_u7f_roll_return_0fe3.h"

#define COPY_CPU(dst, src) do { \
    (dst).ax = (src).ax; (dst).bx = (src).bx; \
    (dst).cx = (src).cx; (dst).dx = (src).dx; \
    (dst).si = (src).si; (dst).di = (src).di; \
    (dst).bp = (src).bp; (dst).sp = (src).sp; \
    (dst).cs = (src).cs; (dst).ip = (src).ip; \
    (dst).ds = (src).ds; (dst).es = (src).es; \
    (dst).ss = (src).ss; (dst).flags = (src).flags; \
} while (0)

pr_u7f_0fd2_composed_status pr_u7f_roll_composed_0fd2(
    uint8_t *ram, size_t ram_size, pr_u7f_0fd2_composed_cpu *cpu,
    pr_u7f_0fde_roll roll, void *context)
{
    pr_u7f_0fd2_cpu prefix;
    pr_u7f_0fe3_cpu tail;
    pr_u7f_0fde_status called;
    if (ram == NULL || cpu == NULL || roll == NULL ||
        ram_size != PR_U7F_0FDE_RAM_SIZE || cpu->ip != 0x0FD2u)
        return PR_U7F_0FD2_COMPOSED_ARGUMENT;

    COPY_CPU(prefix, *cpu);
    if (pr_u7f_roll_prefix_0fd2(ram, ram_size, &prefix) != PR_U7F_0FD2_AT_CALL)
        return PR_U7F_0FD2_COMPOSED_ARGUMENT;
    COPY_CPU(*cpu, prefix);

    called = pr_u7f_roll_call_0fde(ram, ram_size, cpu, roll, context);
    if (called == PR_U7F_0FDE_CALL_ABORT)
        return PR_U7F_0FD2_COMPOSED_CALL_ABORT;
    if (called == PR_U7F_0FDE_TRANSFER)
        return PR_U7F_0FD2_COMPOSED_TRANSFER;
    if (called != PR_U7F_0FDE_RETURNED)
        return PR_U7F_0FD2_COMPOSED_ARGUMENT;

    COPY_CPU(tail, *cpu);
    if (pr_u7f_roll_return_0fe3(ram, ram_size, &tail) != PR_U7F_0FE3_RETURNED)
        return PR_U7F_0FD2_COMPOSED_ARGUMENT;
    COPY_CPU(*cpu, tail);
    return PR_U7F_0FD2_COMPOSED_RETURNED;
}

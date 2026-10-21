/* Composition of the 08AE driver entries 00DC and 017B over the merged member models (docs/re/RES_0899_QUIT_0000_CPU.md). */
#include "poolrad/recovered/resident_08ae_driver_cpu.h"
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_08ae_byte_181a_cpu.h"
#include "poolrad/recovered/resident_08ae_delay_006c_cpu.h"
#include "poolrad/recovered/resident_08ae_dispatch_00dc_cpu.h"
#include "poolrad/recovered/resident_08ae_entry_017b_cpu.h"
#include "poolrad/recovered/resident_08ae_table_1742_cpu.h"
#include "poolrad/recovered/resident_08ae_table_17ae_cpu.h"
#include "poolrad/recovered/resident_08ae_time_0000_cpu.h"
#include "poolrad/recovered/resident_08ae_vector_137c_cpu.h"
#include "poolrad/recovered/resident_08ae_vector_13b1_cpu.h"
#include <string.h>

/* Every member CPU type has the same fourteen uint16_t fields in the same order. */
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)

typedef struct {
    const pr_r08ae_driver_env *env;
    size_t ram_size;
    int time_status;
} glue;

static int dos_137c(void *context, uint8_t *ram, pr_r08ae_137c_cpu *cpu, uint16_t *flags_out)
{
    const glue *g = context; pr_r08ae_driver_cpu c; int r;
    COPY(&c, cpu); r = g->env->dos_vector(g->env->context, ram, &c, flags_out); COPY(cpu, &c);
    return r;
}
static int dos_13b1(void *context, uint8_t *ram, pr_r08ae_13b1_cpu *cpu, uint16_t *flags_out)
{
    const glue *g = context; pr_r08ae_driver_cpu c; int r;
    COPY(&c, cpu); r = g->env->dos_vector(g->env->context, ram, &c, flags_out); COPY(cpu, &c);
    return r;
}
static int out_any(void *context, uint16_t port, uint8_t value)
{
    const glue *g = context;
    return g->env->port_out(g->env->context, port, value);
}
static int in_any(void *context, uint16_t port, uint8_t *value)
{
    const glue *g = context;
    return g->env->port_in(g->env->context, port, value);
}
/* 006C's time source is RES_08AE_0000, reached by PUSH CS; CALL 0. The hook receives the CPU with that frame pushed
 * and IP = 0000; it runs the merged RES_08AE_0000 model (which executes through its own RETF), then puts SP, CS and IP
 * back to the hook's entry values: the frame words are still in RAM and 006C performs the RETF from them itself. */
static int time_0000(void *context, uint8_t *ram, pr_r08ae_006c_cpu *cpu)
{
    glue *g = context;
    pr_bus bus = {ram, NULL, NULL, NULL, NULL};
    pr_r08ae_time_cpu t; pr_r08ae_time_cpu_diagnostic d;
    uint16_t sp = cpu->sp, cs = cpu->cs, ip = cpu->ip;
    COPY(&t, cpu);
    g->time_status = (int)pr_r08ae_time_0000_cpu(&bus, g->ram_size, &t, g->env->gettime, g->env->context, &d);
    if (g->time_status != PR_R08AE_TIME_CPU_OK)
        return 1;
    COPY(cpu, &t);
    cpu->sp = sp; cpu->cs = cs; cpu->ip = ip;
    return 0;
}

/* Runs the member at cpu->ip (the callee entry the stop left), returning its status; OK/RETURN-like statuses are 0. */
static int member(uint8_t *ram, glue *g, pr_r08ae_driver_cpu *c, uint16_t at)
{
    switch (at) {
    case 0x137Cu: {
        pr_r08ae_137c_env e = {g, dos_137c, out_any}; pr_r08ae_137c_cpu m; int s;
        COPY(&m, c); s = (int)pr_r08ae_vector_137c_cpu(ram, g->ram_size, &m, &e); COPY(c, &m); return s;
    }
    case 0x13B1u: {
        pr_r08ae_13b1_env e = {g, dos_13b1, out_any, in_any}; pr_r08ae_13b1_cpu m; int s;
        COPY(&m, c); s = (int)pr_r08ae_vector_13b1_cpu(ram, g->ram_size, &m, &e); COPY(c, &m); return s;
    }
    case 0x1742u: {
        pr_r08ae_1742_cpu m; int s;
        COPY(&m, c); s = (int)pr_r08ae_table_1742_cpu(ram, g->ram_size, &m); COPY(c, &m); return s;
    }
    case 0x17AEu: {
        pr_r08ae_17ae_cpu m; int s;
        COPY(&m, c); s = (int)pr_r08ae_table_17ae_cpu(ram, g->ram_size, &m); COPY(c, &m); return s;
    }
    case 0x181Au: {
        pr_r08ae_181a_cpu m; int s;
        COPY(&m, c); s = (int)pr_r08ae_byte_181a_cpu(ram, g->ram_size, &m); COPY(c, &m); return s;
    }
    case 0x006Cu: {
        pr_r08ae_006c_cpu m; int s;
        COPY(&m, c); s = (int)pr_r08ae_delay_006c_cpu(ram, g->ram_size, &m, time_0000, g, g->env->delay_steps); COPY(c, &m); return s;
    }
    default:
        return -1;
    }
}

pr_r08ae_driver_status pr_r08ae_driver_call(uint8_t *ram, size_t ram_size, pr_r08ae_driver_cpu *c,
                                            const pr_r08ae_driver_env *env, pr_r08ae_driver_diagnostic *d)
{
    glue g;
    uint16_t entry;
    if (ram == NULL || c == NULL || env == NULL || d == NULL || env->dos_vector == NULL || env->port_out == NULL ||
        env->port_in == NULL || env->gettime == NULL || ram_size != PR_R08AE_DRIVER_RAM_SIZE ||
        (c->ip != 0x00DCu && c->ip != 0x017Bu))
        return PR_R08AE_DRIVER_ARGUMENT;
    entry = c->ip;
    g.env = env; g.ram_size = ram_size; g.time_status = -1;
    d->member = entry; d->status = 0; d->time_status = -1; d->calls = 0;
    for (;;) {
        int s, callee;
        if (entry == 0x00DCu) {
            pr_r08ae_00dc_cpu m;
            COPY(&m, c); s = (int)pr_r08ae_dispatch_00dc_cpu(ram, ram_size, &m); COPY(c, &m);
            if (s == PR_R08AE_00DC_RETURN) return PR_R08AE_DRIVER_RETURN;
            if (s == PR_R08AE_00DC_ARGUMENT) { d->member = entry; d->status = s; return PR_R08AE_DRIVER_MEMBER; }
        } else {
            pr_r08ae_017b_cpu m;
            COPY(&m, c); s = (int)pr_r08ae_entry_017b_cpu(ram, ram_size, &m); COPY(c, &m);
            if (s == PR_R08AE_017B_RETURN) return PR_R08AE_DRIVER_RETURN;
            if (s == PR_R08AE_017B_ARGUMENT) { d->member = entry; d->status = s; return PR_R08AE_DRIVER_MEMBER; }
        }
        /* A CALL_* stop: the callee entry is the IP the stop left. */
        callee = c->ip;
        ++d->calls;
        s = member(ram, &g, c, (uint16_t)callee);
        d->time_status = g.time_status;
        if (s != 0) {
            d->member = (uint16_t)callee; d->status = s;
            return PR_R08AE_DRIVER_MEMBER;
        }
        /* The member returned to the return point; the entry model resumes there (any other IP is its ARGUMENT). */
    }
}

/* The t30 RES_0802 cluster composed end to end: 0716 with its seven seams and QUIT driven by the real models
 * (docs/re/RES_0802_0716_COMPOSED.md). */
#include "poolrad/recovered/resident_0802_0716_composed_cpu.h"
#include "poolrad/recovered/resident_0802_0000_cpu.h"
#include "poolrad/recovered/resident_0802_0020_cpu.h"
#include "poolrad/recovered/resident_0802_050f_cpu.h"
#include "poolrad/recovered/resident_0802_05cd_cpu.h"
#include "poolrad/recovered/resident_0802_06e7_cpu.h"
#include "poolrad/recovered/resident_0899_quit_0000_cpu.h"

#define COPY_CPU(d, s) do { (d)->ax = (s)->ax; (d)->bx = (s)->bx; (d)->cx = (s)->cx; (d)->dx = (s)->dx; \
    (d)->si = (s)->si; (d)->di = (s)->di; (d)->bp = (s)->bp; (d)->sp = (s)->sp; (d)->cs = (s)->cs; (d)->ip = (s)->ip; \
    (d)->ds = (s)->ds; (d)->es = (s)->es; (d)->ss = (s)->ss; (d)->flags = (s)->flags; } while (0)
#define CONTINUE (-1)

typedef pr_r0802_0716c_cpu cpu_t;

/* A leaf that returned must be back in 0716's segment at `back`. */
static int back_at(const cpu_t *c, uint16_t text, uint16_t back) { return c->cs == text && c->ip == back; }

static int stop(pr_r0802_0716c_diagnostic *d, uint16_t seam, int leaf_status, pr_r0802_0716c_status status)
{
    d->seam = seam; d->leaf_status = leaf_status;
    return (int)status;
}

/* The prompt 0193, from 0716 (back at 084Dh) or from 025D inside 05CD (back at 02ADh). */
static int prompt(uint8_t *ram, size_t size, cpu_t *c, const pr_r0802_0716c_env *env, uint16_t text, uint16_t back,
                  uint16_t seam, pr_r0802_0716c_diagnostic *d)
{
    pr_r0802_0193_cpu m; pr_r0802_0193_diagnostic pd; int s;
    COPY_CPU(&m, c);
    s = pr_r0802_0193(ram, size, &m, &env->prompt, &pd);
    COPY_CPU(c, &m);
    ++d->prompts;
    if (s == PR_R0802_0193_KEY) {
        if (pd.key_status == PR_R0899_KEY_EXIT) return stop(d, seam, pd.key_status, PR_R0802_0716C_EXIT);
        if (pd.key_status == PR_R0899_KEY_BREAK) return stop(d, seam, pd.key_status, PR_R0802_0716C_BREAK);
        if (pd.key_status == PR_R0899_KEY_LIMIT) return stop(d, seam, pd.key_status, PR_R0802_0716C_LIMIT);
        if (pd.key_status == PR_R0899_KEY_CRT) return stop(d, seam, pd.key_status, PR_R0802_0716C_SERVICE);  /* keyboard refused */
        return stop(d, seam, pd.key_status, PR_R0802_0716C_CALLEE);
    }
    if (s != PR_R0802_0193_RETURNED || !back_at(c, text, back)) return stop(d, seam, s, PR_R0802_0716C_CALLEE);
    ++d->seams_composed;
    return CONTINUE;
}

static int open_05cd(uint8_t *ram, size_t size, cpu_t *c, const pr_r0802_0716c_env *env, uint16_t text,
                     pr_r0802_0716c_diagnostic *d)
{
    pr_r0802_05cd_env e;
    pr_r0802_05cd_diagnostic od;
    e.file = env->file; e.read = env->loader.read; e.read_context = env->loader.read_context;
    e.seek = env->loader.seek; e.seek_context = env->loader.seek_context; e.retries = env->retries;
    unsigned disk_prompts = 0;
    for (;;) {
        pr_r0802_05cd_cpu m; int s;
        COPY_CPU(&m, c);
        s = pr_r0802_05cd(ram, size, &m, &e, &od);
        COPY_CPU(c, &m);
        if (s == PR_R0802_05CD_PROMPT) {                              /* 025D's insert-disk prompt: run 0193, resume at 02ADh */
            int p;
            if (disk_prompts++ >= env->max_disk_prompts) { d->nested = 0x02AAu; return stop(d, 0x0775u, -1, PR_R0802_0716C_LIMIT); }
            p = prompt(ram, size, c, env, text, 0x02ADu, 0x0775u, d);
            if (p != CONTINUE) { d->nested = 0x02AAu; return p; }
            continue;
        }
        switch (s) {
        case PR_R0802_05CD_RETURNED:
            if (!back_at(c, text, 0x0778u)) return stop(d, 0x0775u, s, PR_R0802_0716C_CALLEE);
            ++d->seams_composed;
            return CONTINUE;
        case PR_R0802_05CD_RUNERROR: return stop(d, 0x0775u, s, PR_R0802_0716C_RUNERROR);
        case PR_R0802_05CD_DIVIDE: return stop(d, 0x0775u, s, PR_R0802_0716C_DIVIDE);
        case PR_R0802_05CD_SERVICE: return stop(d, 0x0775u, s, PR_R0802_0716C_SERVICE);
        case PR_R0802_05CD_LIMIT: d->nested = 0x02B7u; return stop(d, 0x0775u, s, PR_R0802_0716C_LIMIT);
        default: return stop(d, 0x0775u, s, PR_R0802_0716C_CALLEE);
        }
    }
}

pr_r0802_0716c_status pr_r0802_0716_composed(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_0716c_env *env,
                                             pr_r0802_0716c_diagnostic *diagnostic)
{
    pr_r0802_0716c_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint16_t text;
    unsigned seams = 0;
    if (ram == NULL || c == NULL || env == NULL || ram_size != PR_R0802_0716C_RAM_SIZE || c->ip != 0x0716u)
        return PR_R0802_0716C_ARGUMENT;
    d->seam = 0; d->nested = 0; d->leaf_status = -1; d->seams_composed = 0; d->prompts = 0; d->decode_arrivals = 0;
    text = c->cs;
    for (;;) {
        pr_r0802_0716_diagnostic ld;
        int s = pr_r0802_0716(ram, ram_size, c, &env->loader, &ld), r = CONTINUE;
        switch (s) {
        case PR_R0802_0716_RETURNED: return PR_R0802_0716C_RETURNED;
        case PR_R0802_0716_ARGUMENT:
            return d->seams_composed ? (pr_r0802_0716c_status)stop(d, 0, s, PR_R0802_0716C_CALLEE) : PR_R0802_0716C_ARGUMENT;
        case PR_R0802_0716_RUNERROR: return (pr_r0802_0716c_status)stop(d, 0, ld.callee_status, PR_R0802_0716C_RUNERROR);
        case PR_R0802_0716_DIVIDE: return (pr_r0802_0716c_status)stop(d, 0, ld.callee_status, PR_R0802_0716C_DIVIDE);
        case PR_R0802_0716_SERVICE: return (pr_r0802_0716c_status)stop(d, 0, ld.callee_status, PR_R0802_0716C_SERVICE);
        case PR_R0802_0716_CALLEE: return (pr_r0802_0716c_status)stop(d, 0, ld.callee_status, PR_R0802_0716C_CALLEE);
        default:
            if (seams++ >= env->max_seams)                            /* the seam loop past its budget: the leaf not begun */
                return (pr_r0802_0716c_status)stop(d, ld.site, -1, PR_R0802_0716C_LIMIT);
            break;
        }
        switch (s) {
        case PR_R0802_0716_OPEN: r = open_05cd(ram, ram_size, c, env, text, d); break;
        case PR_R0802_0716_PROMPT: r = prompt(ram, ram_size, c, env, text, 0x084Du, 0x084Au, d); break;
        case PR_R0802_0716_QUIT: {                                    /* the credited 0899:0000: Halt, never a return */
            pr_r0899_cpu m; pr_r0899_quit_diagnostic qd; int q;
            COPY_CPU(&m, c);
            q = pr_r0899_quit_0000_cpu(ram, ram_size, &m, &env->prompt.key.quit, &qd);
            COPY_CPU(c, &m);
            return (pr_r0802_0716c_status)stop(d, 0x084Du, q, q == PR_R0899_QUIT_EXIT ? PR_R0802_0716C_EXIT : PR_R0802_0716C_CALLEE);
        }
        case PR_R0802_0716_CLOSE: {
            uint16_t site = ld.site, back = (uint16_t)(ld.site + 3u);
            pr_r0802_06e7_cpu m; pr_r0802_06e7_diagnostic cd; int q;
            COPY_CPU(&m, c);
            q = pr_r0802_06e7(ram, ram_size, &m, env->file.close, env->file.close_context, &cd);
            COPY_CPU(c, &m);
            if (q == PR_R0802_06E7_RUNERROR) r = stop(d, site, q, PR_R0802_0716C_RUNERROR);
            else if (q == PR_R0802_06E7_SERVICE) r = stop(d, site, q, PR_R0802_0716C_SERVICE);
            else if (q != PR_R0802_06E7_RETURNED || !back_at(c, text, back)) r = stop(d, site, q, PR_R0802_0716C_CALLEE);
            else ++d->seams_composed;
            break;
        }
        case PR_R0802_0716_ALLOC: {
            pr_r0802_0020_cpu m; pr_r0802_0020_diagnostic ad; int q;
            COPY_CPU(&m, c);
            q = pr_r0802_0020(ram, ram_size, &m, env->heap_steps, &ad);
            COPY_CPU(c, &m);
            if (q == PR_R0802_0020_RUNERROR) r = stop(d, 0x08BCu, q, PR_R0802_0716C_RUNERROR);
            else if (q == PR_R0802_0020_LIMIT) r = stop(d, 0x08BCu, q, PR_R0802_0716C_LIMIT);
            else if (q != PR_R0802_0020_RETURNED || !back_at(c, text, 0x08BFu)) r = stop(d, 0x08BCu, q, PR_R0802_0716C_CALLEE);
            else ++d->seams_composed;
            break;
        }
        case PR_R0802_0716_DECODE: {
            pr_r0802_050f_cpu m; pr_r0802_050f_diagnostic xd; int q;
            COPY_CPU(&m, c);
            q = pr_r0802_050f(ram, ram_size, &m, env->decode_arrivals, &xd);
            COPY_CPU(c, &m);
            d->decode_arrivals = xd.arrivals;
            if (q == PR_R0802_050F_STALLED) r = stop(d, 0x0935u, q, PR_R0802_0716C_STALLED);
            else if (q == PR_R0802_050F_BUDGET) r = stop(d, 0x0935u, q, PR_R0802_0716C_LIMIT);
            else if (q != PR_R0802_050F_RETURNED || !back_at(c, text, 0x0938u)) r = stop(d, 0x0935u, q, PR_R0802_0716C_CALLEE);
            else ++d->seams_composed;
            break;
        }
        case PR_R0802_0716_FREE: {
            pr_r0802_0000_cpu m; pr_r0802_0000_diagnostic fd; int q;
            COPY_CPU(&m, c);
            q = pr_r0802_0000(ram, ram_size, &m, env->heap_steps, &fd);
            COPY_CPU(c, &m);
            if (q == PR_R0802_0000_RUNERROR) r = stop(d, 0x0941u, q, PR_R0802_0716C_RUNERROR);
            else if (q == PR_R0802_0000_LIMIT) r = stop(d, 0x0941u, q, PR_R0802_0716C_LIMIT);
            else if (q != PR_R0802_0000_RETURNED || !back_at(c, text, 0x0944u)) r = stop(d, 0x0941u, q, PR_R0802_0716C_CALLEE);
            else ++d->seams_composed;
            break;
        }
        default:
            return (pr_r0802_0716c_status)stop(d, 0, s, PR_R0802_0716C_CALLEE);
        }
        if (r != CONTINUE) return (pr_r0802_0716c_status)r;
    }
}

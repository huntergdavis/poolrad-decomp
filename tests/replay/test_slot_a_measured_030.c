/* Measured-input replay from the run-030 anchors (no game bytes in this file).
 *
 * Replaces the synthetic inputs of the run003 composition test with values measured in the
 * LOGL traces of run t4492-slot4-logc-030:
 *   - DOS time: 924 GetTime polls in the delay window, three distinct readings
 *     08:19:44.76 x561, 08:19:44.82 x362, 08:19:44.87 x1 (the readings change when the BIOS
 *     tick advances; the loop exits at poll 924, DX:AX = 2,932,951 >= target 2,932,950);
 *   - IN 61h = 0x30 (a constraint from the trace: AL after the IN), not the declared 0x03.
 * Stage 1 starts at the native 00F4 image/CPU and stops at 0165; stage 2 starts at the native
 * 0165 image/CPU and stops at 13C6 and 2E60:0747. Every RAM image is only expected output.
 * Explicit stops: no IRQ0 hook (10D2:13EA) and no DOS/BIOS callback internals are modeled, so
 * the stage-1 RAM differs from native in exactly the pinned set below. */
#include "poolrad/recovered/resident_08ae_dispatch_00dc_cpu.h"
#include "poolrad/recovered/resident_08ae_table_1742_cpu.h"
#include "poolrad/recovered/resident_08ae_table_17ae_cpu.h"
#include "poolrad/recovered/resident_08ae_delay_006c_cpu.h"
#include "poolrad/recovered/resident_08ae_time_0000_cpu.h"
#include "poolrad/recovered/resident_08ae_vector_13b1_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 655360u
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static uint16_t get(const uint8_t *r, unsigned p) { return (uint16_t)(r[p] | ((unsigned)r[p + 1] << 8)); }
static void word(uint8_t *r, unsigned p, uint16_t v) { r[p] = (uint8_t)v; r[p + 1] = (uint8_t)(v >> 8); }
static int load(const char *p, uint8_t *r)
{
    FILE *f = fopen(p, "rb"); size_t n; int c;
    if (!f) return 0;
    n = fread(r, 1, SIZE, f); c = fgetc(f); fclose(f);
    return n == SIZE && c == EOF;
}
static int load_cpu(const char *path, pr_r08ae_00dc_cpu *c)
{
    char line[256]; FILE *f = fopen(path, "r"); unsigned v[14]; int n;
    if (!f) return 0;
    n = fgets(line, sizeof line, f) != NULL; fclose(f);
    if (!n) return 0;
    if (sscanf(line, "AX=%x BX=%x CX=%x DX=%x SI=%x DI=%x BP=%x SP=%x CS=%x IP=%x DS=%x ES=%x SS=%x FLAGS=%x",
               &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13]) != 14)
        return 0;
    *c = (pr_r08ae_00dc_cpu){(uint16_t)v[0], (uint16_t)v[1], (uint16_t)v[2], (uint16_t)v[3], (uint16_t)v[4], (uint16_t)v[5],
                             (uint16_t)v[6], (uint16_t)v[7], (uint16_t)v[8], (uint16_t)v[9], (uint16_t)v[10], (uint16_t)v[11],
                             (uint16_t)v[12], (uint16_t)v[13]};
    return 1;
}
static int same_cpu(const pr_r08ae_00dc_cpu *a, const pr_r08ae_00dc_cpu *b)
{
    return a->ax == b->ax && a->bx == b->bx && a->cx == b->cx && a->dx == b->dx && a->si == b->si && a->di == b->di &&
           a->bp == b->bp && a->sp == b->sp && a->cs == b->cs && a->ip == b->ip && a->ds == b->ds && a->es == b->es &&
           a->ss == b->ss && a->flags == b->flags;
}
/* FNV-1a over (address, model byte, native byte) of every differing byte, ascending. */
static unsigned diff_set(const uint8_t *model, const uint8_t *native, unsigned *count, uint64_t *hash, unsigned *first)
{
    unsigned i, n = 0; uint64_t h = 1469598103934665603ull;
    for (i = 0; i < SIZE; i++)
        if (model[i] != native[i]) {
            unsigned long long parts[3] = {i, model[i], native[i]}; unsigned k;
            if (!n && first) *first = i;
            for (k = 0; k < 3; k++) { h ^= (uint64_t)parts[k]; h *= 1099511628211ull; }
            n++;
        }
    *count = n; *hash = h; return n;
}

/* ---- measured DOS time: the declared service supplies CX:DX only (as the run003 binding). ---- */
typedef struct { unsigned next; unsigned total; } poll_state;
static const struct { unsigned polls; uint16_t cx, dx; } READINGS[3] = {
    {561, 0x0813, 0x2C4C}, {362, 0x0813, 0x2C52}, {1, 0x0813, 0x2C57}};
static int measured_dos_time(void *ctx, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after)
{
    poll_state *s = (poll_state *)ctx; unsigned left, k;
    if (!s || !before || !after || before->ip != 0x00a7 || (before->ax & 0xff00u) != 0x2c00u || s->next >= s->total) return 1;
    left = s->next; for (k = 0; k < 3 && left >= READINGS[k].polls; k++) left -= READINGS[k].polls;
    if (k >= 3) return 1;
    *after = *before; after->ip = 0x00a9; after->cx = READINGS[k].cx; after->dx = READINGS[k].dx; s->next++;
    return 0;
}
static int measured_time(void *ctx, uint8_t *ram, pr_r08ae_006c_cpu *cpu)
{
    poll_state *s = (poll_state *)ctx; pr_r08ae_time_cpu t; pr_r08ae_time_cpu_diagnostic diag;
    _Static_assert(sizeof(pr_r08ae_time_cpu) == sizeof(pr_r08ae_006c_cpu), "time CPU layout");
    uint8_t *copy; pr_bus bus; unsigned original_next; uint16_t sp, cs, ret;
    if (!s || !ram || !cpu || cpu->ip != 0) return 1;
    copy = malloc(PR_BUS_RAM_SIZE); if (!copy) return 1; memcpy(copy, ram, PR_BUS_RAM_SIZE);
    bus = (pr_bus){copy, NULL, NULL, NULL, NULL}; sp = cpu->sp; cs = cpu->cs; ret = get(copy, ((unsigned)cpu->ss << 4) + sp);
    original_next = s->next; memcpy(&t, cpu, sizeof t);
    if (pr_r08ae_time_0000_cpu(&bus, PR_BUS_RAM_SIZE, &t, measured_dos_time, s, &diag) != PR_R08AE_TIME_CPU_OK ||
        t.sp != (uint16_t)(sp + 4) || t.cs != cs || t.ip != ret) { s->next = original_next; free(copy); return 1; }
    memcpy(ram, copy, PR_BUS_RAM_SIZE); free(copy); t.sp = sp; t.ip = 0; memcpy(cpu, &t, sizeof t); return 0;
}

/* ---- stage-2 environment: DOS AH=25 vector restore, IN 61h, ordered OUTs ---- */
typedef struct { unsigned calls, outs, ins; uint8_t input61; uint8_t out_log_port[8]; uint8_t out_log_value[8]; } restore_binding;
/* Declared DOS binding for the observed AH=25 call: vector write, PSP stack pointer and the nine saved
 * registers below the interrupt frame (as the run003 binding). The trace shows the AH=25 callback
 * returning straight to the stub's iret, with no delay loop and no `push cx`; the callback internals
 * are not instruction-traced. */
static int measured_restore(void *ctx, uint8_t *ram, pr_r08ae_13b1_cpu *cpu, uint16_t *flags)
{
    restore_binding *b = (restore_binding *)ctx; unsigned stack, i; uint16_t reg[9];
    if (!b || !ram || !cpu || !flags || b->calls || cpu->ax != 0x2508 || cpu->dx != 0xfea5 || cpu->ds != 0xf000 || cpu->ip != 0x13c6 ||
        cpu->flags != 0x7046 || get(ram, 0x20) != 0x13ea || get(ram, 0x22) != 0x10d2 || ram[0x8140] != 0xcd || ram[0x8141] != 0x20) return 1;
    stack = ((unsigned)cpu->ss << 4) + cpu->sp;
    if (get(ram, stack) != 0x13c8 || get(ram, stack + 2) != cpu->cs) return 1;
    reg[0] = cpu->ax; reg[1] = cpu->bx; reg[2] = cpu->cx; reg[3] = cpu->dx; reg[4] = cpu->si; reg[5] = cpu->di; reg[6] = cpu->bp; reg[7] = cpu->ds; reg[8] = cpu->es;
    for (i = 0; i < 9; i++) word(ram, stack - 18 + 2 * i, reg[i]);
    word(ram, 0x8140 + 0x2e, (uint16_t)(cpu->sp - 18)); word(ram, 0x8140 + 0x30, cpu->ss);
    word(ram, 0x20, cpu->dx); word(ram, 0x22, cpu->ds);
    *flags = 0x7246; b->calls++; return 0;
}
static int measured_out(void *ctx, uint16_t port, uint8_t value)
{
    static const uint16_t ports[] = {0x40, 0x40, 0x61, 0xc0, 0xc0, 0xc0, 0xc0};
    static const uint8_t values[] = {0xff, 0xff, 0x00, 0x9f, 0xbf, 0xdf, 0xff};
    restore_binding *b = (restore_binding *)ctx;
    if (!b || b->calls != 1 || b->outs >= 7 || port != ports[b->outs] || (b->outs >= 2 && b->ins != 1)) return 1;
    if (b->outs == 2 ? value != (uint8_t)(b->input61 & 0xfc) : value != values[b->outs]) return 1;
    b->out_log_port[b->outs] = (uint8_t)port; b->out_log_value[b->outs] = value; b->outs++; return 0;
}
static int measured_in(void *ctx, uint16_t port, uint8_t *value)
{
    restore_binding *b = (restore_binding *)ctx;
    if (!b || !value || port != 0x61 || b->outs != 2 || b->ins) return 1;
    *value = b->input61; b->ins++; return 0;
}

/* Stage 2: native 0165 -> 13C6 -> 0747 with the given IN 61h value. */
static int stage2(const uint8_t *img0165, const pr_r08ae_00dc_cpu *cpu0165, const uint8_t *img13c6, const pr_r08ae_00dc_cpu *cpu13c6,
                  const uint8_t *img0747, const pr_r08ae_00dc_cpu *cpu0747, uint8_t input61, int expect_native)
{
    uint8_t *r = calloc(1, PR_R08AE_00DC_RAM_SIZE); uint8_t *full = calloc(1, PR_R08AE_00DC_RAM_SIZE);
    pr_r08ae_00dc_cpu d = *cpu0165, next; pr_r08ae_17ae_cpu table; pr_r08ae_1742_cpu again;
    pr_r08ae_13b1_cpu restore; restore_binding binding = {0}; pr_r08ae_13b1_env env;
    unsigned count, first = 0; uint64_t hash; static unsigned pinned_13c6_diffs; (void)pinned_13c6_diffs;
    CHECK(r && full); memcpy(r, img0165, SIZE);
    CHECK(pr_r08ae_dispatch_00dc_cpu(r, PR_R08AE_00DC_RAM_SIZE, &d) == PR_R08AE_00DC_CALL_17AE);
    memcpy(&table, &d, sizeof table); CHECK(pr_r08ae_table_17ae_cpu(r, PR_R08AE_17AE_RAM_SIZE, &table) == PR_R08AE_17AE_OK);
    memcpy(&d, &table, sizeof d); CHECK(pr_r08ae_dispatch_00dc_cpu(r, PR_R08AE_00DC_RAM_SIZE, &d) == PR_R08AE_00DC_CALL_1742);
    memcpy(&again, &d, sizeof again); CHECK(pr_r08ae_table_1742_cpu(r, PR_R08AE_1742_RAM_SIZE, &again) == PR_R08AE_1742_OK);
    memcpy(&d, &again, sizeof d); CHECK(pr_r08ae_dispatch_00dc_cpu(r, PR_R08AE_00DC_RAM_SIZE, &d) == PR_R08AE_00DC_CALL_13B1);
    next = d; memcpy(&restore, &next, sizeof restore); memcpy(full, r, PR_R08AE_00DC_RAM_SIZE);
    /* 13C6 stop: the model's hardware frame is pushed, the native BP is before the INT, so SP differs by the 6-byte frame. */
    CHECK(pr_r08ae_vector_13b1_cpu(r, PR_R08AE_13B1_RAM_SIZE, &restore, NULL) == PR_R08AE_13B1_ENV_DOS);
    CHECK(restore.ip == 0x13c6 && restore.sp == (uint16_t)(cpu13c6->sp - 6) && restore.flags == 0x7046);
    CHECK(restore.ax == cpu13c6->ax && restore.bx == cpu13c6->bx && restore.cx == cpu13c6->cx && restore.dx == cpu13c6->dx &&
          restore.si == cpu13c6->si && restore.di == cpu13c6->di && restore.bp == cpu13c6->bp && restore.ds == cpu13c6->ds &&
          restore.es == cpu13c6->es && restore.ss == cpu13c6->ss && restore.cs == cpu13c6->cs);
    if (input61 == 0x30) {   /* RAM at the INT: only the 6 frame bytes differ from the native pre-INT image */
        diff_set(r, img13c6, &count, &hash, &first);
        printf("stage2 13C6: model RAM (frame pushed) vs native pre-INT image: %u byte(s), first %05X\n", count, first);
        CHECK(count == 6u);
    }
    /* Continue the same state through the declared environment. */
    memcpy(r, full, PR_R08AE_00DC_RAM_SIZE); memcpy(&restore, &next, sizeof restore);
    binding.input61 = input61; env = (pr_r08ae_13b1_env){&binding, measured_restore, measured_out, measured_in};
    CHECK(pr_r08ae_vector_13b1_cpu(r, PR_R08AE_13B1_RAM_SIZE, &restore, &env) == PR_R08AE_13B1_OK);
    CHECK(binding.calls == 1 && binding.outs == 7 && binding.ins == 1 && restore.ip == 0x0177);
    memcpy(&next, &restore, sizeof next); CHECK(pr_r08ae_dispatch_00dc_cpu(r, PR_R08AE_00DC_RAM_SIZE, &next) == PR_R08AE_00DC_RETURN);
    CHECK(next.cs == 0x2e60 && next.ip == 0x0747 && next.sp == cpu0747->sp);
    diff_set(r, img0747, &count, &hash, &first);
    printf("stage2 0747 (IN 61h=%02X): flags %04X native %04X; RAM diffs %u byte(s)\n", input61, next.flags, cpu0747->flags, count);
    printf("stage2 OUT 61h value %02X (ordered: 40h<-FF, 40h<-FF, 61h<-%02X, C0h<-9F,BF,DF,FF)\n", binding.out_log_value[2], binding.out_log_value[2]);
    if (expect_native) {
        CHECK(same_cpu(&next, cpu0747) && count == 0u && binding.out_log_value[2] == 0x30);
    } else {          /* the old declared value must NOT reproduce the native stop: flags differ, RAM does not */
        CHECK(!same_cpu(&next, cpu0747) && next.flags == 0x7246 && cpu0747->flags == 0x7206 && count == 0u && binding.out_log_value[2] == 0x00);
    }
    free(r); free(full); return 0;
}

int main(int argc, char **argv)
{
    /* argv: RAM_00F4 CPU_00F4 RAM_0165 CPU_0165 RAM_13C6 CPU_13C6 RAM_0747 CPU_0747 [RESIDUE_OUT] */
    uint8_t *r, *n00f4 = malloc(SIZE), *n0165 = malloc(SIZE), *n13c6 = malloc(SIZE), *n0747 = malloc(SIZE);
    pr_r08ae_00dc_cpu c00f4, c0165, c13c6, c0747, d; pr_r08ae_1742_cpu t; pr_r08ae_006c_cpu delay; poll_state state;
    unsigned count, first = 0, i, classes[5] = {0}; uint64_t hash;
    CHECK((argc == 9 || argc == 10) && n00f4 && n0165 && n13c6 && n0747);
    CHECK(load(argv[1], n00f4) && load_cpu(argv[2], &c00f4) && load(argv[3], n0165) && load_cpu(argv[4], &c0165) &&
          load(argv[5], n13c6) && load_cpu(argv[6], &c13c6) && load(argv[7], n0747) && load_cpu(argv[8], &c0747));
    CHECK(c00f4.cs == 0x10d2 && c00f4.ip == 0x00f4 && c0165.ip == 0x0165 && c13c6.ip == 0x13c6 && c0747.cs == 0x2e60 && c0747.ip == 0x0747);

    /* ---- stage 1: 00F4 -> 0165 with measured time ---- */
    r = calloc(1, PR_R08AE_00DC_RAM_SIZE); CHECK(r); memcpy(r, n00f4, SIZE); d = c00f4;
    CHECK(pr_r08ae_dispatch_00dc_cpu(r, PR_R08AE_00DC_RAM_SIZE, &d) == PR_R08AE_00DC_CALL_1742);
    memcpy(&t, &d, sizeof t); CHECK(pr_r08ae_table_1742_cpu(r, PR_R08AE_1742_RAM_SIZE, &t) == PR_R08AE_1742_OK);
    memcpy(&d, &t, sizeof d); CHECK(pr_r08ae_dispatch_00dc_cpu(r, PR_R08AE_00DC_RAM_SIZE, &d) == PR_R08AE_00DC_CALL_006C);
    CHECK(d.ax == 10 && d.sp == (uint16_t)(c00f4.sp - 6));
    memcpy(&delay, &d, sizeof delay); state = (poll_state){0, 924};
    CHECK(pr_r08ae_delay_006c_cpu(r, PR_R08AE_006C_RAM_SIZE, &delay, measured_time, &state, 200000) == PR_R08AE_006C_OK);
    CHECK(state.next == 924);                                   /* exit exactly at poll 924 */
    memcpy(&d, &delay, sizeof d);
    printf("stage1 CPU: AX=%04X BX=%04X CX=%04X DX=%04X SI=%04X DI=%04X BP=%04X SP=%04X IP=%04X DS=%04X ES=%04X SS=%04X FL=%04X\n",
           d.ax, d.bx, d.cx, d.dx, d.si, d.di, d.bp, d.sp, d.ip, d.ds, d.es, d.ss, d.flags);
    /* The native stop: DX:AX = last poll value, CX = delay argument, SI = hour (08), BX = DI = 0. */
    CHECK(d.ax == c0165.ax && d.dx == c0165.dx && d.cx == c0165.cx && d.bx == c0165.bx && d.si == c0165.si && d.di == c0165.di);
    CHECK(d.bp == c0165.bp && d.sp == c0165.sp && d.cs == c0165.cs && d.ip == c0165.ip && d.ds == c0165.ds && d.es == c0165.es && d.ss == c0165.ss);
    printf("stage1 flags: model %04X native %04X\n", d.flags, c0165.flags);
    /* 0x2C: 2,932,951 = 0x002CC0D7 */
    CHECK(d.dx == 0x002c && d.ax == 0xc0d7 && d.si == 8);
    diff_set(r, n0165, &count, &hash, &first);
    for (i = 0; i < SIZE; i++) if (r[i] != n0165[i]) {
        unsigned cls = i == 0x46c || i == 0x46d ? 0 : (i >= 0x816e && i < 0x8172) ? 1 : (i >= 0x10f00 && i < 0x11100) ? 2 : (i >= 0x1d160 && i < 0x2d160) ? 3 : 4;
        classes[cls]++;
    }
    printf("stage1 RAM: model vs native 0165: %u differing byte(s) [tick %u, psp-ptr %u, resident-data %u, stack %u, other %u] hash %016llx\n",
           count, classes[0], classes[1], classes[2], classes[3], classes[4], (unsigned long long)hash);
    /* Pinned exact residue: what this replay (no IRQ0 hook, no DOS/BIOS callback internals) leaves unreproduced. */
    CHECK(count == 83u && classes[0] == 1u && classes[1] == 1u && classes[2] == 38u && classes[3] == 34u && classes[4] == 9u);
    CHECK(hash == 0x0fb46f0fef1bcf63ull);
    if (argc == 10) {            /* optional: write the residue as "address model native" lines for the audit */
        FILE *out = fopen(argv[9], "w"); CHECK(out);
        for (i = 0; i < SIZE; i++) if (r[i] != n0165[i]) fprintf(out, "%05X %02X %02X\n", i, r[i], n0165[i]);
        CHECK(fclose(out) == 0);
    }
    free(r);

    /* ---- stage 2: native 0165 -> 13C6 -> 0747 ---- */
    CHECK(stage2(n0165, &c0165, n13c6, &c13c6, n0747, &c0747, 0x03, 0) == 0);   /* old declared value: must differ */
    CHECK(stage2(n0165, &c0165, n13c6, &c13c6, n0747, &c0747, 0x30, 1) == 0);   /* measured value: native stop */
    puts("measured-input replay: time sequence, SI=hour and IN 61h=30h reproduce the native CPU stops; pinned RAM residue above");
    return 0;
}

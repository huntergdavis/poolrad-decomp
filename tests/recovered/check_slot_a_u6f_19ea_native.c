/* Optional source-only comparison of ignored run 045 caller branch. */
#include "poolrad/recovered/slot_a_u6f_19ea_caller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE 655360u

static int read_ram(const char *dir, const char *label, uint8_t *ram)
{
    char path[512];
    FILE *f;
    int n = snprintf(path, sizeof path, "%s/ram-%s.bin", dir, label);
    if (n < 0 || (size_t)n >= sizeof path) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    n = fread(ram, 1u, RAM_SIZE, f) == RAM_SIZE && fgetc(f) == EOF;
    fclose(f);
    return n;
}

static int read_cpu(FILE *f, const char *wanted,
                    pr_slot_a_directory_cpu *cpu)
{
    char label[16];
    unsigned v[16];
    uint16_t *fields[16] = {
        &cpu->cs, &cpu->ip, &cpu->ds, &cpu->es,
        &cpu->fs, &cpu->gs, &cpu->ss, &cpu->sp,
        &cpu->bp, &cpu->ax, &cpu->bx, &cpu->cx,
        &cpu->dx, &cpu->si, &cpu->di, &cpu->flags
    };
    size_t i;
    if (fscanf(f, "%15s", label) != 1 || strcmp(label, wanted) != 0)
        return 0;
    for (i = 0; i < 16u; ++i) {
        if (fscanf(f, "%x", &v[i]) != 1 || v[i] > 0xffffu) return 0;
        *fields[i] = (uint16_t)v[i];
    }
    return 1;
}

static int equal_cpu(const pr_slot_a_directory_cpu *a,
                     const pr_slot_a_directory_cpu *b)
{
    return a->ax == b->ax && a->bx == b->bx && a->cx == b->cx &&
           a->dx == b->dx && a->si == b->si && a->di == b->di &&
           a->bp == b->bp && a->sp == b->sp && a->cs == b->cs &&
           a->ds == b->ds && a->es == b->es && a->ss == b->ss &&
           a->fs == b->fs && a->gs == b->gs && a->ip == b->ip &&
           a->flags == b->flags;
}

int main(int argc, char **argv)
{
    static const char *labels[] = {
        "19ea", "19ee", "19f2", "19f4", "1937"
    };
    char path[512];
    FILE *ev;
    uint8_t *actual, *expected;
    pr_slot_a_directory_state state;
    pr_slot_a_directory_cpu observed;
    size_t i;
    int n;

    if (argc != 2) return 2;
    n = snprintf(path, sizeof path, "%s/ev-transcript-19ea.txt", argv[1]);
    if (n < 0 || (size_t)n >= sizeof path) return 2;
    ev = fopen(path, "r");
    actual = malloc(RAM_SIZE); expected = malloc(RAM_SIZE);
    if (!ev || !actual || !expected) return 2;
    memset(&state, 0, sizeof state);
    if (!read_cpu(ev, labels[0], &state.cpu) ||
        !read_ram(argv[1], labels[0], actual)) return 2;
    state.ram = actual; state.ram_size = RAM_SIZE;
    for (i = 1; i < sizeof labels / sizeof labels[0]; ++i) {
        if (pr_slot_a_u6f_19ea_step(&state) != PR_SLOT_A_U6F_19EA_OK ||
            !read_cpu(ev, labels[i], &observed) ||
            !read_ram(argv[1], labels[i], expected) ||
            !equal_cpu(&state.cpu, &observed) ||
            memcmp(actual, expected, RAM_SIZE) != 0) {
            fprintf(stderr, "native mismatch at %s\n", labels[i]);
            return 1;
        }
        printf("%s: 16 CPU fields and %u RAM bytes agree\n",
               labels[i], RAM_SIZE);
    }
    fclose(ev);
    free(actual); free(expected);
    return 0;
}

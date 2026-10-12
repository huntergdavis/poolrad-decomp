#include "poolrad/recovered/slot_a_directory_fullstate.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 0xa0000u
static const char *root;
static uint8_t expected_ram[N];
static uint8_t service_ram[N];
static const char *labels[] = {
    "pre-int", "post-int", "pre-store", "post-store",
    "02d5", "io-entry", "io-after-xchg", "02da"
};
static pr_slot_a_directory_cpu native_cpu[8];

static void read_ev_transcript(void)
{
    char path[1024], line[512];
    FILE *f;
    size_t i, j;
    assert(snprintf(path, sizeof path, "%s/ev-transcript.txt", root) > 0);
    f = fopen(path, "r"); assert(f);
    assert(fgets(line, sizeof line, f) && line[0] == '#');
    for (i = 0; i < 8u; ++i) {
        unsigned long v[16];
        char *token, *end;
        pr_slot_a_directory_cpu *c = &native_cpu[i];
        assert(fgets(line, sizeof line, f));
        token = strtok(line, " \t\r\n");
        assert(token && strcmp(token, labels[i]) == 0);
        for (j = 0; j < 16u; ++j) {
            token = strtok(NULL, " \t\r\n");
            assert(token);
            v[j] = strtoul(token, &end, 16);
            assert(*end == '\0' && v[j] <= 0xffffu);
        }
        assert(strtok(NULL, " \t\r\n") == NULL);
        c->cs = (uint16_t)v[0]; c->ip = (uint16_t)v[1];
        c->ds = (uint16_t)v[2]; c->es = (uint16_t)v[3];
        c->fs = (uint16_t)v[4]; c->gs = (uint16_t)v[5];
        c->ss = (uint16_t)v[6]; c->sp = (uint16_t)v[7];
        c->bp = (uint16_t)v[8]; c->ax = (uint16_t)v[9];
        c->bx = (uint16_t)v[10]; c->cx = (uint16_t)v[11];
        c->dx = (uint16_t)v[12]; c->si = (uint16_t)v[13];
        c->di = (uint16_t)v[14]; c->flags = (uint16_t)v[15];
    }
    assert(fgetc(f) == EOF);
    fclose(f);
}

static void read_ram(const char *name, uint8_t *out)
{
    char path[1024];
    FILE *f;
    assert(snprintf(path, sizeof path, "%s/ram-%s.bin", root, name) > 0);
    f = fopen(path, "rb"); assert(f);
    assert(fread(out, 1u, N, f) == N);
    assert(fgetc(f) == EOF);
    fclose(f);
}

static pr_slot_a_directory_status captured_service(
    void *unused, const pr_slot_a_directory_cpu *before,
    const uint8_t *ram, size_t size, pr_slot_a_directory_cpu *after,
    pr_slot_a_directory_write *writes, size_t capacity, size_t *count)
{
    size_t i, n = 0;
    (void)unused;
    assert(size == N);
    assert(memcmp(before, &native_cpu[0], sizeof *before) == 0);
    *after = native_cpu[1];
    for (i = 0; i < N; ++i) {
        if (ram[i] == service_ram[i]) continue;
        assert(n < capacity);
        writes[n].linear = (uint32_t)i;
        writes[n].old_byte = ram[i];
        writes[n].new_byte = service_ram[i];
        ++n;
    }
    printf("service net changed bytes: %zu\n", n);
    *count = n;
    return PR_SLOT_A_DIRECTORY_OK;
}

static void compare(const char *label, const pr_slot_a_directory_state *s,
                    pr_slot_a_directory_cpu expected)
{
    read_ram(label, expected_ram);
    if (memcmp(&s->cpu, &expected, sizeof expected) != 0) {
        fprintf(stderr, "%s CPU mismatch\n", label);
        abort();
    }
    if (memcmp(s->ram, expected_ram, N) != 0) {
        size_t i;
        for (i = 0; i < N; ++i)
            if (s->ram[i] != expected_ram[i]) {
                fprintf(stderr, "%s RAM first mismatch %05zx\n", label, i);
                abort();
            }
    }
    printf("%s: 16 CPU fields and %u RAM bytes match\n", label, N);
}

int main(int argc, char **argv)
{
    pr_slot_a_directory_state s;
    pr_slot_a_directory_write journal[32];
    size_t i;
    assert(argc == 2); root = argv[1];
    read_ev_transcript();
    s.ram = malloc(N); assert(s.ram); s.ram_size = N;
    read_ram("pre-int", s.ram);
    read_ram("post-int", service_ram);
    s.cpu = native_cpu[0];
    compare(labels[0], &s, native_cpu[0]);
    for (i = 1; i < 8u; ++i) {
        assert(pr_slot_a_directory_step(&s,
            i == 1u ? captured_service : NULL, NULL,
            i == 1u ? journal : NULL, i == 1u ? 32u : 0u) ==
            PR_SLOT_A_DIRECTORY_OK);
        compare(labels[i], &s, native_cpu[i]);
    }
    free(s.ram);
    return 0;
}

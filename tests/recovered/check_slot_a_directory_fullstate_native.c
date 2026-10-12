#include "poolrad/recovered/slot_a_directory_fullstate.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 0xa0000u
static const char *root;
static uint8_t expected_ram[N];
static uint8_t service_ram[N];

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
    *after = *before;
    after->ax = 5u; after->flags = 0x7247u; after->ip = 0x1823u;
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

#define CPU(cseg, coff, dseg, spv, bpv, axv, flv) \
    {axv, 0x35a0u, 0u, 0x3526u, 0x36bbu, 0x3531u, bpv, spv, \
     cseg, dseg, 0x1d16u, 0x1d16u, 0u, 0u, coff, flv}

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
    assert(argc == 2); root = argv[1];
    s.ram = malloc(N); assert(s.ram); s.ram_size = N;
    read_ram("pre-int", s.ram);
    read_ram("post-int", service_ram);
    s.cpu = (pr_slot_a_directory_cpu)CPU(0x131cu,0x1821u,0x1d16u,0x3522u,0x35a6u,0x3900u,0x7246u);
    compare("pre-int", &s, s.cpu);
    assert(pr_slot_a_directory_step(&s, captured_service, NULL, journal, 32u) == PR_SLOT_A_DIRECTORY_OK);
    compare("post-int", &s, (pr_slot_a_directory_cpu)CPU(0x131cu,0x1823u,0x1d16u,0x3522u,0x35a6u,5u,0x7247u));
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) == PR_SLOT_A_DIRECTORY_OK);
    compare("pre-store", &s, (pr_slot_a_directory_cpu)CPU(0x131cu,0x1826u,0x14a0u,0x3524u,0x35a6u,5u,0x7247u));
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) == PR_SLOT_A_DIRECTORY_OK);
    compare("post-store", &s, (pr_slot_a_directory_cpu)CPU(0x131cu,0x1829u,0x14a0u,0x3524u,0x35a6u,5u,0x7247u));
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) == PR_SLOT_A_DIRECTORY_OK);
    compare("02d5", &s, (pr_slot_a_directory_cpu)CPU(0x2a8eu,0x02d5u,0x14a0u,0x35b0u,0x37b2u,5u,0x7247u));
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) == PR_SLOT_A_DIRECTORY_OK);
    compare("io-entry", &s, (pr_slot_a_directory_cpu)CPU(0x131cu,0x04a2u,0x14a0u,0x35acu,0x37b2u,5u,0x7247u));
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) == PR_SLOT_A_DIRECTORY_OK);
    compare("io-after-xchg", &s, (pr_slot_a_directory_cpu)CPU(0x131cu,0x04a8u,0x14a0u,0x35acu,0x37b2u,5u,0x7246u));
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) == PR_SLOT_A_DIRECTORY_OK);
    compare("02da", &s, (pr_slot_a_directory_cpu)CPU(0x2a8eu,0x02dau,0x14a0u,0x35b0u,0x37b2u,5u,0x7246u));
    free(s.ram);
    return 0;
}

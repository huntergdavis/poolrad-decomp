/* Bounded source-only pre-byte model for the reached 131C:16A1 uvga mode.
 * The native P5FIRST record supplies physical planes; this program does not
 * observe the post-MOVSB page and therefore cannot establish native parity.
 * The older 16-bit/planar bridge is intentionally separate. */
#include "poolrad/formats/ega.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RECORD_LIMIT 4096u
#define LINE_LIMIT 512u
#define PLANES 4u
#define PIXELS 8u
#define FIELD_COUNT 26u

typedef enum {
    EXACT_OK, EXACT_ARGUMENT, EXACT_FORMAT, EXACT_MODE, EXACT_ADDRESS
} exact_status;

typedef struct {
    uint32_t src_phys, page_phys, memsize, memmask;
    uint32_t bank_read_full, bank_write_full, page_window_base, page_mask;
    uint32_t source_base, page_base, source_index, page_index;
    uint8_t gc6, gc4, gc5, seq4, seq2;
    uint8_t compat_chain4, ignore_odd_even, bank_read, bank_write, hobit_n;
    uint8_t source[PLANES], page_before[PLANES];
    bool source_uvga, page_uvga;
} exact_input;

typedef struct {
    uint32_t source_index, page_index;
    uint8_t cpu_byte, page_after[PLANES], pixel_index[PIXELS];
} exact_result;

/* The two page_base fields come from different P5FIRST lines: the first is
 * vgapages.base (the A-window page number), the second the planar base. */
static const char *const keys[FIELD_COUNT] = {
    "handler_src", "handler_page", "src_phys", "page_phys",
    "gc6", "gc4", "gc5", "seq4", "seq2", "compat_chain4",
    "ignore_odd_even", "memsize", "memmask", "bank_read",
    "bank_write", "bank_read_full", "bank_write_full", "page_base",
    "page_mask", "source_base", "page_base", "source_index",
    "page_index", "hobit_n", "source_planes", "page_planes"
};
static const unsigned group_sizes[5] = {4u, 7u, 8u, 5u, 2u};

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static bool hex_exact(const char *s, size_t digits, uint32_t *out) {
    uint32_t value = 0;
    size_t i;
    if (!s || strlen(s) != digits || digits == 0u || digits > 8u)
        return false;
    for (i = 0; i < digits; ++i) {
        int nibble = hex_nibble(s[i]);
        if (nibble < 0) return false;
        value = (value << 4) | (uint32_t)nibble;
    }
    *out = value;
    return true;
}

static bool planes_exact(const char *s, uint8_t out[PLANES]) {
    size_t i;
    if (!s || strlen(s) != 11u) return false;
    for (i = 0; i < PLANES; ++i) {
        int high = hex_nibble(s[i * 3u]);
        int low = hex_nibble(s[i * 3u + 1u]);
        if (high < 0 || low < 0 || (i < PLANES - 1u && s[i * 3u + 2u] != ','))
            return false;
        out[i] = (uint8_t)((high << 4) | low);
    }
    return true;
}

static exact_status set_field(exact_input *next, unsigned field,
                              const char *value) {
    uint32_t number;
    size_t width;
    if (field == 0u || field == 1u) {
        if (strcmp(value, "uvga") != 0) return EXACT_MODE;
        if (field == 0u) next->source_uvga = true;
        else next->page_uvga = true;
        return EXACT_OK;
    }
    if (field == 24u || field == 25u)
        return planes_exact(value, field == 24u ? next->source :
                           next->page_before) ? EXACT_OK : EXACT_FORMAT;
    if (field == 23u) {
        if (strcmp(value, "16") != 0) return EXACT_MODE;
        next->hobit_n = 16u;
        return EXACT_OK;
    }
    if (field == 9u || field == 10u) {
        if (strlen(value) != 1u || (value[0] != '0' && value[0] != '1'))
            return EXACT_FORMAT;
        if (field == 9u) next->compat_chain4 = (uint8_t)(value[0] - '0');
        else next->ignore_odd_even = (uint8_t)(value[0] - '0');
        return EXACT_OK;
    }
    width = (field >= 4u && field <= 8u) || field == 13u ||
            field == 14u || field == 17u ?
        2u : (field == 2u || field == 3u ? 5u : 8u);
    if (!hex_exact(value, width, &number)) return EXACT_FORMAT;
    switch (field) {
    case 2u: next->src_phys = number; break;
    case 3u: next->page_phys = number; break;
    case 4u: next->gc6 = (uint8_t)number; break;
    case 5u: next->gc4 = (uint8_t)number; break;
    case 6u: next->gc5 = (uint8_t)number; break;
    case 7u: next->seq4 = (uint8_t)number; break;
    case 8u: next->seq2 = (uint8_t)number; break;
    case 11u: next->memsize = number; break;
    case 12u: next->memmask = number; break;
    case 13u: next->bank_read = (uint8_t)number; break;
    case 14u: next->bank_write = (uint8_t)number; break;
    case 15u: next->bank_read_full = number; break;
    case 16u: next->bank_write_full = number; break;
    case 17u: next->page_window_base = number; break;
    case 18u: next->page_mask = number; break;
    case 19u: next->source_base = number; break;
    case 20u: next->page_base = number; break;
    case 21u: next->source_index = number; break;
    case 22u: next->page_index = number; break;
    default: return EXACT_FORMAT;
    }
    return EXACT_OK;
}

static exact_status parse_record(const char *record, size_t length,
                                 exact_input *out) {
    exact_input next = {0};
    size_t pos = 0, line_number = 0, first_field = 0;
    if (!record || !out || length == 0u || length > RECORD_LIMIT)
        return EXACT_ARGUMENT;
    while (pos < length) {
        char line[LINE_LIMIT], *token;
        size_t end = pos, count, field;
        if (line_number >= 5u) return EXACT_FORMAT;
        while (end < length && record[end] != '\n') ++end;
        count = end - pos;
        if (count && record[pos + count - 1u] == '\r') --count;
        if (count < 9u || count >= sizeof line ||
            memchr(record + pos, '\0', count) != NULL)
            return EXACT_FORMAT;
        memcpy(line, record + pos, count);
        line[count] = '\0';
        if (memcmp(line, "P5FIRST ", 8u) != 0) return EXACT_FORMAT;
        token = strtok(line + 8u, " ");
        for (field = 0; field < group_sizes[line_number]; ++field) {
            const char *key = keys[first_field + field];
            size_t key_length = strlen(key);
            exact_status status;
            if (!token || strncmp(token, key, key_length) != 0 ||
                token[key_length] != '=')
                return EXACT_FORMAT;
            status = set_field(&next, (unsigned)(first_field + field),
                               token + key_length + 1u);
            if (status != EXACT_OK) return status;
            token = strtok(NULL, " ");
        }
        if (token) return EXACT_FORMAT;
        first_field += group_sizes[line_number++];
        pos = end < length ? end + 1u : end;
    }
    if (line_number != 5u || first_field != FIELD_COUNT)
        return EXACT_FORMAT;
    *out = next;
    return EXACT_OK;
}

/* Source: DOSBox-X vga_memory.cpp VGA_Generic_{Read,Write}_Handler.
 * The full VRAM memmask is uint32_t and must be shifted before use. */
static uint32_t generic_index(uint32_t base, const exact_input *input) {
    uint32_t high = input->compat_chain4 ? 0u : UINT32_C(0xFFFF0000);
    uint32_t mask;
    if ((input->gc6 & 2u) != 0u && !input->ignore_odd_even) {
        mask = high + (UINT32_C(1) << input->hobit_n) - 2u;
        return (base & mask & (input->memmask >> 2u)) +
               ((base >> input->hobit_n) & 1u);
    }
    mask = high + (UINT32_C(1) << input->hobit_n) - 1u;
    return base & mask & (input->memmask >> 2u);
}

static exact_status project_exact(const exact_input *input, exact_result *out) {
    exact_result next = {0};
    uint32_t source_base, page_base;
    unsigned plane;
    if (!input || !out) return EXACT_ARGUMENT;
    if (!input->source_uvga || !input->page_uvga ||
        input->gc6 != 0x05u || input->gc4 != 0u ||
        input->gc5 != 0x01u || input->seq4 != 0x06u ||
        input->seq2 != 0x0Fu || input->compat_chain4 != 1u ||
        input->ignore_odd_even != 0u ||
        input->memsize != UINT32_C(0x00200000) ||
        input->memmask != UINT32_C(0x001FFFFF) ||
        input->bank_read != 0u || input->bank_write != 0u ||
        input->bank_read_full != 0u || input->bank_write_full != 0u ||
        input->hobit_n != 16u)
        return EXACT_MODE;
    if (input->src_phys != UINT32_C(0xA23C3) ||
        input->page_phys != UINT32_C(0xA03C3) ||
        input->page_window_base != UINT32_C(0xA0) ||
        input->page_mask != UINT32_C(0xFFFF))
        return EXACT_ADDRESS;
    /* Seq4=06: active chain four is clear, odd/even is disabled, and
     * extended memory selects high-bit position 16. GC6=05 has bit 1
     * clear. GC5=01 is read mode 0 and write mode 1. */
    if ((input->seq4 & 8u) != 0u || (input->seq4 & 4u) == 0u ||
        (input->seq4 & 2u) == 0u || (input->gc6 & 2u) != 0u ||
        (input->gc5 & 3u) != 1u || (input->gc5 & 8u) != 0u)
        return EXACT_MODE;
    source_base = (input->src_phys & input->page_mask) +
                  input->bank_read_full;
    page_base = (input->page_phys & input->page_mask) +
                input->bank_read_full;
    next.source_index = generic_index(source_base, input);
    next.page_index = generic_index(page_base, input);
    if (source_base != input->source_base ||
        page_base != input->page_base ||
        next.source_index != input->source_index ||
        next.page_index != input->page_index ||
        next.source_index >= input->memsize / 4u ||
        next.page_index >= input->memsize / 4u)
        return EXACT_ADDRESS;
    /* VGA read mode 0 loads all four source planes into the latch and
     * returns GC4=00 to AL. Write mode 1 uses that latch; Seq2=0F
     * expands to all four enabled plane bytes. */
    next.cpu_byte = input->source[input->gc4];
    for (plane = 0; plane < PLANES; ++plane)
        next.page_after[plane] = (input->seq2 & (1u << plane)) ?
            input->source[plane] : input->page_before[plane];
    if (pr_ega_planar_byte(next.page_after, next.pixel_index) != PR_EGA_OK)
        return EXACT_MODE;
    *out = next;
    return EXACT_OK;
}

static exact_status parse_and_project(const char *record, size_t length,
                                      exact_input *input, exact_result *result) {
    exact_input next_input;
    exact_result next_result;
    exact_status status;
    if (!input || !result) return EXACT_ARGUMENT;
    status = parse_record(record, length, &next_input);
    if (status != EXACT_OK) return status;
    status = project_exact(&next_input, &next_result);
    if (status != EXACT_OK) return status;
    *input = next_input;
    *result = next_result;
    return EXACT_OK;
}

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "exact uvga fixture failed line %d: %s\n", \
            __LINE__, #expr); return 1; \
} } while (0)

/* Entirely invented plane bytes. The metadata shape is the measured
 * pre-byte mode; no original game pixels are present in this fixture. */
static const char fixture[] =
    "P5FIRST handler_src=uvga handler_page=uvga src_phys=A23C3 page_phys=A03C3\n"
    "P5FIRST gc6=05 gc4=00 gc5=01 seq4=06 seq2=0F compat_chain4=1 ignore_odd_even=0\n"
    "P5FIRST memsize=00200000 memmask=001FFFFF bank_read=00 bank_write=00 "
    "bank_read_full=00000000 bank_write_full=00000000 page_base=A0 page_mask=0000FFFF\n"
    "P5FIRST source_base=000023C3 page_base=000003C3 source_index=000023C3 "
    "page_index=000003C3 hobit_n=16\n"
    "P5FIRST source_planes=12,34,56,78 page_planes=AA,BB,CC,DD\n";

static bool changed(const char *from, const char *to,
                    char output[sizeof fixture]) {
    const char *at = strstr(fixture, from);
    size_t start;
    if (!at || strlen(from) != strlen(to)) return false;
    memcpy(output, fixture, sizeof fixture);
    start = (size_t)(at - fixture);
    memcpy(output + start, to, strlen(to));
    return true;
}

static int synthetic_tests(void) {
    exact_input input, before_input;
    exact_result result, before_result;
    char altered[sizeof fixture];
    struct mutation {
        const char *from, *to;
        exact_status status;
    };
    static const struct mutation cases[] = {
        {"memmask=001FFFFF", "memmask=0000FFFF", EXACT_MODE},
        {"gc6=05", "gc6=04", EXACT_MODE},
        {"seq4=06", "seq4=0E", EXACT_MODE},
        {"compat_chain4=1", "compat_chain4=0", EXACT_MODE},
        {"bank_read_full=00000000", "bank_read_full=00010000", EXACT_MODE},
        {"source_index=000023C3", "source_index=000023C2", EXACT_ADDRESS},
        {"handler_src=uvga", "handler_src=none", EXACT_MODE},
        {"source_planes=12,34,56,78", "source_planes=12,34,56,GG", EXACT_FORMAT},
        {"memmask=001FFFFF", "memmask=001FFFFG", EXACT_FORMAT}
    };
    size_t i;
    CHECK(parse_and_project(fixture, strlen(fixture), &input, &result)
          == EXACT_OK);
    CHECK(input.memmask == UINT32_C(0x001FFFFF));
    CHECK(result.source_index == UINT32_C(0x23C3));
    CHECK(result.page_index == UINT32_C(0x03C3));
    CHECK(result.cpu_byte == 0x12u);
    CHECK(result.page_after[0] == 0x12u && result.page_after[1] == 0x34u &&
          result.page_after[2] == 0x56u && result.page_after[3] == 0x78u);
    before_input = input;
    before_result = result;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        CHECK(changed(cases[i].from, cases[i].to, altered));
        CHECK(parse_and_project(altered, strlen(altered), &input, &result)
              == cases[i].status);
        CHECK(memcmp(&input, &before_input, sizeof input) == 0);
        CHECK(memcmp(&result, &before_result, sizeof result) == 0);
    }
    CHECK(parse_and_project(fixture, RECORD_LIMIT + 1u, &input, &result)
          == EXACT_ARGUMENT);
    CHECK(parse_and_project(fixture, strlen(fixture) - 1u, &input, &result)
          == EXACT_OK); /* final newline optional */
    CHECK(parse_and_project(fixture, strlen(fixture) - 67u, &input, &result)
          == EXACT_FORMAT); /* missing final plane line */
    puts("exact uvga bridge: synthetic four-plane copy, uint32 mask, nine typed rejects PASS");
    return 0;
}

static int project_file(const char *path) {
    char record[RECORD_LIMIT + 1u];
    exact_input input;
    exact_result result;
    exact_status status;
    FILE *file = fopen(path, "rb");
    size_t length;
    int read_error, close_error;
    if (!file) { fputs("exact uvga bridge: cannot open record\n", stderr); return 1; }
    length = fread(record, 1, sizeof record, file);
    read_error = ferror(file);
    close_error = fclose(file);
    if (read_error || close_error || length > RECORD_LIMIT) {
        fputs("exact uvga bridge: read failed or record too large\n", stderr);
        return 1;
    }
    status = parse_and_project(record, length, &input, &result);
    if (status != EXACT_OK) {
        static const char *const names[] = {
            "ok", "argument", "format", "unsupported-mode", "address"
        };
        fprintf(stderr, "exact uvga bridge: stop %s\n", names[(unsigned)status]);
        return 1;
    }
    printf("exact uvga pre-byte input accepted; source/page indices %08X/%08X\n",
           (unsigned)result.source_index, (unsigned)result.page_index);
    puts("static first-byte mode1 projection computed; no post-Move native parity");
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) return synthetic_tests();
    if (argc == 3 && strcmp(argv[1], "--record") == 0)
        return project_file(argv[2]);
    fputs("usage: res464-16a1-uvga-exact-bridge [--record ignored-p5first.txt]\n",
          stderr);
    return 2;
}

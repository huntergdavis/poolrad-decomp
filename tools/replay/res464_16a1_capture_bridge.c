/* Parser/bridge for a future pre-loaded-SYSTEM:16A1 state record.
 * Only synthetic records are supplied in this test program.
 * A parsed record is not proof of a native capture or replay route. */
#include "res464_16a1_explicit_byte_model.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RECORD_LIMIT 4096u
#define LINE_LIMIT 128u

typedef enum {
    BRIDGE_OK, BRIDGE_ARGUMENT, BRIDGE_FORMAT, BRIDGE_DUPLICATE,
    BRIDGE_MISSING, BRIDGE_FRAME, BRIDGE_HARDWARE, BRIDGE_ADDRESS
} bridge_status;

typedef enum {
    F_PHASE, F_PC, F_DS, F_SI, F_ES, F_DI, F_CX, F_DF,
    F_GC6, F_SEQ4, F_SEQ2, F_GC4, F_GC5, F_HANDLER,
    F_COMPAT, F_IGNORE, F_MEMMASK, F_BANK_READ, F_BANK_WRITE,
    F_SOURCE_OFFSET, F_PAGE_OFFSET,
    F_SOURCE0, F_SOURCE1, F_SOURCE2, F_SOURCE3,
    F_PAGE0, F_PAGE1, F_PAGE2, F_PAGE3,
    FIELD_COUNT
} field_id;

static const char *const field_keys[FIELD_COUNT] = {
    "phase", "pc", "ds", "si", "es", "di", "cx", "df",
    "gc6", "seq4", "seq2", "gc4", "gc5", "handler",
    "compatible_chain4", "ignore_odd_even", "memmask", "bank_read",
    "bank_write", "source_planar", "page_planar",
    "source_p0", "source_p1", "source_p2", "source_p3",
    "page_p0", "page_p1", "page_p2", "page_p3"
};

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static bool exact_hex(const char *text, size_t digits, uint16_t *value) {
    size_t i;
    uint16_t result = 0;
    if (strlen(text) != digits) return false;
    for (i = 0; i < digits; ++i) {
        int nibble = hex_digit(text[i]);
        if (nibble < 0) return false;
        result = (uint16_t)((result << 4) | (unsigned)nibble);
    }
    *value = result;
    return true;
}

static bool exact_bool(const char *text, bool *value) {
    if (strcmp(text, "0") == 0) { *value = false; return true; }
    if (strcmp(text, "1") == 0) { *value = true; return true; }
    return false;
}

static bridge_status parse_field(field_id field, const char *text,
                                  uint16_t loaded_cs,
                                  explicit_byte_input *input,
                                  uint16_t *source_offset,
                                  uint16_t *page_offset) {
    uint16_t value;
    bool flag;
    if (field == F_PHASE)
        return strcmp(text, "pre_16A1") == 0 ? BRIDGE_OK : BRIDGE_FRAME;
    if (field == F_PC) {
        char segment[5], offset[5];
        uint16_t seg, off;
        if (strlen(text) != 9u || text[4] != ':') return BRIDGE_FORMAT;
        memcpy(segment, text, 4); segment[4] = '\0';
        memcpy(offset, text + 5, 4); offset[4] = '\0';
        if (!exact_hex(segment, 4, &seg) || !exact_hex(offset, 4, &off))
            return BRIDGE_FORMAT;
        return seg == loaded_cs && off == 0x16A1u ? BRIDGE_OK : BRIDGE_FRAME;
    }
    if (field == F_HANDLER) {
        if (strcmp(text, "unchained") == 0) {
            input->unchained = true;
            return BRIDGE_OK;
        }
        return BRIDGE_HARDWARE;
    }
    if (field == F_DF || field == F_COMPAT || field == F_IGNORE) {
        if (!exact_bool(text, &flag)) return BRIDGE_FORMAT;
        if (field == F_DF) return flag ? BRIDGE_FRAME : BRIDGE_OK;
        if (field == F_COMPAT) input->compatible_chain4 = flag;
        else input->ignore_odd_even = flag;
        return BRIDGE_OK;
    }
    if (!exact_hex(text, field >= F_GC6 && field <= F_GC5 ? 2u :
                         field >= F_SOURCE0 ? 2u : 4u, &value))
        return BRIDGE_FORMAT;
    switch (field) {
    case F_DS: return value == 0xA200u ? BRIDGE_OK : BRIDGE_FRAME;
    case F_SI: return value == 0x03C3u ? BRIDGE_OK : BRIDGE_FRAME;
    case F_ES: return value == 0xA000u ? BRIDGE_OK : BRIDGE_FRAME;
    case F_DI: return value == 0x03C3u ? BRIDGE_OK : BRIDGE_FRAME;
    case F_CX: return value == 0x000Bu ? BRIDGE_OK : BRIDGE_FRAME;
    case F_GC6: input->gc6 = (uint8_t)value; break;
    case F_SEQ4: input->seq4 = (uint8_t)value; break;
    case F_SEQ2: input->seq2 = (uint8_t)value; break;
    case F_GC4: input->gc4 = (uint8_t)value; break;
    case F_GC5: input->gc5 = (uint8_t)value; break;
    case F_MEMMASK: input->memmask = value; break;
    case F_BANK_READ: input->bank_read = value; break;
    case F_BANK_WRITE: input->bank_write = value; break;
    case F_SOURCE_OFFSET: *source_offset = value; break;
    case F_PAGE_OFFSET: *page_offset = value; break;
    case F_SOURCE0: case F_SOURCE1: case F_SOURCE2: case F_SOURCE3:
        input->source[(unsigned)field - F_SOURCE0] = (uint8_t)value;
        break;
    case F_PAGE0: case F_PAGE1: case F_PAGE2: case F_PAGE3:
        input->page_before[(unsigned)field - F_PAGE0] = (uint8_t)value;
        break;
    default: return BRIDGE_FORMAT;
    }
    return BRIDGE_OK;
}

static bridge_status parse_and_project(const char *record, size_t length,
                                       uint16_t loaded_cs,
                                       explicit_byte_input *parsed,
                                       explicit_byte_result *projected) {
    explicit_byte_input next_input = {0};
    explicit_byte_result next_result;
    uint32_t seen = 0;
    uint16_t source_offset = 0, page_offset = 0;
    size_t pos = 0;
    if (!record || !parsed || !projected || loaded_cs == 0 || length == 0 ||
        length > RECORD_LIMIT)
        return BRIDGE_ARGUMENT;
    while (pos < length) {
        char line[LINE_LIMIT], *equals;
        size_t end = pos, count, key_len, field;
        bridge_status status;
        while (end < length && record[end] != '\n') ++end;
        count = end - pos;
        if (count && record[pos + count - 1u] == '\r') --count;
        if (count == 0 || count >= sizeof line ||
            memchr(record + pos, '\0', count) != NULL)
            return BRIDGE_FORMAT;
        memcpy(line, record + pos, count);
        line[count] = '\0';
        equals = strchr(line, '=');
        if (!equals || equals == line || strchr(equals + 1, '='))
            return BRIDGE_FORMAT;
        key_len = (size_t)(equals - line);
        for (field = 0; field < FIELD_COUNT; ++field)
            if (strlen(field_keys[field]) == key_len &&
                memcmp(field_keys[field], line, key_len) == 0)
                break;
        if (field == FIELD_COUNT) return BRIDGE_FORMAT;
        if (seen & (UINT32_C(1) << field)) return BRIDGE_DUPLICATE;
        status = parse_field((field_id)field, equals + 1, loaded_cs,
                             &next_input,
                             &source_offset, &page_offset);
        if (status != BRIDGE_OK) return status;
        seen |= UINT32_C(1) << field;
        pos = end < length ? end + 1u : end;
    }
    if (seen != (UINT32_C(1) << FIELD_COUNT) - 1u)
        return BRIDGE_MISSING;
    next_input.known = HAVE_ALL;
    if (!project_one_byte(&next_input, &next_result))
        return BRIDGE_HARDWARE;
    if (source_offset != next_result.source_planar_offset ||
        page_offset != next_result.page_planar_offset)
        return BRIDGE_ADDRESS;
    *parsed = next_input;
    *projected = next_result;
    return BRIDGE_OK;
}

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "capture bridge fixture failed line %d: %s\n", \
            __LINE__, #expr); \
    return 1; \
} } while (0)

/* These fields are invented test values. There is no native +16A1 capture. */
static const char *const synthetic_lines[FIELD_COUNT] = {
    "phase=pre_16A1", "pc=ABCD:16A1", "ds=A200", "si=03C3",
    "es=A000", "di=03C3", "cx=000B", "df=0",
    "gc6=04", "seq4=04", "seq2=0F", "gc4=00", "gc5=01",
    "handler=unchained", "compatible_chain4=0", "ignore_odd_even=0",
    "memmask=FFFF", "bank_read=0000", "bank_write=0000",
    "source_planar=23C3", "page_planar=03C3",
    "source_p0=80", "source_p1=40", "source_p2=20", "source_p3=10",
    "page_p0=00", "page_p1=00", "page_p2=00", "page_p3=00"
};

static size_t build_record(char out[RECORD_LIMIT], int omitted,
                           const char *override_key,
                           const char *override_line) {
    size_t used = 0, field;
    for (field = 0; field < FIELD_COUNT; ++field) {
        const char *line = synthetic_lines[field];
        size_t count;
        if ((int)field == omitted) continue;
        if (override_key &&
            strncmp(line, override_key, strlen(override_key)) == 0 &&
            line[strlen(override_key)] == '=')
            line = override_line;
        count = strlen(line);
        if (used + count + 1u >= RECORD_LIMIT) return 0;
        memcpy(out + used, line, count);
        used += count;
        out[used++] = '\n';
    }
    out[used] = '\0';
    return used;
}

static int synthetic_tests(void) {
    char record[RECORD_LIMIT];
    explicit_byte_input input, before_input;
    explicit_byte_result result, before_result;
    const uint8_t pixels[PIXELS] = {1,2,4,8,0,0,0,0};
    size_t length, field;
    length = build_record(record, -1, NULL, NULL);
    CHECK(length > 0);
    CHECK(parse_and_project(record, length, 0xABCDu, &input, &result)
          == BRIDGE_OK);
    CHECK(input.known == HAVE_ALL && input.gc6 == 4u && input.seq4 == 4u);
    CHECK(result.source_planar_offset == 0x23C3u);
    CHECK(result.page_planar_offset == 0x03C3u);
    CHECK(result.effective_mask == 0x0Fu);
    CHECK(memcmp(result.pixel_index, pixels, PIXELS) == 0);
    for (field = 0; field < FIELD_COUNT; ++field) {
        length = build_record(record, (int)field, NULL, NULL);
        before_input = input; before_result = result;
        CHECK(parse_and_project(record, length, 0xABCDu, &input, &result)
              == BRIDGE_MISSING);
        CHECK(memcmp(&input, &before_input, sizeof input) == 0);
        CHECK(memcmp(&result, &before_result, sizeof result) == 0);
    }
    length = build_record(record, -1, NULL, NULL);
    CHECK(length + 7u < RECORD_LIMIT);
    memcpy(record + length, "gc6=04\n", 7);
    CHECK(parse_and_project(record, length + 7u, 0xABCDu, &input, &result)
          == BRIDGE_DUPLICATE);
    length = build_record(record, -1, "gc6", "gc6=GG");
    CHECK(parse_and_project(record, length, 0xABCDu, &input, &result)
          == BRIDGE_FORMAT);
    before_input = input; before_result = result;
    length = build_record(record, -1, "pc", "pc=ABCD:16A2");
    CHECK(parse_and_project(record, length, 0xABCDu, &input, &result)
          == BRIDGE_FRAME);
    CHECK(memcmp(&input, &before_input, sizeof input) == 0);
    CHECK(memcmp(&result, &before_result, sizeof result) == 0);
    length = build_record(record, -1, "pc", "pc=ABCE:16A1");
    CHECK(parse_and_project(record, length, 0xABCDu, &input, &result)
          == BRIDGE_FRAME);
    CHECK(memcmp(&input, &before_input, sizeof input) == 0);
    CHECK(memcmp(&result, &before_result, sizeof result) == 0);
    length = build_record(record, -1, "pc", "pc=0AF8:16A1");
    CHECK(parse_and_project(record, length, 0xABCDu, &input, &result)
          == BRIDGE_FRAME);
    CHECK(memcmp(&input, &before_input, sizeof input) == 0);
    CHECK(memcmp(&result, &before_result, sizeof result) == 0);
    length = build_record(record, -1, NULL, NULL);
    CHECK(parse_and_project(record, length, 0xABCEu, &input, &result)
          == BRIDGE_FRAME);
    CHECK(memcmp(&input, &before_input, sizeof input) == 0);
    CHECK(memcmp(&result, &before_result, sizeof result) == 0);
    CHECK(parse_and_project(record, length, 0, &input, &result)
          == BRIDGE_ARGUMENT);
    CHECK(memcmp(&input, &before_input, sizeof input) == 0);
    CHECK(memcmp(&result, &before_result, sizeof result) == 0);
    length = build_record(record, -1, "gc6", "gc6=08");
    CHECK(parse_and_project(record, length, 0xABCDu, &input, &result)
          == BRIDGE_HARDWARE);
    length = build_record(record, -1, "source_planar", "source_planar=23C2");
    CHECK(parse_and_project(record, length, 0xABCDu, &input, &result)
          == BRIDGE_ADDRESS);
    puts("synthetic capture bridge: loaded-PC frame, 29 required-field stops, typed guards");
    return 0;
}

static int project_record_file(const char *path, uint16_t loaded_cs) {
    char record[RECORD_LIMIT + 1u];
    explicit_byte_input input;
    explicit_byte_result result;
    FILE *file = fopen(path, "rb");
    size_t length, plane, pixel;
    bridge_status status;
    int read_error, close_error;
    if (!file) {
        fputs("capture bridge: cannot open record\n", stderr);
        return 1;
    }
    length = fread(record, 1, sizeof record, file);
    read_error = ferror(file);
    close_error = fclose(file);
    if (read_error || close_error || length > RECORD_LIMIT) {
        fputs("capture bridge: read failed or record exceeds 4096 bytes\n", stderr);
        return 1;
    }
    status = parse_and_project(record, length, loaded_cs, &input, &result);
    if (status != BRIDGE_OK) {
        static const char *const names[] = {
            "ok", "argument", "format", "duplicate", "missing-field",
            "wrong-frame", "unsupported-hardware", "planar-address"
        };
        fprintf(stderr, "capture bridge: unsupported stop %s\n",
                names[(unsigned)status]);
        return 1;
    }
    printf("source/page planar offsets %04X/%04X, effective mask %02X\n",
           result.source_planar_offset, result.page_planar_offset,
           result.effective_mask);
    fputs("projected page planes", stdout);
    for (plane = 0; plane < PLANES; ++plane)
        printf(" %02X", result.page_after[plane]);
    fputs("\nprojected palette indices", stdout);
    for (pixel = 0; pixel < PIXELS; ++pixel)
        printf(" %X", result.pixel_index[pixel]);
    putchar('\n');
    return 0;
}

int main(int argc, char **argv) {
    uint16_t loaded_cs;
    if (argc == 1) return synthetic_tests();
    if (argc == 5 && strcmp(argv[1], "--record") == 0 &&
        strcmp(argv[3], "--loaded-cs") == 0 &&
        exact_hex(argv[4], 4u, &loaded_cs) && loaded_cs != 0)
        return project_record_file(argv[2], loaded_cs);
    fputs("usage: res464-16a1-capture-bridge [--record ignored-state.txt --loaded-cs HEX4]\n",
          stderr);
    return 2;
}

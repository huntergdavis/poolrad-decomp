#include "poolrad/formats/dos_character.h"

#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static int test_framing(void) {
    uint8_t bytes[PR_DOS_CHARACTER_SIZE] = {0};
    pr_dos_character_record record, saved;
    static const size_t invalid_sizes[] = {0, 1, 284, 286, (size_t)-1};
    size_t i;
    CHECK(pr_dos_character_open(bytes, sizeof bytes, &record) ==
          PR_DOS_CHARACTER_OK);
    CHECK(record.data == bytes && record.size == sizeof bytes);
    saved = record;
    for (i = 0; i < sizeof invalid_sizes / sizeof invalid_sizes[0]; ++i) {
        CHECK(pr_dos_character_open(bytes, invalid_sizes[i], &record) ==
              PR_DOS_CHARACTER_BAD_SIZE);
        CHECK(record.data == saved.data && record.size == saved.size);
    }
    CHECK(pr_dos_character_open(NULL, sizeof bytes, &record) ==
          PR_DOS_CHARACTER_ARGUMENT);
    CHECK(pr_dos_character_open(bytes, sizeof bytes, NULL) ==
          PR_DOS_CHARACTER_ARGUMENT);
    return 0;
}

static int test_raw_spans(void) {
    uint8_t bytes[PR_DOS_CHARACTER_SIZE] = {0};
    pr_dos_character_record record, altered;
    pr_dos_character_span span, saved;
    CHECK(pr_dos_character_open(bytes, sizeof bytes, &record) ==
          PR_DOS_CHARACTER_OK);
    CHECK(pr_dos_character_raw_span(&record, 0, 285, &span) ==
          PR_DOS_CHARACTER_OK);
    CHECK(span.source_offset == 0 && span.end_offset == 285);
    CHECK(pr_dos_character_raw_span(&record, 284, 1, &span) ==
          PR_DOS_CHARACTER_OK);
    CHECK(span.source_offset == 284 && span.end_offset == 285);
    CHECK(pr_dos_character_raw_span(&record, 285, 0, &span) ==
          PR_DOS_CHARACTER_OK);
    CHECK(span.source_offset == 285 && span.end_offset == 285);
    saved = span;
    CHECK(pr_dos_character_raw_span(&record, 285, 1, &span) ==
          PR_DOS_CHARACTER_RANGE);
    CHECK(pr_dos_character_raw_span(&record, 286, 0, &span) ==
          PR_DOS_CHARACTER_RANGE);
    CHECK(pr_dos_character_raw_span(&record, (size_t)-1, 1, &span) ==
          PR_DOS_CHARACTER_RANGE);
    CHECK(pr_dos_character_raw_span(&record, 1, (size_t)-1, &span) ==
          PR_DOS_CHARACTER_RANGE);
    CHECK(span.source_offset == saved.source_offset &&
          span.end_offset == saved.end_offset);
    CHECK(pr_dos_character_raw_span(NULL, 0, 1, &span) ==
          PR_DOS_CHARACTER_ARGUMENT);
    CHECK(pr_dos_character_raw_span(&record, 0, 1, NULL) ==
          PR_DOS_CHARACTER_ARGUMENT);
    altered = record;
    altered.size = 284;
    CHECK(pr_dos_character_raw_span(&altered, 0, 1, &span) ==
          PR_DOS_CHARACTER_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_framing() || test_raw_spans()) return 1;
    puts("synthetic DOS character framing/span tests passed");
    return 0;
}

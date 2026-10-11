#include "poolrad/formats/ecl.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static void sample(uint8_t data[64]) {
    size_t i;
    memset(data, 0, 64);
    data[0] = 0x88;
    data[1] = 0x13;
    for (i = 0; i < PR_ECL_ENTRY_COUNT; ++i) {
        size_t at = 2 + 4 * i;
        data[at] = 0x01;
        data[at + 1] = 0x01;
        data[at + 2] = (uint8_t)(0x14 + i); /* offsets 22 through 26 */
        data[at + 3] = 0x99;
    }
    data[30] = 0x80;
    data[31] = 3;
    data[32] = 0x04;
    data[33] = 0x28;
    data[34] = 0x00; /* six-bit codes 1, 2, 32, 0 -> "AB " */
    data[35] = 0x80;
    data[36] = 3;
    data[37] = 0x04;
    data[38] = 0x00;
    data[39] = 0x03; /* six-bit codes 1, 0, 0, 3 -> "AC" */
    data[40] = 0x80;
    data[41] = 3;
    data[42] = 0x04;
    data[43] = 0x20;
    data[44] = 0xc4; /* six-bit codes 1, 2, 3, 4 -> "ABCD" */
    data[45] = 0x80;
    data[46] = 2;
    data[47] = 0x04;
    data[48] = 0x20; /* two complete codes 1, 2 -> "AB" */
    data[50] = 0x80;
    data[51] = 0;
    data[60] = 0x80;
    data[61] = 1;
    data[62] = 0x04; /* one complete code, incomplete low two bits */
}

static int test_header_and_addresses(void) {
    uint8_t data[64];
    pr_ecl_record record, altered;
    uint16_t address = 0;
    size_t offset = 0, i;
    sample(data);
    CHECK(pr_ecl_open(data, sizeof data, &record) == PR_ECL_OK);
    CHECK(record.prefix == PR_ECL_DOS_PREFIX && record.size == sizeof data);
    for (i = 0; i < PR_ECL_ENTRY_COUNT; ++i) {
        CHECK(record.entry_address[i] == 0x9914 + i);
        CHECK(record.entry_offset[i] == 22 + i);
    }
    CHECK(pr_ecl_address_to_offset(&record, 0x9900, &offset) == PR_ECL_OK && offset == 2);
    CHECK(pr_ecl_offset_to_address(&record, 63, &address) == PR_ECL_OK &&
          address == 0x993d);
    CHECK(pr_ecl_address_to_offset(&record, 0x98ff, &offset) == PR_ECL_ADDRESS);
    CHECK(pr_ecl_address_to_offset(&record, 0x993e, &offset) == PR_ECL_ADDRESS);
    CHECK(pr_ecl_offset_to_address(&record, 0, &address) == PR_ECL_ADDRESS);
    CHECK(pr_ecl_offset_to_address(&record, 64, &address) == PR_ECL_ADDRESS);
    altered = record;
    altered.size = PR_ECL_MAX_RECORD;
    CHECK(pr_ecl_offset_to_address(&altered, 26370, &address) == PR_ECL_ADDRESS);

    CHECK(pr_ecl_open(data, 21, &record) == PR_ECL_SIZE);
    CHECK(pr_ecl_open(data, PR_ECL_MAX_RECORD + 1, &record) == PR_ECL_SIZE);
    CHECK(pr_ecl_open(NULL, sizeof data, &record) == PR_ECL_ARGUMENT);
    CHECK(pr_ecl_open(data, sizeof data, NULL) == PR_ECL_ARGUMENT);
    data[0] = 0;
    CHECK(pr_ecl_open(data, sizeof data, &record) == PR_ECL_HEADER);
    data[0] = 0x88;
    data[6] = 0;
    CHECK(pr_ecl_open(data, sizeof data, &record) == PR_ECL_HEADER);
    data[6] = 1;
    data[7] = 0;
    CHECK(pr_ecl_open(data, sizeof data, &record) == PR_ECL_HEADER);
    data[7] = 1;
    data[4] = 0xff;
    data[5] = 0x98;
    CHECK(pr_ecl_open(data, sizeof data, &record) == PR_ECL_ADDRESS);
    data[4] = 0x3e;
    data[5] = 0x99;
    CHECK(pr_ecl_open(data, sizeof data, &record) == PR_ECL_ADDRESS);
    return 0;
}

static int test_operands(void) {
    uint8_t data[64];
    pr_ecl_record record;
    pr_ecl_operand_span span, saved;
    size_t i;
    static const uint8_t word_tags[] = {0x01, 0x02, 0x03, 0x81};
    sample(data);
    CHECK(pr_ecl_open(data, sizeof data, &record) == PR_ECL_OK);

    data[55] = 0x00;
    data[56] = 0xa5;
    CHECK(pr_ecl_operand_at(&record, 55, &span) == PR_ECL_OK);
    CHECK(span.tag == 0 && span.source_offset == 55 &&
          span.payload_offset == 56 && span.payload_size == 1 &&
          span.end_offset == 57);
    for (i = 0; i < sizeof word_tags; ++i) {
        data[55] = word_tags[i];
        data[56] = 0x34;
        data[57] = 0x12;
        CHECK(pr_ecl_operand_at(&record, 55, &span) == PR_ECL_OK);
        CHECK(span.tag == word_tags[i] && span.source_offset == 55 &&
              span.payload_offset == 56 && span.payload_size == 2 &&
              span.end_offset == 58);
    }
    data[55] = 0x80;
    data[56] = 2;
    CHECK(pr_ecl_operand_at(&record, 55, &span) == PR_ECL_OK);
    CHECK(span.tag == 0x80 && span.source_offset == 55 &&
          span.payload_offset == 57 && span.payload_size == 2 &&
          span.end_offset == 59);
    data[56] = 0;
    CHECK(pr_ecl_operand_at(&record, 55, &span) == PR_ECL_OK);
    CHECK(span.payload_size == 0 && span.end_offset == 57);

    saved = span;
    data[56] = 255;
    CHECK(pr_ecl_operand_at(&record, 55, &span) == PR_ECL_OPERAND_FRAME);
    CHECK(span.tag == saved.tag && span.source_offset == saved.source_offset &&
          span.payload_offset == saved.payload_offset &&
          span.payload_size == saved.payload_size &&
          span.end_offset == saved.end_offset);
    data[63] = 0x80;
    CHECK(pr_ecl_operand_at(&record, 63, &span) == PR_ECL_OPERAND_FRAME);
    data[63] = 0x00;
    CHECK(pr_ecl_operand_at(&record, 63, &span) == PR_ECL_OPERAND_FRAME);
    data[62] = 0x00;
    CHECK(pr_ecl_operand_at(&record, 62, &span) == PR_ECL_OK &&
          span.end_offset == 64);
    data[62] = 0x81;
    CHECK(pr_ecl_operand_at(&record, 62, &span) == PR_ECL_OPERAND_FRAME);
    data[61] = 0x81;
    CHECK(pr_ecl_operand_at(&record, 61, &span) == PR_ECL_OK &&
          span.end_offset == 64);
    data[62] = 0x80;
    data[63] = 0;
    CHECK(pr_ecl_operand_at(&record, 62, &span) == PR_ECL_OK &&
          span.payload_offset == 64 && span.end_offset == 64);
    data[63] = 1;
    CHECK(pr_ecl_operand_at(&record, 62, &span) == PR_ECL_OPERAND_FRAME);
    data[63] = 0x7f;
    CHECK(pr_ecl_operand_at(&record, 63, &span) == PR_ECL_OPERAND_TAG);
    CHECK(pr_ecl_operand_at(&record, 64, &span) == PR_ECL_OPERAND_FRAME);
    CHECK(pr_ecl_operand_at(&record, (size_t)-1, &span) == PR_ECL_OPERAND_FRAME);
    CHECK(pr_ecl_operand_at(NULL, 55, &span) == PR_ECL_ARGUMENT);
    CHECK(pr_ecl_operand_at(&record, 55, NULL) == PR_ECL_ARGUMENT);
    return 0;
}

static int test_text(void) {
    uint8_t data[64];
    uint8_t large[300] = {0};
    pr_ecl_record record;
    pr_ecl_text text, altered;
    char output[8] = {0};
    char long_output[341];
    size_t written = 99;
    sample(data);
    CHECK(pr_ecl_open(data, sizeof data, &record) == PR_ECL_OK);
    CHECK(pr_ecl_text_at(&record, 30, &text) == PR_ECL_OK);
    CHECK(text.source_offset == 30 && text.payload_offset == 32 &&
          text.packed_size == 3 && text.end_offset == 35);
    CHECK(pr_ecl_text_decode(&record, &text, NULL, 0, &written) == PR_ECL_OUTPUT_SIZE);
    CHECK(written == 3);
    CHECK(pr_ecl_text_decode(&record, &text, output, 3, &written) == PR_ECL_OUTPUT_SIZE);
    CHECK(pr_ecl_text_decode(&record, &text, output, sizeof output, &written) == PR_ECL_OK);
    CHECK(written == 3 && strcmp(output, "AB ") == 0);

    CHECK(pr_ecl_text_at(&record, 35, &text) == PR_ECL_OK);
    CHECK(pr_ecl_text_decode(&record, &text, output, 2, &written) == PR_ECL_OUTPUT_SIZE);
    CHECK(written == 2);
    CHECK(pr_ecl_text_decode(&record, &text, output, 3, &written) == PR_ECL_OK);
    CHECK(written == 2 && strcmp(output, "AC") == 0);

    CHECK(pr_ecl_text_at(&record, 40, &text) == PR_ECL_OK);
    CHECK(pr_ecl_text_decode(&record, &text, output, sizeof output, &written) == PR_ECL_OK);
    CHECK(written == 4 && strcmp(output, "ABCD") == 0);
    CHECK(pr_ecl_text_at(&record, 45, &text) == PR_ECL_OK);
    CHECK(pr_ecl_text_decode(&record, &text, output, sizeof output, &written) == PR_ECL_OK);
    CHECK(written == 2 && strcmp(output, "AB") == 0);
    CHECK(pr_ecl_text_at(&record, 50, &text) == PR_ECL_OK);
    CHECK(pr_ecl_text_decode(&record, &text, output, sizeof output, &written) == PR_ECL_OK);
    CHECK(written == 0 && output[0] == '\0');
    CHECK(pr_ecl_text_at(&record, 60, &text) == PR_ECL_OK);
    CHECK(pr_ecl_text_decode(&record, &text, output, sizeof output, &written) == PR_ECL_OK);
    CHECK(written == 1 && strcmp(output, "A") == 0);

    CHECK(pr_ecl_text_at(&record, 63, &text) == PR_ECL_TEXT_FRAME);
    CHECK(pr_ecl_text_at(&record, 64, &text) == PR_ECL_TEXT_FRAME);
    CHECK(pr_ecl_text_at(&record, (size_t)-1, &text) == PR_ECL_TEXT_FRAME);
    CHECK(pr_ecl_text_at(&record, 29, &text) == PR_ECL_TEXT_FRAME);
    data[61] = 3;
    CHECK(pr_ecl_text_at(&record, 60, &text) == PR_ECL_TEXT_FRAME);
    data[61] = 1;
    CHECK(pr_ecl_text_at(&record, 30, &text) == PR_ECL_OK);
    altered = text;
    altered.packed_size++;
    CHECK(pr_ecl_text_decode(&record, &altered, output, sizeof output, &written) ==
          PR_ECL_TEXT_FRAME);
    CHECK(pr_ecl_text_decode(&record, &text, output, sizeof output, NULL) ==
          PR_ECL_ARGUMENT);

    memcpy(large, data, sizeof data);
    large[31] = 255;
    memset(large + 32, 0xff, 255);
    CHECK(pr_ecl_open(large, sizeof large, &record) == PR_ECL_OK);
    CHECK(pr_ecl_text_at(&record, 30, &text) == PR_ECL_OK);
    CHECK(text.packed_size == 255 && text.end_offset == 287);
    CHECK(pr_ecl_text_decode(&record, &text, long_output, 340, &written) ==
          PR_ECL_OUTPUT_SIZE);
    CHECK(written == 340);
    CHECK(pr_ecl_text_decode(&record, &text, long_output,
                             sizeof long_output, &written) == PR_ECL_OK);
    CHECK(written == 340 && long_output[0] == '?' &&
          long_output[339] == '?' && long_output[340] == '\0');
    return 0;
}

int main(void) {
    if (test_header_and_addresses() || test_operands() || test_text()) return 1;
    puts("synthetic ECL header/address/operand/text tests passed");
    return 0;
}

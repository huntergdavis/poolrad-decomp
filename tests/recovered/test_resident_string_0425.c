#include "poolrad/recovered/resident_string_0425.h"
#include "poolrad/recovered/resident_string_0356.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static int check_text(const char *input, const uint8_t table[10],
                      const char *expected)
{
    uint8_t source[256] = {0}, output[256] = {0};
    size_t input_len = strlen(input), expected_len = strlen(expected);
    if (input_len > 255u || expected_len > 255u) return 1;
    source[0] = (uint8_t)input_len;
    memcpy(source + 1, input, input_len);
    CHECK(pr_rec_res_0802_0425(source, input_len + 1u, table, 10u,
                               output, sizeof output) == PR_REC_0802_0425_OK);
    CHECK(output[0] == expected_len);
    CHECK(memcmp(output + 1, expected, expected_len) == 0);
    return 0;
}

static int read_region(const char *path, long offset, uint8_t *out, size_t size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, offset, SEEK_SET) != 0 || fread(out, 1u, size, f) != size) {
        fclose(f);
        return 0;
    }
    return fclose(f) == 0;
}

static int private_case(const char *save_path, const char *game_path,
                        const char *table_path)
{
    uint8_t count, slot[41], literal[2], table[10], first[256], second[256];
    unsigned i, j;
    CHECK(read_region(save_path, 0x3208L, &count, 1u));
    CHECK(read_region(save_path, 0x3209L, slot, sizeof slot));
    CHECK(read_region(game_path, 0x1dfe6L + 0x15e7L, literal,
                      sizeof literal));
    CHECK(read_region(table_path, 0L, table, sizeof table));
    CHECK(count == 6u && slot[0] == 8u && literal[0] == 1u);
    for (i = 0u; i < sizeof table; ++i) {
        CHECK(table[i] != 0u);
        for (j = i + 1u; j < sizeof table; ++j)
            CHECK(table[i] != table[j]);
    }
    CHECK(pr_rec_res_0802_0356(slot, sizeof slot, literal, sizeof literal,
                               first, sizeof first) == PR_REC_0802_0356_OK);
    CHECK(first[0] == 8u);
    CHECK(pr_rec_res_0802_0425(first, (size_t)first[0] + 1u,
                               table, sizeof table, second, sizeof second)
          == PR_REC_0802_0425_OK);
    CHECK(second[0] == 8u);
    puts("private slot: count 6, 0356 length 8, 0425 length 8; no byte parity claim");
    return 0;
}

int main(int argc, char **argv)
{
    const uint8_t digits[10] = {'0','1','2','3','4','5','6','7','8','9'};
    const uint8_t letters[10] = {'a','b','c','d','e','f','g','h','i','j'};
    uint8_t source[16] = {4u,'a','a','a','b'};
    uint8_t output[16] = {0xabu};
    uint8_t binary[4] = {3u,0u,'q',0xe1u};
    uint8_t zero_table[10] = {0u,'1','2','3','4','5','6','7','8','9'};

    CHECK(check_text("xyz", digits, "XYZ") == 0);          /* all misses */
    CHECK(check_text("abc123def", digits, "ABCDEF") == 0); /* repeated hits */
    CHECK(check_text("aaab", letters, "A") == 0);         /* fixed-index skip */
    CHECK(check_text("mnopqrstuvwx", digits, "MNOPQRST") == 0); /* cap 8 */
    CHECK(check_text("", digits, "") == 0);               /* zero skips UpCase */

    CHECK(pr_rec_res_0802_0425(binary, sizeof binary, zero_table, 10u,
                               output, sizeof output) == PR_REC_0802_0425_OK);
    CHECK(output[0] == 2u && output[1] == 'Q' && output[2] == 0xe1u);

    /* The first SYSTEM copy makes source/output aliasing safe. */
    CHECK(pr_rec_res_0802_0425(source, 5u, letters, 10u,
                               source, sizeof source) == PR_REC_0802_0425_OK);
    CHECK(source[0] == 1u && source[1] == 'A');

    output[0] = 0xabu;
    CHECK(pr_rec_res_0802_0425(NULL, 0u, digits, 10u, output,
                               sizeof output) == PR_REC_0802_0425_ARGUMENT);
    CHECK(pr_rec_res_0802_0425(source, 1u, digits, 10u, output,
                               sizeof output) == PR_REC_0802_0425_BOUNDS);
    CHECK(pr_rec_res_0802_0425(source, 2u, digits, 9u, output,
                               sizeof output) == PR_REC_0802_0425_BOUNDS);
    CHECK(pr_rec_res_0802_0425(source, 2u, digits, 10u, output,
                               1u) == PR_REC_0802_0425_BOUNDS);
    CHECK(output[0] == 0xabu);

    if (argc != 1 && argc != 4) return 1;
    if (argc == 4 && private_case(argv[1], argv[2], argv[3]) != 0)
        return 1;

    puts("resident 0802:0425 synthetic paths passed");
    return 0;
}

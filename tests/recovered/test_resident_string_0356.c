#include "poolrad/recovered/resident_string_0356.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static void pstr(uint8_t *out, const char *text)
{
    size_t n = strlen(text);
    out[0] = (uint8_t)n;
    memcpy(out + 1u, text, n);
}

static int example(const char *source, const char *needle, const char *expected)
{
    uint8_t src[256], key[256], out[260];
    size_t n = strlen(expected);
    pstr(src, source);
    pstr(key, needle);
    memset(out, 0xA5, sizeof out);
    CHECK(pr_rec_res_0802_0356(src, 1u + src[0], key, 1u + key[0],
                               out, sizeof out) == PR_REC_0802_0356_OK);
    CHECK(out[0] == n && memcmp(out + 1u, expected, n) == 0);
    CHECK(out[n + 1u] == 0xA5u);
    return 0;
}

static int cases(void)
{
    uint8_t src[256], key[256], out[260], before[260];
    unsigned i;

    CHECK(example("ABCD", "X", "ABCD") == 0);      /* no match */
    CHECK(example("ABCD", "BC", "AD") == 0);       /* one multi-byte deletion */
    CHECK(example("A-B-C", "-", "ABC") == 0);     /* separated matches */
    CHECK(example("A--B", "-", "A-B") == 0);      /* shifted adjacent match is skipped */
    CHECK(example("aaaa", "aa", "aa") == 0);     /* fixed pre-delete limit */
    CHECK(example("AAAAA", "A", "AA") == 0);     /* index still increments after deletion */
    CHECK(example("A", "XYZ", "A") == 0);        /* wrapped byte limit, no hit */
    CHECK(example("AB", "", "AB") == 0);         /* equal empty Copy, inert Delete(0) */
    CHECK(example("", "", "") == 0);

    /* A 255-byte source and a zero-length key make the fixed limit wrap
     * from 256 to zero; the final store keeps all 255 characters. */
    src[0] = 255u;
    memset(src + 1u, 'Q', 255u);
    key[0] = 0u;
    memset(out, 0xA5, sizeof out);
    CHECK(pr_rec_res_0802_0356(src, sizeof src, key, 1u, out, sizeof out)
          == PR_REC_0802_0356_OK);
    CHECK(out[0] == 255u && memcmp(out + 1u, src + 1u, 255u) == 0);
    CHECK(out[256] == 0xA5u);

    key[0] = 255u;
    memset(key + 1u, 'Q', 255u);
    memset(out, 0xA5, sizeof out);
    CHECK(pr_rec_res_0802_0356(src, sizeof src, key, sizeof key, out, sizeof out)
          == PR_REC_0802_0356_OK);
    CHECK(out[0] == 0u && out[1] == 0xA5u);

    /* Deletions use the original 255-iteration bound but a changing source. */
    key[0] = 1u; key[1] = 'Q';
    memset(out, 0xA5, sizeof out);
    CHECK(pr_rec_res_0802_0356(src, sizeof src, key, 2u, out, sizeof out)
          == PR_REC_0802_0356_OK);
    CHECK(out[0] == 127u);
    for (i = 1u; i <= 127u; ++i) CHECK(out[i] == 'Q');
    CHECK(out[128] == 0xA5u);

    /* The source and destination may be the same host buffer: DOS first
     * copies both inputs into locals and stores the result only at return. */
    pstr(src, "A-B-C"); pstr(key, "-");
    CHECK(pr_rec_res_0802_0356(src, sizeof src, key, sizeof key, src,
                               sizeof src) == PR_REC_0802_0356_OK);
    CHECK(src[0] == 3u && memcmp(src + 1u, "ABC", 3u) == 0);

    memset(out, 0xA5, sizeof out);
    memcpy(before, out, sizeof out);
    CHECK(pr_rec_res_0802_0356(NULL, 0u, key, 2u, out, sizeof out)
          == PR_REC_0802_0356_ARGUMENT);
    CHECK(pr_rec_res_0802_0356(src, 0u, key, 2u, out, sizeof out)
          == PR_REC_0802_0356_BOUNDS);
    CHECK(pr_rec_res_0802_0356(src, 1u, key, 2u, out, sizeof out)
          == PR_REC_0802_0356_BOUNDS);
    CHECK(pr_rec_res_0802_0356(src, sizeof src, key, 1u, out, sizeof out)
          == PR_REC_0802_0356_BOUNDS);
    CHECK(pr_rec_res_0802_0356(src, sizeof src, key, sizeof key, out, 3u)
          == PR_REC_0802_0356_BOUNDS);
    CHECK(memcmp(out, before, sizeof out) == 0);
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

static int private_case(const char *save_path, const char *game_path)
{
    uint8_t count, slot[41], literal[2], out[256];
    CHECK(read_region(save_path, 0x3208L, &count, 1u));
    CHECK(read_region(save_path, 0x3209L, slot, sizeof slot));
    CHECK(read_region(game_path, 0x1DFE6L + 0x15E7L, literal, sizeof literal));
    CHECK(count == 6u && slot[0] == 8u && literal[0] == 1u);
    CHECK(pr_rec_res_0802_0356(slot, sizeof slot, literal, sizeof literal,
                               out, sizeof out) == PR_REC_0802_0356_OK);
    CHECK(out[0] == 8u && memcmp(out, slot, 9u) == 0);
    puts("saved first slot: source length 8, needle length 1, result length 8");
    return 0;
}

int main(int argc, char **argv)
{
    if ((argc != 1 && argc != 3) || cases() != 0) return 1;
    if (argc == 3 && private_case(argv[1], argv[2]) != 0) return 1;
    puts("resident 0802:0356 synthetic paths passed");
    return 0;
}

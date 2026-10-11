#include "poolrad/formats/dos_character.h"

#include <stdio.h>
#include <string.h>

#define MAX_AUDIT_FILE (1024u * 1024u)

typedef struct {
    size_t sav_files, sav_bytes;
    size_t itm_files, itm_bytes, itm_size_units;
    size_t spc_files, spc_bytes, spc_size_units;
    size_t item_pairs, spell_pairs;
} totals;

static const char *suffix(const char *path) {
    const char *dot = strrchr(path, '.');
    return dot ? dot : "";
}

static int same_stem(const char *a, const char *b) {
    const char *a_dot = strrchr(a, '.');
    const char *b_dot = strrchr(b, '.');
    size_t a_size, b_size;
    if (!a_dot || !b_dot) return 0;
    a_size = (size_t)(a_dot - a);
    b_size = (size_t)(b_dot - b);
    return a_size == b_size && memcmp(a, b, a_size) == 0;
}

static int audit_one(const char *path, int ordinal, totals *sum) {
    FILE *file;
    const char *kind = suffix(path);
    long length;
    if (strcmp(kind, ".SAV") != 0 && strcmp(kind, ".ITM") != 0 &&
        strcmp(kind, ".SPC") != 0) {
        fprintf(stderr, "input %d: unsupported suffix\n", ordinal);
        return -1;
    }
    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "input %d: cannot open\n", ordinal);
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        (unsigned long)length > MAX_AUDIT_FILE || fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "input %d: invalid file size\n", ordinal);
        fclose(file);
        return -1;
    }
    if (strcmp(kind, ".SAV") == 0) {
        uint8_t bytes[PR_DOS_CHARACTER_SIZE];
        pr_dos_character_record record;
        pr_dos_character_span whole;
        if (length != PR_DOS_CHARACTER_SIZE ||
            fread(bytes, 1, sizeof bytes, file) != sizeof bytes ||
            pr_dos_character_open(bytes, sizeof bytes, &record) != PR_DOS_CHARACTER_OK ||
            pr_dos_character_raw_span(&record, 0, sizeof bytes, &whole) !=
                PR_DOS_CHARACTER_OK || whole.end_offset != sizeof bytes) {
            fprintf(stderr, "input %d: invalid character framing\n", ordinal);
            fclose(file);
            return -1;
        }
        ++sum->sav_files;
        sum->sav_bytes += (size_t)length;
    } else if (strcmp(kind, ".ITM") == 0) {
        if (length % 63 != 0) {
            fprintf(stderr, "input %d: item sidecar size not divisible by 63\n", ordinal);
            fclose(file);
            return -1;
        }
        ++sum->itm_files;
        sum->itm_bytes += (size_t)length;
        sum->itm_size_units += (size_t)length / 63;
    } else {
        if (length % 9 != 0) {
            fprintf(stderr, "input %d: spell sidecar size not divisible by 9\n", ordinal);
            fclose(file);
            return -1;
        }
        ++sum->spc_files;
        sum->spc_bytes += (size_t)length;
        sum->spc_size_units += (size_t)length / 9;
    }
    fclose(file);
    return 0;
}

int main(int argc, char **argv) {
    totals sum = {0};
    int i, j;
    if (argc < 2) {
        fprintf(stderr, "usage: %s CHRDAT*.SAV CHRDAT*.ITM CHRDAT*.SPC\n", argv[0]);
        return 2;
    }
    for (i = 1; i < argc; ++i)
        if (audit_one(argv[i], i, &sum) != 0) return 1;
    for (i = 1; i < argc; ++i) {
        const char *kind = suffix(argv[i]);
        if (strcmp(kind, ".ITM") != 0 && strcmp(kind, ".SPC") != 0) continue;
        for (j = 1; j < argc; ++j) {
            if (strcmp(suffix(argv[j]), ".SAV") == 0 &&
                same_stem(argv[i], argv[j])) break;
        }
        if (j == argc) {
            fprintf(stderr, "input %d: sidecar without character record\n", i);
            return 1;
        }
        if (strcmp(kind, ".ITM") == 0) ++sum.item_pairs;
        else ++sum.spell_pairs;
    }
    printf("sav_files=%zu sav_bytes=%zu itm_files=%zu itm_bytes=%zu "
           "itm_size_units=%zu spc_files=%zu spc_bytes=%zu spc_size_units=%zu "
           "item_pairs=%zu spell_pairs=%zu failures=0\n",
           sum.sav_files, sum.sav_bytes, sum.itm_files, sum.itm_bytes,
           sum.itm_size_units, sum.spc_files, sum.spc_bytes, sum.spc_size_units,
           sum.item_pairs, sum.spell_pairs);
    return 0;
}

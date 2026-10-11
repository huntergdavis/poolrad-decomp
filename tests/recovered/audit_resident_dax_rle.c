/* Read-only local ECL DAX parity audit. Prints aggregate counts only. */
#include "poolrad/formats/dax.h"
#include "poolrad/recovered/resident_dax_rle.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE (1024u * 1024u)

typedef struct { uint8_t *bytes; } memory_view;

static size_t physical(pr_rec_dax_far p) {
    return (((size_t)p.segment << 4) + p.offset) & (MEMORY_SIZE - 1u);
}

static pr_rec_dax_far far_at(uint16_t segment, uint16_t offset) {
    pr_rec_dax_far p = {segment, offset};
    return p;
}

static int read8(void *ctx, pr_rec_dax_far p, uint8_t *value) {
    *value = ((memory_view *)ctx)->bytes[physical(p)];
    return 0;
}

static int write8(void *ctx, pr_rec_dax_far p, uint8_t value) {
    ((memory_view *)ctx)->bytes[physical(p)] = value;
    return 0;
}

static void put_word(memory_view *m, pr_rec_dax_far p, uint16_t value) {
    m->bytes[physical(p)] = (uint8_t)value;
    p.offset = (uint16_t)(p.offset + 1u);
    m->bytes[physical(p)] = (uint8_t)(value >> 8);
}

static void put_far(memory_view *m, pr_rec_dax_far p, pr_rec_dax_far value) {
    put_word(m, p, value.offset);
    p.offset = (uint16_t)(p.offset + 2u);
    put_word(m, p, value.segment);
}

int main(int argc, char **argv) {
    unsigned archives = 0, blocks = 0, matches = 0;
    size_t decoded_bytes = 0;
    int n;
    memory_view mem;
    if (argc != 9) {
        fputs("usage: audit_resident_dax_rle ECL1.DAX ... ECL8.DAX\n", stderr);
        return 2;
    }
    mem.bytes = calloc(MEMORY_SIZE, 1);
    if (!mem.bytes) return 3;
    for (n = 1; n <= 8; ++n) {
        FILE *f = fopen(argv[n], "rb");
        long length;
        uint8_t *bytes;
        pr_dax_archive archive;
        size_t i;
        if (!f || fseek(f, 0, SEEK_END) || (length = ftell(f)) < 0 ||
            fseek(f, 0, SEEK_SET)) return 4;
        bytes = malloc((size_t)length);
        if (!bytes || fread(bytes, 1, (size_t)length, f) != (size_t)length)
            return 5;
        fclose(f);
        if (pr_dax_open(bytes, (size_t)length, &archive) != PR_DAX_OK)
            return 6;
        for (i = 0; i < archive.count; ++i) {
            const pr_dax_entry *entry = &archive.entries[i];
            uint8_t reference[65536];
            size_t written = 0;
            pr_rec_dax_rle_args args;
            pr_rec_dax_rle_state state = {0};
            pr_rec_dax_memory bus = {&mem, read8, write8};
            pr_rec_dax_far source = far_at(0x2000, 0);
            pr_rec_dax_far destination = far_at(0x4000, 0);
            if (entry->stored_size > 0x8CA0 || !entry->stored_size ||
                !entry->unpacked_size ||
                pr_dax_read(&archive, i, reference, sizeof reference,
                            &written) != PR_DAX_OK) return 7;
            memset(mem.bytes, 0, MEMORY_SIZE);
            memcpy(mem.bytes + physical(source), bytes + entry->offset,
                   entry->stored_size);
            args.source_pointer = far_at(0x1000, 0);
            args.destination_pointer = far_at(0x1000, 4);
            args.packed_size = far_at(0x1000, 8);
            args.unused_output_size = far_at(0xFFFF, 0xFFFF);
            put_far(&mem, args.source_pointer, source);
            put_far(&mem, args.destination_pointer, destination);
            put_word(&mem, args.packed_size, entry->stored_size);
            if (pr_rec_res_0802_050f(&bus, &args, 65536, &state) !=
                PR_REC_DAX_RLE_OK) return 8;
            if (state.source_index < entry->stored_size ||
                state.destination_index != written ||
                memcmp(mem.bytes + physical(destination), reference, written))
                return 9;
            ++blocks;
            ++matches;
            decoded_bytes += written;
        }
        ++archives;
        free(bytes);
    }
    free(mem.bytes);
    printf("archives %u blocks %u byte_matches %u decoded_bytes %zu\n",
           archives, blocks, matches, decoded_bytes);
    return 0;
}

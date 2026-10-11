#include "poolrad/recovered/resident_load_route_27d9.h"

static pr_rec_27d9_status call_one(const pr_rec_27d9_io *io,
                                   pr_rec_27d9_target target,
                                   const pr_rec_27d9_args *args)
{
    if (io->call == 0) return PR_REC_27D9_BOUNDARY;
    return io->call(io->context, target, args) ?
        PR_REC_27D9_BOUNDARY : PR_REC_27D9_OK;
}

static pr_rec_27d9_status call_local_32ca(const pr_rec_27d9_io *io,
                                          uint8_t value)
{
    pr_rec_27d9_args args = {0};
    args.byte_count = 1;
    args.byte_args[0] = value;
    return call_one(io, PR_REC_27D9_LOCAL_32CA, &args);
}

static pr_rec_27d9_status party_tail(const pr_rec_27d9_io *io,
                                     uint8_t call_290b)
{
    pr_rec_27d9_args args = {0};
    args.word_count = 2;
    if (io->read_ds_word == 0 ||
        io->read_ds_word(io->context, 0x5d94u, &args.word_args[0]) ||
        io->read_ds_word(io->context, 0x5d92u, &args.word_args[1]))
        return PR_REC_27D9_BOUNDARY;
    if (call_one(io, PR_REC_27D9_RES_00BA_0767, &args) != PR_REC_27D9_OK)
        return PR_REC_27D9_BOUNDARY;
    if (call_290b) {
        pr_rec_27d9_args no_args = {0};
        return call_one(io, PR_REC_27D9_RES_00BA_290B, &no_args);
    }
    return PR_REC_27D9_OK;
}

pr_rec_27d9_status pr_rec_res_00ba_27d9(const pr_rec_27d9_io *io)
{
    uint8_t mode;
    pr_rec_27d9_args args = {0};
    if (io == 0) return PR_REC_27D9_ARGUMENT;
    if (io->read_ds_byte == 0) return PR_REC_27D9_BOUNDARY;
    if (io->read_ds_byte(io->context, 0x49f3u, &mode))
        return PR_REC_27D9_BOUNDARY;

    switch (mode) {
    case 0: {
        const pr_rec_27d9_string empty = {0};
        /* CS:0x27D4 is a zero-length Pascal literal. SYSTEM 0x0B0B
         * copies it into [BP-1], retaining the destination pointer. */
        args.byte_count = 6;
        args.byte_args[0] = 1;
        args.byte_args[1] = 1;
        args.byte_args[2] = 0x26;
        args.byte_args[3] = 0x16;
        args.byte_args[4] = 0;
        args.byte_args[5] = 0x0f;
        args.string = &empty;
        return call_one(io, PR_REC_27D9_RES_03F1_02E8, &args);
    }
    case 1: {
        uint8_t first, second;
        if (call_local_32ca(io, 1) != PR_REC_27D9_OK ||
            io->read_ds_byte(io->context, 0x49fdu, &first) ||
            io->read_ds_byte(io->context, 0x49feu, &second))
            return PR_REC_27D9_BOUNDARY;
        args.byte_count = 2;
        args.byte_args[0] = first;
        args.byte_args[1] = second;
        if (call_one(io, PR_REC_27D9_OVL_03D0_003E, &args) != PR_REC_27D9_OK)
            return PR_REC_27D9_BOUNDARY;
        args.byte_count = 3;
        args.byte_args[0] = 3;
        args.byte_args[1] = 3;
        args.byte_args[2] = 1;
        return call_one(io, PR_REC_27D9_OVL_03D0_0039, &args);
    }
    case 2:
    case 6: {
        pr_rec_27d9_string local = {3, {0}};
        if (call_local_32ca(io, 1) != PR_REC_27D9_OK ||
            io->read_cs_bytes == 0 ||
            io->read_cs_bytes(io->context, 0x27d6u, 3, local.bytes))
            return PR_REC_27D9_BOUNDARY;
        /* SYSTEM 0x0B0B copies the three-byte CS literal into [BP-4]. */
        args.byte_count = 2;
        args.byte_args[0] = mode == 2 ? 0x1d : 1;
        args.byte_args[1] = 0;
        args.string = &local;
        args.has_ds_pointer = 1;
        args.ds_offset = 0x6abeu;
        if (call_one(io, PR_REC_27D9_OVL_03D0_002A, &args) != PR_REC_27D9_OK)
            return PR_REC_27D9_BOUNDARY;
        return party_tail(io, (uint8_t)(mode == 2));
    }
    case 4:
    case 3: {
        pr_rec_27d9_args no_args = {0};
        if (call_local_32ca(io, (uint8_t)(mode == 3 ? 0 : 1)) !=
                PR_REC_27D9_OK ||
            call_one(io, PR_REC_27D9_OVL_03CA_0025, &no_args) !=
                PR_REC_27D9_OK)
            return PR_REC_27D9_BOUNDARY;
        return party_tail(io, 1);
    }
    default:
        return PR_REC_27D9_OK;
    }
}

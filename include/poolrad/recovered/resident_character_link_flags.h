#ifndef POOLRAD_RECOVERED_RESIDENT_CHARACTER_LINK_FLAGS_H
#define POOLRAD_RECOVERED_RESIDENT_CHARACTER_LINK_FLAGS_H

#include "poolrad/recovered/resident_character_link_check.h"

/* START.EXE RES_00BA_2E46 near-calls the reviewed RES_00BA_2E03 before
 * checking another byte in the same indexed DS table. It uses the same
 * far/DS read callbacks and host statuses. Character = [BP+6,+8]. */
pr_rec_character_link_check_status pr_rec_res_00ba_2e46(
    const pr_rec_character_link_check_io *io, pr_rec_far_ptr character,
    uint8_t *result);

#endif

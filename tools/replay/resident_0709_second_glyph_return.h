#ifndef POOLRAD_RESIDENT_0709_SECOND_GLYPH_RETURN_H
#define POOLRAD_RESIDENT_0709_SECOND_GLYPH_RETURN_H

#include "resident_1816_caller_routes.h"

/* Execute the reached mode-one EGA route of RES_0709:01DF from the second
 * glyph entry through RETF 0C at 05E8. Stop at printer 0709:0694.
 * Return 2 for arguments outside the pinned synthetic route. RAM is a flat
 * 20-bit real-mode image; OUT does not emulate EGA hardware planes. */
int pr_01df_second_glyph_return(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu);

/* At the second drawer's 0694 return, execute the two-character printer's
 * final loop check and RETF 0A to the RES_00BA:1858 continuation. */
int pr_0694_final_printer_return(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu);

#endif

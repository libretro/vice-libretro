/*
 * petembedded.c - Code for embedding pet data files.
 *
 * This feature is only active when --enable-embedded is given to the
 * configure script, its main use is to make developing new ports easier
 * and to allow ports for platforms which don't have a filesystem, or a
 * filesystem which is hard/impossible to load data files from.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#ifdef USE_EMBEDDED
#include <string.h>
#include <stdio.h>

#include "embedded.h"
#include "machine.h"
#include "pets.h"
#include "petrom.h"

#include "petbasic1.h"
#include "petbasic2.h"
#include "petbasic4.h"
#include "petchargen1.h"
#include "petchargen2.h"
#include "petedit1g.h"
#include "petedit2b.h"
#include "petedit2g.h"
#include "petedit4b40.h"
#include "petedit4b80.h"
#include "petedit4g40.h"
#include "petkernal1.h"
#include "petkernal2.h"
#include "petkernal4.h"
#include "superpet_char.h"
#include "superpet_waterloo_a000.h"
#include "superpet_waterloo_b000.h"
#include "superpet_waterloo_c000.h"
#include "superpet_waterloo_d000.h"
#include "superpet_waterloo_e000.h"
#include "superpet_waterloo_f000.h"

#include "crtc_amber_vpl.h"
#include "crtc_green_vpl.h"
#include "crtc_white_vpl.h"

static embedded_t petfiles[] = {
    { PET_CHARGEN1_NAME, -0x800, 0x1000, 0x800, pet_chargen1_rom },
    { PET_CHARGEN2_NAME, -0x800, 0x1000, 0x800, pet_chargen2_rom },
    { PET_BASIC4NAME, 0x2000, 0x3000, 0x3000, pet_basic4_rom },
    { PET_KERNAL4NAME, 0x1000, 0x1000, 0x1000, pet_kernal4_rom },
    { PET_EDITOR4B80NAME, -0x800, 0x1000, 0x800, pet_edit4b80_rom },
    { PET_KERNAL1NAME, 0x1000, 0x1000, 0x1000, pet_kernal1_rom },
    { PET_BASIC1NAME, 0x2000, 0x3000, 0x2000, pet_basic1_rom },
    { PET_BASIC2NAME, 0x2000, 0x3000, 0x2000, pet_basic2_rom },
    { PET_KERNAL2NAME, 0x1000, 0x1000, 0x1000, pet_kernal2_rom },
    { PET_EDITOR1G40NAME, -0x800, 0x1000, 0x800, pet_edit1g_rom },
    { PET_EDITOR2B40NAME, -0x800, 0x1000, 0x800, pet_edit2b_rom },
    { PET_EDITOR2G40NAME, -0x800, 0x1000, 0x800, pet_edit2g_rom },
    { PET_EDITOR4B40NAME, -0x800, 0x1000, 0x800, pet_edit4b40_rom },
    { PET_EDITOR4G40NAME, -0x800, 0x1000, 0x800, pet_edit4g40_rom },
    { SUPERPET_CHARGEN_NAME, -0x800, 0x1000, 0x1000, superpet_char_rom },
    { SUPERPET_6809_A_NAME, -0x1000, 0x6000, 0x1000, superpet_waterloo_a000_rom },
    { SUPERPET_6809_B_NAME, -0x1000, 0x5000, 0x1000, superpet_waterloo_b000_rom },
    { SUPERPET_6809_C_NAME, -0x1000, 0x4000, 0x1000, superpet_waterloo_c000_rom },
    { SUPERPET_6809_D_NAME, -0x1000, 0x3000, 0x1000, superpet_waterloo_d000_rom },
    { SUPERPET_6809_E_NAME, -0x800, 0x2000, 0x800, superpet_waterloo_e000_rom },
    { SUPERPET_6809_F_NAME, -0x1000, 0x1000, 0x1000, superpet_waterloo_f000_rom },
    EMBEDDED_LIST_END
};

static embedded_palette_t palette_files[] = {
    { "amber", "amber.vpl", 2, crtc_amber_vpl },
    { "green", "green.vpl", 2, crtc_green_vpl },
    { "white", "white.vpl", 2, crtc_white_vpl },
    EMBEDDED_PALETTE_LIST_END
};

static size_t embedded_match_file(const char *name, unsigned char *dest, int minsize, int maxsize, embedded_t *emb)
{
    int i = 0;

    while (emb[i].name != NULL) {
        if (!strcmp(name, emb[i].name) && minsize == emb[i].minsize && maxsize == emb[i].maxsize) {
            if (emb[i].esrc != NULL) {
                if (emb[i].size != minsize) {
                    memcpy(dest, emb[i].esrc, maxsize);
                } else {
                    memcpy(dest + maxsize - minsize, emb[i].esrc, minsize);
                }
            }
            return emb[i].size;
        }
        i++;
    }
    return 0;
}

size_t embedded_check_file(const char *name, unsigned char *dest, int minsize, int maxsize)
{
    size_t retval;

    if ((retval = embedded_check_extra(name, dest, minsize, maxsize)) != 0) {
        return retval;
    }

    if ((retval = embedded_match_file(name, dest, minsize, maxsize, petfiles)) != 0) {
        return retval;
    }
    return 0;
}

int embedded_palette_load(const char *fname, palette_t *p)
{
    int i = 0;
    int j;
    unsigned char *entries;

    while (palette_files[i].name1 != NULL) {
        if (!strcmp(palette_files[i].name1, fname) || !strcmp(palette_files[i].name2, fname)) {
            entries = palette_files[i].palette;
            for (j = 0; j < palette_files[i].num_entries; j++) {
                p->entries[j].red = entries[(j * 4) + 0];
                p->entries[j].green = entries[(j * 4) + 1];
                p->entries[j].blue = entries[(j * 4) + 2];
            }
            return 0;
        }
        i++;
    }
    return -1;
}
#endif

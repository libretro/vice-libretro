/*
 * c128embedded.c - Code for embedding c128 data files.
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
#include "c64mem.h"
#include "c64rom.h"
#include "c128rom.h"

#define C128_KERNAL_ROM_SIZE        0x2000
#define C128_BASIC_ROM_SIZE         0x8000
#define C128_EDITOR_ROM_SIZE        0x1000
#define C128_Z80BIOS_ROM_SIZE       0x1000
#define C128_CHARGEN_ROM_SIZE       0x2000

#define C128_BASIC_ROM_IMAGELO_SIZE 0x4000
#define C128_BASIC_ROM_IMAGEHI_SIZE 0x4000
#define C128_KERNAL_ROM_IMAGE_SIZE  0x4000

#define C128_KERNAL64_ROM_SIZE      0x2000
#define C128_BASIC64_ROM_SIZE       0x2000

#include "c64basic.h"
#include "c64kernal.h"
#include "c128basichi.h"
#include "c128basiclo.h"
#include "c128kernal.h"
#include "c128chargen.h"

#include "viciipalette.h"
#include "vdc_comp_vpl.h"
#include "vdc_deft_vpl.h"
#include "vdc_scart_vpl.h"

static embedded_t c128files[] = {
    { C128_KERNAL_NAME, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, kernal128_rom },
    { C128_KERNAL_CH_NAME, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, NULL },
    { C128_KERNAL_DE_NAME, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, NULL },
    { C128_KERNAL_FI_NAME, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, NULL },
    { C128_KERNAL_FR_NAME, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, NULL },
    { C128_KERNAL_IT_NAME, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, NULL },
    { C128_KERNAL_NO_NAME, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, NULL },
    { C128_KERNAL_SE_NAME, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE, NULL },
    { C128_CHARGEN_NAME, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, chargen128_rom },
    { C128_CHARGEN_CH_NAME, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, NULL },
    { C128_CHARGEN_DE_NAME, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, NULL },
    { C128_CHARGEN_FI_NAME, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, NULL },
    { C128_CHARGEN_FR_NAME, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, NULL },
    { C128_CHARGEN_IT_NAME, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, NULL },
    { C128_CHARGEN_NO_NAME, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, NULL },
    { C128_CHARGEN_SE_NAME, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE, NULL },
    { C128_BASICLO_NAME, C128_BASIC_ROM_IMAGELO_SIZE, C128_BASIC_ROM_IMAGELO_SIZE, C128_BASIC_ROM_IMAGELO_SIZE, c128basiclo_rom },
    { C128_BASICHI_NAME, C128_BASIC_ROM_IMAGEHI_SIZE, C128_BASIC_ROM_IMAGEHI_SIZE, C128_BASIC_ROM_IMAGEHI_SIZE, c128basichi_rom },
    { C64_BASIC_NAME, C64_BASIC_ROM_SIZE, C64_BASIC_ROM_SIZE, C64_BASIC_ROM_SIZE, basic64_rom },
    { C64_KERNAL_REV3_NAME, C64_KERNAL_ROM_SIZE, C64_KERNAL_ROM_SIZE, C64_KERNAL_ROM_SIZE, kernal64_rom },
    EMBEDDED_LIST_END
};

static embedded_palette_t palette_files[] = {
    VICII_PALETTE_FILES
    { "vdc_comp", "vdc_comp.vpl", 16, vdc_comp_vpl },
    { "vdc_deft", "vdc_deft.vpl", 16, vdc_deft_vpl },
    { "vdc_scart", "vdc_scart.vpl", 16, vdc_scart_vpl },
    EMBEDDED_PALETTE_LIST_END
};

static size_t embedded_match_file(const char *name, unsigned char *dest, int minsize, int maxsize, embedded_t *emb)
{
    int i = 0;

    while (emb[i].name != NULL) {
        /* Search only */
        if (!strcmp(name, emb[i].name) && !minsize && !maxsize)
            return emb[i].size;
        /* Copy data */
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

    if ((retval = embedded_match_file(name, dest, minsize, maxsize, c128files)) != 0) {
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

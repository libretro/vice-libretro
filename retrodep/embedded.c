/*
 * embedded.c - Code for embedding data files.
 *
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
#include "driverom.h"

//#define USE_PRINTER

#ifdef USE_PRINTER
#define NL10_ROM_SIZE 0x8000
#define MAX_ROM_SIZE (8 * 1024)
#include "printer.h"
#endif

#include "drivedos1540.h"
#include "drivedos1541.h"
#include "drived1541II.h"
#include "drivedos1551.h"
#include "drivedos1570.h"
#include "drivedos1571.h"
#include "drived1571cr.h"
#include "drivedos1581.h"
#include "drivedos1001.h"
#include "drivedos2031.h"
#include "drivedos2040.h"
#include "drivedos3040.h"
#include "drivedos4040.h"
#include "drivedos9000.h"
#ifdef USE_PRINTER
#include "printermps803.h"
#include "printernl10cbm.h"
#endif

static embedded_t commonfiles[] = {
#ifdef USE_PRINTER
    { MPS803_ROM_NAME, MAX_ROM_SIZE, MAX_ROM_SIZE, MAX_ROM_SIZE, printerdrv_mps803_rom },
    { NL10_ROM_NAME, NL10_ROM_SIZE, NL10_ROM_SIZE, NL10_ROM_SIZE, printerdrv_nl10_rom },
#endif
    { DRIVE_ROM1540_NAME, DRIVE_ROM1540_SIZE, DRIVE_ROM1540_SIZE_EXPANDED, DRIVE_ROM1540_SIZE, drive_rom1540_rom },
    { DRIVE_ROM1541_NAME, DRIVE_ROM1541_SIZE, DRIVE_ROM1541_SIZE_EXPANDED, DRIVE_ROM1541_SIZE, drive_rom1541_rom },
    { DRIVE_ROM1541II_NAME, DRIVE_ROM1541II_SIZE, DRIVE_ROM1541II_SIZE_EXPANDED, DRIVE_ROM1541II_SIZE, drive_rom1541ii_rom },
    { DRIVE_ROM1551_NAME, DRIVE_ROM1551_SIZE, DRIVE_ROM1551_SIZE, DRIVE_ROM1551_SIZE, drive_rom1551_rom },
    { DRIVE_ROM1570_NAME, DRIVE_ROM1570_SIZE, DRIVE_ROM1570_SIZE, DRIVE_ROM1570_SIZE, drive_rom1570_rom },
    { DRIVE_ROM1571_NAME, DRIVE_ROM1571_SIZE, DRIVE_ROM1571_SIZE, DRIVE_ROM1571_SIZE, drive_rom1571_rom },
    { DRIVE_ROM1571CR_NAME, DRIVE_ROM1571CR_SIZE, DRIVE_ROM1571CR_SIZE, DRIVE_ROM1571CR_SIZE, drive_rom1571cr_rom },
    { DRIVE_ROM1581_NAME, DRIVE_ROM1581_SIZE, DRIVE_ROM1581_SIZE, DRIVE_ROM1581_SIZE, drive_rom1581_rom },
    { DRIVE_ROM1001_NAME, DRIVE_ROM1001_SIZE, DRIVE_ROM1001_SIZE, DRIVE_ROM1001_SIZE, drive_rom1001_rom },
    { DRIVE_ROM2031_NAME, DRIVE_ROM2031_SIZE, DRIVE_ROM2031_SIZE, DRIVE_ROM2031_SIZE, drive_rom2031_rom },
    { DRIVE_ROM2040_NAME, DRIVE_ROM2040_SIZE, DRIVE_ROM2040_SIZE, DRIVE_ROM2040_SIZE, drive_rom2040_rom },
    { DRIVE_ROM3040_NAME, DRIVE_ROM3040_SIZE, DRIVE_ROM3040_SIZE, DRIVE_ROM3040_SIZE, drive_rom3040_rom },
    { DRIVE_ROM4040_NAME, DRIVE_ROM4040_SIZE, DRIVE_ROM4040_SIZE, DRIVE_ROM4040_SIZE, drive_rom4040_rom },
    { DRIVE_ROM9000_NAME, DRIVE_ROM9000_SIZE, DRIVE_ROM9000_SIZE, DRIVE_ROM9000_SIZE, drive_rom9000_rom },
    { NULL }
};

static size_t embedded_match_file(const char *name, unsigned char *dest, int minsize, int maxsize, embedded_t *emb)
{
    int i = 0;
    int load_at_start;

    if (minsize < 0) {
        minsize = -minsize;
        load_at_start = 1;
    } else {
        load_at_start = 0;
    }

    while (emb[i].name != NULL) {
        if (!strcmp(name, emb[i].name) && minsize == emb[i].minsize && maxsize == emb[i].maxsize) {
            if (emb[i].esrc != NULL) {
                if (emb[i].size != minsize || load_at_start) {
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

size_t embedded_check_extra(const char *name, unsigned char *dest, int minsize, int maxsize)
{
    size_t retval;

    if ((retval = embedded_match_file(name, dest, minsize, maxsize, commonfiles)) != 0) {
        return retval;
    }
    return 0;
}
#endif

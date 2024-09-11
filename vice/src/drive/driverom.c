/*
 * driverom.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

/* #define DBGDRIVEROM */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "drive.h"
#include "drivetypes.h"
#include "driverom.h"
#include "log.h"
#include "machine-bus.h"
#include "machine-drive.h"
#include "resources.h"
#include "sysfile.h"
#include "traps.h"
#include "types.h"
#include "snapshot.h"

#ifdef DBGDRIVEROM
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* patch for 1541 driverom at $EAAF */
/* skips RAM and ROM check for fast drive reset */
/*
static unsigned char rompatch[26]=
{
    0x9D, 0x00, 0x01,
    0x9D, 0x00, 0x02,
    0x9D, 0x00, 0x03,
    0x9D, 0x00, 0x04,
    0x9D, 0x00, 0x05,
    0x9D, 0x00, 0x06,
    0x9D, 0x00, 0x07,
    0xE8,
    0xD0, 0xE6,
    0xF0, 0x59
};
*/

/* Logging goes here.  */
static log_t driverom_log;

/* If nonzero, we are far enough in init that we can load ROMs.  */
static int drive_rom_load_ok = 0;


int driverom_load(const char *resource_name, uint8_t *drive_rom, unsigned
                  int *loaded, int min, int max, const char *name,
                  unsigned int type, unsigned int *size)
{
    const char *rom_name = NULL;
    int filesize;
    unsigned int dnr;

    DBG(("driverom_load res:%s loaded:%u min:%d max:%d name:%s type:%u size:%u\n",
       resource_name, *loaded, min, max, name, type, size ? *size : 0));

    if (!drive_rom_load_ok) {
        return 0;
    }

    resources_get_string(resource_name, &rom_name);

    DBG(("driverom_load rom_name: %s\n", rom_name));

    filesize = sysfile_load(rom_name, "DRIVES", drive_rom, min, max);

    if (filesize < 0) {
#ifdef __LIBRETRO__
        log_message(driverom_log, "'%s' ROM image not found. "
                  "Hardware-level %s emulation is not available.", rom_name, name);
#else
        log_error(driverom_log, "%s ROM image not found. "
                  "Hardware-level %s emulation is not available.", name, name);
#endif
        if (size != NULL) {
            *size = 0;
        }
        if (loaded != NULL) {
            *loaded = 0;
        }
        /* disable the drives that used the ROM which could not be loaded */
        for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
            diskunit_context_t *unit = diskunit_context[dnr];
            if (unit->type == type) {
                unit->type = DRIVE_TYPE_NONE;
                drive_disable(diskunit_context[dnr]);
                machine_bus_status_drivetype_set(dnr + 8, 0);
            }
        }
        return -1;
    }

    *loaded = 1;
    if (size != NULL) {
        *size = (unsigned int)filesize;
    }
    /* Align to the end of available space */
    if ((filesize <= min) && (min < max)) {
        DBG(("driverom_load align drive rom\n"));
        memmove(drive_rom, &drive_rom[max - min], min);
    }

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        diskunit_context_t *unit = diskunit_context[dnr];

        if (unit->type == type) {
            DBG(("driverom_load prepare drive rom and reset\n"));
            machine_drive_rom_setup_image(dnr);
            driverom_initialize_traps(diskunit_context[dnr]);
            drive_cpu_trigger_reset(dnr);
        }
    }
    return 0;
}

int driverom_load_images(void)
{
    drive_rom_load_ok = 1;

    machine_drive_rom_load();

    if (machine_drive_rom_check_loaded(DRIVE_TYPE_ANY) < 0) {
        log_error(driverom_log,
                  "No ROM image found at all!  "
                  "Hardware-level emulation is not available.");
        return -1;
    }

    return 0;
}

void driverom_initialize_traps(diskunit_context_t *unit)
{
    memcpy(unit->trap_rom, unit->rom, DRIVE_ROM_SIZE);

    unit->trap = -1;
    unit->trapcont = -1;

    DBG(("driverom_initialize_traps type: %u trap idle: %s\n", unit->type,
           unit->idling_method == DRIVE_IDLE_TRAP_IDLE ? "enabled" : "disabled"));

    if (unit->idling_method != DRIVE_IDLE_TRAP_IDLE) {
        return;
    }

    switch (unit->type) {
        case DRIVE_TYPE_1540:
        case DRIVE_TYPE_1541:
        case DRIVE_TYPE_1541II:
        case DRIVE_TYPE_1570:
        case DRIVE_TYPE_1571:
        case DRIVE_TYPE_1571CR:
            unit->trap = 0xec9b;
            unit->trapcont = 0xebff;
            break;
        case DRIVE_TYPE_1551:
            unit->trap = 0xead9;
            unit->trapcont = 0xeabd;
            break;
        case DRIVE_TYPE_1581:
            unit->trap = 0xb158;
            unit->trapcont = 0xb105;
            break;
        case DRIVE_TYPE_2000:
            unit->trap = 0xf3c0;
            unit->trapcont = 0xf368;
            break;
        case DRIVE_TYPE_4000:
            unit->trap = 0xf3ec;
            unit->trapcont = 0xf394;
            break;
        case DRIVE_TYPE_2031:
            unit->trap = 0xece9;
            unit->trapcont = 0xec4d;
            break;
        case DRIVE_TYPE_2040:
            unit->trap = 0xe2d3;
            unit->trapcont = 0xe27e;
            break;
        case DRIVE_TYPE_3040:
            unit->trap = 0xd508;
            unit->trapcont = 0xd4b8;
            break;
        case DRIVE_TYPE_4040:
            unit->trap = 0xd507;
            unit->trapcont = 0xd4b7;
            break;
        default:
            break;
    }
    if (unit->trap >= 0
        && unit->trap_rom[unit->trap - 0x8000] == 0x4c
        && unit->trap_rom[unit->trap - 0x8000 + 1] == (unit->trapcont & 0xff)
        && unit->trap_rom[unit->trap - 0x8000 + 2] == (unit->trapcont >> 8)) {
        unit->trap_rom[unit->trap - 0x8000] = TRAP_OPCODE;
        if (unit->type == DRIVE_TYPE_1551) {
            unit->trap_rom[0xeabf - 0x8000] = 0xea;
            unit->trap_rom[0xeac0 - 0x8000] = 0xea;
            unit->trap_rom[0xead0 - 0x8000] = 0x08;
        }
        return;
    }
    unit->trap = -1;
    unit->trapcont = -1;
}

/* -------------------------------------------------------------------- */

#define ROM_SNAP_MAJOR 1
#define ROM_SNAP_MINOR 0

/* TODO: pass diskunit_context_t as parameter instead */
int driverom_snapshot_write(snapshot_t *s, const drive_t *drive)
{
    char snap_module_name[10];
    snapshot_module_t *m;
    diskunit_context_t *unit = drive->diskunit;
    const uint8_t *base;
    int len;

    sprintf(snap_module_name, "DRIVEROM%u", drive->diskunit->mynumber);

    m = snapshot_module_create(s, snap_module_name, ROM_SNAP_MAJOR, ROM_SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    switch (unit->type) {
        case DRIVE_TYPE_1540:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM1540_SIZE;
            break;
        case DRIVE_TYPE_1541:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM1541_SIZE;
            break;
        case DRIVE_TYPE_1541II:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM1541II_SIZE;
            break;
        case DRIVE_TYPE_1551:
            base = unit->rom;
            len = DRIVE_ROM1551_SIZE;
            break;
        case DRIVE_TYPE_1570:
            base = unit->rom;
            len = DRIVE_ROM1570_SIZE;
            break;
        case DRIVE_TYPE_1571:
            base = unit->rom;
            len = DRIVE_ROM1571_SIZE;
            break;
        case DRIVE_TYPE_1571CR:
            base = unit->rom;
            len = DRIVE_ROM1571CR_SIZE;
            break;
        case DRIVE_TYPE_1581:
            base = unit->rom;
            len = DRIVE_ROM1581_SIZE;
            break;
        case DRIVE_TYPE_2000:
            base = unit->rom;
            len = DRIVE_ROM2000_SIZE;
            break;
        case DRIVE_TYPE_4000:
            base = unit->rom;
            len = DRIVE_ROM4000_SIZE;
            break;
        case DRIVE_TYPE_CMDHD:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROMCMDHD_SIZE;
            break;
        case DRIVE_TYPE_2031:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM2031_SIZE;
            break;
        case DRIVE_TYPE_2040:
            base = &(unit->rom[DRIVE_ROM_SIZE - DRIVE_ROM2040_SIZE]);
            len = DRIVE_ROM2040_SIZE;
            break;
        case DRIVE_TYPE_3040:
            base = &(unit->rom[DRIVE_ROM_SIZE - DRIVE_ROM3040_SIZE]);
            len = DRIVE_ROM3040_SIZE;
            break;
        case DRIVE_TYPE_4040:
            base = &(unit->rom[DRIVE_ROM_SIZE - DRIVE_ROM4040_SIZE]);
            len = DRIVE_ROM4040_SIZE;
            break;
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM1001_SIZE;
            break;
        case DRIVE_TYPE_9000:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM9000_SIZE;
            break;
        default:
            return -1;
    }

    if (SMW_BA(m, base, len) < 0) {
        if (m != NULL) {
            snapshot_module_close(m);
        }
        return -1;
    }

    return snapshot_module_close(m);
}

int driverom_snapshot_read(snapshot_t *s, drive_t *drive)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    diskunit_context_t *unit = drive->diskunit;
    char snap_module_name[10];
    uint8_t *base;
    int len;

    sprintf(snap_module_name, "DRIVEROM%u", drive->diskunit->mynumber);

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);
    if (m == NULL) {
        return 0;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, ROM_SNAP_MAJOR, ROM_SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        log_error(driverom_log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  ROM_SNAP_MAJOR, ROM_SNAP_MINOR);
        snapshot_module_close(m);
        return -1;
    }

    switch (unit->type) {
        case DRIVE_TYPE_1540:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM1540_SIZE;
            break;
        case DRIVE_TYPE_1541:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM1541_SIZE;
            break;
        case DRIVE_TYPE_1541II:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM1541II_SIZE;
            break;
        case DRIVE_TYPE_1551:
            base = unit->rom;
            len = DRIVE_ROM1551_SIZE;
            break;
        case DRIVE_TYPE_1570:
            base = unit->rom;
            len = DRIVE_ROM1570_SIZE;
            break;
        case DRIVE_TYPE_1571:
            base = unit->rom;
            len = DRIVE_ROM1571_SIZE;
            break;
        case DRIVE_TYPE_1571CR:
            base = unit->rom;
            len = DRIVE_ROM1571CR_SIZE;
            break;
        case DRIVE_TYPE_1581:
            base = unit->rom;
            len = DRIVE_ROM1581_SIZE;
            break;
        case DRIVE_TYPE_2000:
            base = unit->rom;
            len = DRIVE_ROM2000_SIZE;
            break;
        case DRIVE_TYPE_4000:
            base = unit->rom;
            len = DRIVE_ROM4000_SIZE;
            break;
        case DRIVE_TYPE_CMDHD:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROMCMDHD_SIZE;
            break;
        case DRIVE_TYPE_2031:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM2031_SIZE;
            break;
        case DRIVE_TYPE_2040:
            base = &(unit->rom[DRIVE_ROM_SIZE - DRIVE_ROM2040_SIZE]);
            len = DRIVE_ROM2040_SIZE;
            break;
        case DRIVE_TYPE_3040:
            base = &(unit->rom[DRIVE_ROM_SIZE - DRIVE_ROM3040_SIZE]);
            len = DRIVE_ROM3040_SIZE;
            break;
        case DRIVE_TYPE_4040:
            base = &(unit->rom[DRIVE_ROM_SIZE - DRIVE_ROM4040_SIZE]);
            len = DRIVE_ROM4040_SIZE;
            break;
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM1001_SIZE;
            break;
        case DRIVE_TYPE_9000:
            base = &(unit->rom[0x4000]);
            len = DRIVE_ROM9000_SIZE;
            break;
        default:
            return -1;
    }

    if (SMR_BA(m, base, len) < 0) {
        if (m != NULL) {
            snapshot_module_close(m);
        }
        return -1;
    }

    machine_drive_rom_do_checksum(drive->diskunit->mynumber);

    return snapshot_module_close(m);
}

void driverom_init(void)
{
    driverom_log = log_open("DriveROM");
}

/*
 * cpmcart.c
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on code written by
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

#include "vice.h"
#if 0
#include <stdlib.h>

#include "6510core.h"
#include "alarm.h"
#include "c64cia.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#endif
#include "cpmcart.h"
#if 0
#include "daa.h"
#include "export.h"
#include "interrupt.h"
#include "log.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#endif
#include "snapshot.h"
#include "types.h"
#include "z80regs.h"

#ifdef Z80_4MHZ
#define CLK_ADD(clock, amount) clock = z80cpu_clock_add(clock, amount)
#else
#define CLK_ADD(clock, amount) clock += amount
#endif
#if 0
static uint8_t reg_a = 0;
static uint8_t reg_b = 0;
static uint8_t reg_c = 0;
static uint8_t reg_d = 0;
static uint8_t reg_e = 0;
static uint8_t reg_f = 0;
static uint8_t reg_h = 0;
static uint8_t reg_l = 0;
static uint8_t reg_ixh = 0;
static uint8_t reg_ixl = 0;
static uint8_t reg_iyh = 0;
static uint8_t reg_iyl = 0;
static uint16_t reg_sp = 0;
static uint32_t z80_reg_pc = 0;
static uint8_t reg_i = 0;
static uint8_t reg_r = 0;

static uint8_t iff1 = 0;
static uint8_t iff2 = 0;
static uint8_t im_mode = 0;

static uint8_t reg_a2 = 0;
static uint8_t reg_b2 = 0;
static uint8_t reg_c2 = 0;
static uint8_t reg_d2 = 0;
static uint8_t reg_e2 = 0;
static uint8_t reg_f2 = 0;
static uint8_t reg_h2 = 0;
static uint8_t reg_l2 = 0;

static int z80_started = 0;
static int cpmcart_enabled = 0;

static read_func_ptr_t cpmcart_mem_read_tab[0x101];
static store_func_ptr_t cpmcart_mem_write_tab[0x101];

static uint8_t cpmcart_wrap_read(uint16_t addr)
{
    uint32_t address = ((uint32_t)addr + 0x1000) & 0xffff;
    return cpmcart_mem_read_tab[addr >> 8]((uint16_t)address);
}

static void cpmcart_wrap_store(uint16_t addr, uint8_t value)
{
}

static void set_read_item(int index, uint8_t (*func)(uint16_t addr))
{
}

static void set_write_item(int index, void (*func)(uint16_t addr, uint8_t val))
{
}

static void cpmcart_mem_init(void)
{
}
#endif
#if 0
static void cpmcart_io_store(uint16_t addr, uint8_t byte)
{
}
#endif
int cpmcart_cart_enabled(void)
{
    return 0;
}
#if 0
static int cpmcart_dump(void)
{
    return 0;
}
#endif
#if 0
static io_source_t cpmcart_device = {
    "CP/M Cartridge",
    IO_DETACH_RESOURCE,
    "CPMCart",
    0xde00, 0xdeff, 0xff,
    0,
    cpmcart_io_store,
    NULL, /* no read */
    NULL, /* no peek */
    cpmcart_dump,
    CARTRIDGE_CPM,
    0,
    0
};

static io_source_list_t *cpmcart_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_CPM, 0, 0, &cpmcart_device, NULL, CARTRIDGE_CPM
};

static int set_cpmcart_enabled(int value, void *param)
{
    return 0;
}

static const resource_int_t resources_int[] = {
    { "CPMCart", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &cpmcart_enabled, set_cpmcart_enabled, NULL },
    RESOURCE_INT_LIST_END
};
#endif
int cpmcart_resources_init(void)
{
    return 0;
}

#if 0
static const cmdline_option_t cmdline_options[] =
{
    { "-cpmcart", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CPMCart", (resource_value_t)1,
      NULL, "Enable the CP/M cartridge" },
    { "+cpmcart", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CPMCart", (resource_value_t)0,
      NULL, "Disable the CP/M cartridge" },
    CMDLINE_LIST_END
};
#endif
int cpmcart_cmdline_options_init(void)
{
    return 0;
}
#if 0
struct cpmcart_ba_s {
    cpmcart_ba_check_callback_t *check;
    cpmcart_ba_steal_callback_t *steal;
    int *cpu_ba;
    int cpu_ba_mask;
    int enabled;
};

static struct cpmcart_ba_s cpmcart_ba = {
    NULL, NULL, NULL, 0, 0
};
#endif
void cpmcart_ba_register(cpmcart_ba_check_callback_t *ba_check,
                         cpmcart_ba_steal_callback_t *ba_steal,
                         int *ba_var, int ba_mask)
{
}

void cpmcart_reset(void)
{
}

#if 0
#ifdef Z80_4MHZ
static int z80_half_cycle = 0;

inline static CLOCK z80cpu_clock_add(CLOCK clock, int amount)
{
    return 0;
}

void cpmcart_clock_stretch(void)
{
}
#endif

/* ------------------------------------------------------------------------- */
#endif
z80_regs_t z80_regs;
#if 0
static void import_registers(void)
{
}

static void export_registers(void)
{
}

/* Extended opcodes.  */

static void opcode_cb(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint16_t ip12, uint16_t ip23)
{
}

static void opcode_dd_cb(uint8_t iip2, uint8_t iip3, uint16_t iip23)
{
}

static void opcode_dd(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint16_t ip12, uint16_t ip23)
{
}

static void opcode_ed(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint16_t ip12, uint16_t ip23)
{
}

static void opcode_fd_cb(uint8_t iip2, uint8_t iip3, uint16_t iip23)
{
}

static void opcode_fd(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint16_t ip12, uint16_t ip23)
{
}

/* ------------------------------------------------------------------------- */

/* Z80 mainloop.  */

static void cpmcart_mainloop(interrupt_cpu_status_t *cpu_int_status, alarm_context_t *cpu_alarm_context)
{
}
#endif
void cpmcart_check_and_run_z80(void)
{
}
#if 0
/* ------------------------------------------------------------------------- */

/* CPMCART snapshot module format:

   type  | name           | description
   ------------------------------------
   DWORD | CLK            | main CPU clock
   BYTE  | A              | A register
   BYTE  | B              | B register
   BYTE  | C              | C register
   BYTE  | D              | D register
   BYTE  | E              | E register
   BYTE  | F              | F register
   BYTE  | H              | H register
   BYTE  | L              | L register
   BYTE  | IXH            | IXH register
   BYTE  | IXL            | IXL register
   BYTE  | IYH            | IYH register
   BYTE  | IYL            | IYL register
   WORD  | SP             | stack pointer register
   DWORD | PC             | program counter register
   BYTE  | I              | I register
   BYTE  | R              | R register
   BYTE  | IFF1           | IFF1 register
   BYTE  | IFF2           | IFF2 register
   BYTE  | im mode        | im mode flag
   BYTE  | A2             | A2 register
   BYTE  | B2             | B2 register
   BYTE  | C2             | C2 register
   BYTE  | D2             | D2 register
   BYTE  | E2             | E2 register
   BYTE  | F2             | F2 register
   BYTE  | H2             | H2 register
   BYTE  | L2             | L2 register
   DWORD | opcode info    | last opcode info
   DWORD | opcode address | last opcode address
 */

static char snap_module_name[] = "CPMCART";
#define SNAP_MAJOR 0
#define SNAP_MINOR 0
#endif
int cpmcart_snapshot_write_module(snapshot_t *s)
{
    return -1;
}

int cpmcart_snapshot_read_module(snapshot_t *s)
{
    return -1;
}

int cpmcart_enable(void)
{
    return 0;
}

int cpmcart_disable(void)
{
    return 0;
}

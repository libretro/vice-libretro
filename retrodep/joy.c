/*
 * joy.c - Joystick support for libretro
 *
 */

#include "vice.h"

#include <stdio.h>

#include "cmdline.h"
#include "keyboard.h"
#include "joy.h"
#include "joystick.h"
#include "machine.h"
#include "resources.h"
#include "types.h"

/* ------------------------------------------------------------------------- */

int joystick_arch_init_resources(void)
{
    return 0;
}

int joy_arch_resources_init(void)
{
    return 0;
}

int joy_arch_cmdline_options_init(void)
{
    return 0;
}

int joystick_init_cmdline_options(void)
{
    return 0;
}

/* ------------------------------------------------------------------------- */

int joy_arch_set_device(int port, int new_dev)
{
    return 0;
}

int joy_arch_init(void)
{
    return 0;
}

void joystick_update(void)
{
}

/* ------------------------------------------------------------------------- */

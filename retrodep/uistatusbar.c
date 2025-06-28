/*
 * uistatusbar.c - SDL statusbar.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
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

#include <stdio.h>

#include "resources.h"
#include "types.h"
#include "ui.h"
#include "uiapi.h"
#include "uistatusbar.h"
#include "videoarch.h"
#include "vsync.h"
#include "vsyncapi.h"
#include "joystick.h"
#include "archdep.h"
#include "drive.h"
#include "keyboard.h"
#include "keymap.h"

#include "libretro-core.h"
#include "libretro-graph.h"
#include "libretro-mapper.h"

extern unsigned int mouse_value[2 + 1];
extern unsigned int vice_led_state[RETRO_LED_NUM];
extern unsigned int opt_joyport_type;
extern unsigned int opt_autoloadwarp;
extern int RGBc(int r, int g, int b);
extern bool retro_statusbar;
extern bool retro_capslock;
extern bool c128_capslock;
extern float retro_refresh;
extern int runstate;
extern dc_storage *dc;

#if defined(__XVIC__)
extern int vic20mem_forced;
#endif

/* ----------------------------------------------------------------- */
/* static functions/variables */

#define STATUSBAR_JOY_POS           0
#define STATUSBAR_TAPE_POS          53
#define STATUSBAR_DRIVE_POS         59
#define STATUSBAR_DRIVE8_TRACK_POS  59
#define STATUSBAR_DRIVE9_TRACK_POS  57
#define STATUSBAR_DRIVE10_TRACK_POS 55
#define STATUSBAR_DRIVE11_TRACK_POS 53
#define STATUSBAR_PAUSE_POS         61
#define STATUSBAR_SPEED_POS         61
#define MAX_STATUSBAR_LEN           64

unsigned char statusbar_text[RETRO_PATH_MAX] = {0};
static unsigned char statusbar_chars[MAX_STATUSBAR_LEN] = {0};
static unsigned char statusbar_resolution[10] = {0};
static unsigned char statusbar_model[10] = {0};
static unsigned char statusbar_memory[10] = {0};

static unsigned char* joystick_value_human(char val, int vice_device)
{
    static unsigned char str[6] = {0};
    snprintf(str, sizeof(str), "%3s", "   ");

    if (val & JOYPAD_N)
        str[1] = 30;

    else if (val & JOYPAD_S)
        str[1] = 28;

    if (val & JOYPAD_W)
        str[0] = 27;

    else if (val & JOYPAD_E)
        str[2] = 29;

    if (vice_device == 0)
    {
        str[1] = (val & JOYPAD_FIRE)  ? (str[1] | 0x80) : str[1];
        str[1] = (val & JOYPAD_FIRE2) ? ('2' | 0x80) : str[1];
        str[1] = (val & JOYPAD_FIRE3) ? ('3' | 0x80) : str[1];
    }
    else if (vice_device > 0)
    {
        str[1] = (val & JOYPAD_FIRE)  ? ('L' | 0x80) : str[1];
        str[1] = (val & JOYPAD_FIRE2) ? ('R' | 0x80) : str[1];
        str[1] = (val & JOYPAD_FIRE3) ? ('M' | 0x80) : str[1];
    }

    return str;
}


static int per = 0;
static int fps = 0;
static char fps_str[3] = {0};
static int warp = 0;
static int paused = 0;

static void display_speed(void)
{
    sprintf(&statusbar_chars[STATUSBAR_SPEED_POS], "%2s", fps_str);

    if (uistatusbar_state & UISTATUSBAR_ACTIVE) {
        uistatusbar_state |= UISTATUSBAR_REPAINT;
    }
}

unsigned int statusbar_message_timer = 0;
static int drive_enabled[NUM_DISK_UNITS] = {0};
static int drive_empty[NUM_DISK_UNITS] = {0};
static int drive_pwm[NUM_DISK_UNITS] = {0};

static void clear_drive_statusbar_chars(signed char unit)
{
    unsigned char i;
    unsigned char i_max = (unit < 0) ? NUM_DISK_UNITS : unit + 1;

    if (unit > -1)
        i = unit;

    for (i = i; i < i_max; i++)
    {
        if (drive_enabled[i] && drive_empty[i])
        {
            switch (i)
            {
                case 1:
                    statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS] = '#';
                    statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS + 1] = '9';
                    break;
                case 2:
                    statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS] = '#';
                    statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS + 1] = '0';
                    break;
                case 3:
                    statusbar_chars[STATUSBAR_DRIVE11_TRACK_POS] = '#';
                    statusbar_chars[STATUSBAR_DRIVE11_TRACK_POS + 1] = '1';
                    break;
                default:
                case 0:
                    statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS] = '#';
                    statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS + 1] = '8';
                    break;
            }
        }
        else if (drive_enabled[i] && !drive_empty[i])
        {
            switch (i)
            {
                case 1:
                    statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS] = '0';
                    statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS + 1] = '0';
                    break;
                case 2:
                    statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS] = '0';
                    statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS + 1] = '0';
                    break;
                case 3:
                    statusbar_chars[STATUSBAR_DRIVE11_TRACK_POS] = '0';
                    statusbar_chars[STATUSBAR_DRIVE11_TRACK_POS + 1] = '0';
                    break;
                default:
                case 0:
                    statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS] = '0';
                    statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS + 1] = '0';
                    break;
            }
        }
    }
}

void display_current_image(const char *image, bool inserted)
{
    static char imagename[RETRO_PATH_MAX] = {0};
    static char imagename_prev[RETRO_PATH_MAX] = {0};
    unsigned char* imagename_local;

    if (strcmp(image, ""))
    {
        snprintf(imagename, sizeof(imagename), "%.100s", image);
        snprintf(imagename_prev, sizeof(imagename_prev), "%.100s", imagename);
    }
    else
    {
        if (strcmp(imagename_prev, ""))
            snprintf(imagename, sizeof(imagename), "%.100s", imagename_prev);
    }

    if (strcmp(imagename, ""))
    {
        imagename_local = utf8_to_local_string_alloc(imagename);
        snprintf(statusbar_text, sizeof(statusbar_text), "%s%.98s", "  ", imagename_local);

        /* Skip the initial insert message with forced message mode */
        if (runstate || (!runstate && !(opt_statusbar & STATUSBAR_MESSAGES)))
            statusbar_message_timer = 2 * retro_refresh;

        if (inserted)
            statusbar_text[0] = (7 | 0x80);
        else if (!strcmp(image, ""))
            statusbar_text[0] = (8 | 0x80);

        free(imagename_local);
        imagename_local = NULL;
    }

    if (dc_get_image_type(dc->files[dc->index]) != DC_IMAGE_TYPE_FLOPPY)
        drive_empty[0] = true;

    clear_drive_statusbar_chars(0);
}

/* ----------------------------------------------------------------- */
/* ui.h */
static void ui_display_speed(void)
{
    double vsync_metric_cpu_percent;
    double vsync_metric_emulated_fps;
    int vsync_metric_warp_enabled;

    vsyncarch_get_metrics(&vsync_metric_cpu_percent, &vsync_metric_emulated_fps, &vsync_metric_warp_enabled);

    per = (int)(vsync_metric_cpu_percent + .5);
    if (per > 999) {
        per = 999;
    }
    
    fps = (int)(vsync_metric_emulated_fps + .5);
    if (fps > 999) {
        fps /= 1000;
        fps = (fps > 9) ? 9 : fps;
        snprintf(fps_str, sizeof(fps_str), "%dK", fps);
    } else if (fps > 99) {
        fps /= 100;
        fps = (fps > 9) ? 9 : fps;
        snprintf(fps_str, sizeof(fps_str), "%dC", fps);
    } else {
        snprintf(fps_str, sizeof(fps_str), "%2d", fps);
    }

    warp = vsync_metric_warp_enabled;

    display_speed();
}

void ui_display_paused(int flag)
{
    paused = flag;

    display_speed();
}

void ui_display_reset(int device, int mode)
{
}

/* ----------------------------------------------------------------- */
/* uiapi.h */

/* Display a mesage without interrupting emulation */
void ui_display_statustext(const char *text, bool fade_out)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: \"%s\", %i\n", __func__, text, fade_out);
#endif
}

/* Drive related UI */
void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color)
{
    int drive_number;
    int drive_state = (int)state;

    for (drive_number = 0; drive_number < 4; ++drive_number) {
        drive_enabled[drive_number] = state & (1 << drive_number);
        if (drive_state & 1) {
            ui_display_drive_led(drive_number, 0, 0, 0);
        }
        drive_state >>= 1;
    }

#if !defined(__X64DTV__)
    /* Never disable #8 */
    drive_enabled[0] = 1;
#endif

    if (uistatusbar_state & UISTATUSBAR_ACTIVE) {
        uistatusbar_state |= UISTATUSBAR_REPAINT;
    }
}

void ui_display_drive_track(
      unsigned int drive_number, unsigned int drive_base, unsigned int half_track_number, unsigned int disk_side)
{
    if (drive_empty[drive_number] || !drive_enabled[drive_number])
        return;

    unsigned int track_number = half_track_number / 2;

#ifdef SDL_DEBUG
    fprintf(stderr, "%s\n", __func__);
#endif

    switch (drive_number) {
        case 1:
            statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS] = (track_number / 10) + '0';
            statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS + 1] = (track_number % 10) + '0';
            break;
        case 2:
            statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS] = (track_number / 10) + '0';
            statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS + 1] = (track_number % 10) + '0';
            break;
        case 3:
            statusbar_chars[STATUSBAR_DRIVE11_TRACK_POS] = (track_number / 10) + '0';
            statusbar_chars[STATUSBAR_DRIVE11_TRACK_POS + 1] = (track_number % 10) + '0';
            break;
        default:
        case 0:
            statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS] = (track_number / 10) + '0';
            statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS + 1] = (track_number % 10) + '0';
            break;
    }

    if (uistatusbar_state & UISTATUSBAR_ACTIVE) {
        uistatusbar_state |= UISTATUSBAR_REPAINT;
    }
}

/* The pwm value will vary between 0 and 1000.  */
void ui_display_drive_led(unsigned int drive_number, unsigned int drive_base, unsigned int pwm1, unsigned int led_pwm2)
{
    drive_pwm[drive_number] = pwm1;
    vice_led_state[RETRO_LED_DRIVE] = (drive_pwm[drive_number] > 1) ? 1 : 0;
    return;

    char c;
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: drive %i, pwm1 = %i, led_pwm2 = %u\n", __func__, drive_number, pwm1, led_pwm2);
#endif

    c = "8901"[drive_number] | ((pwm1 > 500) ? 0x80 : 0);
    statusbar_chars[STATUSBAR_DRIVE_POS + (drive_number * 5)] = c;
    statusbar_chars[STATUSBAR_DRIVE_POS + (drive_number * 5) + 1] = 'T';

    if (uistatusbar_state & UISTATUSBAR_ACTIVE) {
        uistatusbar_state |= UISTATUSBAR_REPAINT;
    }
}

void ui_display_drive_current_image(unsigned int unit_number, unsigned int drive_number, const char *image)
{
    drive_empty[unit_number] = !image[0];

    if (dc_get_image_type(image) != DC_IMAGE_TYPE_FLOPPY)
        drive_empty[unit_number] = 1;

    clear_drive_statusbar_chars(unit_number);

#ifdef SDL_DEBUG
    fprintf(stderr, "%s\n", __func__);
#endif
}

/* Tape related UI */

int tape_enabled = 0;
int tape_control = 0;
int tape_counter = 0;
int tape_motor = 0;

static void display_tape(void)
{
    const char tape_chars[5] = {23, 20, 22, 21, 'R'};
    unsigned char tmpstr[5]  = {0};

    if (tape_enabled)
        vice_led_state[RETRO_LED_TAPE] = (tape_control == 1 && tape_motor) ? 1 : 0;
    else
        tape_control = 0;

    if (tape_enabled)
        snprintf(tmpstr, sizeof(tmpstr), "%c%03d", tape_chars[tape_control], tape_counter);

    /* Skip null terminator */
    strncpy(&statusbar_chars[STATUSBAR_TAPE_POS], tmpstr, sizeof(tmpstr)-1);

    if (uistatusbar_state & UISTATUSBAR_ACTIVE) {
        uistatusbar_state |= UISTATUSBAR_REPAINT;
    }
}

void ui_set_tape_status(int port, int tape_status)
{
    tape_enabled = tape_status;

    display_tape();
}

void ui_display_tape_motor_status(int port, int motor)
{
    tape_motor = motor;

    display_tape();
}

void ui_display_tape_control_status(int port, int control)
{
    tape_control = control;

    display_tape();
}

void ui_display_tape_counter(int port, int counter)
{
    if (tape_counter != counter) {
        tape_counter = (counter < 1000) ? counter : 0;
        display_tape();

        if (tape_motor)
           tape_motor = 2;
    }
}

void ui_display_tape_current_image(int port, const char *image)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %s\n", __func__, image);
#endif
}

/* Recording UI */
void ui_display_playback(int playback_status, char *version)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %i, \"%s\"\n", __func__, playback_status, version);
#endif
}

void ui_display_recording(int recording_status)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %i\n", __func__, recording_status);
#endif
}

void ui_display_event_time(unsigned int current, unsigned int total)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %i, %i\n", __func__, current, total);
#endif
}

/* Joystick UI */
void ui_display_joyport(uint16_t *joyport)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %02x %02x %02x %02x %02x\n", __func__, joyport[0], joyport[1], joyport[2], joyport[3], joyport[4]);
#endif
}

/* Volume UI */
void ui_display_volume(int vol)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s: %i\n", __func__, vol);
#endif
}

/* ----------------------------------------------------------------- */
/* resources */

static int statusbar_enabled;

static int set_statusbar(int val, void *param)
{
    statusbar_enabled = val ? 1 : 0;

    if (statusbar_enabled) {
        uistatusbar_open();
    } else {
        uistatusbar_close();
    }

    return 0;
}

static const resource_int_t resources_int[] = {
    { "SDLStatusbar", 0, RES_EVENT_NO, NULL,
      &statusbar_enabled, set_statusbar, NULL },
    RESOURCE_INT_LIST_END
};

int uistatusbar_init_resources(void)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s\n", __func__);
#endif
    return resources_register_int(resources_int);
}

/* Joyport bar */
static void display_joyport(void)
{
    unsigned char tmpstr[25] = {0};
    unsigned char tmplen = 0;

#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XVIC__)
    unsigned char joy1[2];
    unsigned char joy2[2];
    snprintf(joy1, sizeof(joy1), "%s", "1");
    snprintf(joy2, sizeof(joy2), "%s", "2");

    /* Lightpen/gun */
    if (opt_joyport_type > 10 && cur_port == 1)
        tmplen += snprintf(tmpstr, sizeof(tmpstr), "L%s%3s ", joy1, joystick_value_human(mouse_value[1], 1));
    /* Mouse */
    else if (opt_joyport_type > 2 && cur_port == 1)
        tmplen += snprintf(tmpstr, sizeof(tmpstr), "M%s%3s ", joy1, joystick_value_human(mouse_value[1], 1));
    /* Paddles */
    else if (opt_joyport_type == 2)
        tmplen += snprintf(tmpstr, sizeof(tmpstr), "P%s%3s ", joy1, joystick_value_human(mouse_value[1], 1));
    /* Joystick */
    else
        tmplen += snprintf(tmpstr, sizeof(tmpstr), "J%s%3s ", joy1, joystick_value_human(get_joystick_value(1-1), 0));

    /* Lightpen/gun */
    if (opt_joyport_type > 10 && cur_port == 2)
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "L%s%3s ", joy2, joystick_value_human(mouse_value[2], 1));
    /* Mouse */
    else if (opt_joyport_type > 2 && cur_port == 2)
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "M%s%3s ", joy2, joystick_value_human(mouse_value[2], 1));
    /* Paddles */
    else if (opt_joyport_type == 2)
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "P%s%3s ", joy2, joystick_value_human(mouse_value[2], 1));
    /* Joystick */
    else
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "J%s%3s ", joy2, joystick_value_human(get_joystick_value(2-1), 0));
#elif defined(__XVIC__)
    char joy1[2];
    snprintf(joy1, sizeof(joy1), "%s", "1");

    /* Lightpen/gun */
    if (opt_joyport_type > 10)
       tmplen += snprintf(tmpstr, sizeof(tmpstr), "L%s%3s ", joy1, joystick_value_human(mouse_value[1], 1));
    /* Mouse */
    else if (opt_joyport_type > 2)
       tmplen += snprintf(tmpstr, sizeof(tmpstr), "M%s%3s ", joy1, joystick_value_human(mouse_value[1], 1));
    /* Paddles */
    else if (opt_joyport_type == 2)
       tmplen += snprintf(tmpstr, sizeof(tmpstr), "P%s%3s ", joy1, joystick_value_human(mouse_value[1], 1));
    /* Joystick */
    else
       tmplen += snprintf(tmpstr, sizeof(tmpstr), "J%s%3s ", joy1, joystick_value_human(get_joystick_value(1-1), 0));

    tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "%6s", "");
#endif

#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XVIC__)
    if (vice_opt.UserportJoyType != -1)
    {
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "J%d%3s ", 3, joystick_value_human(get_joystick_value(3-1), 0));
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "J%d%3s ", 4, joystick_value_human(get_joystick_value(4-1), 0));
    }
#elif defined(__XVIC__)
    if (vice_opt.UserportJoyType != -1)
    {
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "J%d%3s ", 2, joystick_value_human(get_joystick_value(2-1), 0));
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "J%d%3s ", 3, joystick_value_human(get_joystick_value(3-1), 0));
    }
#elif defined(__XPET__) || defined(__XCBM2__)
    if (vice_opt.UserportJoyType != -1)
    {
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "J%d%3s ", 1, joystick_value_human(get_joystick_value(1-1), 0));
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "J%d%3s ", 2, joystick_value_human(get_joystick_value(2-1), 0));
    }
    else
    {
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "%6s", "");
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "%6s", "");
    }
#endif

#if !defined(__XPET__) && !defined(__XCBM2__)
    if (vice_opt.UserportJoyType == -1)
#endif
    {
        int offset_multi = ceil((retrow - retrow_crop + 8) / 16.0f);
        int offset = 7 - offset_multi;

        for (int i = 0; i < offset + 1; i++)
            tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "%1s", "");

#if defined(__X128__)
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "%2s %2s", "CL", "SL");

        if (c128_capslock)
        {
            tmpstr[6+6+offset+1] |= 0x80;
            tmpstr[6+6+offset+2] |= 0x80;
        }
#else
        tmplen += snprintf(tmpstr + tmplen, sizeof(tmpstr) - tmplen, "%2s %2s", "  ", "SL");
#endif
        if (retro_capslock)
        {
            tmpstr[6+6+offset+3+1] |= 0x80;
            tmpstr[6+6+offset+3+2] |= 0x80;
        }
    }

    if (opt_statusbar & STATUSBAR_BASIC)
        snprintf(tmpstr, sizeof(tmpstr), "%24s", "");

    snprintf(&statusbar_chars[STATUSBAR_JOY_POS], sizeof(statusbar_chars), "%-51s", tmpstr);

    if (opt_statusbar & STATUSBAR_BASIC)
        return;

    tmpstr[0] = '\0';

    /* Resolution */
    snprintf(statusbar_resolution, sizeof(statusbar_resolution), "%dx%d", retrow_crop, retroh_crop);

#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__) || defined(__X128__)
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
    /* Model */
    unsigned model = vice_opt.Model;
    if (request_model_set > -1 && request_model_set != model)
        model = request_model_set;

    switch (model)
    {
        default:
        case C64MODEL_C64_PAL:
        case C64MODEL_C64_PAL_N:
        case C64MODEL_C64_OLD_PAL:
        case C64MODEL_C64_NTSC:
        case C64MODEL_C64_OLD_NTSC:
            strcpy(tmpstr, "  C64"); break;
        case C64MODEL_C64C_PAL:
        case C64MODEL_C64C_NTSC:
            strcpy(tmpstr, " C64C"); break;
        case C64MODEL_C64SX_PAL:
        case C64MODEL_C64SX_NTSC:
            strcpy(tmpstr, "SX-64"); break;
        case C64MODEL_PET64_PAL:
        case C64MODEL_PET64_NTSC:
            strcpy(tmpstr, "PET64"); break;
        case C64MODEL_C64_GS:
            strcpy(tmpstr, "C64GS"); break;
        case C64MODEL_C64_JAP:
            strcpy(tmpstr, "C64JP"); break;
        case C64MODEL_ULTIMAX:
            strcpy(tmpstr, "  MAX"); break;
    }
#elif defined(__X128__)
    /* Model */
    unsigned model = vice_opt.Model;
    if (request_model_set > -1 && request_model_set != model)
        model = request_model_set;

    switch (model)
    {
        default:
        case C128MODEL_C128_PAL:
        case C128MODEL_C128_NTSC:
            strcpy(tmpstr, " C128"); break;
        case C128MODEL_C128D_PAL:
        case C128MODEL_C128D_NTSC:
            strcpy(tmpstr, "C128D"); break;
        case C128MODEL_C128DCR_PAL:
        case C128MODEL_C128DCR_NTSC:
            strcpy(tmpstr, "CDCR"); break;
    }
#endif

    /* Memory */
    float memory_size = 0;
    const char *memory_unit = {0};
#if defined(__XSCPU64__)
    memory_size = vice_opt.SIMMSize * 1024;
#else
    memory_size = vice_opt.REUsize;
#endif

    if (memory_size > 512)
    {
        memory_size /= 1024;
        memory_unit = "M";
    }
    else
        memory_unit = "K";

    snprintf(statusbar_memory, sizeof(statusbar_memory), (memory_size > 0 && memory_size < 10) ? "%3.1f%s" : "%3.0f%s", memory_size, memory_unit);
    snprintf(statusbar_model, sizeof(statusbar_model), "%-5s", tmpstr);
#elif defined(__XVIC__)
    /* Model */
    unsigned model = vice_opt.Model;
    if (request_model_set > -1 && request_model_set != model)
        model = request_model_set;

    switch (model)
    {
        case VIC20MODEL_VIC20_PAL:
        case VIC20MODEL_VIC20_NTSC:
            strcpy(tmpstr, "VIC20"); break;
        case VIC20MODEL_VIC21:
            strcpy(tmpstr, "VIC21"); break;
    }

    /* Memory */
    unsigned memory = 0;
    const char *memory_unit = "K";
    memory = (vic20mem_forced > -1) ? vic20mem_forced : (vice_opt.VIC20Memory > -1) ? vice_opt.VIC20Memory : 0;
    if (!memory && vice_opt.Model == VIC20MODEL_VIC21)
        memory = 3;

    int vic20mems[6] = {0, 3, 8, 16, 24, 35};

    snprintf(statusbar_memory, sizeof(statusbar_memory), "%2d%s", vic20mems[memory], memory_unit);
    snprintf(statusbar_model, sizeof(statusbar_model), "%-5s", tmpstr);
#endif

    if (uistatusbar_state & UISTATUSBAR_ACTIVE) {
        uistatusbar_state |= UISTATUSBAR_REPAINT;
    }
}

/* ----------------------------------------------------------------- */
/* uistatusbar.h */

int uistatusbar_state = 0;

void uistatusbar_open(void)
{
    uistatusbar_state = UISTATUSBAR_ACTIVE | UISTATUSBAR_REPAINT;
}

void uistatusbar_close(void)
{
    uistatusbar_state = UISTATUSBAR_REPAINT;
}


#define COLOR_BLACK   0
#define COLOR_WHITE   RGBc(255, 255, 255)
#define COLOR_RED     RGBc(204,   0,   0)
#define COLOR_GREEN_B RGBc(  0, 204,   0)
#define COLOR_GREEN   RGBc(  0,  85,   0)
#define COLOR_GREEN_D RGBc(  0,  34,   0)
#define COLOR_BROWN   RGBc(143, 140, 129)
#define COLOR_BROWN_D RGBc( 89,  79,  78)

#define LED_WIDTH(chars) ((char_width + (led_padding * char_scale_x)) * chars) + (led_padding * char_scale_x * 2)

void uistatusbar_draw(void)
{
    unsigned int i = 0;
    unsigned char c = 0;
    unsigned char s[2] = {0};

    unsigned int char_width = 6;
    unsigned int char_offset = 1;
    unsigned int char_scale_x = 1;
    int x = 0, y = 0;

    unsigned int color_f, color_b;
    unsigned int color_black, color_white,
            color_red, color_greenb, color_green, color_greend,
            color_brown, color_brownd;

    color_black  = COLOR_BLACK;
    color_white  = COLOR_WHITE;
    color_red    = COLOR_RED;
    color_greenb = COLOR_GREEN_B;
    color_green  = COLOR_GREEN;
    color_greend = COLOR_GREEN_D;
    color_brown  = COLOR_BROWN;
    color_brownd = COLOR_BROWN_D;
    color_f      = color_white;
    color_b      = color_black;

    if (retrow >= 704)
       char_scale_x = 2;

    char_width  *= char_scale_x;
    char_offset *= char_scale_x;

    /* LED section position */
    int led_x       = 0;
    int led_width   = 0;
    int led_padding = 1;

    /* Statusbar position */
    x = 0;
    if (opt_statusbar & STATUSBAR_TOP)
        y = retroYS_crop_offset + led_padding + 1;
    else
        y = retroh_crop + retroYS_crop_offset - 8 - led_padding;

    /* Statusbar background */
    int bkg_x      = retroXS_offset;
    int bkg_y      = y - (led_padding * 2);
    int bkg_width  = retrow_crop;
    int bkg_height = 9 + (led_padding * 2);

    /* Right alignment offset */
    int x_align_offset = char_scale_x > 1 ? (char_scale_x * 2) + (char_scale_x / 2) : char_scale_x * 2;

    /* Power LED size */
    led_width = LED_WIDTH(2) + char_scale_x;

    /* Drive LED size */
    if (drive_enabled[0])
    {
        unsigned char drive_i;

        for (drive_i = 0; drive_i < NUM_DISK_UNITS; drive_i++)
        {
            if (drive_enabled[drive_i])
                led_width += LED_WIDTH(2);
        }
    }

    /* Tape LED size */
    if (tape_enabled)
        led_width += LED_WIDTH(4) - char_scale_x;

    /* LED horizontal start */
    led_x = retroXS_crop_offset + retrow_crop - led_width + char_scale_x - 1;

    /* Basic mode statusbar background */
    if (opt_statusbar & STATUSBAR_BASIC && !statusbar_message_timer)
    {
        bkg_width = led_width;
        bkg_x     = led_x;
    }

    /* Black background paint */
    draw_fbox(bkg_x, bkg_y, bkg_width, bkg_height, 0, GRAPH_ALPHA_100);

    /* Update FPS */
    ui_display_speed();

    /* Update joyports + misc */
    display_joyport();

    /* Message */
    if (statusbar_message_timer)
    {
        draw_text(bkg_x + char_offset, y, color_f, color_b, GRAPH_ALPHA_100, GRAPH_BG_ALL, char_scale_x, 1, 100, statusbar_text);
        draw_fbox(led_x, bkg_y, led_width, bkg_height, 0, GRAPH_ALPHA_100);
    }
    else if (!(opt_statusbar & STATUSBAR_BASIC))
    {
        int8_t resolution_pos = -20;
        uint8_t memory_pos    = 30;
        uint8_t model_pos     = 60;
        uint16_t max_pos      = bkg_x + (retrow_crop / 2) + (model_pos * char_scale_x);

        /* Sacrifice memory slot if there is not enough width */
        if (led_x < max_pos)
        {
            memory_pos = 0;
            model_pos  = 34;
        }

        if (resolution_pos)
            draw_text(bkg_x + (retrow_crop / 2) + (resolution_pos * char_scale_x), y, color_f, color_b, GRAPH_ALPHA_100, GRAPH_BG_ALL, char_scale_x, 1, 10, statusbar_resolution);
        if (memory_pos)
            draw_text(bkg_x + (retrow_crop / 2) + (memory_pos * char_scale_x), y, color_f, color_b, GRAPH_ALPHA_100, GRAPH_BG_ALL, char_scale_x, 1, 10, statusbar_memory);
        if (model_pos)
            draw_text(bkg_x + (retrow_crop / 2) + (model_pos * char_scale_x), y, color_f, color_b, GRAPH_ALPHA_100, GRAPH_BG_ALL, char_scale_x, 1, 10, statusbar_model);
    }

    /* Tape indicator + drive & power LEDs */
    for (i = 0; i < MAX_STATUSBAR_LEN; ++i)
    {
        c = statusbar_chars[i];
        if (c == 0)
            continue;
        
        if (statusbar_message_timer && i < STATUSBAR_TAPE_POS - 1)
            continue;

        if (!retro_statusbar && statusbar_message_timer && (opt_statusbar & STATUSBAR_MESSAGES))
            continue;

        /* Default colors */
        color_f = color_white;
        color_b = color_black;

        /* Drive/tape LED color */
        if (i >= STATUSBAR_TAPE_POS)
        {
            if (tape_enabled && i < STATUSBAR_TAPE_POS + 4)
            {
                if (tape_motor)
                    color_b = color_brown;
                else
                    color_b = color_brownd;
            }
        }

        /* Drive loading */
        if (   ((i == STATUSBAR_DRIVE8_TRACK_POS  || i == STATUSBAR_DRIVE8_TRACK_POS  + 1) && drive_enabled[0])
            || ((i == STATUSBAR_DRIVE9_TRACK_POS  || i == STATUSBAR_DRIVE9_TRACK_POS  + 1) && drive_enabled[1])
            || ((i == STATUSBAR_DRIVE10_TRACK_POS || i == STATUSBAR_DRIVE10_TRACK_POS + 1) && drive_enabled[2])
            || ((i == STATUSBAR_DRIVE11_TRACK_POS || i == STATUSBAR_DRIVE11_TRACK_POS + 1) && drive_enabled[3])
           )
        {
            unsigned char unit = 0;

            if (i == STATUSBAR_DRIVE9_TRACK_POS || i == STATUSBAR_DRIVE9_TRACK_POS + 1)
                unit = 1;
            else if (i == STATUSBAR_DRIVE10_TRACK_POS || i == STATUSBAR_DRIVE10_TRACK_POS + 1)
                unit = 2;
            else if (i == STATUSBAR_DRIVE11_TRACK_POS || i == STATUSBAR_DRIVE11_TRACK_POS + 1)
                unit = 3;

            color_b = color_green;

            /* Switch drives around left-to-right */
            if (drive_enabled[3])
            {
                switch (unit)
                {
                    case 0:
                        unit = 3;
                        if (i == STATUSBAR_DRIVE8_TRACK_POS)
                            c = statusbar_chars[STATUSBAR_DRIVE11_TRACK_POS];
                        else if (i == STATUSBAR_DRIVE8_TRACK_POS + 1)
                            c = statusbar_chars[STATUSBAR_DRIVE11_TRACK_POS + 1];
                        break;
                    case 1:
                        unit = 2;
                        if (i == STATUSBAR_DRIVE9_TRACK_POS)
                            c = statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS];
                        else if (i == STATUSBAR_DRIVE9_TRACK_POS + 1)
                            c = statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS + 1];
                        break;
                    case 2:
                        unit = 1;
                        if (i == STATUSBAR_DRIVE10_TRACK_POS)
                            c = statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS];
                        else if (i == STATUSBAR_DRIVE10_TRACK_POS + 1)
                            c = statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS + 1];
                        break;
                    case 3:
                        unit = 0;
                        if (i == STATUSBAR_DRIVE11_TRACK_POS)
                            c = statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS];
                        else if (i == STATUSBAR_DRIVE11_TRACK_POS + 1)
                            c = statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS + 1];
                        break;
                }
            }
            else if (drive_enabled[2])
            {
                switch (unit)
                {
                    case 0:
                        unit = 2;
                        if (i == STATUSBAR_DRIVE8_TRACK_POS)
                            c = statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS];
                        else if (i == STATUSBAR_DRIVE8_TRACK_POS + 1)
                            c = statusbar_chars[STATUSBAR_DRIVE10_TRACK_POS + 1];
                        break;
                    case 1:
                        unit = 1;
                        if (i == STATUSBAR_DRIVE9_TRACK_POS)
                            c = statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS];
                        else if (i == STATUSBAR_DRIVE9_TRACK_POS + 1)
                            c = statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS + 1];
                        break;
                    case 2:
                        unit = 0;
                        if (i == STATUSBAR_DRIVE10_TRACK_POS)
                            c = statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS];
                        else if (i == STATUSBAR_DRIVE10_TRACK_POS + 1)
                            c = statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS + 1];
                        break;
                }
            }
            else if (drive_enabled[1])
            {
                switch (unit)
                {
                    case 0:
                        unit = 1;
                        if (i == STATUSBAR_DRIVE8_TRACK_POS)
                            c = statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS];
                        else if (i == STATUSBAR_DRIVE8_TRACK_POS + 1)
                            c = statusbar_chars[STATUSBAR_DRIVE9_TRACK_POS + 1];
                        break;
                    case 1:
                        unit = 0;
                        if (i == STATUSBAR_DRIVE9_TRACK_POS)
                            c = statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS];
                        else if (i == STATUSBAR_DRIVE9_TRACK_POS + 1)
                            c = statusbar_chars[STATUSBAR_DRIVE8_TRACK_POS + 1];
                        break;
                }
            }

            if (drive_empty[unit])
            {
                color_f = color_green;
                color_b = color_greend;
            }

            if (drive_pwm[unit] > 1)
                color_b = color_greenb;

            if (opt_statusbar & STATUSBAR_MINIMAL)
                c = ' ';
        }
        /* Power LED color */
        else if (i == STATUSBAR_SPEED_POS || i == STATUSBAR_SPEED_POS + 1)
        {
            color_b = color_red;

            if (opt_statusbar & STATUSBAR_MINIMAL)
                c = ' ';
        }

        /* Character background mode */
        int graph_bg = (i >= STATUSBAR_TAPE_POS) ? GRAPH_BG_OUTLINE : (c & 0x80) ? GRAPH_BG_ALL : GRAPH_BG_NONE;

        /* Right alignment for tape/drive/power */
        int x_align = retroXS_offset;
#if defined(__XVIC__)
        /* VIC-20 crop horizontal centering shenanigans */
        if (i >= STATUSBAR_TAPE_POS && retroXS_offset && retro_region != RETRO_REGION_PAL)
            x_align -= retroXS_crop_offset;
#endif
        if (i >= STATUSBAR_TAPE_POS)
            x_align += x_align_offset + retrow_crop - (MAX_STATUSBAR_LEN * char_width);

        if (drive_enabled[0])
        {
            if (i == STATUSBAR_DRIVE8_TRACK_POS || i == STATUSBAR_DRIVE8_TRACK_POS + 1)
                x_align -= 4 * char_scale_x;

            if (drive_enabled[1])
            {
                if (i == STATUSBAR_DRIVE9_TRACK_POS || i == STATUSBAR_DRIVE9_TRACK_POS + 1)
                    x_align -= 8 * char_scale_x;
            }

            /* Drives 10 & 11 only shown if tape is not in use */
            if (!tape_enabled)
            {
                if (drive_enabled[2])
                {
                    if (i == STATUSBAR_DRIVE10_TRACK_POS || i == STATUSBAR_DRIVE10_TRACK_POS + 1)
                        x_align -= 12 * char_scale_x;
                }
                if (drive_enabled[3])
                {
                    if (i == STATUSBAR_DRIVE11_TRACK_POS || i == STATUSBAR_DRIVE11_TRACK_POS + 1)
                        x_align -= 16 * char_scale_x;
                }
            }
            else if (tape_enabled)
            {
                signed char x_extra = char_width;

                if (drive_enabled[1])
                    x_extra += char_width * 2;
                else
                    x_extra -= (char_width - 1) * 2;

                if (i == STATUSBAR_TAPE_POS)
                    x_align -= ((char_width + x_extra + 1) * char_scale_x) - char_width;
                else if (i >= STATUSBAR_TAPE_POS && i < STATUSBAR_TAPE_POS + 4)
                    x_align -= ((char_width + x_extra) * char_scale_x) - char_width;
            }
        }
        else
        if (tape_enabled)
        {
            if (i == STATUSBAR_TAPE_POS)
                x_align -= (3 * char_scale_x) - (char_width * 4);
            else if (i > STATUSBAR_TAPE_POS && i < STATUSBAR_TAPE_POS + 4)
                x_align -= (2 * char_scale_x) - (char_width * 4);
        }

        int x_char = x + char_offset + x_align + (i * char_width);

        /* Output */
        snprintf(s, sizeof(s), "%c", c);

        /* LED color background boxes */
        if (i >= STATUSBAR_TAPE_POS)
        {
            /* 11 10 09 08 sp */
            /* >tape 09 08 sp */
            int8_t box_start = 0;
            int8_t box_end   = 0;

            switch (i)
            {
                case STATUSBAR_SPEED_POS:
                case STATUSBAR_DRIVE8_TRACK_POS:
                case STATUSBAR_DRIVE9_TRACK_POS:
                case STATUSBAR_DRIVE10_TRACK_POS:
                case STATUSBAR_DRIVE11_TRACK_POS:
                    box_start = char_scale_x;
                    break;

                case STATUSBAR_SPEED_POS + 1:
                case STATUSBAR_DRIVE8_TRACK_POS + 1:
                case STATUSBAR_DRIVE9_TRACK_POS + 1:
                case STATUSBAR_DRIVE10_TRACK_POS + 1:
                case STATUSBAR_DRIVE11_TRACK_POS + 1:
                    box_start = -char_scale_x;
                    box_end   = char_scale_x;
                    break;
            }

            if (tape_enabled)
            {
                if (i > STATUSBAR_TAPE_POS && i < STATUSBAR_TAPE_POS + 3)
                {
                    if (i > STATUSBAR_TAPE_POS + 1)
                        box_start = -char_scale_x;
                    else
                        box_start = 0;
                    box_end = 0;
                }
            }

            draw_fbox(x_char - box_start, y - 1, char_width + char_scale_x + box_start + box_end, char_width / char_scale_x + 3, color_b, GRAPH_ALPHA_100);
        }

        draw_text(x_char, y, color_f, color_black + 1, GRAPH_ALPHA_100, graph_bg, char_scale_x, 1, 10, s);
    }
}

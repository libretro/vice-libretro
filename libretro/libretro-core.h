#ifndef LIBRETRO_CORE_H
#define LIBRETRO_CORE_H

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef USE_LIBRETRO_VFS
#include <streams/file_stream_transforms.h>
#endif

#include "libretro-glue.h"
#include "libretro-dc.h"
#include "string/stdstring.h"
#include "file/file_path.h"
#include "encodings/utf.h"
#include "compat/strcasestr.h"

/* Log */
#if defined(__ANDROID__) || defined(ANDROID)
#include <android/log.h>
#define LOG_TAG "RetroArch.vice"
#endif

/* File helpers functions */
#define RETRO_PATH_MAX 512

/* Types */
#define UINT16 uint16_t
#define UINT32 uint32_t
typedef uint32_t uint32;
typedef uint8_t uint8;

#define RGB565(r, g, b) ((((r>>3)<<11) | ((g>>2)<<5) | (b>>3)))
#define RGB888(r, g, b) (((r * 255 / 31) << 16) | ((g * 255 / 31) << 8) | (b * 255 / 31))
#define ARGB888(a, r, g, b) ((a << 24) | (r << 16) | (g << 8) | b)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Screen */
#if defined(__X128__)
#define WINDOW_WIDTH  856
#define WINDOW_HEIGHT 312
#elif defined(__XPET__)
#define WINDOW_WIDTH  704
#define WINDOW_HEIGHT 288
#elif defined(__XCBM2__)
#define WINDOW_WIDTH  704
#define WINDOW_HEIGHT 366
#elif defined(__XVIC__)
#define WINDOW_WIDTH  448
#define WINDOW_HEIGHT 288
#else
#define WINDOW_WIDTH  384
#define WINDOW_HEIGHT 288
#endif
#define RETRO_BMP_SIZE (WINDOW_WIDTH * WINDOW_HEIGHT * 2)
extern unsigned short int retro_bmp[RETRO_BMP_SIZE];
extern unsigned short int pix_bytes;

#define MANUAL_CROP_OPTIONS \
   { \
      { "0", NULL }, \
      { "1", NULL }, { "2", NULL }, { "3", NULL }, { "4", NULL }, { "5", NULL }, { "6", NULL }, { "7", NULL }, { "8", NULL }, { "9", NULL }, { "10", NULL }, \
      { "11", NULL }, { "12", NULL }, { "13", NULL }, { "14", NULL }, { "15", NULL }, { "16", NULL }, { "17", NULL }, { "18", NULL }, { "19", NULL }, { "20", NULL }, \
      { "21", NULL }, { "22", NULL }, { "23", NULL }, { "24", NULL }, { "25", NULL }, { "26", NULL }, { "27", NULL }, { "28", NULL }, { "29", NULL }, { "30", NULL }, \
      { "31", NULL }, { "32", NULL }, { "33", NULL }, { "34", NULL }, { "35", NULL }, { "36", NULL }, { "37", NULL }, { "38", NULL }, { "39", NULL }, { "40", NULL }, \
      { "41", NULL }, { "42", NULL }, { "43", NULL }, { "44", NULL }, { "45", NULL }, { "46", NULL }, { "47", NULL }, { "48", NULL }, { "49", NULL }, { "50", NULL }, \
      { "51", NULL }, { "52", NULL }, { "53", NULL }, { "54", NULL }, { "55", NULL }, { "56", NULL }, { "57", NULL }, { "58", NULL }, { "59", NULL }, { "60", NULL }, \
      { NULL, NULL }, \
   }

#define PALETTE_GAMMA_OPTIONS \
   { \
      { "1000", "1.00" }, { "1100", "1.10" }, { "1200", "1.20" }, { "1300", "1.30" }, { "1400", "1.40" }, \
      { "1500", "1.50" }, { "1600", "1.60" }, { "1700", "1.70" }, { "1800", "1.80" }, { "1900", "1.90" }, \
      { "2000", "2.00" }, { "2100", "2.10" }, { "2200", "2.20" }, { "2300", "2.30" }, { "2400", "2.40" }, \
      { "2500", "2.50" }, { "2600", "2.60" }, { "2700", "2.70" }, { "2800", "2.80" }, { "2900", "2.90" }, \
      { "3000", "3.00" }, { "3100", "3.10" }, { "3200", "3.20" }, { "3300", "3.30" }, { "3400", "3.40" }, \
      { "3500", "3.50" }, { "3600", "3.60" }, { "3700", "3.70" }, { "3800", "3.80" }, { "3900", "3.90" }, \
      { "4000", "4.00" }, \
      { NULL, NULL }, \
   }

#define PALETTE_COLOR_OPTIONS \
   { \
      { "20", "1%" }, { "40", "2%" }, { "60", "3%" }, { "80", "4%" }, { "100", "5%" }, \
      { "120", "6%" }, { "140", "7%" }, { "160", "8%" }, { "180", "9%" }, { "200", "10%" }, \
      { "220", "11%" }, { "240", "12%" }, { "260", "13%" }, { "280", "14%" }, { "300", "15%" }, \
      { "320", "16%" }, { "340", "17%" }, { "360", "18%" }, { "380", "19%" }, { "400", "20%" }, \
      { "420", "21%" }, { "440", "22%" }, { "460", "23%" }, { "480", "24%" }, { "500", "25%" }, \
      { "520", "26%" }, { "540", "27%" }, { "560", "28%" }, { "580", "29%" }, { "600", "30%" }, \
      { "620", "31%" }, { "640", "32%" }, { "660", "33%" }, { "680", "34%" }, { "700", "35%" }, \
      { "720", "36%" }, { "740", "37%" }, { "760", "38%" }, { "780", "39%" }, { "800", "40%" }, \
      { "820", "41%" }, { "840", "42%" }, { "860", "43%" }, { "880", "44%" }, { "900", "45%" }, \
      { "920", "46%" }, { "940", "47%" }, { "960", "48%" }, { "980", "49%" }, { "1000", "50%" }, \
      { "1020", "51%" }, { "1040", "52%" }, { "1060", "53%" }, { "1080", "54%" }, { "1100", "55%" }, \
      { "1120", "56%" }, { "1140", "57%" }, { "1160", "58%" }, { "1180", "59%" }, { "1200", "60%" }, \
      { "1220", "61%" }, { "1240", "62%" }, { "1260", "63%" }, { "1280", "64%" }, { "1300", "65%" }, \
      { "1320", "66%" }, { "1340", "67%" }, { "1360", "68%" }, { "1380", "69%" }, { "1400", "70%" }, \
      { "1420", "71%" }, { "1440", "72%" }, { "1460", "73%" }, { "1480", "74%" }, { "1500", "75%" }, \
      { "1520", "76%" }, { "1540", "77%" }, { "1560", "78%" }, { "1580", "79%" }, { "1600", "80%" }, \
      { "1620", "81%" }, { "1640", "82%" }, { "1660", "83%" }, { "1680", "84%" }, { "1700", "85%" }, \
      { "1720", "86%" }, { "1740", "87%" }, { "1760", "88%" }, { "1780", "89%" }, { "1800", "90%" }, \
      { "1820", "91%" }, { "1840", "92%" }, { "1860", "93%" }, { "1880", "94%" }, { "1900", "95%" }, \
      { "1920", "96%" }, { "1940", "97%" }, { "1960", "98%" }, { "1980", "99%" }, { "2000", "100%" }, \
      { NULL, NULL }, \
   }

#define ANALOG_STICK_SPEED_OPTIONS \
   { \
      { "0.1", "10%" }, { "0.2", "20%" }, { "0.3", "30%" }, { "0.4", "40%" }, { "0.5", "50%" },  \
      { "0.6", "60%" }, { "0.7", "70%" }, { "0.8", "80%" }, { "0.9", "90%" }, { "1.0", "100%" }, \
      { "1.1", "110%" },{ "1.2", "120%" },{ "1.3", "130%" },{ "1.4", "140%" },{ "1.5", "150%" }, \
      { "1.6", "160%" },{ "1.7", "170%" },{ "1.8", "180%" },{ "1.9", "190%" },{ "2.0", "200%" }, \
      { "2.1", "210%" },{ "2.2", "220%" },{ "2.3", "230%" },{ "2.4", "240%" },{ "2.5", "250%" }, \
      { "2.6", "260%" },{ "2.7", "270%" },{ "2.8", "280%" },{ "2.9", "290%" },{ "3.0", "300%" }, \
      { NULL, NULL }, \
   }

/* Statusbar */
#define STATUSBAR_BOTTOM   0x01
#define STATUSBAR_TOP      0x02
#define STATUSBAR_BASIC    0x04
#define STATUSBAR_MINIMAL  0x08
#define STATUSBAR_MESSAGES 0x10

/* Autoloadwarp */
#define AUTOLOADWARP_DISK 0x01
#define AUTOLOADWARP_TAPE 0x02
#define AUTOLOADWARP_MUTE 0x04

/* Variables */
extern unsigned int retro_renderloop;
extern unsigned int retroXS;
extern unsigned int retroYS;
extern unsigned int retroXS_offset;
extern unsigned int retroYS_offset;
extern unsigned int retrow;
extern unsigned int retroh;
extern unsigned int retrow_crop;
extern unsigned int retroh_crop;
extern int retroXS_crop_offset;
extern int retroYS_crop_offset;
extern unsigned int statusbar_message_timer;
extern unsigned int opt_statusbar;
extern unsigned int cur_port;
extern unsigned int retro_region;
extern int request_model_set;
extern bool pending_AutostartTapeBasicLoad;

extern int tape_enabled;
extern int tape_counter;
extern int tape_control;
extern int tape_motor;

extern unsigned int retro_warpmode;
extern int crop_id;
extern int crop_id_prev;
extern bool crop_delay;

#define CROP_NONE            0
#define CROP_SMALL           1
#define CROP_MEDIUM          2
#define CROP_MAXIMUM         3
#define CROP_MANUAL          4
#define CROP_AUTO            5
#define CROP_AUTO_DISABLE    6

#define CROP_MODE_BOTH       0
#define CROP_MODE_VERTICAL   1
#define CROP_MODE_HORIZONTAL 2
#define CROP_MODE_16_9       3
#define CROP_MODE_16_10      4
#define CROP_MODE_4_3        5
#define CROP_MODE_5_4        6

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
/* PAL: 384x272, NTSC: 384x247, VIC-II: 320x200 */
#include "viciitypes.h"
#include "vicii-timing.h"
#define CROP_WIDTH_MAX   320
#define CROP_HEIGHT_MAX  200
#define CROP_TOP_BORDER  VICII_NO_BORDER_FIRST_DISPLAYED_LINE - vicii.first_displayed_line
#define CROP_LEFT_BORDER vicii.screen_leftborderwidth
#if defined(__X128__)
#define CROP_VDC_WIDTH_MAX   640
#define CROP_VDC_HEIGHT_MAX  200
#define CROP_VDC_TOP_BORDER  44
#define CROP_VDC_LEFT_BORDER 108
#endif
#elif defined(__XVIC__)
/* PAL: 448x284, NTSC: 400x234, VIC: 352x184 */
#include "victypes.h"
#include "vic-timing.h"
#define CROP_WIDTH_MAX   352
#define CROP_HEIGHT_MAX  184
#define CROP_TOP_BORDER_PAL  VIC_PAL_NO_BORDER_FIRST_DISPLAYED_LINE  - vic.first_displayed_line
#define CROP_TOP_BORDER_NTSC VIC_NTSC_NO_BORDER_FIRST_DISPLAYED_LINE - vic.first_displayed_line
#define CROP_TOP_BORDER  CROP_TOP_BORDER_PAL
#define CROP_LEFT_BORDER vic.screen_leftborderwidth
#elif defined(__XPLUS4__)
/* PAL: 384x288, NTSC: 384x242, TED: 320x200 */
#include "tedtypes.h"
#include "ted-timing.h"
#define CROP_WIDTH_MAX   320
#define CROP_HEIGHT_MAX  200
#define CROP_TOP_BORDER_PAL  TED_PAL_NO_BORDER_FIRST_DISPLAYED_LINE  - ted.first_displayed_line
#define CROP_TOP_BORDER_NTSC TED_NTSC_NO_BORDER_FIRST_DISPLAYED_LINE - ted.first_displayed_line
#define CROP_TOP_BORDER  CROP_TOP_BORDER_PAL
#define CROP_LEFT_BORDER ted.screen_leftborderwidth
#else /* CRTC */
#define CROP_WIDTH_MAX   320
#define CROP_HEIGHT_MAX  200
#define CROP_TOP_BORDER  33
#define CROP_LEFT_BORDER 32
#endif

/* LED interface */
enum
{
   RETRO_LED_POWER = 0,
   RETRO_LED_DRIVE,
   RETRO_LED_TAPE,
   RETRO_LED_NUM
};

/* Runstate */
enum
{
   RUNSTATE_FIRST_START = 0,
   RUNSTATE_LOADED_CONTENT,
   RUNSTATE_RUNNING
};

/* Retropad options */
enum
{
   RETROPAD_OPTIONS_DISABLED = 0,
   RETROPAD_OPTIONS_ROTATE,
   RETROPAD_OPTIONS_JUMP,
   RETROPAD_OPTIONS_ROTATE_JUMP
};

/* Functions */
extern long retro_ticks(void);
extern void reload_restart(void);
extern void emu_reset(int type);
extern int RGBc(int r, int g, int b);
extern void display_retro_message(const char *message);
extern void statusbar_message_show(signed char icon, const char *format, ...);
extern void set_variable(const char *key, const char *value);
extern char* get_variable(const char *key);

extern void emu_function(int function);
enum EMU_FUNCTIONS
{
   EMU_VKBD = 0,
   EMU_STATUSBAR,
   EMU_JOYPORT,
   EMU_RESET,
   EMU_FREEZE,
   EMU_SAVE_DISK,
   EMU_ASPECT_RATIO,
   EMU_CROP,
   EMU_TURBO_FIRE,
   EMU_WARP_MODE,
   EMU_DATASETTE_HOTKEYS,
   EMU_DATASETTE_STOP,
   EMU_DATASETTE_START,
   EMU_DATASETTE_FORWARD,
   EMU_DATASETTE_REWIND,
   EMU_DATASETTE_RESET,
   EMU_FUNCTION_COUNT
};

/* VICE options */
struct vice_core_options
{
   int Model;
   int UserportJoyType;
   int AutostartWarp;
   int AttachDevice8Readonly;
   int EasyFlashWriteCRT;
   int Printer;
   int VirtualDevices;
   int DriveTrueEmulation;
   int DriveSoundEmulation;
   int DatasetteSound;
   int AudioLeak;
   int SoundSampleRate;
   int SidEngine;
   int SidModel;
   int SidExtra;
   int SidResidSampling;
   int SidResidPassband;
   int SidResidGain;
   int SidResidFilterBias;
   int SidResid8580FilterBias;
   int SFXSoundExpanderChip;
   char ExternalPalette[RETRO_PATH_MAX];
   int Filter;
   int FilterOddLinePhase;
   int FilterOddLineOffset;
   int ColorGamma;
   int ColorTint;
   int ColorSaturation;
   int ColorContrast;
   int ColorBrightness;
#if !defined(__XPET__)
   char CartridgeFile[RETRO_PATH_MAX];
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
   int REUsize;
#endif
#if defined(__X128__)
   int VDC64KB;
   int VDCFilter;
   int C128ColumnKey;
   int Go64Mode;
#elif defined(__XSCPU64__)
   int SIMMSize;
   int SpeedSwitch;
#elif defined(__XVIC__)
   int VIC20Memory;
#endif
};

extern struct vice_core_options vice_opt;

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
#define AUDIOLEAK_RESOURCE "VICIIAudioLeak"
#elif defined(__XVIC__)
#define AUDIOLEAK_RESOURCE "VICAudioLeak"
#elif defined(__XPLUS4__)
#define AUDIOLEAK_RESOURCE "TEDAudioLeak"
#elif defined(__XPET__) || defined(__XCBM2__)
#define AUDIOLEAK_RESOURCE "CrtcAudioLeak"
#endif

struct vice_raster_s
{
   unsigned first_line;
   unsigned first_line_prev;
   unsigned first_line_maybe;
   unsigned first_line_active;

   unsigned last_line;
   unsigned last_line_prev;
   unsigned last_line_maybe;
   unsigned last_line_active;

   unsigned counter;
   unsigned blanked;
};

extern struct vice_raster_s vice_raster;

/* Dynamic cartridge core option info */
struct vice_cart_info
{
   char *value;
   char *label;
};

/* VICE includes */
#if defined(__X64__) || defined(__X64SC__)
#define DRIVE_TYPE_DEFAULT 1542
#include "c64.h"
#include "c64mem.h"
#include "c64model.h"
#elif defined(__XSCPU64__)
#define DRIVE_TYPE_DEFAULT 1542
#include "scpu64.h"
#include "scpu64mem.h"
#include "c64model.h"
#elif defined(__X64DTV__)
#define DRIVE_TYPE_DEFAULT 0
#include "c64dtv.h"
#include "c64dtvmem.h"
#include "c64dtvmodel.h"
#elif defined(__X128__)
#define DRIVE_TYPE_DEFAULT 1571
#include "c128.h"
#include "c128mem.h"
#include "c128model.h"
#elif defined(__XVIC__)
#define DRIVE_TYPE_DEFAULT 1540
#include "vic20.h"
#include "vic20mem.h"
#include "vic20model.h"
#include "vic20cart.h"
#include "vic20-generic.h"
#elif defined(__XPLUS4__)
#define DRIVE_TYPE_DEFAULT 1551
#include "plus4.h"
#include "plus4model.h"
#elif defined(__XCBM2__) || defined(__XCBM5x0__)
#define DRIVE_TYPE_DEFAULT 2031
#include "cbm2.h"
#include "cbm2model.h"
#elif defined(__XPET__)
#define DRIVE_TYPE_DEFAULT 2031
#include "pet.h"
#include "petmodel.h"
#endif

#define DISK_IMAGE_TYPE_FS 0xFFFF

#endif /* LIBRETRO_CORE_H */

/*
 * ui.c - libretro user interface.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "vice.h"
#include "machine.h"
#include "archdep.h"
#include "cmdline.h"
#include "uistatusbar.h"
#include "resources.h"
#include "sid.h"
#include "sid-resources.h"
#include "util.h"
#if !defined(__XCBM5x0__)
#include "userport_joystick.h"
#endif

#if defined(__XPET__)
#include "petmodel.h"
#include "keyboard.h"
#elif defined(__XCBM2__) || defined(__XCBM5x0__)
#include "cbm2model.h"
#elif defined(__XPLUS4__)
#include "plus4model.h"
#elif defined(__XVIC__)
#include "vic20model.h"
#include "vic20mem.h"
#elif defined(__X128__)
#include "c64model.h"
#include "c128model.h"
#include "c128rom.h"
#include "c128mem.h"
#include "c128kernal.h"
#include "c128kernal64.h"
BYTE c128memrom_kernal128_rom_original[C128_KERNAL_ROM_IMAGE_SIZE] = {0};
BYTE c128memrom_kernal64_rom_original[C128_KERNAL64_ROM_SIZE] = {0};
#elif defined(__XSCPU64__)
#include "c64model.h"
#include "scpu64.h"
#include "scpu64mem.h"
#include "scpu64rom.h"
BYTE scpu64rom_scpu64_rom_original[SCPU64_SCPU64_ROM_MAXSIZE] = {0};
#elif defined(__X64DTV__)
#include "c64dtvmodel.h"
#else
#include "c64model.h"
#include "c64rom.h"
#include "c64mem.h"
#include "c64memrom.h"
BYTE c64memrom_kernal64_rom_original[C64_KERNAL_ROM_SIZE] = {0};
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
extern unsigned int opt_jiffydos_kernal_skip;
#endif

#include "libretro-core.h"
#if defined(__XVIC__)
extern void vic20mem_set(void);
#elif defined(__XSCPU64__)
extern unsigned int opt_supercpu_kernal;
#endif

extern bool retro_ui_finalized;
extern unsigned int opt_jiffydos;
extern unsigned int opt_autoloadwarp;
extern char retro_system_data_directory[RETRO_PATH_MAX];
extern retro_log_printf_t log_cb;

static const cmdline_option_t cmdline_options[] = {
   { NULL }
};

/* Initialization  */
int ui_resources_init(void)
{
   if (machine_class != VICE_MACHINE_VSID)
      return uistatusbar_init_resources();
   return 0;
}

void ui_resources_shutdown(void)
{
}

int ui_init(int argc, char **argv)
{
   return 0;
}

void ui_shutdown(void)
{
}

void ui_check_mouse_cursor(void)
{
}

char* ui_get_file(const char *format, ...)
{
   return NULL;
}

void ui_message(const char* format, ...)
{
   char text[512];
   va_list ap;

   if (format == NULL)
      return;

   va_start(ap, format);
   vsprintf(text, format, ap);
   va_end(ap);
   log_cb(RETRO_LOG_INFO, "%s\n", text);
}

void ui_error(const char *format, ...)
{
   char text[512];
   va_list ap;

   if (format == NULL)
      return;

   va_start(ap, format);
   vsprintf(text, format, ap);
   va_end(ap);
   log_cb(RETRO_LOG_ERROR, "%s\n", text);
   display_retro_message(text);
}

int ui_emulation_is_paused(void)
{
   return 0;
}

void ui_pause_emulation(int flag)
{
}

void ui_update_menus(void)
{
}

int ui_extend_image_dialog(void)
{
   return 0;
}

void ui_dispatch_events(void)
{
}

int ui_cmdline_options_init(void)
{
   return cmdline_register_options(cmdline_options);
}

int ui_init_finish(void)
{
   return 0;
}

extern int log_resources_set_int(const char *name, int value);
extern int log_resources_set_string(const char *name, const char* value);

int ui_init_finalize(void)
{
   /* Dump machine specific defaults for 'vicerc' usage, if not already dumped */
   char resources_dump_path[RETRO_PATH_MAX] = {0};
   snprintf(resources_dump_path, sizeof(resources_dump_path), "%s%s%s%s",
         retro_system_data_directory, FSDEV_DIR_SEP_STR, "vicerc-dump-", machine_get_name());
   if (!util_file_exists(resources_dump_path))
      resources_dump(resources_dump_path);

   /* Mute sound at startup to hide 6581 ReSID init pop, and set back to 100 in retro_run() after 3 frames */
   resources_set_int("SoundVolume", 0);

   /* Sensible defaults */
   log_resources_set_int("SoundFragmentSize", SOUND_FRAGMENT_SMALL);
   log_resources_set_int("Mouse", 1);
   log_resources_set_int("Printer4", 1);
   log_resources_set_int("AutostartPrgMode", 1);
   log_resources_set_int("AutostartDelayRandom", 0);
   log_resources_set_int("FSDeviceLongNames", 1);

   /* Machine specific defaults */
#if defined(__X64DTV__)
   log_resources_set_int("Drive8Type", 0);
#elif defined(__X128__)
   log_resources_set_int("VDCStretchVertical", 0);
#elif defined(__XPET__) || defined(__XCBM2__)
   log_resources_set_int("CrtcStretchVertical", 0);
#endif

   /*** Core options ***/

   /* Video */
#if defined(__XVIC__)
   if (!strcmp(vice_opt.ExternalPalette, "default"))
      log_resources_set_int("VICExternalPalette", 0);
   else
   {
      log_resources_set_string("VICPaletteFile", vice_opt.ExternalPalette);
      log_resources_set_int("VICExternalPalette", 1);
   }
#elif defined(__XPLUS4__)
   if (!strcmp(vice_opt.ExternalPalette, "default"))
      log_resources_set_int("TEDExternalPalette", 0);
   else
   {
      log_resources_set_string("TEDPaletteFile", vice_opt.ExternalPalette);
      log_resources_set_int("TEDExternalPalette", 1);
   }
#elif defined(__XPET__)
   if (!strcmp(vice_opt.ExternalPalette, "default"))
      log_resources_set_int("CrtcExternalPalette", 0);
   else
   {
      log_resources_set_string("CrtcPaletteFile", vice_opt.ExternalPalette);
      log_resources_set_int("CrtcExternalPalette", 1);
   }
#elif defined(__XCBM2__)
   if (!strcmp(vice_opt.ExternalPalette, "default"))
      log_resources_set_int("CrtcExternalPalette", 0);
   else
   {
      log_resources_set_string("CrtcPaletteFile", vice_opt.ExternalPalette);
      log_resources_set_int("CrtcExternalPalette", 1);
   }
#elif !defined(__X64DTV__)
   if (!strcmp(vice_opt.ExternalPalette, "default"))
      log_resources_set_int("VICIIExternalPalette", 0);
   else
   {
      log_resources_set_string("VICIIPaletteFile", vice_opt.ExternalPalette);
      log_resources_set_int("VICIIExternalPalette", 1);
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   log_resources_set_int("VICIIFilter", (vice_opt.Filter > -1) ? 1 : 0);
   log_resources_set_int("VICIIPALBlur", vice_opt.Filter);
   log_resources_set_int("VICIIPALOddLinePhase", vice_opt.FilterOddLinePhase);
   log_resources_set_int("VICIIPALOddLineOffset", vice_opt.FilterOddLineOffset);
#elif defined(__XVIC__)
   log_resources_set_int("VICFilter", (vice_opt.Filter > -1) ? 1 : 0);
   log_resources_set_int("VICPALBlur", vice_opt.Filter);
   log_resources_set_int("VICPALOddLinePhase", vice_opt.FilterOddLinePhase);
   log_resources_set_int("VICPALOddLineOffset", vice_opt.FilterOddLineOffset);
#elif defined(__XPLUS4__)
   log_resources_set_int("TEDFilter", (vice_opt.Filter > -1) ? 1 : 0);
   log_resources_set_int("TEDPALBlur", vice_opt.Filter);
   log_resources_set_int("TEDPALOddLinePhase", vice_opt.FilterOddLinePhase);
   log_resources_set_int("TEDPALOddLineOffset", vice_opt.FilterOddLineOffset);
#elif defined(__XPET__) || defined(__XCBM2__)
   log_resources_set_int("CrtcFilter", (vice_opt.Filter > -1) ? 1 : 0);
   log_resources_set_int("CrtcPALBlur", vice_opt.Filter);
   log_resources_set_int("CrtcPALOddLinePhase", vice_opt.FilterOddLinePhase);
   log_resources_set_int("CrtcPALOddLineOffset", vice_opt.FilterOddLineOffset);
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   log_resources_set_int("VICIIColorGamma", vice_opt.ColorGamma);
   log_resources_set_int("VICIIColorTint", vice_opt.ColorTint);
   log_resources_set_int("VICIIColorSaturation", vice_opt.ColorSaturation);
   log_resources_set_int("VICIIColorContrast", vice_opt.ColorContrast);
   log_resources_set_int("VICIIColorBrightness", vice_opt.ColorBrightness);
#elif defined(__XVIC__)
   log_resources_set_int("VICColorGamma", vice_opt.ColorGamma);
   log_resources_set_int("VICColorTint", vice_opt.ColorTint);
   log_resources_set_int("VICColorSaturation", vice_opt.ColorSaturation);
   log_resources_set_int("VICColorContrast", vice_opt.ColorContrast);
   log_resources_set_int("VICColorBrightness", vice_opt.ColorBrightness);
#elif defined(__XPLUS4__)
   log_resources_set_int("TEDColorGamma", vice_opt.ColorGamma);
   log_resources_set_int("TEDColorTint", vice_opt.ColorTint);
   log_resources_set_int("TEDColorSaturation", vice_opt.ColorSaturation);
   log_resources_set_int("TEDColorContrast", vice_opt.ColorContrast);
   log_resources_set_int("TEDColorBrightness", vice_opt.ColorBrightness);
#endif

   /* Input */
#if !defined(__XCBM5x0__)
   if (vice_opt.UserportJoyType == -1)
      log_resources_set_int("UserportJoy", 0);
   else
   {
      log_resources_set_int("UserportJoyType", vice_opt.UserportJoyType);
      log_resources_set_int("UserportJoy", 1);
   }
#endif

   /* Media */
   log_resources_set_int("AutostartWarp", vice_opt.AutostartWarp);
   log_resources_set_int("DriveTrueEmulation", vice_opt.DriveTrueEmulation);
   log_resources_set_int("VirtualDevices", vice_opt.VirtualDevices);
   log_resources_set_int("AttachDevice8Readonly", vice_opt.AttachDevice8Readonly);
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
   log_resources_set_int("EasyFlashWriteCRT", vice_opt.EasyFlashWriteCRT);
#endif

   /* ROM */
#if defined(__XSCPU64__)
   /* Replace kernal always from backup, because kernal loading replaces embedded data */
   memcpy(scpu64rom_scpu64_rom, scpu64rom_scpu64_rom_original, SCPU64_SCPU64_ROM_MAXSIZE);
   switch (opt_supercpu_kernal)
   {
      case 2:
         log_resources_set_string("SCPU64Name", "scpu-dos-2.04.bin");
         break;
      case 1:
         log_resources_set_string("SCPU64Name", "scpu-dos-1.4.bin");
         break;
      default:
         log_resources_set_string("SCPU64Name", "scpu64");
         break;
   }
#endif

   /* JiffyDOS */
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
   /* Replace kernal always from backup, because kernal loading replaces embedded data */
#if defined(__X64__) || defined(__X64SC__)
   memcpy(c64memrom_kernal64_rom, c64memrom_kernal64_rom_original, C64_KERNAL_ROM_SIZE);
#elif defined(__X128__)
   memcpy(c128kernal64_embedded, c128memrom_kernal64_rom_original, C128_KERNAL64_ROM_SIZE);
   memcpy(kernal_int, c128memrom_kernal128_rom_original, C128_KERNAL_ROM_IMAGE_SIZE);
#endif
   opt_jiffydos_kernal_skip = 0;
   if (opt_jiffydos)
   {
      char tmp_str[RETRO_PATH_MAX] = {0};
      int drive_type;
      resources_get_int("Drive8Type", &drive_type);

      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_1541-II.bin");
      log_resources_set_string("DosName1541ii", (const char*)tmp_str);
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_1571_repl310654.bin");
      log_resources_set_string("DosName1571", (const char*)tmp_str);
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_1581.bin");
      log_resources_set_string("DosName1581", (const char*)tmp_str);

#if defined(__X64__) || defined(__X64SC__)
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_C64.bin");
      log_resources_set_string("KernalName", (const char*)tmp_str);
#elif defined(__X128__)
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_C64.bin");
      log_resources_set_string("Kernal64Name", (const char*)tmp_str);
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_C128.bin");
      log_resources_set_string("KernalIntName", (const char*)tmp_str);
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
      /* 1541-II ROM will not work unless drive type is set back to whatever it already is ?! */
      log_resources_set_int("Drive8Type", drive_type);
#endif
   }
   else
   {
      log_resources_set_string("DosName1541ii", "d1541II");
      log_resources_set_string("DosName1571", "dos1571");
      log_resources_set_string("DosName1581", "dos1581");
#if defined(__X64__) || defined(__X64SC__)
      log_resources_set_string("KernalName", "kernal");
#elif defined(__X128__)
      log_resources_set_string("Kernal64Name", "kernal64");
      log_resources_set_string("KernalIntName", "kernal");
#endif
   }
#endif

   /* Model */
#if defined(__XPET__)
   petmodel_set(vice_opt.Model);
   keyboard_init();
#elif defined(__XCBM2__) || defined(__XCBM5x0__)
   cbm2model_set(vice_opt.Model);
#elif defined(__XVIC__)
   vic20model_set(vice_opt.Model);
#elif defined(__XPLUS4__)
   plus4model_set(vice_opt.Model);
#elif defined(__X128__)
   c128model_set(vice_opt.Model);
#elif defined(__X64DTV__)
   dtvmodel_set(vice_opt.Model);
#else
   c64model_set(vice_opt.Model);
#endif

#if defined(__X64__) || defined(__X64SC__)
   /* JiffyDOS SX-64 requires setting kernal after model change */
   if (opt_jiffydos)
   {
      char tmp_str[RETRO_PATH_MAX] = {0};
      switch (vice_opt.Model)
      {
         case C64MODEL_C64SX_PAL:
         case C64MODEL_C64SX_NTSC:
            snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_SX-64.bin");
            log_resources_set_string("KernalName", (const char*)tmp_str);

            /* Also must prevent `set_kernal_rom_name()` resetting to default kernel after restart */
            opt_jiffydos_kernal_skip = 1;
            break;
         default:
            break;
      }
   }
#endif

   /* Audio */
   if (vice_opt.DriveSoundEmulation)
   {
      log_resources_set_int("DriveSoundEmulationVolume", vice_opt.DriveSoundEmulation);
      log_resources_set_int("DriveSoundEmulation", 1);
   }
   else
      log_resources_set_int("DriveSoundEmulation", 0);

   if (vice_opt.DriveSoundEmulation && opt_autoloadwarp & AUTOLOADWARP_DISK && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))
      log_resources_set_int("DriveSoundEmulationVolume", 0);

#if !defined(__XSCPU64__) && !defined(__X64DTV__)
   if (vice_opt.DatasetteSound)
      log_resources_set_int("DatasetteSound", 1);
   else
      log_resources_set_int("DatasetteSound", 0);

   if (vice_opt.DatasetteSound && opt_autoloadwarp & AUTOLOADWARP_TAPE && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))
      log_resources_set_int("DatasetteSound", 0);
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__)
   log_resources_set_int("VICIIAudioLeak", vice_opt.AudioLeak);
   if (vice_opt.AudioLeak && opt_autoloadwarp && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))
      log_resources_set_int("VICIIAudioLeak", 0);
#elif defined(__XVIC__)
   log_resources_set_int("VICAudioLeak", vice_opt.AudioLeak);
   if (vice_opt.AudioLeak && opt_autoloadwarp && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))
      log_resources_set_int("VICAudioLeak", 0);
#elif defined(__XPLUS4__)
   log_resources_set_int("TEDAudioLeak", vice_opt.AudioLeak);
   if (vice_opt.AudioLeak && opt_autoloadwarp && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))
      log_resources_set_int("TEDAudioLeak", 0);
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
   if (vice_opt.SFXSoundExpanderChip)
   {
      log_resources_set_int("SFXSoundExpanderChip", vice_opt.SFXSoundExpanderChip);
      log_resources_set_int("SFXSoundExpander", 1);
   }
   else
      log_resources_set_int("SFXSoundExpander", 0);
#endif

   /* SID */
#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__)
   log_resources_set_int("SidEngine", vice_opt.SidEngine);
   log_resources_set_int("SidModel", vice_opt.SidModel);
   log_resources_set_int("SidResidSampling", vice_opt.SidResidSampling);
   log_resources_set_int("SidResidPassband", vice_opt.SidResidPassband);
   log_resources_set_int("SidResidGain", vice_opt.SidResidGain);
   log_resources_set_int("SidResidFilterBias", vice_opt.SidResidFilterBias);
   log_resources_set_int("SidResid8580Passband", vice_opt.SidResidPassband);
   log_resources_set_int("SidResid8580Gain", vice_opt.SidResidGain);
   log_resources_set_int("SidResid8580FilterBias", vice_opt.SidResid8580FilterBias);

   int sid_stereo;
   resources_get_int("SidStereo", &sid_stereo);
   /* Do not override if SidStereo is set in vicerc */
   if (sid_stereo == 0)
   {
      if (vice_opt.SidExtra)
      {
         log_resources_set_int("Sid2AddressStart", vice_opt.SidExtra);
         log_resources_set_int("SidStereo", 1);
      }
      else
         log_resources_set_int("SidStereo", 0);
   }
#else
   log_resources_set_int("SidEngine", 0);
#endif

   /* Misc model specific */
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
   if (vice_opt.REUsize)
   {
      log_resources_set_int("REUsize", vice_opt.REUsize);
      log_resources_set_int("REU", 1);
   }
   else
      log_resources_set_int("REU", 0);
#endif
#if defined(__X128__)
   log_resources_set_int("Go64Mode", vice_opt.Go64Mode);
   log_resources_set_int("C128ColumnKey", vice_opt.C128ColumnKey);
   log_resources_set_int("VDC64KB", vice_opt.VDC64KB);
#elif defined(__XVIC__)
   log_resources_set_int("MegaCartNvRAMWriteBack", 1);
   vic20mem_set();
#elif defined(__XSCPU64__)
   log_resources_set_int("SIMMSize", vice_opt.SIMMSize);
   log_resources_set_int("SpeedSwitch", vice_opt.SpeedSwitch);
#endif

#if !defined(__XPET__)
   /* Cartridge */
   if (strcmp(vice_opt.CartridgeFile, ""))
      log_resources_set_string("CartridgeFile", vice_opt.CartridgeFile);
#endif

   retro_ui_finalized = true;
   return 0;
}


#if defined(__X64__)
int c64ui_init_early(void)
{
   memcpy(c64memrom_kernal64_rom_original, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);
   return 0;
}
#elif defined(__X64SC__)
int c64scui_init_early(void)
{
   memcpy(c64memrom_kernal64_rom_original, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);
   return 0;
}
#elif defined(__X64DTV__)
int c64dtvui_init_early(void)
{
   return 0;
}
#elif defined(__XSCPU64__)
int scpu64ui_init_early(void)
{
   memcpy(scpu64rom_scpu64_rom_original, scpu64rom_scpu64_rom, SCPU64_SCPU64_ROM_MAXSIZE);
   return 0;
}
#elif defined(__X128__)
int c128ui_init_early(void)
{
   memcpy(c128memrom_kernal128_rom_original, kernal_int, C128_KERNAL_ROM_IMAGE_SIZE);
   memcpy(c128memrom_kernal64_rom_original, c128kernal64_embedded, C128_KERNAL64_ROM_SIZE);
   return 0;
}
#elif defined(__XVIC__)
int vic20ui_init_early(void)
{
   return 0;
}
#elif defined(__XPET__)
int petui_init_early(void)
{
   return 0;
}
#elif defined(__XPLUS4__)
int plus4ui_init_early(void)
{
   return 0;
}
#elif defined(__XCBM2__)
int cbm2ui_init_early(void)
{
   return 0;
}
#elif defined(__XCBM5x0__)
int cbm5x0ui_init_early(void)
{
   return 0;
}
#endif

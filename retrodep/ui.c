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
#include "uiapi.h"
#include "uistatusbar.h"
#include "resources.h"
#include "sid.h"
#include "sid-resources.h"
#include "util.h"
#include "videoarch.h"
#include "keyboard.h"
#include "driverom.h"
#if !defined(__XCBM5x0__)
#include "userport.h"
#endif

#if defined(__XPET__)
#include "petmodel.h"
#elif defined(__XCBM2__) || defined(__XCBM5x0__)
#include "cbm2model.h"
#elif defined(__XPLUS4__)
#include "plus4model.h"
#elif defined(__XVIC__)
#include "vic20model.h"
#include "vic20mem.h"
#elif defined(__X128__)
#include "c64model.h"
#include "c64rom.h"
#include "c128model.h"
#include "c128rom.h"
#include "c128mem.h"
#include "c128memrom.h"
#elif defined(__XSCPU64__)
#include "c64model.h"
#include "scpu64.h"
#include "scpu64mem.h"
#include "scpu64rom.h"
#elif defined(__X64DTV__)
#include "c64dtvmodel.h"
#else
#include "c64model.h"
#include "c64rom.h"
#include "c64mem.h"
#include "c64memrom.h"
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
extern bool opt_jiffydos_kernal_skip;
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__) || defined(__X128__) || defined(__XVIC__)
extern int request_model_auto_set;
extern bool opt_model_auto;
#endif

#include "libretro-core.h"
#if defined(__XVIC__)
extern void vic20mem_set(void);
#elif defined(__XSCPU64__)
extern unsigned int opt_supercpu_kernal;
#endif

extern dc_storage* dc;
extern bool retro_ui_finalized;
extern bool opt_jiffydos;
extern unsigned int opt_autoloadwarp;
extern char full_path[RETRO_PATH_MAX];
extern char retro_system_data_directory[RETRO_PATH_MAX];
extern bool log_resource_set;
extern retro_log_printf_t log_cb;

bool pending_AutostartTapeBasicLoad = false;

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

int ui_init()
{
   return 0;
}

void ui_init_with_args(int *argc, char **argv)
{
}

int ui_init_finish(void)
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

ui_jam_action_t ui_jam_dialog(const char *format, ...)
{
   char message[512];

   va_list ap;
   va_start(ap, format);
   vsnprintf(message, sizeof(message), format, ap);
   va_end(ap);

   log_cb(RETRO_LOG_ERROR, "%s\n", message);
   machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);

   return UI_JAM_NONE;
}

void arch_ui_activate(void)
{
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

void ui_actions_init(void)
{
}

void ui_actions_shutdown(void)
{
}

const char *ui_action_get_name(int action)
{
    return "";
}

int ui_action_get_id(const char *name)
{
    return 0;
}

void ui_action_trigger(int action)
{
}

video_canvas_t *ui_get_active_canvas(void)
{
    return NULL;
}

extern int log_resources_set_int(const char *name, int value);
extern int log_resources_set_string(const char *name, const char* value);

int ui_init_finalize(void)
{
   /* Dump machine specific defaults for 'vicerc' usage, if not already dumped */
   char resources_dump_path[RETRO_PATH_MAX] = {0};
   snprintf(resources_dump_path, sizeof(resources_dump_path), "%s%s%s%s",
         retro_system_data_directory, ARCHDEP_DIR_SEP_STR, "vicerc-dump-", machine_get_name());
   if (!util_file_exists(resources_dump_path))
      resources_dump(resources_dump_path);

   /* Model */
#if defined(__XPET__)
   petmodel_set(vice_opt.Model);
#elif defined(__XCBM2__) || defined(__XCBM5x0__)
   cbm2model_set(vice_opt.Model);
#elif defined(__XVIC__)
   vic20model_set(opt_model_auto && request_model_auto_set > -1 ? request_model_auto_set : vice_opt.Model);
#elif defined(__XPLUS4__)
   plus4model_set(vice_opt.Model);
#elif defined(__X128__)
   c128model_set(opt_model_auto && request_model_auto_set > -1 ? request_model_auto_set : vice_opt.Model);
#elif defined(__X64DTV__)
   dtvmodel_set(vice_opt.Model);
#else
   c64model_set(opt_model_auto && request_model_auto_set > -1 ? request_model_auto_set : vice_opt.Model);
#endif

   /* ROM */
#if defined(__XSCPU64__)
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
   opt_jiffydos_kernal_skip = false;
   if (opt_jiffydos)
   {
      char tmp_str[RETRO_PATH_MAX] = {0};
      int drive_type;
      resources_get_int("Drive8Type", &drive_type);

      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, ARCHDEP_DIR_SEP_CHR, "JiffyDOS_1541-II.bin");
      log_resources_set_string("DosName1541ii", (const char*)tmp_str);
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, ARCHDEP_DIR_SEP_CHR, "JiffyDOS_1571_repl310654.bin");
      log_resources_set_string("DosName1571", (const char*)tmp_str);
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, ARCHDEP_DIR_SEP_CHR, "JiffyDOS_1581.bin");
      log_resources_set_string("DosName1581", (const char*)tmp_str);

#if defined(__X64__) || defined(__X64SC__)
      switch (vice_opt.Model)
      {
         case C64MODEL_C64SX_PAL:
         case C64MODEL_C64SX_NTSC:
            snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, ARCHDEP_DIR_SEP_CHR, "JiffyDOS_SX-64.bin");
            log_resources_set_string("KernalName", (const char*)tmp_str);

            /* Also must prevent `set_kernal_rom_name()` resetting to default kernel after restart */
            opt_jiffydos_kernal_skip = true;
            break;
         default:
            snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, ARCHDEP_DIR_SEP_CHR, "JiffyDOS_C64.bin");
            log_resources_set_string("KernalName", (const char*)tmp_str);
            break;
      }
#elif defined(__X128__)
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, ARCHDEP_DIR_SEP_CHR, "JiffyDOS_C64.bin");
      log_resources_set_string("Kernal64Name", (const char*)tmp_str);
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, ARCHDEP_DIR_SEP_CHR, "JiffyDOS_C128.bin");
      log_resources_set_string("KernalIntName", (const char*)tmp_str);
#endif

      /* 1541-II ROM will not work unless drive type is set back to whatever it already is ?! */
      log_resources_set_int("Drive8Type", drive_type);
   }
   else
   {
      int drive_type;
      resources_get_int("Drive8Type", &drive_type);

      log_resources_set_string("DosName1541ii", DRIVE_ROM1541II_NAME);
      log_resources_set_string("DosName1571", DRIVE_ROM1571_NAME);
      log_resources_set_string("DosName1581", DRIVE_ROM1581_NAME);
#if defined(__X64__) || defined(__X64SC__)
      switch (vice_opt.Model)
      {
         case C64MODEL_C64SX_PAL:
         case C64MODEL_C64SX_NTSC:
            log_resources_set_string("KernalName", C64_KERNAL_SX64_NAME);

            opt_jiffydos_kernal_skip = false;
            break;
         default:
            log_resources_set_string("KernalName", C64_KERNAL_REV3_NAME);
            break;
      }
#elif defined(__X128__)
      log_resources_set_string("Kernal64Name", C64_KERNAL_REV3_NAME);
      log_resources_set_string("KernalIntName", C128_KERNAL_NAME);
#endif

      /* 1541-II ROM will not work unless drive type is set back to whatever it already is ?! */
      log_resources_set_int("Drive8Type", drive_type);
   }
#endif

   /* Keyboard map */
   keyboard_init();

   /* Mute sound at startup to hide 6581 ReSID init pop */
   resources_set_int("SoundVolume", 0);

   /* M3U parsed non-basic tape load force */
   if (pending_AutostartTapeBasicLoad)
      log_resources_set_int("AutostartTapeBasicLoad", 0);

   /* Sensible defaults */
   log_resources_set_int("AutostartPrgMode", 1);
   log_resources_set_int("AutostartDelayRandom", 0);
   log_resources_set_int("FSDeviceLongNames", 1);
   log_resources_set_int("Mouse", 1);

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
#endif

#if defined(__X128__)
   log_resources_set_int("VDCFilter", (vice_opt.VDCFilter > -1) ? 1 : 0);
   log_resources_set_int("VDCPALBlur", vice_opt.VDCFilter);
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
      log_resources_set_int("UserportDevice", 0);
   else
      log_resources_set_int("UserportDevice", vice_opt.UserportJoyType + USERPORT_DEVICE_JOYSTICK_CGA);
#endif

   /* Media */
   log_resources_set_int("AutostartWarp", vice_opt.AutostartWarp);
   log_resources_set_int("VirtualDevice4", vice_opt.VirtualDevices);
   log_resources_set_int("VirtualDevice8", !vice_opt.DriveTrueEmulation);
   log_resources_set_int("VirtualDevice9", !vice_opt.DriveTrueEmulation);
   log_resources_set_int("Drive8TrueEmulation", vice_opt.DriveTrueEmulation);
   log_resources_set_int("Drive9TrueEmulation", vice_opt.DriveTrueEmulation);
   log_resources_set_int("AttachDevice8d0Readonly", vice_opt.AttachDevice8Readonly);
   log_resources_set_int("AttachDevice8d1Readonly", vice_opt.AttachDevice8Readonly);
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
   log_resources_set_int("EasyFlashWriteCRT", vice_opt.EasyFlashWriteCRT);
#endif

   /* Audio */
   if (vice_opt.DriveSoundEmulation)
   {
      log_resources_set_int("DriveSoundEmulationVolume", vice_opt.DriveSoundEmulation);
      log_resources_set_int("DriveSoundEmulation", 1);
   }
   else
      log_resources_set_int("DriveSoundEmulation", 0);

   if (vice_opt.DriveSoundEmulation && (opt_autoloadwarp & AUTOLOADWARP_DISK) && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))
      log_resources_set_int("DriveSoundEmulationVolume", 0);

   if ((!string_is_empty(dc->files[dc->index]) && strendswith(dc->files[dc->index], ".d81"))
    || (!string_is_empty(full_path) && strendswith(full_path, ".d81")))
      log_resources_set_int("DriveSoundEmulationVolume", 0);

#if !defined(__XSCPU64__) && !defined(__X64DTV__)
   if (vice_opt.DatasetteSound)
      log_resources_set_int("DatasetteSound", 1);
   else
      log_resources_set_int("DatasetteSound", 0);

   if (vice_opt.DatasetteSound && (opt_autoloadwarp & AUTOLOADWARP_TAPE) && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))
      log_resources_set_int("DatasetteSound", 0);
#endif

   log_resources_set_int(AUDIOLEAK_RESOURCE, vice_opt.AudioLeak);
   if (vice_opt.AudioLeak && opt_autoloadwarp && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))
      log_resources_set_int(AUDIOLEAK_RESOURCE, 0);

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

   /* Printer */
   log_resources_set_int("Printer4", vice_opt.Printer);

   retro_ui_finalized = true;
   log_resource_set = true;
   return 0;
}


#if defined(__X64__)
int c64ui_init_early(void) { return 0; }
int c64ui_init(void) { return 0; }
void c64ui_shutdown(void) {}
#elif defined(__X64SC__)
int c64scui_init_early(void) { return 0; }
int c64scui_init(void) { return 0; }
void c64scui_shutdown(void) {}
#elif defined(__X64DTV__)
int c64dtvui_init_early(void) { return 0; }
int c64dtvui_init(void) { return 0; }
void c64dtvui_shutdown(void) {}
#elif defined(__XSCPU64__)
int scpu64ui_init_early(void) { return 0; }
int scpu64ui_init(void) { return 0; }
void scpu64ui_shutdown(void) {}
#elif defined(__X128__)
int c128ui_init_early(void) { return 0; }
int c128ui_init(void) { return 0; }
void c128ui_shutdown(void) {}
#elif defined(__XVIC__)
int vic20ui_init_early(void) { return 0; }
int vic20ui_init(void) { return 0; }
void vic20ui_shutdown(void) {}
#elif defined(__XPET__)
int petui_init_early(void) { return 0; }
int petui_init(void) { return 0; }
void petui_shutdown(void) {}
#elif defined(__XPLUS4__)
int plus4ui_init_early(void) { return 0; }
int plus4ui_init(void) { return 0; }
void plus4ui_shutdown(void) {}
#elif defined(__XCBM2__)
int cbm2ui_init_early(void) { return 0; }
int cbm2ui_init(void) { return 0; }
void cbm2ui_shutdown(void) {}
#elif defined(__XCBM5x0__)
int cbm5x0ui_init_early(void) { return 0; }
int cbm5x0ui_init(void) { return 0; }
void cbm5x0ui_shutdown(void) {}
#endif

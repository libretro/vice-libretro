#include "libretro.h"
#include "libretro-core.h"
#include "libretro-mapper.h"
#include "libretro-graph.h"
#include "encodings/utf.h"

#include "archdep.h"
#include "main.h"
#include "mem.h"
#include "machine.h"
#include "maincpu.h"
#include "snapshot.h"
#include "autostart.h"
#include "util.h"
#include "crt.h"
#include "drive.h"
#include "tape.h"
#include "tapeport.h"
#include "diskimage.h"
#include "fsdevice.h"
#include "vdrive.h"
#include "vdrive-internal.h"
#include "charset.h"
#include "attach.h"
#include "interrupt.h"
#include "datasette.h"
#include "cartridge.h"
#include "initcmdline.h"
#include "vsync.h"
#include "log.h"
#include "keyboard.h"
#include "keymap.h"
#include "kbdbuf.h"
#include "joystick.h"
#include "resources.h"
#include "sid.h"
#include "sid-resources.h"
#include "uistatusbar.h"
#if !defined(__XCBM5x0__)
#include "userport.h"
#endif

#ifdef USE_LIBRETRO_VFS
#undef utf8_to_local_string_alloc
#define utf8_to_local_string_alloc strdup
#endif

/* Main CPU loop */
long retro_now = 0;
unsigned retro_renderloop = 1;
bool retro_sound_keep_alive = false;

/* VKBD */
extern bool retro_vkbd;
extern void print_vkbd(void);
unsigned int opt_vkbd_theme = 0;
libretro_graph_alpha_t opt_vkbd_alpha = GRAPH_ALPHA_75;
libretro_graph_alpha_t opt_vkbd_dim_alpha = GRAPH_ALPHA_25;

/* Core vars */
static bool autosys = true;
static bool autoloadrun = false;
static bool noautostart = false;
static bool noautostart_locked = false;
static bool tde_locked = false;
static char* autostartString = NULL;
static char* autostartProgram = NULL;
char full_path[RETRO_PATH_MAX] = {0};

static struct vice_cart_info vice_carts[RETRO_NUM_CORE_OPTION_VALUES_MAX] = {0};
struct vice_raster_s vice_raster;

static snapshot_stream_t* snapshot_stream = NULL;
static int load_trap_happened = 0;
static int save_trap_happened = 0;

unsigned int retro_devices[RETRO_DEVICES] = {0};
unsigned int opt_video_options_display = 0;
unsigned int opt_audio_options_display = 0;
unsigned int opt_mapping_options_display = 1;
unsigned int retro_region = 0;
float retro_refresh = 0;
static unsigned int sound_sample_rate_prev = 0;

bool retro_ui_finalized = false;
bool log_resource_set = false;

extern uint8_t mem_ram[];
#if defined(__X64__) || defined(__X64SC__)
int mem_ram_size = C64_RAM_SIZE;
#elif defined(__X64DTV__)
int mem_ram_size = 0x200000;
#elif defined(__XSCPU64__)
int mem_ram_size = SCPU64_RAM_SIZE;
#elif defined(__X128__)
int mem_ram_size = C128_RAM_SIZE;
#elif defined(__XPLUS4__)
int mem_ram_size = 0x10000;
#elif defined(__XVIC__)
int mem_ram_size = 0x8000;
#elif defined(__CBM2__) || defined(__CBM5x0__)
int mem_ram_size = CBM2_RAM_SIZE;
#elif defined(__XPET__)
int mem_ram_size = 0x20000;
#else
int mem_ram_size = 0;
#endif

#define GMOD2_EEPROM_SIZE 2048

/* Core geometry */
unsigned int defaultw = WINDOW_WIDTH;
unsigned int defaulth = WINDOW_HEIGHT;
unsigned int retrow = WINDOW_WIDTH;
unsigned int retroh = WINDOW_HEIGHT;
unsigned int retroXS = 0;
unsigned int retroYS = 0;
unsigned int retroXS_offset = 0;
unsigned int retroYS_offset = 0;

unsigned short int pix_bytes = 2;
static bool pix_bytes_initialized = false;
unsigned short int retro_bmp[RETRO_BMP_SIZE] = {0};
unsigned int retro_bmp_offset = 0;

int crop_id = -1;
int crop_id_prev = -1;
unsigned int opt_crop_id = 0;
unsigned int crop_mode_id = 0;
bool crop_delay = true;
unsigned int retrow_crop = 0;
unsigned int retroh_crop = 0;
int retroXS_crop_offset = 0;
int retroYS_crop_offset = 0;
unsigned int opt_aspect_ratio = 0;
bool opt_aspect_ratio_locked = false;
static unsigned int manual_crop_top = 0;
static unsigned int manual_crop_bottom = 0;
static unsigned int manual_crop_left = 0;
static unsigned int manual_crop_right = 0;

/* Audio output buffer */
static struct {
   int16_t *data;
   int32_t size;
   int32_t capacity;
} output_audio_buffer = {NULL, 0, 0};

/* Audio buffer copy for auto warp detection */
int16_t *audio_buffer;
bool audio_is_playing = false;
static bool audio_is_ignored = false;
static uint8_t audio_ignore_count = 0;

/* Core options */
struct vice_core_options vice_opt;

#if defined(__XVIC__)
int vic20mem_forced = -1;
#endif

#if defined(__X128__)
int c128_vdc = 0;
int is_vdc(void)
{
   int key;
   resources_get_int("C128ColumnKey", &key);
   c128_vdc = !key;
   return c128_vdc;
}
int set_vdc(int enabled)
{
   log_resources_set_int("C128ColumnKey", !enabled);
   c128_vdc = enabled;
   crop_id_prev = -1;
}
#endif

static bool request_reload_restart = false;
static bool request_restart = false;
static bool request_update_work_disk = false;
int request_model_set = -1;
int request_model_auto_set = -1;
static int request_model_prev = -1;
bool opt_model_auto = true;
static bool opt_model_auto_locked = false;
unsigned int opt_autostart = 1;
unsigned int opt_autoloadwarp = 0;
unsigned int opt_warp_boost = 0;
unsigned int opt_read_vicerc = 0;
unsigned int opt_work_disk_type = 0;
unsigned int opt_work_disk_unit = 8;
bool opt_floppy_multidrive = false;
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
static bool opt_jiffydos_allow = true;
bool opt_jiffydos = false;
bool opt_jiffydos_kernal_skip = false;
static bool opt_reu_allow = true;
#endif
#if defined(__XSCPU64__)
unsigned int opt_supercpu_kernal = 0;
#endif
#define SOUND_VOLUME_COUNTER 10
static unsigned int sound_volume_counter = SOUND_VOLUME_COUNTER;
bool sound_drive_mute = false;
unsigned int opt_audio_leak_volume = 0;
int opt_datasette_sound_volume = 0;
unsigned int opt_statusbar = 0;
unsigned int opt_reset_type = 0;
bool opt_keyrah_keypad = false;
bool opt_keyboard_pass_through = false;
unsigned int opt_keyboard_keymap = KBD_INDEX_POS;
unsigned int opt_retropad_options = RETROPAD_OPTIONS_DISABLED;
unsigned int opt_joyport_type = 0;
bool opt_joyport_paddles_split = true;
int opt_joyport_pointer_color = -1;
unsigned int opt_dpadmouse_speed = 6;
unsigned int opt_mouse_speed = 100;
unsigned int opt_analogmouse = 0;
unsigned int opt_analogmouse_deadzone = 20;
float opt_analogmouse_speed_left = 1.0;
float opt_analogmouse_speed_right = 1.0;

extern bool datasette_hotkeys;
extern unsigned int cur_port;
extern bool cur_port_locked;

extern bool retro_turbo_fire;
extern bool turbo_fire_locked;
extern unsigned int turbo_fire_button;
extern unsigned int turbo_pulse;

extern char *fsdevice_get_path(unsigned int unit);

#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__) || defined(__X128__)
extern int cart_getid_slotmain(void);
#endif

retro_log_printf_t log_cb = NULL;
static retro_set_led_state_t led_state_cb = NULL;
static retro_video_refresh_t video_cb = NULL;
static retro_audio_sample_t audio_cb = NULL;
static retro_audio_sample_batch_t audio_batch_cb = NULL;
static retro_environment_t environ_cb = NULL;

retro_input_state_t input_state_cb = NULL;
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

static retro_input_poll_t input_poll_cb = NULL;
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }

static struct retro_perf_callback perf_cb;

bool libretro_supports_bitmasks = false;
static bool libretro_supports_ff_override = false;
bool libretro_ff_enabled = false;
static bool libretro_supports_option_categories = false;
#define HAVE_NO_LANGEXTRA

char retro_save_directory[RETRO_PATH_MAX] = {0};
char retro_temp_directory[RETRO_PATH_MAX] = {0};
char retro_system_directory[RETRO_PATH_MAX] = {0};
char retro_system_data_directory[RETRO_PATH_MAX] = {0};
static char retro_content_directory[RETRO_PATH_MAX] = {0};

/* Disk Control context */
dc_storage* dc = NULL;
char dc_savestate_filename[RETRO_PATH_MAX] = {0};

int runstate = RUNSTATE_FIRST_START; /* used to detect whether we are just starting the core from scratch */
/* runstate = RUNSTATE_FIRST_START: first time retro_run() is called after loading and starting core */
/* runstate = RUNSTATE_LOADED_CONTENT: load content was selected while core is running, so do an autostart_reset() */
/* runstate = RUNSTATE_RUNNING: core is running normally */

/* Display disk name and label instead of "Changing disk in tray"- maybe make it configurable */
bool display_disk_name = false;
/* See which looks best in most cases and tweak (or make configurable) */
int disk_label_mode = DISK_LABEL_MODE_ASCII_OR_CAMELCASE;

/* Forward declarations */
bool retro_disk_set_eject_state(bool ejected);
static void update_variables(void);

/* Display message on next retro_run */
static bool retro_message = false;
static char retro_message_msg[1024] = {0};
void display_retro_message(const char *message)
{
   snprintf(retro_message_msg, sizeof(retro_message_msg), "%s", message);
   retro_message = true;
}

extern bool retro_statusbar;
extern void display_current_image(const char *image, bool inserted);
extern int ui_init_finalize(void);

static char* x_strdup(const char* str)
{
   return str ? strdup(str) : NULL;
}

/* Args for cmdline */
static char ARGUV[64][1024];
static unsigned char ARGUC = 0;

/* Args for core */
static char XARGV[64][1024];
static const char* xargv_cmd[64];
static int PARAMCOUNT = 0;

static char CMDFILE[512] = {0};
int loadcmdfile(const char *argv)
{
   int res = 0;
   FILE *fp = fopen(argv, "r");

   CMDFILE[0] = '\0';
   if (fp != NULL)
   {
      if (fgets(CMDFILE, 512, fp) != NULL)
      {
         res = 1;
         snprintf(CMDFILE, sizeof(CMDFILE), "%s", trimwhitespace(CMDFILE));
      }
      fclose (fp);
   }
   return res;
}

static void parse_cmdline(const char *argv)
{
   char *p, *p2, *start_of_word;
   int c, c2;
   static char buffer[512*4];
   enum states { DULL, IN_WORD, IN_STRING } state = DULL;

   ARGUC = 0;

   strcpy(buffer, argv);
   strcat(buffer, " \0");

   for (p = buffer; *p != '\0'; p++)
   {
      c = (unsigned char) *p; /* convert to unsigned char for is* functions */
      switch (state)
      {
         case DULL: /* not in a word, not in a double quoted string */
            if (isspace(c)) /* still not in a word, so ignore this char */
               continue;
            /* not a space -- if it's a double quote we go to IN_STRING, else to IN_WORD */
            if (c == '"')
            {
               state = IN_STRING;
               start_of_word = p + 1; /* word starts at *next* char, not this one */
               continue;
            }
            state = IN_WORD;
            start_of_word = p; /* word starts here */
            continue;
         case IN_STRING:
            /* we're in a double quoted string, so keep going until we hit a close " */
            if (c == '"')
            {
               /* word goes from start_of_word to p-1 */
               /* ... do something with the word ... */
               for (c2 = 0,p2 = start_of_word; p2 < p; p2++, c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               /* terminate word */
               ARGUV[ARGUC][c2] = '\0';
               ARGUC++;

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_STRING or we handled the end above */
         case IN_WORD:
            /* we're in a word, so keep going until we get to a space */
            if (isspace(c))
            {
               /* word goes from start_of_word to p-1 */
               /* ... do something with the word ... */
               for (c2 = 0,p2 = start_of_word; p2 <p; p2++,c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               /* terminate word */
               ARGUV[ARGUC][c2] = '\0';
               ARGUC++;

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_WORD or we handled the end above */
      }
   }
}

static void Add_Option(const char* option)
{
   sprintf(XARGV[PARAMCOUNT++], "%s", option);
}

static int check_joystick_control(const char* filename)
{
   int port = 0;
   if (filename != NULL)
   {
      if (strcasestr(filename, "_j1.") || strcasestr(filename, "(j1)."))
         port = 1;
      else if (strcasestr(filename, "_j2.") || strcasestr(filename, "(j2)."))
         port = 2;
   }
   return port;
}

static int retro_disk_get_image_unit(void)
{
   if (dc->index < dc->count)
   {
      if (dc_get_image_type(dc->files[dc->index]) == DC_IMAGE_TYPE_TAPE)
         dc->unit = 1;
      else if (dc_get_image_type(dc->files[dc->index]) == DC_IMAGE_TYPE_FLOPPY)
         dc->unit = 8;
      else if (dc_get_image_type(dc->files[dc->index]) == DC_IMAGE_TYPE_MEM)
         dc->unit = 0;
      else
         dc->unit = 8;
   }
   else
      dc->unit = 8;

   return dc->unit;
}

static void toggle_tde(int on)
{
   if (tde_locked)
      return;

   tde_locked = true;
   vice_opt.DriveTrueEmulation = on;

   if (retro_ui_finalized)
   {
      log_resources_set_int("Drive8TrueEmulation", on);
      log_resources_set_int("Drive9TrueEmulation", on);
      log_resources_set_int("VirtualDevice8", !on);
      log_resources_set_int("VirtualDevice9", !on);
   }
}

/* ReSID 6581 init pop mute shenanigans */
void sound_volume_counter_reset(void)
{
   resources_set_int("SoundVolume", 0);
   sound_volume_counter = SOUND_VOLUME_COUNTER;
}

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
static bool reu_allow(const char *string)
{
   /* Do not allow REU with cartridges */
   if ((!string_is_empty(string) && string_ends_with(string, "crt")) || !opt_reu_allow)
   {
      if (vice_opt.REUsize)
      {
         log_cb(RETRO_LOG_INFO, "REU is not compatible with cartridges, disabling...\n");
         if (retro_ui_finalized)
            log_resources_set_int("REU", 0);
      }
      vice_opt.REUsize = 0;

      return false;
   }

   return true;
}
#endif

#if defined(__XVIC__)

#define CARTRIDGE_VIC20_CART_GAMEBASE 0x100
#define CARTRIDGE_VIC20_CART_TOSEC    0x101
#define CARTRIDGE_VIC20_CART_NOINTRO  0x102

static int vic20_cart_is_multipart(int prev_type, const char* argv)
{
   int mp_type = prev_type;

   if (strcasestr(argv, "-2000.") || strcasestr(argv, "-4000.")
    || strcasestr(argv, "-6000.") || strcasestr(argv, "-a000."))
      mp_type = CARTRIDGE_VIC20_CART_GAMEBASE;
   else if (strcasestr(argv, "[2000]") || strcasestr(argv, "[4000]")
         || strcasestr(argv, "[6000]") || strcasestr(argv, "[A000]"))
      mp_type = CARTRIDGE_VIC20_CART_TOSEC;
   else if (strcasestr(argv, "$2000") || strcasestr(argv, "$4000")
         || strcasestr(argv, "$6000") || strcasestr(argv, "$A000"))
      mp_type = CARTRIDGE_VIC20_CART_NOINTRO;

   return mp_type;
}

static int vic20_autodetect_cartridge_type(const char* argv)
{
   FILE *fd;
   int addr = 0, len = 0, type = 0;
   char buf[RETRO_PATH_MAX] = {0};

   fd = fopen(argv, MODE_READ);
   fseek(fd, 0, SEEK_END);
   len = ftell(fd);
   fseek(fd, 0, SEEK_SET);
   switch (len & 0xfff)
   {
      case 0: /* plain binary */
         addr = 0;
         type = CARTRIDGE_VIC20_DETECT;
         break;
      case 2: /* load address */
         addr = fgetc(fd);
         addr = (addr & 0xff) | ((fgetc(fd) << 8) & 0xff00);
         len -= 2; /* remove load address from length */
         type = CARTRIDGE_VIC20_GENERIC;
         break;
      default: /* not a valid file */
         /* M3U analyzing */
         if (strcasestr(argv, ".m3u"))
         {
            fseek(fd, 0, SEEK_SET);
            if (fgets(buf, sizeof(buf), fd) != NULL)
            {
               buf[strcspn(buf, "\r\n")] = 0;
               argv = buf;
            }
         }
         break;
   }
   fclose(fd);

   if (type > 0)
   {
      if (len == 0)
         type = -1;
      else if (len == 0x200000 && addr == 0)
         type = CARTRIDGE_VIC20_MEGACART;
      else if ((addr == 0x6000 || addr == 0x7000) && (len <= 0x4000))
         type = CARTRIDGE_VIC20_16KB_6000;
      else if ((addr == 0x4000 || addr == 0x5000) && (len <= 0x4000))
         type = CARTRIDGE_VIC20_16KB_4000;
      else if ((addr == 0x2000 || addr == 0x3000) && (len <= 0x4000))
         type = CARTRIDGE_VIC20_16KB_2000;
      else if ((addr == 0xA000) && (len <= 0x2000))
         type = CARTRIDGE_VIC20_8KB_A000;
      else if ((addr == 0xB000) && (len <= 0x1000))
         type = CARTRIDGE_VIC20_4KB_B000;
      else if (len <= 0x2000)
         type = CARTRIDGE_VIC20_8KB_A000;
   }

   if (strcasestr(argv, ".20"))
      type = CARTRIDGE_VIC20_16KB_2000;
   else if (strcasestr(argv, ".40"))
      type = CARTRIDGE_VIC20_16KB_4000;
   else if (strcasestr(argv, ".60"))
      type = CARTRIDGE_VIC20_16KB_6000;
   else if (strcasestr(argv, ".70"))
      type = CARTRIDGE_VIC20_4KB_6000;
   else if (strcasestr(argv, ".a0"))
      type = CARTRIDGE_VIC20_8KB_A000;
   else if (strcasestr(argv, ".b0"))
      type = CARTRIDGE_VIC20_4KB_B000;

   /* Multipart ROM combinations */
   type = vic20_cart_is_multipart(type, argv);

   return type;
}

static void vic20_mem_force(const char* argv)
{
   char buf[6]      = {0};
   int vic20mem     = 0;
   int vic20mems[6] = {0, 3, 8, 16, 24, 35};

   for (int i = 0; i < sizeof(vic20mems)/sizeof(vic20mems[0]); i++)
   {
      vic20mem = vic20mems[i];

      snprintf(buf, sizeof(buf), "%c%d%c", '(', vic20mem, 'k');
      if (strcasestr(argv, buf))
      {
         vic20mem_forced = i;
         log_cb(RETRO_LOG_INFO, "VIC-20 memory expansion force found in filename \"%s\": %dkB\n", argv, vic20mem);
         break;
      }

      snprintf(buf, sizeof(buf), "%c%d%c", '[', vic20mem, 'k');
      if (strcasestr(argv, buf))
      {
         vic20mem_forced = i;
         log_cb(RETRO_LOG_INFO, "VIC-20 memory expansion force found in filename \"%s\": %dkB\n", argv, vic20mem);
         break;
      }

      snprintf(buf, sizeof(buf), "%c%d%c", ARCHDEP_DIR_SEP_CHR, vic20mem, 'k');
      if (strcasestr(argv, buf))
      {
         vic20mem_forced = i;
         log_cb(RETRO_LOG_INFO, "VIC-20 memory expansion force found in path \"%s\": %dkB\n", argv, vic20mem);
         break;
      }
   }
}

void vic20mem_set(void)
{
   unsigned int vic20mem = 0;
   vic20mem = (vic20mem_forced > -1) ? vic20mem_forced : vice_opt.VIC20Memory;

   /* Super VIC uses memory blocks 1+2 by default */
   if (!vic20mem && vice_opt.Model == VIC20MODEL_VIC21)
      vic20mem = 3;

   unsigned int vic_blocks = 0;
   switch (vic20mem)
   {
      case 1:
         vic_blocks |= VIC_BLK0;
         break;
      case 2:
         vic_blocks |= VIC_BLK1;
         break;
      case 3:
         vic_blocks |= VIC_BLK1;
         vic_blocks |= VIC_BLK2;
         break;
      case 4:
         vic_blocks |= VIC_BLK1;
         vic_blocks |= VIC_BLK2;
         vic_blocks |= VIC_BLK3;
         break;
      case 5:
         vic_blocks = VIC_BLK_ALL;
         break;
      default:
         break;
   }

   log_resources_set_int("RAMBlock0", (vic_blocks & VIC_BLK0) ? 1 : 0);
   log_resources_set_int("RAMBlock1", (vic_blocks & VIC_BLK1) ? 1 : 0);
   log_resources_set_int("RAMBlock2", (vic_blocks & VIC_BLK2) ? 1 : 0);
   log_resources_set_int("RAMBlock3", (vic_blocks & VIC_BLK3) ? 1 : 0);
   log_resources_set_int("RAMBlock5", (vic_blocks & VIC_BLK5) ? 1 : 0);
}

static void vic20_autosys_run(const char* full_path)
{
   if (!strcasestr(full_path, "(SYS ") && !strcasestr(full_path, "[SYS "))
      return;

   char command[20] = {0};
   char tmp_path[RETRO_PATH_MAX] = {0};
   snprintf(tmp_path, sizeof(tmp_path), "%s", path_basename(full_path));
   char *token = strtok((char*)tmp_path, " ");
   char *token_prev = token;
   while (token != NULL)
   {
      token = strtok(NULL, " ");
      if (strendswith(token_prev, "SYS"))
      {
         snprintf(command, sizeof(command), "%s", token);
         token = NULL;
      }
      token_prev = token;
   }

   if (!string_is_empty(command))
   {
      if (strcasestr(full_path, "(SYS"))
         token = strtok((char*)command, ")");
      else if (strcasestr(full_path, "[SYS"))
         token = strtok((char*)command, "]");

      log_cb(RETRO_LOG_INFO, "Executing \"SYS %s\"\n", command);
      kbdbuf_feed("SYS ");
      kbdbuf_feed(command);
      kbdbuf_feed("\r");
   }

   token = NULL;
   token_prev = NULL;
}
#endif /* __XVIC__ */

static int process_cmdline(const char* argv)
{
   int i = 0;
   bool is_fliplist = false;
   int joystick_control = 0;

   noautostart = (noautostart_locked) ? noautostart : !opt_autostart;
   PARAMCOUNT = 0;
   dc_reset(dc);
   snprintf(full_path, sizeof(full_path), "%s", argv);

   cur_port_locked = false;
   free(autostartString);
   autostartString = NULL;
   free(autostartProgram);
   autostartProgram = NULL;

   /* Load command line arguments from cmd file */
   if (strendswith(argv, ".cmd"))
   {
      if (loadcmdfile(argv))
      {
         log_cb(RETRO_LOG_INFO, "Starting game from command line \"%s\".\n", argv);
         vice_opt.Model = 99; /* set model to unknown for custom settings - prevents overriding of command line options */
      }
      else
      {
         log_cb(RETRO_LOG_ERROR, "Failed to load command line from \"%s\".\n", argv);
      }
      parse_cmdline(CMDFILE);
   }
   else
      parse_cmdline(argv);

   /* Core command line is now parsed to ARGUV, ARGUC. */
   /* Build command file for VICE in XARGV, PARAMCOUNT. */
   bool single_image = strcmp(ARGUV[0], CORE_NAME) != 0;

   /* Allow using command lines without CORE_NAME by not allowing single_image */
   if (strendswith(argv, ".cmd"))
      single_image = false;

   /* If first command line argument is CORE_NAME, it's an extended command line
    * otherwise it's just image filename */
   if (single_image)
   {
      /* Command doesn't start with core name, so add it first */
      Add_Option(CORE_NAME);

      /* Ignore parsed arguments, read filename directly from argv */

      /* Check original filename for joystick control,
       * not the first name from M3U or VFL */
      joystick_control = check_joystick_control(argv);
      if (joystick_control)
      {
         cur_port = joystick_control;
         cur_port_locked = true;
      }

      /* "Browsed" file in ZIP */
      char browsed_file[RETRO_PATH_MAX] = {0};
      if (strstr(argv, ".zip#") || strstr(argv, ".7z#"))
      {
         char *token = strtok((char*)argv, "#");
         while (token != NULL)
         {
            snprintf(browsed_file, sizeof(browsed_file), "%s", token);
            token = strtok(NULL, "#");
         }
      }
      snprintf(full_path, sizeof(full_path), "%s", argv);

      /* ZIP + NIB vars, use the same temp directory for single NIBs */
      char zip_basename[RETRO_PATH_MAX] = {0};
      snprintf(zip_basename, sizeof(zip_basename), "%s", path_basename(full_path));
      path_remove_extension(zip_basename);

      char nib_input[RETRO_PATH_MAX] = {0};
      char nib_output[RETRO_PATH_MAX] = {0};

      /* NIB convert to G64 */
      if (dc_get_image_type(argv) == DC_IMAGE_TYPE_NIBBLER)
      {
         snprintf(nib_input, sizeof(nib_input), "%s", argv);
         snprintf(nib_output, sizeof(nib_output), "%s%s%s.g64", retro_temp_directory, ARCHDEP_DIR_SEP_STR, zip_basename);
         path_mkdir(retro_temp_directory);
         nib_convert(nib_input, nib_output);
         argv = nib_output;
      }

      /* ZIP */
      if (strendswith(argv, ".zip") || strendswith(argv, ".7z"))
      {
         path_mkdir(retro_temp_directory);
         if (strendswith(argv, ".zip"))
            zip_uncompress(full_path, retro_temp_directory, NULL);
         else if (strendswith(argv, ".7z"))
            sevenzip_uncompress(full_path, retro_temp_directory, NULL);

         /* Default to directory mode */
         snprintf(full_path, sizeof(full_path), "%s", retro_temp_directory);

         FILE *zip_m3u;
         zip_m3u_t zip_m3u_list = {0};
         snprintf(zip_m3u_list.path, sizeof(zip_m3u_list.path), "%s%s%s.m3u",
               retro_temp_directory, ARCHDEP_DIR_SEP_STR, utf8_to_local_string_alloc(zip_basename));

         DIR *zip_dir;
         struct dirent *zip_dirp;

         /* Convert all NIBs to G64 */
         zip_dir = opendir(retro_temp_directory);
         while ((zip_dirp = readdir(zip_dir)) != NULL)
         {
            if (dc_get_image_type(zip_dirp->d_name) == DC_IMAGE_TYPE_NIBBLER)
            {
               snprintf(nib_input, sizeof(nib_input), "%s%s%s", retro_temp_directory, ARCHDEP_DIR_SEP_STR, zip_dirp->d_name);
               snprintf(nib_output, sizeof(nib_output), "%s%s%s.g64", retro_temp_directory, ARCHDEP_DIR_SEP_STR, path_remove_extension(zip_dirp->d_name));
               nib_convert(nib_input, nib_output);
            }
         }
         closedir(zip_dir);

         if (string_is_empty(browsed_file))
            m3u_scan_recurse(retro_temp_directory, &zip_m3u_list);

         switch (zip_m3u_list.mode)
         {
            case 0: /* Extracted path */
               if (browsed_file[0] != '\0')
               {
                  if (dc_get_image_type(browsed_file) == DC_IMAGE_TYPE_NIBBLER)
                     snprintf(full_path, sizeof(full_path), "%s%s%s.g64", retro_temp_directory, ARCHDEP_DIR_SEP_STR, path_remove_extension(browsed_file));
                  else
                     snprintf(full_path, sizeof(full_path), "%s%s%s", retro_temp_directory, ARCHDEP_DIR_SEP_STR, browsed_file);
               }
               break;
            case 1: /* Generated playlist */
               zip_m3u = fopen(zip_m3u_list.path, "w");
               qsort(zip_m3u_list.list, zip_m3u_list.num, RETRO_PATH_MAX, qstrcmp);

               /* Flip memory image first if last */
               if (     dc_get_image_type(zip_m3u_list.list[0]) != DC_IMAGE_TYPE_MEM
                     && dc_get_image_type(zip_m3u_list.list[zip_m3u_list.num - 1]) == DC_IMAGE_TYPE_MEM)
               {
                  for (int l = zip_m3u_list.num; l > -1; l--)
                  {
                     int k = l + 1;
                     strlcpy(zip_m3u_list.list[k], zip_m3u_list.list[l], sizeof(zip_m3u_list.list[l]));
                  }
                  strlcpy(zip_m3u_list.list[0], zip_m3u_list.list[zip_m3u_list.num], sizeof(zip_m3u_list.list[0]));
               }

               for (int l = 0; l < zip_m3u_list.num; l++)
                  fprintf(zip_m3u, "%s\n", zip_m3u_list.list[l]);
               fclose(zip_m3u);
               snprintf(full_path, sizeof(full_path), "%s", zip_m3u_list.path);
               log_cb(RETRO_LOG_INFO, "->M3U: %s\n", zip_m3u_list.path);
               break;
         }

         argv = full_path;
      }

      /* Final argv has to be valid or empty */
      if (!path_is_valid(argv))
         argv = "";

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
      /* Do not allow JiffyDOS with non-floppies */
      if (dc_get_image_type(argv) == DC_IMAGE_TYPE_TAPE
       || dc_get_image_type(argv) == DC_IMAGE_TYPE_MEM)
         opt_jiffydos_allow = false;

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
      /* Do not allow REU with cartridges */
      opt_reu_allow = reu_allow(argv);
#endif

      /* REU image check */
      if (path_is_valid(argv))
      {
         char reu_path[RETRO_PATH_MAX] = {0};
         char reu_base[RETRO_PATH_MAX] = {0};
         char reu_name[RETRO_PATH_MAX] = {0};

         snprintf(reu_base, sizeof(reu_base), "%s", argv);
         path_basedir(reu_base);

         snprintf(reu_name, sizeof(reu_name), "%s", argv);
         snprintf(reu_name, sizeof(reu_name), "%s", path_basename(reu_name));
         path_remove_extension(reu_name);

         snprintf(reu_path, sizeof(reu_path), "%s%s%s", reu_base, reu_name, ".reu");

         /* Scan differently named REU in TEMP */
         if (!path_is_valid(reu_path) && strstartswith(argv, retro_temp_directory))
         {
            DIR *reu_dir;
            struct dirent *reu_dirp;
            reu_dir = opendir(retro_temp_directory);
            while ((reu_dirp = readdir(reu_dir)) != NULL)
            {
               if (strendswith(reu_dirp->d_name, ".reu"))
                  snprintf(reu_path, sizeof(reu_path), "%s%s%s", retro_temp_directory, ARCHDEP_DIR_SEP_STR, reu_dirp->d_name);
            }
            closedir(reu_dir);
         }

         if (path_is_valid(reu_path))
         {
            char reu_size[6] = {0};
            struct stat reu_stat;
            stat(reu_path, &reu_stat);
            snprintf(reu_size, sizeof(reu_size), "%u", (unsigned)reu_stat.st_size / 1024);

            Add_Option("-reu");
            Add_Option("-reusize");
            Add_Option(reu_size);
            Add_Option("+reuimagerw");
            Add_Option("-reuimage");
            Add_Option(reu_path);
         }
      }
#endif

#if defined(__XVIC__)
      /* Pretend to launch cartridge if using core option cartridge while launching
       * without content, because XVIC does not care about CartridgeFile resource */
      if (string_is_empty(argv) && !string_is_empty(vice_opt.CartridgeFile))
         argv = vice_opt.CartridgeFile;

      if (strendswith(argv, ".20"))
      {
         Add_Option("-cart2");
         /* For some unknown reason single cart at
          * $2000 also has to be in $6000.. */
         Add_Option(argv);
         Add_Option("-cart6");
      }
      else if (strendswith(argv, ".40"))
         Add_Option("-cart4");
      else if (strendswith(argv, ".60")
            || strendswith(argv, ".70"))
         Add_Option("-cart6");
      else if (strendswith(argv, ".a0"))
         Add_Option("-cartA");
      else if (strendswith(argv, ".b0"))
         Add_Option("-cartB");
      else if (strendswith(argv, ".prg")
            || strendswith(argv, ".crt")
            || strendswith(argv, ".rom")
            || strendswith(argv, ".bin")
            || strendswith(argv, ".m3u"))
      {
         /* There are PRGs that are actually carts, so we need to save the hassle
          * of mass renaming by differentiating them from regular program-PRGs.
          * Also separated cart PRGs meant to be assigned to specific memory
          * addresses require special care for hassle-free usage. */
         if (path_is_valid(argv))
         {
            char cart_base[RETRO_PATH_MAX] = {0};
            char cart_2000[RETRO_PATH_MAX] = {0};
            char cart_4000[RETRO_PATH_MAX] = {0};
            char cart_6000[RETRO_PATH_MAX] = {0};
            char cart_A000[RETRO_PATH_MAX] = {0};

            char cartmega_base[RETRO_PATH_MAX] = {0};
            char cartmega_nvram[RETRO_PATH_MAX] = {0};

            /* Need to examine the first playlist entry */
            if (strcasestr(argv, ".m3u"))
            {
               FILE *fd;
               char buf[RETRO_PATH_MAX] = {0};
               char basepath[RETRO_PATH_MAX] = {0};
               char fullpath[RETRO_PATH_MAX] = {0};
               snprintf(basepath, sizeof(basepath), "%s", argv);
               path_basedir(basepath);

               fd = fopen(argv, MODE_READ);
               if (fgets(buf, sizeof(buf), fd) != NULL)
               {
                  buf[strcspn(buf, "\r\n")] = 0;
                  snprintf(fullpath, sizeof(fullpath), "%s%s", basepath, buf);
               }
               fclose(fd);

               /* Only replace 'argv' with memory types */
               if (dc_get_image_type(fullpath) == DC_IMAGE_TYPE_MEM)
                  argv = fullpath;
            }

            int type = vic20_autodetect_cartridge_type(argv);

            switch (type)
            {
               case CARTRIDGE_VIC20_16KB_2000:
                  Add_Option("-cart2");
                  Add_Option(argv);
                  Add_Option("-cart6");
                  break;
               case CARTRIDGE_VIC20_8KB_2000:
               case CARTRIDGE_VIC20_4KB_2000:
                  Add_Option("-cart2");
                  break;
               case CARTRIDGE_VIC20_16KB_4000:
               case CARTRIDGE_VIC20_8KB_4000:
               case CARTRIDGE_VIC20_4KB_4000:
                  Add_Option("-cart4");
                  break;
               case CARTRIDGE_VIC20_16KB_6000:
               case CARTRIDGE_VIC20_8KB_6000:
               case CARTRIDGE_VIC20_4KB_6000:
                  Add_Option("-cart6");
                  break;
               case CARTRIDGE_VIC20_8KB_A000:
               case CARTRIDGE_VIC20_4KB_A000:
                  Add_Option("-cartA");
                  break;
               case CARTRIDGE_VIC20_4KB_B000:
                  Add_Option("-cartB");
                  break;
               case CARTRIDGE_VIC20_GENERIC:
                  Add_Option("-cartgeneric");
                  break;
               case CARTRIDGE_VIC20_MEGACART:
                  snprintf(cartmega_base, sizeof(cartmega_base), "%s", path_basename(argv));
                  path_remove_extension(cartmega_base);
                  snprintf(cartmega_nvram, sizeof(cartmega_nvram), "%s%s%s%s",
                           retro_save_directory, ARCHDEP_DIR_SEP_STR, cartmega_base, ".nvr");
                  Add_Option("-mcnvramfile");
                  Add_Option(cartmega_nvram);
                  Add_Option("-cartmega");
                  break;
               /* Separate ROM combination shenanigans */
               case CARTRIDGE_VIC20_CART_GAMEBASE:
               case CARTRIDGE_VIC20_CART_TOSEC:
               case CARTRIDGE_VIC20_CART_NOINTRO:
                  switch (type)
                  {
                     case CARTRIDGE_VIC20_CART_GAMEBASE: /* Gamebase */
                        snprintf(cart_base, sizeof(cart_base), "%s", argv);
                        path_remove_extension(cart_base);

                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "-2000", strlen("-2000"), "", strlen("")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "-4000", strlen("-4000"), "", strlen("")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "-6000", strlen("-6000"), "", strlen("")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "-a000", strlen("-a000"), "", strlen("")));

                        snprintf(cart_2000, sizeof(cart_2000), "%s%s%s", cart_base, "-2000", ".prg");
                        snprintf(cart_4000, sizeof(cart_4000), "%s%s%s", cart_base, "-4000", ".prg");
                        snprintf(cart_6000, sizeof(cart_6000), "%s%s%s", cart_base, "-6000", ".prg");
                        snprintf(cart_A000, sizeof(cart_A000), "%s%s%s", cart_base, "-a000", ".prg");
                        break;

                     case CARTRIDGE_VIC20_CART_TOSEC: /* TOSEC */
                        snprintf(cart_base, sizeof(cart_base), "%s", argv);
                        path_remove_extension(cart_base);
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[4000]", strlen("[4000]"), "[2000]", strlen("[2000]")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[6000]", strlen("[6000]"), "[2000]", strlen("[2000]")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[A000]", strlen("[A000]"), "[2000]", strlen("[2000]")));
                        snprintf(cart_2000, sizeof(cart_2000), "%s%s", cart_base, ".crt");

                        snprintf(cart_base, sizeof(cart_base), "%s", argv);
                        path_remove_extension(cart_base);
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[2000]", strlen("[2000]"), "[4000]", strlen("[4000]")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[6000]", strlen("[6000]"), "[4000]", strlen("[4000]")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[A000]", strlen("[A000]"), "[4000]", strlen("[4000]")));
                        snprintf(cart_4000, sizeof(cart_4000), "%s%s", cart_base, ".crt");

                        snprintf(cart_base, sizeof(cart_base), "%s", argv);
                        path_remove_extension(cart_base);
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[2000]", strlen("[2000]"), "[6000]", strlen("[6000]")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[4000]", strlen("[4000]"), "[6000]", strlen("[6000]")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[A000]", strlen("[A000]"), "[6000]", strlen("[6000]")));
                        snprintf(cart_6000, sizeof(cart_6000), "%s%s", cart_base, ".crt");

                        snprintf(cart_base, sizeof(cart_base), "%s", argv);
                        path_remove_extension(cart_base);
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[2000]", strlen("[2000]"), "[A000]", strlen("[A000]")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[4000]", strlen("[4000]"), "[A000]", strlen("[A000]")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "[6000]", strlen("[6000]"), "[A000]", strlen("[A000]")));
                        snprintf(cart_A000, sizeof(cart_A000), "%s%s", cart_base, ".crt");
                        break;

                     case CARTRIDGE_VIC20_CART_NOINTRO: /* No-Intro */
                        snprintf(cart_base, sizeof(cart_base), "%s", argv);
                        path_remove_extension(cart_base);
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$4000", strlen("$4000"), "$2000", strlen("$2000")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$6000", strlen("$6000"), "$2000", strlen("$2000")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$A000", strlen("$A000"), "$2000", strlen("$2000")));
                        snprintf(cart_2000, sizeof(cart_2000), "%s%s", cart_base, ".20");

                        snprintf(cart_base, sizeof(cart_base), "%s", argv);
                        path_remove_extension(cart_base);
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$2000", strlen("$2000"), "$4000", strlen("$4000")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$6000", strlen("$6000"), "$4000", strlen("$4000")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$A000", strlen("$A000"), "$4000", strlen("$4000")));
                        snprintf(cart_4000, sizeof(cart_4000), "%s%s", cart_base, ".40");

                        snprintf(cart_base, sizeof(cart_base), "%s", argv);
                        path_remove_extension(cart_base);
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$2000", strlen("$2000"), "$6000", strlen("$6000")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$4000", strlen("$4000"), "$6000", strlen("$6000")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$A000", strlen("$A000"), "$6000", strlen("$6000")));
                        snprintf(cart_6000, sizeof(cart_6000), "%s%s", cart_base, ".60");

                        snprintf(cart_base, sizeof(cart_base), "%s", argv);
                        path_remove_extension(cart_base);
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$2000", strlen("$2000"), "$A000", strlen("$A000")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$4000", strlen("$4000"), "$A000", strlen("$A000")));
                        snprintf(cart_base, sizeof(cart_base), "%s", string_replace_substring(cart_base, strlen(cart_base), "$6000", strlen("$6000"), "$A000", strlen("$A000")));
                        snprintf(cart_A000, sizeof(cart_A000), "%s%s", cart_base, ".a0");
                        break;
                  }

                  if (path_is_valid(cart_2000))
                  {
                     Add_Option("-cart2");
                     Add_Option(cart_2000);

                     /* If there is no proper $6000, single $2000 must be also inserted there.. */
                     if (!path_is_valid(cart_6000))
                     {
                        Add_Option("-cart6");
                        Add_Option(cart_2000);
                     }
                  }
                  if (path_is_valid(cart_4000))
                  {
                     Add_Option("-cart4");
                     Add_Option(cart_4000);
                  }
                  if (path_is_valid(cart_6000))
                  {
                     Add_Option("-cart6");
                     Add_Option(cart_6000);
                  }
                  if (path_is_valid(cart_A000))
                  {
                     Add_Option("-cartA");
                     Add_Option(cart_A000);
                  }

                  argv = "";
                  break;

               default:
                  if (!type && strendswith(argv, ".prg") && vice_opt.VIC20Memory == -1)
                  {
                     FILE *fd;
                     unsigned size = 0;
                     unsigned free = 3585;

                     fd   = fopen(argv, MODE_READ);
                     size = archdep_file_size(fd);
                     fclose(fd);

                     /* Default to internal memory if PRG size fits */
                     vic20mem_forced = 0;
                     if (size <= free)
                        break;

                     /* Otherwise use maximum memory */
                     vic20mem_forced = 5;
                  }
                  break;
            }
         }
      }

      /* Memory expansion force:
       * First from content path,
       * then from playlist entry after m3u parsing */
      vic20_mem_force(argv);
#elif defined(__XPLUS4__)
      /* Cartridges must not be injected like PRGs */
      if (strendswith(argv, ".crt") || strendswith(argv, ".bin"))
         Add_Option("-cart");
#endif

      if (strendswith(argv, "m3u"))
      {
         /* Parse the m3u file */
         dc_parse_m3u(dc, argv);
         is_fliplist = true;

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
         /* Do not allow JiffyDOS with non-floppies */
         if (!string_is_empty(dc->files[0]))
         {
            if (dc_get_image_type(dc->files[0]) == DC_IMAGE_TYPE_TAPE
             || dc_get_image_type(dc->files[0]) == DC_IMAGE_TYPE_MEM)
               opt_jiffydos_allow = false;
         }
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
         /* Do not allow REU with cartridges */
         opt_reu_allow = reu_allow(dc->files[0]);
#endif
#endif

#if defined(__XVIC__)
         /* Memory expansion force */
         if (vic20mem_forced < 0)
            vic20_mem_force(dc->files[0]);
#elif defined(__XPLUS4__)
         /* Cartridges must not be injected like PRGs */
         if (strendswith(dc->files[0], ".crt") || strendswith(dc->files[0], ".bin"))
            Add_Option("-cart");
#endif
      }
      else if (strendswith(argv, "vfl"))
      {
         /* Parse the vfl file */
         dc_parse_vfl(dc, argv);
         is_fliplist = true;
      }
      else if (path_is_directory(argv))
      {
         Add_Option("-iecdevice8");
         Add_Option("-device8");
         Add_Option("1");
         Add_Option("-fs8");

         /* Delayed autoload via kbdbuf_feed */
         if (!noautostart)
            autoloadrun = true;
      }

      /* GMod2 cart eeprom handling to 'saves' */
      {
         const char *content_path = !string_is_empty(dc->files[0]) ? dc->files[0] : argv;
         int carttype = (dc_get_image_type(content_path) == DC_IMAGE_TYPE_MEM) ? crt_getid(content_path) : 0;
         if (carttype == CARTRIDGE_GMOD2)
         {
            char eeprom_path[RETRO_PATH_MAX];
            char content_base[RETRO_PATH_MAX];
            strlcpy(content_base, content_path, sizeof(content_base));
            path_remove_extension(content_base);
            snprintf(eeprom_path, sizeof(eeprom_path), "%s%s%s.bin", retro_save_directory, ARCHDEP_DIR_SEP_STR, path_basename(content_base));
            if (!path_is_valid(eeprom_path))
            {
               uint8_t blank[GMOD2_EEPROM_SIZE] = {0};
               int ret = util_file_save(eeprom_path, blank, GMOD2_EEPROM_SIZE);
               log_cb(RETRO_LOG_INFO, "Initialized GMod2 eeprom: \"%s\".\n", eeprom_path);
            }

            Add_Option("-gmod2eepromimage");
            Add_Option(eeprom_path);
         }
      }

      if (!is_fliplist)
      {
         /* Add image name as autostart parameter */
         if (argv[0])
            Add_Option(argv);
      }
      else
      {
         /* Some debugging */
         log_cb(RETRO_LOG_INFO, "M3U/VFL parsed, %d file(s) found\n", dc->count);
         for (unsigned i = 0; i < dc->count; i++)
            log_cb(RETRO_LOG_DEBUG, "File %d: %s\n", i+1, dc->files[i]);

         if (!dc->command)
         {
            char option[RETRO_PATH_MAX] = {0};
            if (!string_is_empty(dc->load[0]))
               snprintf(option, sizeof(option), "%s:%s", dc->files[0], dc->load[0]);
            else
               snprintf(option, sizeof(option), "%s", dc->files[0]);

            /* Add first disk from list as autostart parameter */
            if (dc->count != 0)
               Add_Option(option);
         }
         else
         {
            /* Re-parse command line from M3U #COMMAND: */
            log_cb(RETRO_LOG_INFO, "Starting game from command line: %s\n", dc->command);
            vice_opt.Model = 99; /* set model to unknown for custom settings - prevents overriding of command line options */
            parse_cmdline(dc->command);
            /* Reset parameters list for VICE */
            PARAMCOUNT = 0;
            single_image = false;
         }
      }
   }

   /* It might be single_image initially, but changed by M3U file #COMMAND line */
   if (!single_image)
   {
      /* Command doesn't start with core name, so add it first */
      if (ARGUC == 0 || strcmp(ARGUV[0], CORE_NAME) != 0)
         Add_Option(CORE_NAME);

      bool is_flipname_param = false;
      /* Scan vice arguments for special processing */
      for (i = 0; i < ARGUC; i++)
      {
         const char* arg = ARGUV[i];

         /* Previous arg was '-flipname' */
         if (is_flipname_param)
         {
            is_flipname_param = false;
            /* Parse the vfl file, don't pass to vice */
            dc_parse_vfl(dc, arg);
            /* Don't pass -flipname argument to vice - it has no use of it */
            /* and we won't have to take care of cleaning it up */
            is_fliplist = true;
         }
         else if (!strcmp(arg, "-j1"))
         {
            cur_port = 1;
            cur_port_locked = true;
         }
         else if (!strcmp(arg, "-j2"))
         {
            cur_port = 2;
            cur_port_locked = true;
         }
         else if (strendswith(arg, "m3u"))
         {
            /* Parse the m3u file, don't pass to vice */
            dc_parse_m3u(dc, arg);
            is_fliplist = true;
         }
         else if (!strcmp(arg, "-flipname"))
         {
            /* Set flag for next arg */
            is_flipname_param = true;
         }
         else if (!strcmp(arg, "-noautostart"))
         {
            /* User ask to not automatically start image in drive */
            noautostart = true;
         }
         else if (!strcmp(arg, "-autostart"))
         {
            /* User ask to automatically start image in drive */
            noautostart = false;
         }
         else
         {
            /* Fill cmd arg path from argv if there is none */
            if (strstr(arg, ".") && !strstr(arg, ARCHDEP_DIR_SEP_STR))
            {
               char arg_path[RETRO_PATH_MAX] = {0};
               char arg_full[RETRO_PATH_MAX] = {0};
               strlcpy(arg_path, argv, sizeof(arg_path));
               path_basedir(arg_path);
               strlcpy(arg_full, arg_path, sizeof(arg_full));
               strcat(arg_full, arg);
               Add_Option(arg_full);
            }
            else
               Add_Option(arg);
         }
      }

      if (is_fliplist)
      {
         /* Some debugging */
         log_cb(RETRO_LOG_INFO, "M3U/VFL parsed, %d file(s) found\n", dc->count);
      }
   }

#if defined(__XVIC__) || defined(__XPLUS4__)
   /* Disable floppy drive when using carts with cores that won't disable it via autostart */
   /* Also disable injection autostart with proper carts to avoid screen garbage */
   for (int i = 0; i < PARAMCOUNT; i++)
   {
      if (strstr(XARGV[i], "-cart") && XARGV[i][0] == '-')
      {
         Add_Option("-drive8type");
         Add_Option("0");
         noautostart = noautostart_locked = true;
         break;
      }
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
   if (!opt_jiffydos_allow)
      opt_jiffydos = false;
#endif

   return 0;
}

void autodetect_drivetype(int unit)
{
   int ret = 0;
   int drive_type = 0;
   int set_drive_type = 0;
   char drive_type_resource_var[20] = {0};
   char attached_image_safe[RETRO_PATH_MAX] = {0};
   const char *attached_image = NULL;

   /* Autodetect drive type */
   const vdrive_t *vdrive;
   const struct disk_image_s *diskimg;

   snprintf(drive_type_resource_var, sizeof(drive_type_resource_var), "Drive%dType", unit);
   resources_get_int(drive_type_resource_var, &drive_type);
   attached_image = file_system_get_disk_name(unit, 0);
   strlcpy(attached_image_safe, attached_image, sizeof(attached_image_safe));

   vdrive = file_system_get_vdrive(unit);
   if (vdrive == NULL)
      log_cb(RETRO_LOG_ERROR, "Failed to get vdrive reference for unit %d.\n", unit);
   else
   {
      diskimg = vdrive->image;
      if (diskimg == NULL)
         log_cb(RETRO_LOG_ERROR, "Failed to get disk image for unit %d.\n", unit);
      else
      {
         /* G64/G71 exceptions for preventing unwanted drive type sets */
         if (diskimg->type == DISK_IMAGE_TYPE_G64)
            set_drive_type = DRIVE_TYPE_1541II;
         else if (diskimg->type == DISK_IMAGE_TYPE_G71)
            set_drive_type = DRIVE_TYPE_1571;
         /* Force 1541 to 1541-II */
         else if (diskimg->type == DRIVE_TYPE_1541)
#if defined(__X128__)
            set_drive_type = DRIVE_TYPE_DEFAULT;
#else
            set_drive_type = DRIVE_TYPE_1541II;
#endif
         else
            set_drive_type = diskimg->type;

         if (set_drive_type == drive_type)
            return;

         log_cb(RETRO_LOG_INFO, "Autodetected image type %u.\n", diskimg->type);
         if (log_resources_set_int(drive_type_resource_var, set_drive_type) < 0)
            log_cb(RETRO_LOG_ERROR, "Failed to set drive type.\n");

         /* Change from 1581 to 1541 will not detect disk properly without reattaching (?!) */
         file_system_detach_disk(unit, 0);
         ret = file_system_attach_disk(unit, 0, attached_image);
         if (ret < 0)
            file_system_attach_disk(unit, 0, attached_image_safe);

         /* Don't bother with drive sound muting when autoloadwarp is on */
         if (opt_autoloadwarp & AUTOLOADWARP_DISK)
            return;

         /* Drive motor sound keeps on playing if the drive type is changed while the motor is running */
         /* Also happens when toggling TDE */
         switch (set_drive_type)
         {
            case DRIVE_TYPE_1541:
            case DRIVE_TYPE_1541II:
            case DRIVE_TYPE_1571:
               resources_set_int("DriveSoundEmulationVolume", vice_opt.DriveSoundEmulation);
               break;
            default:
               resources_set_int("DriveSoundEmulationVolume", 0);
               break;
         }
      }
   }
}

void update_work_disk(void)
{
   request_update_work_disk   = false;
   const char* attached_image = NULL;
   unsigned work_disk_type    = opt_work_disk_type;
   unsigned work_disk_unit    = opt_work_disk_unit;

   /* Path vars */
   char work_disk_basename[10]             = {0};
   char work_disk_extension[4]             = {0};
   char work_disk_filename[RETRO_PATH_MAX] = {0};
   char work_disk_filepath[RETRO_PATH_MAX] = {0};

   snprintf(work_disk_basename, sizeof(work_disk_basename), "%s", "vice_work");

   switch (work_disk_type)
   {
      default:
      case DISK_IMAGE_TYPE_D64:
         snprintf(work_disk_extension, sizeof(work_disk_extension), "%s", "d64");
         break;
      case DISK_IMAGE_TYPE_D71:
         snprintf(work_disk_extension, sizeof(work_disk_extension), "%s", "d71");
         break;
      case DISK_IMAGE_TYPE_D81:
         snprintf(work_disk_extension, sizeof(work_disk_extension), "%s", "d81");
         break;
      case DISK_IMAGE_TYPE_FS:
         snprintf(work_disk_extension, sizeof(work_disk_extension), "%s", "");
         break;
   }

   if (strcmp(work_disk_extension, ""))
      snprintf(work_disk_filename, sizeof(work_disk_filename), "%s.%s", work_disk_basename, work_disk_extension);
   else
      snprintf(work_disk_filename, sizeof(work_disk_filename), "%s", work_disk_basename);

   path_join((char*)&work_disk_filepath, retro_save_directory, work_disk_filename);

   /* Skip if device unit collides with autostart */
   if (!string_is_empty(full_path) && work_disk_unit == 8 && dc->unit == 8)
      work_disk_type = 0;
   if (work_disk_type)
   {
      /* Create disk */
      if (!path_is_valid(work_disk_filepath))
      {
         switch (work_disk_type)
         {
            default:
               {
                  /* Label format */
                  char format_name[28];
                  snprintf(format_name, sizeof(format_name), "%s-%s", "work", work_disk_extension);
                  charset_petconvstring((uint8_t *)format_name, 0);

                  if (vdrive_internal_create_format_disk_image(work_disk_filepath, format_name, work_disk_type))
                     log_cb(RETRO_LOG_INFO, "Work disk creation failed: \"%s\"\n", work_disk_filepath);
                  else
                     log_cb(RETRO_LOG_INFO, "Work disk created: \"%s\"\n", work_disk_filepath);
               }
               break;
            case DISK_IMAGE_TYPE_FS:
               if (path_mkdir(work_disk_filepath))
                  log_cb(RETRO_LOG_INFO, "Work directory creation failed: \"%s\"\n", work_disk_filepath);
               else
                  log_cb(RETRO_LOG_INFO, "Work directory created: \"%s\"\n", work_disk_filepath);
               break;
         }
      }

      /* Attach disk */
      if (path_is_valid(work_disk_filepath))
      {
         /* Detach previous disks */
         if (string_is_empty(full_path) && (attached_image = file_system_get_disk_name(8, 0)) != NULL)
            file_system_detach_disk(8, 0);

         if ((attached_image = file_system_get_disk_name(9, 0)) != NULL)
         {
            file_system_detach_disk(9, 0);
            log_resources_set_int("Drive9Type", DRIVE_TYPE_NONE);
         }

         if (string_is_empty(full_path) && (attached_image = fsdevice_get_path(8)) != NULL)
         {
            log_resources_set_int("IECDevice8", 0);
            log_resources_set_int("FileSystemDevice8", 0);
            log_resources_set_string("FSDevice8Dir", "");
         }

         if ((attached_image = fsdevice_get_path(9)) != NULL)
         {
            log_resources_set_int("IECDevice9", 0);
            log_resources_set_int("FileSystemDevice9", 0);
            log_resources_set_string("FSDevice9Dir", "");
         }

         switch (work_disk_type)
         {
            default:
               if (work_disk_unit == 9)
                  log_resources_set_int("Drive9Type", work_disk_type);
               file_system_attach_disk(work_disk_unit, 0, work_disk_filepath);
               autodetect_drivetype(work_disk_unit);
               log_cb(RETRO_LOG_INFO, "Work disk \"%s\" attached to drive #%d.\n", work_disk_filepath, work_disk_unit);
               break;
            case DISK_IMAGE_TYPE_FS:
               switch (work_disk_unit)
               {
                  default:
                  case 8:
                     log_resources_set_int("IECDevice8", 1);
                     log_resources_set_int("FileSystemDevice8", 1);
                     log_resources_set_string("FSDevice8Dir", work_disk_filepath);
                     break;
                  case 9:
                     log_resources_set_int("IECDevice9", 1);
                     log_resources_set_int("FileSystemDevice9", 1);
                     log_resources_set_string("FSDevice9Dir", work_disk_filepath);
                     break;
               }
               log_cb(RETRO_LOG_INFO, "Work directory \"%s\" attached to drive #%d.\n", work_disk_filepath, work_disk_unit);
               break;
         }

         if (string_is_empty(full_path))
            display_current_image(work_disk_filename, true);
      }
   }
   else
   {
      /* Detach work disk if disabled while running */
      if ((attached_image = file_system_get_disk_name(8, 0)) != NULL && strstr(attached_image, work_disk_basename))
      {
         if (string_is_empty(full_path) || (!string_is_empty(full_path) && !strstr(full_path, work_disk_filename)))
         {
            log_cb(RETRO_LOG_INFO, "Work disk \"%s\" detached from drive #%d.\n", attached_image, 8);
            file_system_detach_disk(8, 0);
            log_resources_set_int("Drive8Type", DRIVE_TYPE_DEFAULT);
            if (string_is_empty(full_path))
               display_current_image("", false);
         }
      }

      if ((attached_image = fsdevice_get_path(8)) != NULL && strstr(attached_image, work_disk_basename))
      {
         if (string_is_empty(full_path) || (!string_is_empty(full_path) && !strstr(full_path, work_disk_filename)))
         {
            log_cb(RETRO_LOG_INFO, "Work directory \"%s\" detached from drive #%d.\n", attached_image, 8);
            log_resources_set_int("IECDevice8", 0);
            log_resources_set_int("FileSystemDevice8", 0);
            if (string_is_empty(full_path))
               display_current_image("", false);
         }
      }

      if ((attached_image = file_system_get_disk_name(9, 0)) != NULL && strstr(attached_image, work_disk_basename))
      {
         log_cb(RETRO_LOG_INFO, "Work disk \"%s\" detached from drive #%d.\n", attached_image, 9);
         file_system_detach_disk(9, 0);
         log_resources_set_int("Drive9Type", DRIVE_TYPE_NONE);
         if (string_is_empty(full_path))
            display_current_image("", false);
      }

      if ((attached_image = fsdevice_get_path(9)) != NULL && strstr(attached_image, work_disk_basename))
      {
         log_cb(RETRO_LOG_INFO, "Work directory \"%s\" detached from drive #%d.\n", attached_image, 9);
         log_resources_set_int("IECDevice9", 0);
         log_resources_set_int("FileSystemDevice9", 0);
         if (string_is_empty(full_path))
            display_current_image("", false);
      }
   }
}

/* Update autostart image from vice and add disk in drive to fliplist */
void update_from_vice(void)
{
   const char* attachedImage = NULL;

   free(autostartString);
   autostartString = NULL;
   free(autostartProgram);
   autostartProgram = NULL;

   autostartString  = x_strdup(cmdline_get_autostart_string());
   autostartProgram = x_strdup(dc->load[dc->index]);

   if (!autostartString && !string_is_empty(dc->files[dc->index]))
   {
      free(autostartString);
      autostartString = x_strdup(dc->files[dc->index]);
   }
   else if (!autostartString && !string_is_empty(full_path))
   {
      free(autostartString);
      autostartString = x_strdup(full_path);
   }

#if defined(__XVIC__)
   /* Fake vicerc cartridge file as autostart launch if not using the core option */
   if (string_is_empty(vice_opt.CartridgeFile))
   {
      const char *cartfile_vicerc;

      resources_get_string("CartridgeFile", &cartfile_vicerc);
      if (!string_is_empty(cartfile_vicerc))
      {
         sprintf(vice_opt.CartridgeFile, "%s", cartfile_vicerc);
         process_cmdline("");
         reload_restart();
         return;
      }
   }
#endif

   if (autostartString)
      log_cb(RETRO_LOG_INFO, "Image for autostart: \"%s\".\n", autostartString);
   else
      log_cb(RETRO_LOG_INFO, "No image for autostart.\n");

   /* If flip list is empty, get current tape or floppy image name and add to the list */
   if (dc->count == 0)
   {
      /* Do not add core option cartridges to the list */
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__) || defined(__X128__)
      if ((attachedImage = cartridge_get_filename_by_slot(cart_getid_slotmain())) != NULL
            && strcmp(vice_opt.CartridgeFile, attachedImage))
#elif defined(__XVIC__)
      if ((attachedImage = generic_get_file_name(0)) != NULL
            && strcmp(vice_opt.CartridgeFile, attachedImage))
#else
      if ((attachedImage = cartridge_get_filename_by_slot(0)) != NULL
#if !defined(__XPET__)
            && strcmp(vice_opt.CartridgeFile, attachedImage)
#endif
         )
#endif
      {
         dc->unit = 0;
         dc_add_file(dc, attachedImage, NULL, NULL, NULL);
      }
      else if ((attachedImage = tape_get_file_name(TAPEPORT_PORT_1)) != NULL)
      {
         dc->unit = 1;
         dc_add_file(dc, attachedImage, NULL, NULL, NULL);
      }
      else if (!string_is_empty(full_path) && strendswith(full_path, "tcrt"))
      {
         /* Special case for Tapecarts */
         dc->unit = 1;
         dc_add_file(dc, full_path, NULL, NULL, NULL);
      }
      else
      {
         /* Only add images to the list from device 8, otherwise leads to confusion when other devices have disks,
          * because Disk Control operates only on device 8 for now. */
         int unit = 8;
         if ((attachedImage = file_system_get_disk_name(unit, 0)) != NULL)
         {
            dc->unit = unit;
            dc_add_file(dc, attachedImage, NULL, NULL, NULL);
         }
      }
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
   /* Disable JiffyDOS with tapes and carts */
   if (opt_jiffydos && dc->unit <= 1 && dc->count > 0)
   {
      opt_jiffydos_allow = opt_jiffydos = false;
      reload_restart();
      return;
   }
#endif

   /* Logging */
   if (dc->count > 0)
   {
      if (dc->unit == 1)
         log_cb(RETRO_LOG_INFO, "Tape image list has %d file(s)\n", dc->count);
      else if (dc->unit >= 8 && dc->unit <= 11)
         log_cb(RETRO_LOG_INFO, "Drive #%d image list has %d file(s)\n", dc->unit, dc->count);
      else if (dc->unit == 0)
         log_cb(RETRO_LOG_INFO, "Cartridge image list has %d file(s)\n", dc->count);

      for (unsigned i = 0; i < dc->count; i++)
         log_cb(RETRO_LOG_DEBUG, "File %d: %s\n", i+1, dc->files[i]);
   }

   /* If flip list is not empty, but there is no image attached to drive, attach the first one from list.
    * This can only happen if flip list was loaded via cmd file or from m3u with #COMMAND */
   if (dc->count)
   {
      if (dc->unit == 1)
      {
         if ((attachedImage = tape_get_file_name(TAPEPORT_PORT_1)) == NULL)
         {
            attachedImage = dc->files[0];
            autostartProgram = x_strdup(dc->load[0]);
            /* Don't attach if we will autostart from it just in a moment */
            if (autostartString != NULL || noautostart)
            {
               log_cb(RETRO_LOG_INFO, "Attaching first tape \"%s\".\n", attachedImage);
               tape_image_attach(dc->unit, attachedImage);
            }
         }
         else
            /* In 3.9 some tapes fail to start properly without forced restart.. */
            request_restart = true;
      }
      else if (dc->unit == 8)
      {
         if ((attachedImage = file_system_get_disk_name(dc->unit, 0)) == NULL)
         {
            attachedImage = dc->files[0];
            autostartProgram = x_strdup(dc->load[0]);
            /* Don't attach if we will autostart from it just in a moment */
            if (autostartString != NULL || noautostart)
            {
               log_cb(RETRO_LOG_INFO, "Attaching first disk \"%s\" to drive #%d.\n", attachedImage, dc->unit);
               file_system_attach_disk(dc->unit, 0, attachedImage);
            }
         }

         /* Append rest of the disks to the config if M3U is a MultiDrive-M3U */
         if (strstr(full_path, "(MD)") != NULL || opt_floppy_multidrive)
         {
            for (unsigned i = 1; i < dc->count; i++)
            {
               if (i < NUM_DISK_UNITS)
               {
                  if (strstr(dc->labels[i], M3U_SAVEDISK_LABEL))
                     continue;

                  log_cb(RETRO_LOG_INFO, "Attaching disk \"%s\" to drive #%d.\n", dc->files[i], dc->unit + i);
                  file_system_attach_disk(dc->unit + i, 0, dc->files[i]);
                  autodetect_drivetype(dc->unit + i);
               }
               else
               {
                  log_cb(RETRO_LOG_WARN, "Too many disks for MultiDrive!\n");
                  break;
               }
            }
         }
      }
      else if (dc->unit == 0)
      {
         if (attachedImage == NULL)
         {
            attachedImage = dc->files[0];
            autostartProgram = NULL;
            /* Don't attach if we will autostart from it just in a moment */
            if (autostartString != NULL || noautostart)
            {
               log_cb(RETRO_LOG_INFO, "Attaching first cart \"%s\".\n", attachedImage);
#if defined(__XVIC__)
               cartridge_attach_image(vic20_autodetect_cartridge_type(attachedImage), attachedImage);
#elif defined(__XPLUS4__)
               cartridge_attach_image(CARTRIDGE_PLUS4_DETECT, attachedImage);
#else
               cartridge_attach_image(dc->unit, attachedImage);
#endif
            }
         }
         else
            /* Due to a bug in 3.5, some carts (Up 'n Down) get stuck in black screen when
             * launched via cmdline, but continue with reset and subsequent attaches.. */
            request_restart = true;
      }
   }

   /* Scan for save disk 0, append if exists */
   if (dc)
   {
      bool file_check = dc_save_disk_toggle(dc, true, false);
      /* Insert save disk only if not booting from floppies */
      if (file_check)
         dc_save_disk_toggle(dc, false, (dc_get_image_type(dc->files[0]) == DC_IMAGE_TYPE_FLOPPY) ? false : true);
   }

   /* Disable autostart only with disks or tapes */
   if (!string_is_empty(attachedImage))
   {
      if (noautostart)
         autostart_disable();
      else if (!noautostart && !string_is_empty(autostartString) &&
            strcmp(autostartString, attachedImage) &&
            string_is_empty(autostartProgram) &&
            dc_get_image_type(attachedImage) != DC_IMAGE_TYPE_MEM)
      {
         free(autostartString);
         autostartString = NULL;
      }
   }

   /* If there an image attached, but autostart is empty, autostart from the image */
   if (string_is_empty(autostartString) && !string_is_empty(attachedImage) && !noautostart && !CMDFILE[0])
   {
      log_cb(RETRO_LOG_INFO, "Autostarting from attached or first image \"%s\".\n", attachedImage);
      autostartString = x_strdup(attachedImage);
      if (!string_is_empty(autostartProgram))
         charset_petconvstring((uint8_t *)autostartProgram, 0);

      autostart_autodetect(autostartString, autostartProgram, 0, AUTOSTART_MODE_RUN);
   }

   /* If vice has image attached to drive, tell libretro that the 'tray' is closed */
   if (!string_is_empty(attachedImage))
   {
      dc->eject_state = false;
      display_current_image(dc->labels[dc->index], true);
   }
   else if (autostartString && strendswith(autostartString, "vsf"))
   {
      char vsf_label[RETRO_PATH_MAX];
      fill_pathname(vsf_label, path_basename(autostartString), "", sizeof(vsf_label));
      dc->eject_state = false;
      display_current_image(vsf_label, true);
   }
   else if (!opt_work_disk_type)
   {
      dc->eject_state = true;
      display_current_image("", false);
   }
}

void build_params(void)
{
   int i;

   if (PARAMCOUNT == 0)
   {
      /* No game loaded - set command line to core name */
      Add_Option(CORE_NAME);
   }

   for (i = 0; i < PARAMCOUNT; i++)
   {
      xargv_cmd[i] = (char*)(XARGV[i]);
      log_cb(RETRO_LOG_INFO, "Arg%d: %s\n", i, XARGV[i]);
   }

   xargv_cmd[PARAMCOUNT] = NULL;
}

extern char archdep_startup_error[];

static void archdep_startup_error_log_lines()
{
   /* Message may contain several lines, log them separately for better readability. */
   for (char *p = archdep_startup_error, *p_end; strlen(p); p = p_end)
   {
      if (!(p_end = strchr(p, '\n')))
         p_end = p+strlen(p);
      else
         *(p_end++) = 0;
      log_cb(RETRO_LOG_ERROR, "VICE: %s\n", p);
   }
}

int pre_main(void)
{
   int argc = PARAMCOUNT;

   /* start core with full params */
   build_params();

   *archdep_startup_error = 0;
   if (main_program(argc, (char**)xargv_cmd) < 0)
   {
      log_cb(RETRO_LOG_WARN, "Core startup failed with error:\n");
      archdep_startup_error_log_lines();
      log_cb(RETRO_LOG_INFO, "Core startup retry without parameters.\n");

      /* Show only first line in message to indicate something went wrong. */
      if (!string_is_empty(archdep_startup_error))
      {
         struct retro_message rmsg;
         rmsg.msg = archdep_startup_error;
         rmsg.frames = 500;
         environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &rmsg);
      }

      /* start core with empty params */
      xargv_cmd[0] = CORE_NAME;
      xargv_cmd[1] = NULL;
      argc = 1;

      *archdep_startup_error = 0;
      if (main_program(argc, (char**)xargv_cmd) < 0)
      {
         log_cb(RETRO_LOG_ERROR, "Core startup without parameters failed with error:\n");
         archdep_startup_error_log_lines();
         environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
      }
   }

   return 0;
}

static void content_exceptions(void)
{
   /* Mute floppy startup sound when not using floppies */
   {
      const char *content_path = full_path;
      if (!string_is_empty(dc->files[dc->index]))
         content_path = dc->files[dc->index];

      if (dc_get_image_type(content_path) == DC_IMAGE_TYPE_TAPE ||
          dc_get_image_type(content_path) == DC_IMAGE_TYPE_MEM)
         sound_drive_mute = true;

#if defined(__XVIC__)
      /* M3U has to be inspected */
      if (strcasestr(content_path, ".m3u"))
      {
         FILE *fd;
         char buf[RETRO_PATH_MAX] = {0};
         char basepath[RETRO_PATH_MAX] = {0};
         char fullpath[RETRO_PATH_MAX] = {0};
         snprintf(basepath, sizeof(basepath), "%s", content_path);
         path_basedir(basepath);

         fd = fopen(content_path, MODE_READ);
         if (fgets(buf, sizeof(buf), fd) != NULL)
         {
            buf[strcspn(buf, "\r\n")] = 0;
            snprintf(fullpath, sizeof(fullpath), "%s%s", basepath, buf);
         }
         fclose(fd);

         if (dc_get_image_type(fullpath) == DC_IMAGE_TYPE_TAPE ||
             dc_get_image_type(fullpath) == DC_IMAGE_TYPE_MEM)
            sound_drive_mute = true;
      }
#endif
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__) || defined(__X128__) || defined(__XVIC__)
   /* Automatic model detection */
   if (!opt_model_auto_locked && !string_is_empty(full_path))
   {
      if (strstr(full_path, "NTSC") ||
          strstr(full_path, "(USA)") ||
          strstr(full_path, "(Japan)") ||
          strstr(full_path, "(Japan, USA)"))
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
         request_model_auto_set = C64MODEL_C64_NTSC;
#elif defined(__X128__)
         request_model_auto_set = C128MODEL_C128_NTSC;
#elif defined(__XVIC__)
         request_model_auto_set = VIC20MODEL_VIC20_NTSC;
#endif
      }

      if (strstr(full_path, "PAL") ||
          strstr(full_path, "(Europe)") ||
          strstr(full_path, "(Finland)") ||
          strstr(full_path, "(France)") ||
          strstr(full_path, "(Germany)") ||
          strstr(full_path, "(Netherlands)") ||
          strstr(full_path, "(Sweden)"))
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
         request_model_auto_set = C64MODEL_C64_PAL;
#elif defined(__X128__)
         request_model_auto_set = C128MODEL_C128_PAL;
#elif defined(__XVIC__)
         request_model_auto_set = VIC20MODEL_VIC20_PAL;
#endif
      }

#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
      if (strstr(full_path, "(GS)"))
         request_model_auto_set = C64MODEL_C64_GS;
      else if (strstr(full_path, "(MAX)"))
         request_model_auto_set = C64MODEL_ULTIMAX;
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
      /* Respect the revision */
      switch (request_model_auto_set)
      {
         case C64MODEL_C64_PAL:
         case C64MODEL_C64C_PAL:
            if (vice_opt.Model == C64MODEL_C64_NTSC || vice_opt.Model == C64MODEL_C64_PAL)
               request_model_auto_set = C64MODEL_C64_PAL;
            else if (vice_opt.Model == C64MODEL_C64C_NTSC || vice_opt.Model == C64MODEL_C64C_PAL)
               request_model_auto_set = C64MODEL_C64C_PAL;
            break;
         case C64MODEL_C64_NTSC:
         case C64MODEL_C64C_NTSC:
            if (vice_opt.Model == C64MODEL_C64_NTSC || vice_opt.Model == C64MODEL_C64_PAL)
               request_model_auto_set = C64MODEL_C64_NTSC;
            else if (vice_opt.Model == C64MODEL_C64C_NTSC || vice_opt.Model == C64MODEL_C64C_PAL)
               request_model_auto_set = C64MODEL_C64C_NTSC;
            break;
      }

      if (request_model_auto_set == C64MODEL_C64_NTSC)
         log_cb(RETRO_LOG_INFO, "Requesting C64 NTSC mode.\n");
      else if (request_model_auto_set == C64MODEL_C64C_NTSC)
         log_cb(RETRO_LOG_INFO, "Requesting C64C NTSC mode.\n");
      else if (request_model_auto_set == C64MODEL_C64_PAL)
         log_cb(RETRO_LOG_INFO, "Requesting C64 PAL mode.\n");
      else if (request_model_auto_set == C64MODEL_C64C_PAL)
         log_cb(RETRO_LOG_INFO, "Requesting C64C PAL mode.\n");
      else if (request_model_auto_set == C64MODEL_C64_GS)
         log_cb(RETRO_LOG_INFO, "Requesting C64GS mode.\n");
      else if (request_model_auto_set == C64MODEL_ULTIMAX)
         log_cb(RETRO_LOG_INFO, "Requesting ULTIMAX mode.\n");
#elif defined(__X128__)
      /* Respect the revision */
      switch (request_model_auto_set)
      {
         case C128MODEL_C128_PAL:
         case C128MODEL_C128D_PAL:
         case C128MODEL_C128DCR_PAL:
            if (vice_opt.Model == C128MODEL_C128_NTSC || vice_opt.Model == C128MODEL_C128_PAL)
               request_model_auto_set = C128MODEL_C128_PAL;
            else if (vice_opt.Model == C128MODEL_C128D_NTSC || vice_opt.Model == C128MODEL_C128D_PAL)
               request_model_auto_set = C128MODEL_C128D_PAL;
            else if (vice_opt.Model == C128MODEL_C128DCR_NTSC || vice_opt.Model == C128MODEL_C128DCR_PAL)
               request_model_auto_set = C128MODEL_C128DCR_PAL;
            break;
         case C128MODEL_C128_NTSC:
         case C128MODEL_C128D_NTSC:
         case C128MODEL_C128DCR_NTSC:
            if (vice_opt.Model == C128MODEL_C128_NTSC || vice_opt.Model == C128MODEL_C128_PAL)
               request_model_auto_set = C128MODEL_C128_NTSC;
            else if (vice_opt.Model == C128MODEL_C128D_NTSC || vice_opt.Model == C128MODEL_C128D_PAL)
               request_model_auto_set = C128MODEL_C128D_NTSC;
            else if (vice_opt.Model == C128MODEL_C128DCR_NTSC || vice_opt.Model == C128MODEL_C128DCR_PAL)
               request_model_auto_set = C128MODEL_C128DCR_NTSC;
            break;
      }

      if (request_model_auto_set == C128MODEL_C128_NTSC)
         log_cb(RETRO_LOG_INFO, "Requesting C128 NTSC mode.\n");
      else if (request_model_auto_set == C128MODEL_C128D_NTSC)
         log_cb(RETRO_LOG_INFO, "Requesting C128D NTSC mode.\n");
      else if (request_model_auto_set == C128MODEL_C128DCR_NTSC)
         log_cb(RETRO_LOG_INFO, "Requesting C128DCR NTSC mode.\n");
      else if (request_model_auto_set == C128MODEL_C128_PAL)
         log_cb(RETRO_LOG_INFO, "Requesting C128 PAL mode.\n");
      else if (request_model_auto_set == C128MODEL_C128D_PAL)
         log_cb(RETRO_LOG_INFO, "Requesting C128D PAL mode.\n");
      else if (request_model_auto_set == C128MODEL_C128DCR_PAL)
         log_cb(RETRO_LOG_INFO, "Requesting C128DCR PAL mode.\n");
#elif defined(__XVIC__)
      if (request_model_auto_set == VIC20MODEL_VIC20_NTSC)
         log_cb(RETRO_LOG_INFO, "Requesting NTSC mode.\n");
      else if (request_model_auto_set == VIC20MODEL_VIC20_PAL)
         log_cb(RETRO_LOG_INFO, "Requesting PAL mode.\n");
#endif

      /* Lock automatic model when requested */
      if (request_model_auto_set > -1)
         opt_model_auto_locked = true;
   }
#endif

   /* Reset file path tag model force */
   request_model_prev = -1;

   /* Tapecart needs TDE */
   if (
         (!string_is_empty(full_path) && strendswith(full_path, "tcrt")) ||
         (!string_is_empty(dc->files[0]) && strendswith(dc->files[0], "tcrt"))
      )
   {
      if (!vice_opt.DriveTrueEmulation)
      {
         log_cb(RETRO_LOG_INFO, "Tapecart does not work without TDE, enabling...\n");
         toggle_tde(1);
         reload_restart();
      }

      /* 3.7: Content is garbage without restart even without TDE change (?!) */
      request_restart = true;
   }

   /* D2M/D4M does not accept TDE */
   if (vice_opt.DriveTrueEmulation && (
         (!string_is_empty(full_path) && (strendswith(full_path, "d2m") || strendswith(full_path, "d4m"))) ||
         (!string_is_empty(dc->files[0]) && (strendswith(dc->files[0], "d2m") || strendswith(dc->files[0], "d4m")))
      ))
   {
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
      /* No JiffyDOS without TDE */
      opt_jiffydos_allow = opt_jiffydos = false;
#endif
#if defined(__XSCPU64__)
      /* SuperCPU kernal must be "internal" */
      opt_supercpu_kernal = 0;
#endif

      log_cb(RETRO_LOG_INFO, "D2M/D4M does not work with TDE, disabling...\n");
      toggle_tde(0);
      reload_restart();
   }
}

void reload_restart(void)
{
   /* Clear request */
   request_reload_restart = false;

   /* Reset tick */
   retro_now = 0;

   /* Reset autoloadwarp audio ignore */
   audio_is_ignored = false;

   /* Prevent reset audio pop */
   sound_volume_counter_reset();

   /* Disable warp */
   if (vsync_get_warp_mode())
      vsync_set_warp_mode(0);

   /* Cleanup previous autostart without resource default reset */
   initcmdline_cleanup(false);

   /* Reset autostart */
   autostart_reset();

   /* Reset Datasette */
   datasette_control(TAPEPORT_PORT_1, DATASETTE_CONTROL_RESET);

   /* Model and TDE & JiffyDOS exceptions per content */
   content_exceptions();

   /* Reset UI state */
   retro_ui_finalized = false;

   /* No need to update variables again on startup */
   if (runstate > RUNSTATE_FIRST_START)
      update_variables();

   /* Some resources are not set until we call this */
   ui_init_finalize();

   /* And process command line */
   build_params();
   if (initcmdline_restart(PARAMCOUNT, (char**)xargv_cmd) < 0)
   {
      log_cb(RETRO_LOG_ERROR, "Restart failed\n");
      /* Nevermind, the core is already running */
   }

   /* Now read disk image and autostart file (may be the same or not) from vice */
   update_from_vice();
}

/* Audio buffer */
static void ensure_output_audio_buffer_capacity(int32_t capacity)
{
   if (capacity <= output_audio_buffer.capacity) {
      return;
   }

   output_audio_buffer.data = realloc(output_audio_buffer.data, capacity * sizeof(*output_audio_buffer.data));
   output_audio_buffer.capacity = capacity;
   log_cb(RETRO_LOG_DEBUG, "Output audio buffer capacity set to %d\n", capacity);
}

static void init_output_audio_buffer(int32_t capacity)
{
   output_audio_buffer.data = NULL;
   output_audio_buffer.size = 0;
   output_audio_buffer.capacity = 0;
   ensure_output_audio_buffer_capacity(capacity);
}

static void free_output_audio_buffer()
{
   free(output_audio_buffer.data);
   output_audio_buffer.data = NULL;
   output_audio_buffer.size = 0;
   output_audio_buffer.capacity = 0;
}

static void upload_output_audio_buffer()
{
   audio_batch_cb(output_audio_buffer.data, output_audio_buffer.size / 2);
   output_audio_buffer.size = 0;
}

/* FPS counter + mapper tick */
long retro_ticks(void)
{
   if (!perf_cb.get_time_usec)
      return retro_now;

   return perf_cb.get_time_usec();
}

unsigned int vice_led_state[RETRO_LED_NUM] = {0};
static unsigned int retro_led_state[RETRO_LED_NUM] = {0};
static void retro_led_interface(void)
{
   /* 0: Power
    * 1: Floppy
    * 2: Tape */

   unsigned int led_state[RETRO_LED_NUM] = {0};
   unsigned int l                        = 0;

   led_state[RETRO_LED_POWER] = (request_restart) ? 0 : 1;
   led_state[RETRO_LED_DRIVE] = (vice_opt.DriveTrueEmulation) ? vice_led_state[RETRO_LED_DRIVE] : 0;
   led_state[RETRO_LED_TAPE]  = vice_led_state[RETRO_LED_TAPE];

   for (l = 0; l < RETRO_LED_NUM; l++)
   {
      if (retro_led_state[l] != led_state[l])
      {
         retro_led_state[l] = led_state[l];
         led_state_cb(l, led_state[l]);
      }
   }
}

void retro_fastforwarding(bool enabled)
{
   struct retro_fastforwarding_override ff_override;
   bool frontend_ff_enabled = false;

   if (!libretro_supports_ff_override)
      return;

   environ_cb(RETRO_ENVIRONMENT_GET_FASTFORWARDING, &frontend_ff_enabled);
   if (enabled && frontend_ff_enabled)
      return;

   ff_override.fastforward    = enabled;
   ff_override.inhibit_toggle = enabled;
   libretro_ff_enabled        = enabled;

   environ_cb(RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE, &ff_override);
}

bool audio_playing(void)
{
   static unsigned int audio_timer_playing = 0;
   static unsigned int audio_timer_stopped = 0;
   bool audio_alive = false;

   if (sound_volume_counter)
      return audio_is_playing;

   if (audio_is_ignored)
   {
      audio_is_playing = false;
      return audio_is_playing;
   }

   if (audio_buffer)
   {
      for (unsigned i = 2; i < 24; i++)
      {
         int target = (i % 2 == 0) ? 0 : 1;

         if (audio_buffer[i] != audio_buffer[target] &&
             abs(audio_buffer[i] - audio_buffer[target]) > 5 &&
             abs(audio_buffer[i] - audio_buffer[target]) < 30000 &&
             !(audio_buffer[i] == 0 || audio_buffer[target] == 0) &&
             !(audio_buffer[i] == 1 || audio_buffer[target] == 1))
         {
            audio_alive = true;
            audio_timer_playing++;

            if (audio_timer_playing > 2)
            {
               audio_timer_playing = 0;
               audio_timer_stopped = 0;
               audio_is_playing = true;
#if 0
               printf("%s: PLAYING %02d i=%08d tar=%08d delta=%08d\n", __func__, i, audio_buffer[i], audio_buffer[target], abs(audio_buffer[i] - audio_buffer[target]));
#endif
               return audio_is_playing;
            }
         }
      }
   }

   if (audio_alive && audio_timer_playing)
      return audio_is_playing;

   if (!audio_alive)
      audio_timer_stopped++;

   if (audio_timer_stopped > 8)
   {
      audio_timer_playing = 0;
      audio_timer_stopped = 0;
      audio_is_playing = false;
#if 0
      printf("%s: NOT PLAYING\n", __func__);
#endif
   }

   return audio_is_playing;
}

static void retro_set_paths(void)
{
   const char *system_dir = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
   {
      strlcpy(retro_system_directory,
              system_dir,
              sizeof(retro_system_directory));
   }

   const char *content_dir = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir) && content_dir)
   {
      strlcpy(retro_content_directory,
              content_dir,
              sizeof(retro_content_directory));
   }

   const char *save_dir = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
   {
      /* If save directory is defined use it, otherwise use system directory */
      strlcpy(retro_save_directory,
              string_is_empty(save_dir) ? retro_system_directory : save_dir,
              sizeof(retro_save_directory));
   }
   else if (!retro_save_directory)
   {
      /* Make retro_save_directory the same in case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY is not implemented by the frontend */
      strlcpy(retro_save_directory,
              retro_system_directory,
              sizeof(retro_save_directory));
   }

   if (string_is_empty(retro_system_directory))
   {
#if defined(ANDROID)
      strlcpy(retro_system_directory, "/mnt/sdcard", sizeof(retro_system_directory));
#elif defined(VITA)
      strlcpy(retro_system_directory, "ux0:/data", sizeof(retro_system_directory));
#elif defined(__SWITCH__)
      strlcpy(retro_system_directory, "/", sizeof(retro_system_directory));
#else
      strlcpy(retro_system_directory, ".", sizeof(retro_system_directory));
#endif
   }

   /* Temp directory for ZIPs and NIB->G64 conversions */
   snprintf(retro_temp_directory, sizeof(retro_temp_directory), "%s%s%s",
            retro_save_directory, ARCHDEP_DIR_SEP_STR, "TEMP");

   /* Use system directory for data files such as JiffyDOS and keymaps */
   snprintf(retro_system_data_directory, sizeof(retro_system_data_directory), "%s%s%s",
            retro_system_directory, ARCHDEP_DIR_SEP_STR, "vice");
}

static void free_vice_carts(void)
{
   size_t i;
   for (i = 0; i < RETRO_NUM_CORE_OPTION_VALUES_MAX; i++)
   {
      if (vice_carts[i].value)
      {
         free(vice_carts[i].value);
         vice_carts[i].value = NULL;
      }
      if (vice_carts[i].label)
      {
         free(vice_carts[i].label);
         vice_carts[i].label = NULL;
      }
   }
}

static void retro_set_core_options()
{
   /* Core categories */
   static struct retro_core_option_v2_category option_cats_us[] =
   {
      {
         "system",
         "System",
         "Configure system options."
      },
      {
         "audio",
         "Audio",
         "Configure audio options."
      },
      {
         "video",
         "Video",
         "Configure video options."
      },
      {
         "media",
         "Media",
         "Configure media options."
      },
      {
         "input",
         "Input",
         "Configure input options."
      },
      {
         "hotkey",
         "Hotkey Mapping",
         "Configure keyboard hotkey mapping options."
      },
      {
         "retropad",
         "RetroPad Mapping",
         "Configure RetroPad mapping options."
      },
      {
         "osd",
         "On-Screen Display",
         "Configure OSD options."
      },
      { NULL, NULL, NULL },
   };

   /* Core options */
   static struct retro_core_option_v2_definition option_defs_us[] =
   {
#if defined(__XVIC__)
      {
         "vice_vic20_model",
         "System > Model",
         "Model",
         "'Automatic' switches region per file path tags.\nChanging while running resets the system!",
         NULL,
         "system",
         {
            { "VIC20 PAL auto", "VIC-20 PAL Automatic" },
            { "VIC20 NTSC auto", "VIC-20 NTSC Automatic" },
            { "VIC20 PAL", "VIC-20 PAL" },
            { "VIC20 NTSC", "VIC-20 NTSC" },
            { "VIC21", "Super VIC (+16K) NTSC" },
            { NULL, NULL },
         },
         "VIC20 PAL auto"
      },
      {
         "vice_vic20_memory_expansions",
         "System > Memory Expansion",
         "Memory Expansion",
         "Can be forced with filename tags '(8k)', '(8kb)', '[8k]', '[8kb]' or directory tags '8k', '8kb'.\n'Automatic' tries to decide based on PRG size.\nChanging while running resets the system!",
         NULL,
         "system",
         {
            { "none", "disabled" },
            { "3kB", "3kB" },
            { "8kB", "8kB" },
            { "16kB", "16kB" },
            { "24kB", "24kB" },
            { "35kB", "35kB" },
            { "auto", "Automatic" },
            { NULL, NULL },
         },
         "none"
      },
#elif defined(__XPLUS4__)
      {
         "vice_plus4_model",
         "System > Model",
         "Model",
         "Changing while running resets the system!",
         NULL,
         "system",
         {
            { "C16 PAL", "C16 PAL" },
            { "C16 NTSC", "C16 NTSC" },
            { "PLUS4 PAL", "Plus/4 PAL" },
            { "PLUS4 NTSC", "Plus/4 NTSC" },
            { "V364 NTSC", "V364 NTSC" },
            { "232 NTSC", "C232 NTSC" },
            { NULL, NULL }
         },
         "PLUS4 PAL"
      },
#elif defined(__X128__)
      {
         "vice_c128_model",
         "System > Model",
         "Model",
         "'Automatic' switches region per file path tags.\nChanging while running resets the system!",
         NULL,
         "system",
         {
            { "C128 PAL auto", "C128 PAL Automatic" },
            { "C128 NTSC auto", "C128 NTSC Automatic" },
            { "C128 D PAL auto", "C128D PAL Automatic" },
            { "C128 D NTSC auto", "C128D NTSC Automatic" },
            { "C128 DCR PAL auto", "C128DCR PAL Automatic" },
            { "C128 DCR NTSC auto", "C128DCR NTSC Automatic" },
            { "C128 PAL", "C128 PAL" },
            { "C128 NTSC", "C128 NTSC" },
            { "C128 D PAL", "C128D PAL" },
            { "C128 D NTSC", "C128D NTSC" },
            { "C128 DCR PAL", "C128DCR PAL" },
            { "C128 DCR NTSC", "C128DCR NTSC" },
            { NULL, NULL },
         },
         "C128 PAL auto"
      },
      {
         "vice_c128_ram_expansion_unit",
         "System > RAM Expansion Unit",
         "RAM Expansion Unit",
         "Not allowed with cartridges. Changing while running resets the system!",
         NULL,
         "system",
         {
            { "none", "disabled" },
            { "128kB", "128kB (1700)" },
            { "256kB", "256kB (1764)" },
            { "512kB", "512kB (1750)" },
            { "1024kB", "1024kB" },
            { "2048kB", "2048kB" },
            { "4096kB", "4096kB" },
            { "8192kB", "8192kB" },
            { "16384kB", "16384kB" },
            { NULL, NULL },
         },
         "none"
      },
      {
         "vice_c128_video_output",
         "System > Video Output",
         "Video Output",
         "",
         NULL,
         "system",
         {
            { "VICII", "VIC-II (40 cols)" },
            { "VDC", "VDC (80 cols)" },
            { NULL, NULL },
         },
         "VICII"
      },
      {
         "vice_c128_vdc_ram",
         "System > VDC Video RAM",
         "VDC Video RAM",
         "",
         NULL,
         "system",
         {
            { "default", "16kB" },
            { "64", "64kB" },
            { NULL, NULL },
         },
         "default"
      },
      {
         "vice_c128_go64",
         "System > GO64",
         "GO64",
         "Start in C64 compatibility mode.\nChanging while running resets the system!",
         NULL,
         "system",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#elif defined(__XPET__)
      {
         "vice_pet_model",
         "System > Model",
         "Model",
         "Changing while running resets the system!",
         NULL,
         "system",
         {
            { "2001", "PET 2001" },
            { "3008", "PET 3008" },
            { "3016", "PET 3016" },
            { "3032", "PET 3032" },
            { "3032B", "PET 3032B" },
            { "4016", "PET 4016" },
            { "4032", "PET 4032" },
            { "4032B", "PET 4032B" },
            { "8032", "PET 8032" },
            { "8096", "PET 8096" },
            { "8296", "PET 8296" },
            { "SUPERPET", "Super PET" },
            { NULL, NULL },
         },
         "8032"
      },
#elif defined(__XCBM2__)
      {
         "vice_cbm2_model",
         "System > Model",
         "Model",
         "Changing while running resets the system!",
         NULL,
         "system",
         {
            { "610 PAL", "CBM 610 PAL" },
            { "610 NTSC", "CBM 610 NTSC" },
            { "620 PAL", "CBM 620 PAL" },
            { "620 NTSC", "CBM 620 NTSC" },
            { "620PLUS PAL", "CBM 620+ PAL" },
            { "620PLUS NTSC", "CBM 620+ NTSC" },
            { "710 NTSC", "CBM 710 NTSC" },
            { "720 NTSC", "CBM 720 NTSC" },
            { "720PLUS NTSC", "CBM 720+ NTSC" },
            { NULL, NULL },
         },
         "610 PAL"
      },
#elif defined(__XCBM5x0__)
      {
         "vice_cbm5x0_model",
         "System > Model",
         "Model",
         "Changing while running resets the system!",
         NULL,
         "system",
         {
            { "510 PAL", "CBM 510 PAL" },
            { "510 NTSC", "CBM 510 NTSC" },
            { NULL, NULL },
         },
         "510 PAL"
      },
#elif defined(__X64DTV__)
      {
         "vice_c64dtv_model",
         "System > Model",
         "Model",
         "Changing while running resets the system!",
         NULL,
         "system",
         {
            { "DTV2 PAL", "DTV v2 PAL" },
            { "DTV2 NTSC", "DTV v2 NTSC" },
            { "DTV3 PAL", "DTV v3 PAL" },
            { "DTV3 NTSC", "DTV v3 NTSC" },
            { "HUMMER NTSC", "Hummer NTSC" },
            { NULL, NULL },
         },
         "DTV3 PAL"
      },
#else
      {
         "vice_c64_model",
         "System > Model",
         "Model",
         "'Automatic' switches region per file path tags.\nChanging while running resets the system!",
         NULL,
         "system",
         {
            { "C64 PAL auto", "C64 PAL Automatic" },
            { "C64 NTSC auto", "C64 NTSC Automatic" },
            { "C64C PAL auto", "C64C PAL Automatic" },
            { "C64C NTSC auto", "C64C NTSC Automatic" },
            { "C64 PAL", "C64 PAL" },
            { "C64 NTSC", "C64 NTSC" },
            { "C64C PAL", "C64C PAL" },
            { "C64C NTSC", "C64C NTSC" },
            { "C64SX PAL", "SX-64 PAL" },
            { "C64SX NTSC", "SX-64 NTSC" },
            { "PET64 PAL", "Educator 64 (PET 64) PAL" },
            { "PET64 NTSC", "Educator 64 (PET 64) NTSC" },
            { "C64 GS PAL", "C64 Games System PAL" },
            { "C64 JAP NTSC", "C64 Japanese NTSC" },
#if 0
            { "C64 OLD PAL", NULL },
            { "C64 PAL N", NULL },
            { "C64 OLD NTSC", NULL },
#endif
            { NULL, NULL },
         },
         "C64 PAL auto"
      },
#if defined(__XSCPU64__)
      {
         "vice_supercpu_simm_size",
         "System > SuperCPU SIMM Size",
         "SuperCPU SIMM Size",
         "Changing while running resets the system!",
         NULL,
         "system",
         {
            { "0", "disabled" },
            { "1", "1024kB" },
            { "2", "2048kB" },
            { "4", "4096kB" },
            { "8", "8192kB" },
            { "16", "16384kB" },
            { NULL, NULL },
         },
         "16"
      },
#else
      {
         "vice_ram_expansion_unit",
         "System > RAM Expansion Unit",
         "RAM Expansion Unit",
         "Not allowed with cartridges. Changing while running resets the system!",
         NULL,
         "system",
         {
            { "none", "disabled" },
            { "128kB", "128kB (1700)" },
            { "256kB", "256kB (1764)" },
            { "512kB", "512kB (1750)" },
            { "1024kB", "1024kB" },
            { "2048kB", "2048kB" },
            { "4096kB", "4096kB" },
            { "8192kB", "8192kB" },
            { "16384kB", "16384kB" },
            { NULL, NULL },
         },
         "none"
      },
#endif /* __XSCPU64__ */
#endif
#if defined(__XSCPU64__)
      {
         "vice_supercpu_speed_switch",
         "System > SuperCPU Speed Switch",
         "SuperCPU Speed Switch",
         "",
         NULL,
         "system",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "enabled"
      },
      {
         "vice_supercpu_kernal",
         "System > SuperCPU Kernal",
         "SuperCPU Kernal",
         "JiffyDOS does not work with the internal kernal! ROMs required in 'system/vice/SCPU64':\n- 'scpu-dos-1.4.bin'\n- 'scpu-dos-2.04.bin'",
         NULL,
         "system",
         {
            { "0", "Internal" },
            { "1", "1.40" },
            { "2", "2.04" },
            { NULL, NULL },
         },
         "0"
      },
#endif
#if !defined(__X64DTV__)
      {
         "vice_printer",
         "System > Printer",
         "Printer",
         "Output is written to 'saves/" ARCHDEP_PRINTER_DEFAULT_DEV1 "'.",
         NULL,
         "system",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
      {
         "vice_jiffydos",
         "System > JiffyDOS",
         "JiffyDOS",
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
         "'True Drive Emulation' & 1541/1571/1581 drive & ROMs required in 'system/vice':\n- 'JiffyDOS_C64.bin'\n- 'JiffyDOS_1541-II.bin'\n- 'JiffyDOS_1571_repl310654.bin'\n- 'JiffyDOS_1581.bin'",
#elif defined(__X128__)
         "'True Drive Emulation' & 1541/1571/1581 drive & ROMs required in 'system/vice':\n- 'JiffyDOS_C128.bin'\n- 'JiffyDOS_C64.bin' (GO64)\n- 'JiffyDOS_1541-II.bin'\n- 'JiffyDOS_1571_repl310654.bin'\n- 'JiffyDOS_1581.bin'",
#endif
         NULL,
         "system",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
      {
         "vice_read_vicerc",
         "System > Read 'vicerc'",
         "Read 'vicerc'",
         "Process first found 'vicerc' in this order:\n1. 'saves/[content].vicerc'\n2. 'saves/vicerc'\n3. 'system/vice/vicerc'",
         NULL,
         "system",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "enabled"
      },
#if !defined(__X64DTV__)
      {
         "vice_reset",
         "System > Reset Type",
         "Reset Type",
         "Reset hotkey action. Core restart will autostart.\n- 'Autostart' hard resets and reruns content.\n- 'Soft' keeps some code in memory.\n- 'Hard' erases all memory.\n- 'Freeze' is for cartridges.",
         NULL,
         "system",
         {
            { "autostart", "Autostart" },
            { "soft", "Soft" },
            { "hard", "Hard" },
            { "freeze", "Freeze" },
            { NULL, NULL },
         },
         "autostart"
      },
      {
         "vice_autoloadwarp",
         "Media > Automatic Load Warp",
         "Automatic Load Warp",
         "Toggle warp mode during disk and/or tape access if there is no audio output. Mutes 'Drive Sound Emulation', 'Datasette Sound' and 'Audio Leak Emulation' when not ignoring audio.\n'True Drive Emulation' required with disks!",
         NULL,
         "media",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { "mute", "Ignore audio" },
            { "disk", "Disk only" },
            { "disk_mute", "Disk only (Ignore audio)" },
            { "tape", "Tape only" },
            { "tape_mute", "Tape only (Ignore audio)" },
            { NULL, NULL },
         },
         "disabled"
      },
#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__)
      {
         "vice_warp_boost",
         "Media > Warp Boost",
         "Warp Boost",
         "Make warp mode faster by changing SID engine to 'FastSID' while warping. Affects audio detection during warp.",
         NULL,
         "media",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
      {
         "vice_autostart",
         "Media > Autostart",
         "Autostart",
         "'ON' always runs content, 'OFF' runs only PRG/CRT, 'Warp' turns warp mode on during autostart loading.",
         NULL,
         "media",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { "warp", "Warp" },
            { NULL, NULL },
         },
         "enabled"
      },
      {
         "vice_drive_true_emulation",
         "Media > True Drive Emulation",
         "True Drive Emulation",
         "Loads much slower, but some games need it.\nRequired for 'JiffyDOS', 'Automatic Load Warp' and LED driver interface!",
         NULL,
         "media",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "enabled"
      },
      {
         "vice_virtual_device_traps",
         "Media > Virtual Device Traps",
         "Virtual Device Traps",
         "Required for printer device, but causes loading issues on rare cases.",
         NULL,
         "media",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_floppy_multidrive",
         "Media > Floppy MultiDrive",
         "Floppy MultiDrive",
         "Insert each disk in different drives. Can be forced with '(MD)' file path tag. Maximum is 4 disks due to external drive limit!\nCore restart required.",
         NULL,
         "media",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_floppy_write_protection",
         "Media > Floppy Write Protection",
         "Floppy Write Protection",
         "Set device 8 read only.",
         NULL,
         "media",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
      {
         "vice_easyflash_write_protection",
         "Media > EasyFlash Write Protection",
         "EasyFlash Write Protection",
         "Set EasyFlash cartridges read only.",
         NULL,
         "media",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
      {
         "vice_work_disk",
         "Media > Global Work Disk",
         "Global Work Disk",
         "Work disk in device 8 will not be inserted when floppy content is launched. Files and directories are named 'vice_work'.",
         NULL,
         "media",
         {
            { "disabled", NULL },
            { "8_d64", "D64 - 664 blocks, 170kB - Device 8" },
            { "9_d64", "D64 - 664 blocks, 170kB - Device 9" },
            { "8_d71", "D71 - 1328 blocks, 340kB - Device 8" },
            { "9_d71", "D71 - 1328 blocks, 340kB - Device 9" },
            { "8_d81", "D81 - 3160 blocks, 800kB - Device 8" },
            { "9_d81", "D81 - 3160 blocks, 800kB - Device 9" },
            { "8_fs", "FileSystem - Device 8" },
            { "9_fs", "FileSystem - Device 9" },
            { NULL, NULL },
         },
         "disabled"
      },
#endif /* !defined(__X64DTV__) */
#if !defined(__XPET__) && !defined(__X64DTV__)
      /* Sublabel and options filled dynamically in retro_set_environment() */
      {
         "vice_cartridge",
         "Media > Cartridge",
         "Cartridge",
         "",
         NULL,
         "media",
         {
            { NULL, NULL },
         },
         NULL
      },
#endif
      {
         "vice_video_options_display",
         "Show Video Options",
         NULL,
         "",
         NULL,
         NULL,
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      {
         "vice_aspect_ratio",
         "Video > Pixel Aspect Ratio",
         "Pixel Aspect Ratio",
         "Hotkey toggling disables this option until core restart.",
         NULL,
         "video",
         {
            { "auto", "Automatic" },
            { "pal", "PAL" },
            { "ntsc", "NTSC" },
            { "raw", "1:1" },
            { NULL, NULL },
         },
         "auto"
      },
#endif
      {
         "vice_crop",
         "Video > Crop",
         "Crop",
         "Remove borders according to 'Crop Mode'.",
         NULL,
         "video",
         {
            { "disabled", NULL },
            { "small", "Small" },
            { "medium", "Medium" },
            { "maximum", "Maximum" },
            { "auto", "Automatic" },
            { "auto_disable", "Auto-Disable" },
            { "manual", "Manual" },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_crop_delay",
         "Video > Automatic Crop Delay",
         "Automatic Crop Delay",
         "Patient or instant geometry change.",
         NULL,
         "video",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "enabled"
      },
      {
         "vice_crop_mode",
         "Video > Crop Mode",
         "Crop Mode",
         "'Horizontal + Vertical' & 'Maximum' removes borders completely.",
         NULL,
         "video",
         {
            { "both", "Horizontal + Vertical" },
            { "horizontal", "Horizontal" },
            { "vertical", "Vertical" },
            { "16:9", "16:9" },
            { "16:10", "16:10" },
            { "4:3", "4:3" },
            { "5:4", "5:4" },
            { NULL, NULL },
         },
         "both"
      },
      {
         "vice_manual_crop_top",
         "Video > Manual Crop Top",
         "Manual Crop Top",
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "VIC-II top border height:\n- 35px PAL\n- 23px NTSC",
#elif defined(__XVIC__)
         "VIC top border height:\n- 48px PAL\n- 22px NTSC",
#elif defined(__XPLUS4__)
         "TED top border height:\n- 40px PAL\n- 18px NTSC",
#else
         "",
#endif
         NULL,
         "video",
         MANUAL_CROP_OPTIONS,
         "0",
      },
      {
         "vice_manual_crop_bottom",
         "Video > Manual Crop Bottom",
         "Manual Crop Bottom",
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "VIC-II bottom border height:\n- 37px PAL\n- 24px NTSC",
#elif defined(__XVIC__)
         "VIC bottom border height:\n- 52px PAL\n- 28px NTSC",
#elif defined(__XPLUS4__)
         "TED bottom border height:\n- 48px PAL\n- 24px NTSC",
#else
         "",
#endif
         NULL,
         "video",
         MANUAL_CROP_OPTIONS,
         "0",
      },
      {
         "vice_manual_crop_left",
         "Video > Manual Crop Left",
         "Manual Crop Left",
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "VIC-II left border width:\n- 32px",
#elif defined(__XVIC__)
         "VIC left border width:\n- 48px PAL\n- 32px NTSC",
#elif defined(__XPLUS4__)
         "TED left border width:\n- 32px",
#else
         "",
#endif
         NULL,
         "video",
         MANUAL_CROP_OPTIONS,
         "0",
      },
      {
         "vice_manual_crop_right",
         "Video > Manual Crop Right",
         "Manual Crop Right",
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "VIC-II right border width:\n- 32px",
#elif defined(__XVIC__)
         "VIC right border width:\n- 48px PAL\n- 16px NTSC",
#elif defined(__XPLUS4__)
         "TED right border width:\n- 32px",
#else
         "",
#endif
         NULL,
         "video",
         MANUAL_CROP_OPTIONS,
         "0",
      },
#if defined(__X128__)
      {
         "vice_vdc_filter",
         "Video > VDC Filter",
         "VDC Filter",
         "VDC PAL emulation filter with custom horizontal blur.",
         NULL,
         "video",
         {
            { "disabled", NULL },
            { "enabled_noblur", "0%" },
            { "enabled_lowblur", "10%" },
            { "enabled_medblur", "50%" },
            { "enabled", "100%" },
         },
         "disabled"
      },
#endif
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_filter",
         "Video > VIC-II Filter",
         "VIC-II Filter",
#elif defined(__XVIC__)
         "vice_vic_filter",
         "Video > VIC Filter",
         "VIC Filter",
#elif defined(__XPLUS4__)
         "vice_ted_filter",
         "Video > TED Filter",
         "TED Filter",
#elif defined(__XPET__) || defined(__XCBM2__)
         "vice_crtc_filter",
         "Video > CRTC Filter",
         "CRTC Filter",
#endif
         "PAL emulation filter with custom horizontal blur.",
         NULL,
         "video",
         {
            { "disabled", NULL },
            { "enabled_noblur", "0%" },
            { "enabled_lowblur", "10%" },
            { "enabled_medblur", "50%" },
            { "enabled", "100%" },
         },
         "disabled"
      },
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_filter_oddline_offset",
         "Video > VIC-II Filter Oddline Offset",
         "VIC-II Filter Oddline Offset",
#elif defined(__XVIC__)
         "vice_vic_filter_oddline_offset",
         "Video > VIC Filter Oddline Offset",
         "VIC Filter Oddline Offset",
#elif defined(__XPLUS4__)
         "vice_ted_filter_oddline_offset",
         "Video > TED Filter Oddline Offset",
         "TED Filter Oddline Offset",
#endif
         "PAL emulation filter oddline offset.",
         NULL,
         "video",
         PALETTE_COLOR_OPTIONS,
         "1000"
      },
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_filter_oddline_phase",
         "Video > VIC-II Filter Oddline Phase",
         "VIC-II Filter Oddline Phase",
#elif defined(__XVIC__)
         "vice_vic_filter_oddline_phase",
         "Video > VIC Filter Oddline Phase",
         "VIC Filter Oddline Phase",
#elif defined(__XPLUS4__)
         "vice_ted_filter_oddline_phase",
         "Video > TED Filter Oddline Phase",
         "TED Filter Oddline Phase",
#endif
         "PAL emulation filter oddline phase. Applies with 'Internal' palette only!",
         NULL,
         "video",
         PALETTE_COLOR_OPTIONS,
         "1000"
      },
#endif
#if defined(__XVIC__)
      {
         "vice_vic20_external_palette",
         "Video > VIC Color Palette",
         "VIC Color Palette",
         "'Colodore' is recommended for the most accurate colors.",
         NULL,
         "video",
         {
            { "default", "Internal" },
            { "colodore_vic", "Colodore" },
            { "mike-pal", "Mike (PAL)" },
            { "mike-ntsc", "Mike (NTSC)" },
            { "palette", "PALette 6561-101 v1" },
            { "vice", "VICE" },
            { NULL, NULL },
         },
         "default"
      },
#elif defined(__XPLUS4__)
      {
         "vice_plus4_external_palette",
         "Video > TED Color Palette",
         "TED Color Palette",
         "'Colodore' is recommended for the most accurate colors.",
         NULL,
         "video",
         {
            { "default", "Internal" },
            { "colodore_ted", "Colodore" },
            { "ITU-R_BT601_CRT", "ITU-R BT601 CRT" },
            { "ITU-R_BT709_HDTV", "ITU-R BT709 HDTV" },
            { "ITU-R_BT2020", "ITU-R BT2020" },
            { "yape-pal", "YAPE (PAL)" },
            { "yape-ntsc", "YAPE (NTSC)" },
            { NULL, NULL },
         },
         "default"
      },
#elif defined(__XPET__)
      {
         "vice_pet_external_palette",
         "Video > CRTC Color Palette",
         "CRTC Color Palette",
         "",
         NULL,
         "video",
         {
            { "default", "Internal" },
            { "green", "Green" },
            { "amber", "Amber" },
            { "white", "White" },
            { NULL, NULL },
         },
         "default"
      },
#elif defined(__XCBM2__)
      {
         "vice_cbm2_external_palette",
         "Video > CRTC Color Palette",
         "CRTC Color Palette",
         "",
         NULL,
         "video",
         {
            { "default", "Internal" },
            { "green", "Green" },
            { "amber", "Amber" },
            { "white", "White" },
            { NULL, NULL },
         },
         "default"
      },
#elif !defined(__X64DTV__)
      {
         "vice_external_palette",
         "Video > VIC-II Color Palette",
         "VIC-II Color Palette",
         "'Colodore' is recommended for most accurate colors.",
         NULL,
         "video",
         {
            { "default", "Internal" },
            { "c64hq", "C64HQ" },
            { "c64s", "C64S" },
            { "ccs64", "CCS64" },
            { "cjam", "Christopher Jam" },
            { "colodore", "Colodore" },
            { "community-colors", "Community Colors" },
            { "deekay", "Deekay/Crest" },
            { "frodo", "Frodo" },
            { "godot", "Godot" },
            { "lemon64", "Lemon64" },
            { "palette", "PALette" },
            { "palette_6569R1_v1r", "PALette 6569R1" },
            { "palette_6569R5_v1r", "PALette 6569R5" },
            { "palette_8565R2_v1r", "PALette 8565R2" },
            { "palette_C64_amber", "PALette Amber P3/602nm" },
            { "palette_C64_cyan", "PALette Cyan" },
            { "palette_C64_green", "PALette Green P1/525nm" },
            { "pc64", "PC64" },
            { "pepto-pal", "Pepto (PAL)" },
            { "pepto-palold", "Pepto (old PAL)" },
            { "pepto-ntsc", "Pepto (NTSC)" },
            { "pepto-ntsc-sony", "Pepto (NTSC, Sony Matrix)" },
            { "pixcen", "PixCen" },
            { "ptoing", "Ptoing" },
            { "rgb", "RGB" },
            { "the64", "THE64" },
            { "vice", "VICE" },
            { NULL, NULL },
         },
         "default"
      },
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_color_gamma",
         "Video > VIC-II Color Gamma",
         "VIC-II Color Gamma",
#elif defined(__XVIC__)
         "vice_vic_color_gamma",
         "Video > VIC Color Gamma",
         "VIC Color Gamma",
#elif defined(__XPLUS4__)
         "vice_ted_color_gamma",
         "Video > TED Color Gamma",
         "TED Color Gamma",
#endif
         "Gamma for the internal palette.",
         NULL,
         "video",
         PALETTE_GAMMA_OPTIONS,
         "2800"
      },
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_color_brightness",
         "Video > VIC-II Color Brightness",
         "VIC-II Color Brightness",
#elif defined(__XVIC__)
         "vice_vic_color_brightness",
         "Video > VIC Color Brightness",
         "VIC Color Brightness",
#elif defined(__XPLUS4__)
         "vice_ted_color_brightness",
         "Video > TED Color Brightness",
         "TED Color Brightness",
#endif
         "Brightness for the internal palette.",
         NULL,
         "video",
         PALETTE_COLOR_OPTIONS,
         "1000"
      },
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_color_contrast",
         "Video > VIC-II Color Contrast",
         "VIC-II Color Contrast",
#elif defined(__XVIC__)
         "vice_vic_color_contrast",
         "Video > VIC Color Contrast",
         "VIC Color Contrast",
#elif defined(__XPLUS4__)
         "vice_ted_color_contrast",
         "Video > TED Color Contrast",
         "TED Color Contrast",
#endif
         "Contrast for the internal palette.",
         NULL,
         "video",
         PALETTE_COLOR_OPTIONS,
         "1000"
      },
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_color_saturation",
         "Video > VIC-II Color Saturation",
         "VIC-II Color Saturation",
#elif defined(__XVIC__)
         "vice_vic_color_saturation",
         "Video > VIC Color Saturation",
         "VIC Color Saturation",
#elif defined(__XPLUS4__)
         "vice_ted_color_saturation",
         "Video > TED Color Saturation",
         "TED Color Saturation",
#endif
         "Saturation for the internal palette.",
         NULL,
         "video",
         PALETTE_COLOR_OPTIONS,
         "1000"
      },
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_color_tint",
         "Video > VIC-II Color Tint",
         "VIC-II Color Tint",
#elif defined(__XVIC__)
         "vice_vic_color_tint",
         "Video > VIC Color Tint",
         "VIC Color Tint",
#elif defined(__XPLUS4__)
         "vice_ted_color_tint",
         "Video > TED Color Tint",
         "TED Color Tint",
#endif
         "Tint for the internal palette.",
         NULL,
         "video",
         PALETTE_COLOR_OPTIONS,
         "1000"
      },
#endif
      {
         "vice_gfx_colors",
         "Video > Color Depth",
         "Color Depth",
         "Full restart required.",
         NULL,
         "video",
         {
            { "16bit", "16-bit (RGB565)" },
            { "24bit", "24-bit (XRGB8888)" },
            { NULL, NULL },
         },
#if defined(__X64__) || defined(PSP) || defined(VITA) || defined(__SWITCH__) || defined(DINGUX) || defined(ANDROID)
         "16bit"
#else
         "24bit"
#endif
      },
      {
         "vice_vkbd_theme",
         "OSD > Virtual KBD Theme",
         "Virtual KBD Theme",
         "The keyboard comes up with RetroPad Select by default.",
         NULL,
         "osd",
         {
            { "auto", "Automatic (shadow)" },
            { "auto_outline", "Automatic (outline)" },
            { "brown", "Brown (shadow)" },
            { "brown_outline", "Brown (outline)" },
            { "beige", "Beige (shadow)" },
            { "beige_outline", "Beige (outline)" },
            { "dark", "Dark (shadow)" },
            { "dark_outline", "Dark (outline)" },
            { "light", "Light (shadow)" },
            { "light_outline", "Light (outline)" },
            { NULL, NULL },
         },
         "auto"
      },
      {
         "vice_vkbd_transparency",
         "OSD > Virtual KBD Transparency",
         "Virtual KBD Transparency",
         "Keyboard transparency can be toggled with RetroPad A.",
         NULL,
         "osd",
         {
            { "0%",   NULL },
            { "25%",  NULL },
            { "50%",  NULL },
            { "75%",  NULL },
            { "100%", NULL },
            { NULL, NULL },
         },
         "25%"
      },
      {
         "vice_vkbd_dimming",
         "OSD > Virtual KBD Dimming",
         "Virtual KBD Dimming",
         "Dimming level of the surrounding area.",
         NULL,
         "osd",
         {
            { "0%",   NULL },
            { "25%",  NULL },
            { "50%",  NULL },
            { "75%",  NULL },
            { "100%", NULL },
            { NULL, NULL },
         },
         "25%"
      },
      {
         "vice_statusbar",
         "OSD > Statusbar Mode",
         "Statusbar Mode",
         "- 'Full': Joyports + Messages + LEDs\n- 'Basic': Messages + LEDs\n- 'Minimal': LED colors only",
         NULL,
         "osd",
         {
            { "bottom", "Bottom Full" },
            { "bottom_minimal", "Bottom Full Minimal" },
            { "bottom_basic", "Bottom Basic" },
            { "bottom_basic_minimal", "Bottom Basic Minimal" },
            { "top", "Top Full" },
            { "top_minimal", "Top Full Minimal" },
            { "top_basic", "Top Basic" },
            { "top_basic_minimal", "Top Basic Minimal" },
            { NULL, NULL },
         },
         "bottom"
      },
      {
         "vice_statusbar_startup",
         "OSD > Statusbar Startup",
         "Statusbar Startup",
         "Show statusbar on startup.",
         NULL,
         "osd",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_statusbar_messages",
         "OSD > Statusbar Messages",
         "Statusbar Messages",
         "Show messages when statusbar is hidden.",
         NULL,
         "osd",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_joyport_pointer_color",
         "OSD > Light Pen/Gun Pointer Color",
         "Light Pen/Gun Pointer Color",
         "Crosshair color for light pens and guns.",
         NULL,
         "osd",
         {
            { "disabled", NULL },
            { "black", "Black" },
            { "white", "White" },
            { "red", "Red" },
            { "green", "Green" },
            { "blue", "Blue" },
            { "yellow", "Yellow" },
            { "purple", "Purple" },
            { NULL, NULL },
         },
         "blue"
      },
      {
         "vice_audio_options_display",
         "Show Audio Options",
         NULL,
         "",
         NULL,
         NULL,
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#if !defined(__X64DTV__)
      {
         "vice_drive_sound_emulation",
         "Audio > Drive Sound Emulation",
         "Drive Sound Emulation",
         "'True Drive Emulation' & D64/D71 disk image required.",
         NULL,
         "audio",
         {
            { "disabled", NULL },
            { "5%", NULL },
            { "10%", NULL },
            { "15%", NULL },
            { "20%", NULL },
            { "25%", NULL },
            { "30%", NULL },
            { "35%", NULL },
            { "40%", NULL },
            { "45%", NULL },
            { "50%", NULL },
            { "55%", NULL },
            { "60%", NULL },
            { "65%", NULL },
            { "70%", NULL },
            { "75%", NULL },
            { "80%", NULL },
            { "85%", NULL },
            { "90%", NULL },
            { "95%", NULL },
            { "100%", NULL },
            { NULL, NULL },
         },
         "20%"
      },
#endif
#if !defined(__XSCPU64__) && !defined(__X64DTV__)
      {
         "vice_datasette_sound",
         "Audio > Datasette Sound",
         "Datasette Sound",
         "TAP tape image required.",
         NULL,
         "audio",
         {
            { "disabled", NULL },
            { "5%", NULL },
            { "10%", NULL },
            { "15%", NULL },
            { "20%", NULL },
            { "25%", NULL },
            { "30%", NULL },
            { "35%", NULL },
            { "40%", NULL },
            { "45%", NULL },
            { "50%", NULL },
            { "55%", NULL },
            { "60%", NULL },
            { "65%", NULL },
            { "70%", NULL },
            { "75%", NULL },
            { "80%", NULL },
            { "85%", NULL },
            { "90%", NULL },
            { "95%", NULL },
            { "100%", NULL },
            { "-1", "100% + Mute" },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__) || defined(__XPET__) || defined(__XCBM2__)
      {
         "vice_audio_leak_emulation",
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "Audio > VIC-II Audio Leak Emulation",
         "VIC-II Audio Leak Emulation",
#elif defined(__XVIC__)
         "Audio > VIC Audio Leak Emulation",
         "VIC Audio Leak Emulation",
#elif defined(__XPLUS4__)
         "Audio > TED Audio Leak Emulation",
         "TED Audio Leak Emulation",
#elif defined(__XPET__) || defined(__XCBM2__)
         "Audio > CRTC Audio Leak Emulation",
         "CRTC Audio Leak Emulation",
#endif
         "Graphic chip audio leak emulation.",
         NULL,
         "audio",
         {
            { "disabled", NULL },
            { "1", "100%" },
            { "2", "200%" },
            { "3", "300%" },
            { "4", "400%" },
            { "5", "500%" },
            { "6", "600%" },
            { "7", "700%" },
            { "8", "800%" },
            { "9", "900%" },
            { "10", "1000%" },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__)
      {
         "vice_sid_engine",
         "Audio > SID Engine",
         "SID Engine",
#if defined(__X64DTV__)
         "'ReSID-DTV' is accurate, 'FastSID' is the last resort.",
#else
         "'ReSID' is accurate, 'ReSID-FP' is more accurate, 'FastSID' is the last resort.",
#endif
         NULL,
         "audio",
         {
            { "FastSID", NULL },
#if defined(__X64DTV__)
            { "ReSID", "ReSID-DTV" },
#else
            { "ReSID", NULL },
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__) || defined(__X128__)
#ifdef HAVE_RESID33
            { "ReSID-3.3", NULL },
#endif
            { "ReSID-FP", NULL },
#endif
            { NULL, NULL },
         },
         "ReSID"
      },
#if !defined(__X64DTV__)
      {
         "vice_sid_model",
         "Audio > SID Model",
         "SID Model",
         "C64 has '6581', C64C has '8580'.",
         NULL,
         "audio",
         {
            { "default", "Default" },
            { "6581", NULL },
            { "8580", NULL },
            { "8580RD", "8580 ReSID + digi boost" },
            { NULL, NULL },
         },
         "default"
      },
      {
         "vice_sid_extra",
         "Audio > SID Extra",
         "SID Extra",
         "Second SID base address.",
         NULL,
         "audio",
         {
            { "disabled", NULL },
            { "0xd420", "$D420" },
            { "0xd500", "$D500" },
            { "0xde00", "$DE00" },
            { "0xdf00", "$DF00" },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
      {
         "vice_resid_sampling",
         "Audio > ReSID Sampling",
         "ReSID Sampling",
         "'Resampling' provides best quality. 'Fast' improves performance dramatically on PS Vita.",
         NULL,
         "audio",
         {
            { "fast", "Fast" },
            { "interpolation", "Interpolation" },
            { "fast resampling", "Fast Resampling" },
            { "resampling", "Resampling" },
            { NULL, NULL },
         },
#if defined(__X64__) || defined(__XCBM5x0__) || defined(__XCBM2__) || defined(PSP) || defined(VITA) || defined(__SWITCH__) || defined(DINGUX) || defined(ANDROID)
         "fast"
#else
         "resampling"
#endif
      },
      {
         "vice_resid_passband",
         "Audio > ReSID Filter Passband",
         "ReSID Filter Passband",
         "Resampling filter passband in percentage of the total bandwidth.",
         NULL,
         "audio",
         {
            { "0", NULL },
            { "10", NULL },
            { "20", NULL },
            { "30", NULL },
            { "40", NULL },
            { "50", NULL },
            { "60", NULL },
            { "70", NULL },
            { "80", NULL },
            { "90", NULL },
            { NULL, NULL },
         },
         "90"
      },
      {
         "vice_resid_gain",
         "Audio > ReSID Filter Gain",
         "ReSID Filter Gain",
         "Filter gain in percent.",
         NULL,
         "audio",
         {
            { "90", NULL },
            { "91", NULL },
            { "92", NULL },
            { "93", NULL },
            { "94", NULL },
            { "95", NULL },
            { "96", NULL },
            { "97", NULL },
            { "98", NULL },
            { "99", NULL },
            { "100", NULL },
            { NULL, NULL },
         },
         "97"
      },
      {
         "vice_resid_filterbias",
#if defined(__X64DTV__)
         "Audio > ReSID Filter Bias",
         "ReSID Filter Bias",
         "Filter bias, which can be used to adjust DAC bias in millivolts.",
#else
         "Audio > ReSID Filter 6581 Bias",
         "ReSID Filter 6581 Bias",
         "Filter bias for 6581, which can be used to adjust DAC bias in millivolts.",
#endif
         NULL,
         "audio",
         {
            { "-5000", NULL },
            { "-4500", NULL },
            { "-4000", NULL },
            { "-3500", NULL },
            { "-3000", NULL },
            { "-2500", NULL },
            { "-2000", NULL },
            { "-1500", NULL },
            { "-1000", NULL },
            { "-500", NULL },
            { "0", NULL },
            { "500", NULL },
            { "1000", NULL },
            { "1500", NULL },
            { "2000", NULL },
            { "2500", NULL },
            { "3000", NULL },
            { "3500", NULL },
            { "4000", NULL },
            { "4500", NULL },
            { "5000", NULL },
            { NULL, NULL },
         },
         "500"
      },
#if !defined(__X64DTV__)
      {
         "vice_resid_8580filterbias",
         "Audio > ReSID Filter 8580 Bias",
         "ReSID Filter 8580 Bias",
         "Filter bias for 8580, which can be used to adjust DAC bias in millivolts.",
         NULL,
         "audio",
         {
            { "-5000", NULL },
            { "-4500", NULL },
            { "-4000", NULL },
            { "-3500", NULL },
            { "-3000", NULL },
            { "-2500", NULL },
            { "-2000", NULL },
            { "-1500", NULL },
            { "-1000", NULL },
            { "-500", NULL },
            { "0", NULL },
            { "500", NULL },
            { "1000", NULL },
            { "1500", NULL },
            { "2000", NULL },
            { "2500", NULL },
            { "3000", NULL },
            { "3500", NULL },
            { "4000", NULL },
            { "4500", NULL },
            { "5000", NULL },
            { NULL, NULL },
         },
         "0"
      },
#endif
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
      {
         "vice_sfx_sound_expander",
         "Audio > SFX Sound Expander",
         "SFX Sound Expander",
         "Sound synthesizer cartridge with 9 voices.",
         NULL,
         "audio",
         {
            { "disabled", NULL },
            { "3526", "Yamaha YM3526" },
            { "3812", "Yamaha YM3812" },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
      {
         "vice_sound_sample_rate",
         "Audio > Sample Rate",
         "Sample Rate",
         "Sound sample rate in Hz.",
         NULL,
         "audio",
         {
            { "22050", NULL },
            { "44100", NULL },
            { "48000", NULL },
            { "96000", NULL },
            { NULL, NULL },
         },
         "48000"
      },
#if !defined(__XPET__) && !defined(__XCBM2__)
      {
         "vice_analogmouse",
         "Input > Analog Stick Mouse",
         "Analog Stick Mouse",
         "Override analog stick remappings when non-joysticks are used. 'OFF' controls mouse/paddles with both analogs when remappings are empty.",
         NULL,
         "input",
         {
            { "disabled", NULL },
            { "left", "Left Analog" },
            { "right", "Right Analog" },
            { "both", "Both Analogs" },
            { NULL, NULL },
         },
         "left"
      },
      {
         "vice_analogmouse_deadzone",
         "Input > Analog Stick Mouse Deadzone",
         "Analog Stick Mouse Deadzone",
         "Required distance from stick center to register input.",
         NULL,
         "input",
         {
            { "0", "0%" },
            { "5", "5%" },
            { "10", "10%" },
            { "15", "15%" },
            { "20", "20%" },
            { "25", "25%" },
            { "30", "30%" },
            { "35", "35%" },
            { "40", "40%" },
            { "45", "45%" },
            { "50", "50%" },
            { NULL, NULL },
         },
         "20"
      },
      {
         "vice_analogmouse_speed",
         "Input > Left Analog Stick Mouse Speed",
         "Left Analog Stick Mouse Speed",
         "Mouse speed for left analog stick.",
         NULL,
         "input",
         ANALOG_STICK_SPEED_OPTIONS,
         "1.0"
      },
      {
         "vice_analogmouse_speed_right",
         "Input > Right Analog Stick Mouse Speed",
         "Right Analog Stick Mouse Speed",
         "Mouse speed for right analog stick.",
         NULL,
         "input",
         ANALOG_STICK_SPEED_OPTIONS,
         "1.0"
      },
      {
         "vice_dpadmouse_speed",
         "Input > D-Pad Mouse Speed",
         "D-Pad Mouse Speed",
         "Mouse speed for directional pad.",
         NULL,
         "input",
         {
            { "0", "disabled" },
            { "1", "16%" },
            { "2", "33%" },
            { "3", "50%" },
            { "4", "66%" },
            { "5", "83%" },
            { "6", "100%" },
            { "7", "116%" },
            { "8", "133%" },
            { "9", "150%" },
            { "10", "166%" },
            { "11", "183%" },
            { "12", "200%" },
            { "13", "216%" },
            { "14", "233%" },
            { "15", "250%" },
            { "16", "266%" },
            { "17", "283%" },
            { "18", "300%" },
            { NULL, NULL },
         },
         "6"
      },
      {
         "vice_mouse_speed",
         "Input > Mouse Speed",
         "Mouse Speed",
         "Global mouse speed.",
         NULL,
         "input",
         {
            { "10", "10%" },
            { "20", "20%" },
            { "30", "30%" },
            { "40", "40%" },
            { "50", "50%" },
            { "60", "60%" },
            { "70", "70%" },
            { "80", "80%" },
            { "90", "90%" },
            { "100", "100%" },
            { "110", "110%" },
            { "120", "120%" },
            { "130", "130%" },
            { "140", "140%" },
            { "150", "150%" },
            { "160", "160%" },
            { "170", "170%" },
            { "180", "180%" },
            { "190", "190%" },
            { "200", "200%" },
            { "210", "210%" },
            { "220", "220%" },
            { "230", "230%" },
            { "240", "240%" },
            { "250", "250%" },
            { "260", "260%" },
            { "270", "270%" },
            { "280", "280%" },
            { "290", "290%" },
            { "300", "300%" },
            { NULL, NULL },
         },
         "100"
      },
#endif
      {
         "vice_physical_keyboard_pass_through",
         "Input > Keyboard Pass-through",
         "Keyboard Pass-through",
         "'ON' passes all physical keyboard events to the core. 'OFF' prevents RetroPad keys from generating keyboard events.\nNOTE: This is a legacy option for old frontends that do not block keyboard events when using RetroPad, so it does nothing with current RetroArch.",
         NULL,
         "input",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#if !defined(__XSCPU64__) && !defined(__X64DTV__)
      {
         "vice_datasette_hotkeys",
         "Input > Datasette Hotkeys",
         "Datasette Hotkeys",
         "Toggle all Datasette hotkeys.",
         NULL,
         "input",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
#if !defined(__XPET__) && !defined(__XCBM2__)
      {
         "vice_keyrah_keypad_mappings",
         "Input > Keyrah Keypad Mappings",
         "Keyrah Keypad Mappings",
         "Hardcoded keypad to joyport mappings for Keyrah hardware.",
         NULL,
         "input",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XCBM5x0__)
      {
         "vice_keyboard_keymap",
         "Input > Keyboard Keymap",
         "Keyboard Keymap",
#if defined(__XPLUS4__)
         "User-defined keymaps go in 'system/vice/PLUS4'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#elif defined(__XVIC__)
         "User-defined keymaps go in 'system/vice/VIC20'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#elif defined(__X128__)
         "User-defined keymaps go in 'system/vice/C128'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#elif defined(__XSCPU64__)
         "User-defined keymaps go in 'system/vice/SCPU64'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#else
         "User-defined keymaps go in 'system/vice/C64'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#endif
         NULL,
         "input",
         {
            { "positional", "Positional" },
            { "symbolic", "Symbolic" },
            { "positional-user", "Positional (User-defined)" },
            { "symbolic-user", "Symbolic (User-defined)" },
            { NULL, NULL },
         },
         "positional"
      },
#endif
      {
         "vice_mapping_options_display",
         "Show Mapping Options",
         NULL,
         "",
         NULL,
         NULL,
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "enabled"
      },
      /* Hotkeys */
      {
         "vice_mapper_vkbd",
         "Hotkey > Toggle Virtual Keyboard",
         "Toggle Virtual Keyboard",
         "Press the mapped key to toggle the virtual keyboard.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_statusbar",
         "Hotkey > Toggle Statusbar",
         "Toggle Statusbar",
         "Press the mapped key to toggle the statusbar.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "RETROK_F12"
      },
#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XVIC__)
      {
         "vice_mapper_joyport_switch",
         "Hotkey > Switch Joyport",
         "Switch Joyport",
         "Press the mapped key to switch joyports 1 & 2.\nSwitching disables 'RetroPad Port' option until core restart.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "RETROK_RCTRL"
      },
#endif
      {
         "vice_mapper_turbo_fire_toggle",
         "Hotkey > Toggle Turbo Fire",
         "Toggle Turbo Fire",
         "Press the mapped key to toggle turbo fire.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_save_disk_toggle",
         "Hotkey > Toggle Save Disk",
         "Toggle Save Disk",
         "Press the mapped key to create/insert/eject a save disk.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "---"
      },
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      {
         "vice_mapper_aspect_ratio_toggle",
         "Hotkey > Toggle Aspect Ratio",
         "Toggle Aspect Ratio",
         "Press the mapped key to toggle aspect ratio.\nToggling disables 'Pixel Aspect Ratio' option until core restart.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "---"
      },
#endif
      {
         "vice_mapper_crop_toggle",
         "Hotkey > Toggle Crop",
         "Toggle Crop",
         "Press the mapped key to toggle crop.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "---"
      },
#if !defined(__XSCPU64__) && !defined(__X64DTV__)
      /* Datasette controls */
      {
         "vice_mapper_datasette_toggle_hotkeys",
         "Hotkey > Toggle Datasette Hotkeys",
         "Toggle Datasette Hotkeys",
         "Press the mapped key to toggle Datasette hotkeys.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_datasette_start",
         "Hotkey > Datasette PLAY",
         "Datasette PLAY",
         "Press play on tape.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "RETROK_UP"
      },
      {
         "vice_mapper_datasette_stop",
         "Hotkey > Datasette STOP",
         "Datasette STOP",
         "Press stop on tape.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "RETROK_DOWN"
      },
      {
         "vice_mapper_datasette_rewind",
         "Hotkey > Datasette REWIND",
         "Datasette REWIND",
         "Press rewind on tape.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "RETROK_LEFT"
      },
      {
         "vice_mapper_datasette_forward",
         "Hotkey > Datasette F.FWD",
         "Datasette F.FWD",
         "Press fast forward on tape.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "RETROK_RIGHT"
      },
      {
         "vice_mapper_datasette_reset",
         "Hotkey > Datasette RESET",
         "Datasette RESET",
         "Reset tape to beginning.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "---"
      },
#endif
      {
         "vice_mapper_reset",
         "Hotkey > Reset",
         "Reset",
         "Press the mapped key to trigger the selected 'Reset Type'.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "RETROK_END"
      },
      {
         "vice_mapper_warp_mode",
         "Hotkey > Hold Warp Mode",
         "Hold Warp Mode",
         "Hold the mapped key for warp mode.",
         NULL,
         "hotkey",
         {{ NULL, NULL }},
         "---"
      },
      /* Button mappings */
      {
         "vice_mapper_up",
         "RetroPad > Up",
         "Up",
         "Unmapped defaults to joystick up.",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_down",
         "RetroPad > Down",
         "Down",
         "Unmapped defaults to joystick down.",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_left",
         "RetroPad > Left",
         "Left",
         "Unmapped defaults to joystick left.",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_right",
         "RetroPad > Right",
         "Right",
         "Unmapped defaults to joystick right.",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_b",
         "RetroPad > B",
         "B",
         "Unmapped defaults to fire button.\nVKBD: Press selected key.",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_a",
         "RetroPad > A",
         "A",
         "Unmapped defaults to 2nd fire button.\nVKBD: Toggle transparency. Remapping to non-keyboard keys overrides VKBD function!",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_y",
         "RetroPad > Y",
         "Y",
         "VKBD: Toggle 'ShiftLock'. Remapping to non-keyboard keys overrides VKBD function!",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_x",
         "RetroPad > X",
         "X",
         "VKBD: Press 'Space'. Remapping to non-keyboard keys overrides VKBD function!",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "RETROK_SPACE"
      },
      {
         "vice_mapper_select",
         "RetroPad > Select",
         "Select",
         "VKBD comes up with RetroPad Select by default.",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "TOGGLE_VKBD"
      },
      {
         "vice_mapper_start",
         "RetroPad > Start",
         "Start",
         "VKBD: Press 'Return'. Remapping to non-keyboard keys overrides VKBD function!",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_l",
         "RetroPad > L",
         "L",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_r",
         "RetroPad > R",
         "R",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_l2",
         "RetroPad > L2",
         "L2",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "RETROK_ESCAPE"
      },
      {
         "vice_mapper_r2",
         "RetroPad > R2",
         "R2",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "RETROK_RETURN"
      },
      {
         "vice_mapper_l3",
         "RetroPad > L3",
         "L3",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_r3",
         "RetroPad > R3",
         "R3",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      /* Left Stick */
      {
         "vice_mapper_lu",
         "RetroPad > Left Analog > Up",
         "Left Analog > Up",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_ld",
         "RetroPad > Left Analog > Down",
         "Left Analog > Down",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_ll",
         "RetroPad > Left Analog > Left",
         "Left Analog > Left",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_lr",
         "RetroPad > Left Analog > Right",
         "Left Analog > Right",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      /* Right Stick */
      {
         "vice_mapper_ru",
         "RetroPad > Right Analog > Up",
         "Right Analog > Up",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_rd",
         "RetroPad > Right Analog > Down",
         "Right Analog > Down",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_rl",
         "RetroPad > Right Analog > Left",
         "Right Analog > Left",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_rr",
         "RetroPad > Right Analog > Right",
         "Right Analog > Right",
         "",
         NULL,
         "retropad",
         {{ NULL, NULL }},
         "---"
      },
#if !defined(__XPET__) && !defined(__XCBM2__)
      /* Turbo Fire */
      {
         "vice_turbo_fire",
         "RetroPad > Turbo Fire",
         "Turbo Fire",
         "Hotkey toggling disables this option until core restart.",
         NULL,
         "input",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_turbo_fire_button",
         "RetroPad > Turbo Button",
         "Turbo Button",
         "Replace the mapped button with turbo fire button.",
         NULL,
         "input",
         {
            { "B", "RetroPad B" },
            { "A", "RetroPad A" },
            { "Y", "RetroPad Y" },
            { "X", "RetroPad X" },
            { "L", "RetroPad L" },
            { "R", "RetroPad R" },
            { "L2", "RetroPad L2" },
            { "R2", "RetroPad R2" },
            { NULL, NULL },
         },
         "B"
      },
      {
         "vice_turbo_pulse",
         "RetroPad > Turbo Pulse",
         "Turbo Pulse",
         "Frames in a button cycle.\n- '2' = 1 frame down, 1 frame up\n- '4' = 2 frames down, 2 frames up\n- '6' = 3 frames down, 3 frames up\netc.",
         NULL,
         "input",
         {
            { "2", "2 frames" },
            { "4", "4 frames" },
            { "6", "6 frames" },
            { "8", "8 frames" },
            { "10", "10 frames" },
            { "12", "12 frames" },
            { NULL, NULL },
         },
         "6"
      },
#endif
#if !defined(__XCBM5x0__)
      {
         "vice_userport_joytype",
         "Input > Userport Joystick Adapter",
         "Userport Joystick Adapter",
         "Required for more than 2 joysticks, for example IK+ Gold with 3 players.",
         NULL,
         "input",
         {
            { "disabled", NULL },
            { "CGA", "Protovision / Classical Games" },
            { "HIT", "Digital Excess & Hitmen" },
            { "Kingsoft", "Kingsoft" },
            { "Starbyte", "Starbyte" },
            { "Synergy", "Synergy" },
            { "Hummer", "C64DTV Hummer" },
            { "OEM", "VIC-20 OEM" },
            { "PET", "PET" },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XVIC__)
      {
         "vice_joyport",
         "RetroPad > Joystick Port",
         "Joystick Port",
         "Most games use port 2, some use port 1.\nFilename forcing or hotkey toggling disables this option until core restart.",
         NULL,
         "input",
         {
            { "1", "Port 1" },
            { "2", "Port 2" },
            { NULL, NULL },
         },
         "2"
      },
#endif
#if !defined(__XPET__) && !defined(__XCBM2__)
      {
         "vice_joyport_type",
         "RetroPad > Joystick Port Type",
         "Joystick Port Type",
         "Non-joysticks are plugged in current port only and are controlled with left analog stick or mouse.",
         NULL,
         "input",
         {
            { "1", "Joystick" },
            { "2", "Paddles (Split)" },
            { "2R", "Paddles (Raw)" },
            { "3", "Mouse (1351)" },
            { "4", "Mouse (NEOS)" },
            { "5", "Mouse (Amiga)" },
            { "6", "Trackball (Atari CX-22)" },
            { "7", "Mouse (Atari ST)" },
            { "8", "Mouse (SmartMouse)" },
            { "9", "Mouse (Micromys)" },
            { "10", "Koalapad" },
            { "11", "Light Pen (Up trigger)" },
            { "12", "Light Pen (Left trigger)" },
            { "13", "Light Pen (Datel)" },
            { "16", "Light Pen (Inkwell)" },
            { "14", "Light Gun (Magnum Light Phaser)" },
            { "15", "Light Gun (Stack Light Rifle)" },
            { NULL, NULL },
         },
         "1"
      },
      {
         "vice_retropad_options",
         "RetroPad > Face Button Options",
         "RetroPad Face Button Options",
         "Rotate face buttons clockwise and/or make 2nd fire press up.",
         NULL,
         "input",
         {
            { "disabled", "B = Fire, A = 2nd fire" },
            { "jump", "B = Fire, A = Up" },
            { "rotate", "Y = Fire, B = 2nd fire" },
            { "rotate_jump", "Y = Fire, B = Up" },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
      { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
   };

   free_vice_carts();

   /* Fill in the values for all the mappers */
   int i = 0;
   int j = 0;
   int hotkey = 0;
   int hotkeys_skipped = 0;
   /* Count special hotkeys */
   while (retro_keys[j].value[0] && j < RETRO_NUM_CORE_OPTION_VALUES_MAX - 1)
   {
      if (retro_keys[j].id < 0)
         hotkeys_skipped++;
      ++j;
   }
   while (option_defs_us[i].key)
   {
      if (strstr(option_defs_us[i].key, "vice_mapper_"))
      {
         /* Show different key list for hotkeys (special negatives removed) */
         if (  strstr(option_defs_us[i].key, "vice_mapper_vkbd")
            || strstr(option_defs_us[i].key, "vice_mapper_statusbar")
            || strstr(option_defs_us[i].key, "vice_mapper_joyport_switch")
            || strstr(option_defs_us[i].key, "vice_mapper_reset")
            || strstr(option_defs_us[i].key, "vice_mapper_aspect_ratio_toggle")
            || strstr(option_defs_us[i].key, "vice_mapper_crop_toggle")
            || strstr(option_defs_us[i].key, "vice_mapper_warp_mode")
            || strstr(option_defs_us[i].key, "vice_mapper_turbo_fire_toggle")
            || strstr(option_defs_us[i].key, "vice_mapper_save_disk_toggle")
            || strstr(option_defs_us[i].key, "vice_mapper_datasette_toggle_hotkeys")
            || strstr(option_defs_us[i].key, "vice_mapper_datasette_start")
            || strstr(option_defs_us[i].key, "vice_mapper_datasette_stop")
            || strstr(option_defs_us[i].key, "vice_mapper_datasette_rewind")
            || strstr(option_defs_us[i].key, "vice_mapper_datasette_forward")
            || strstr(option_defs_us[i].key, "vice_mapper_datasette_reset"))
            hotkey = 1;
         else
            hotkey = 0;

         j = 0;
         if (hotkey)
         {
            while (retro_keys[j].value[0] && j < RETRO_NUM_CORE_OPTION_VALUES_MAX - 1)
            {
               if (j == 0) /* "---" unmapped */
               {
                  option_defs_us[i].values[j].value = retro_keys[j].value;
                  option_defs_us[i].values[j].label = retro_keys[j].label;
               }
               else
               {
                  option_defs_us[i].values[j].value = retro_keys[j + hotkeys_skipped + 1].value;
                  option_defs_us[i].values[j].label = retro_keys[j + hotkeys_skipped + 1].label;
               }
               ++j;
            }
         }
         else
         {
            while (retro_keys[j].value[0] && j < RETRO_NUM_CORE_OPTION_VALUES_MAX - 1)
            {
               option_defs_us[i].values[j].value = retro_keys[j].value;
               option_defs_us[i].values[j].label = retro_keys[j].label;
               ++j;
            }
         }
         option_defs_us[i].values[j].value = NULL;
         option_defs_us[i].values[j].label = NULL;
      }
      else if (!strcmp(option_defs_us[i].key, "vice_cartridge"))
      {
         j = 0;
         option_defs_us[i].values[j].value = "none";
         option_defs_us[i].values[j].label = "disabled";
         ++j;

         DIR *cart_dir;
         struct dirent *cart_dirp;

         char machine_directory[RETRO_PATH_MAX] = {0};
         snprintf(machine_directory, sizeof(machine_directory), "%s%s%s",
               retro_system_data_directory, ARCHDEP_DIR_SEP_STR, machine_name);

         /* Scan system/vice/machine directory for cartridges */
         if (path_is_directory(machine_directory))
         {
            cart_dir = opendir(machine_directory);
            while ((cart_dirp = readdir(cart_dir)) != NULL && j < RETRO_NUM_CORE_OPTION_VALUES_MAX - 1)
            {
               /* Blacklisted */
               if (!strcmp(cart_dirp->d_name, "scpu-dos-1.4.bin") ||
                   !strcmp(cart_dirp->d_name, "scpu-dos-2.04.bin"))
                  continue;

               if (dc_get_image_type(cart_dirp->d_name) == DC_IMAGE_TYPE_MEM)
               {
                  char cart_value[RETRO_PATH_MAX] = {0};
                  char cart_label[128] = {0};
                  snprintf(cart_value, sizeof(cart_value), "%s", cart_dirp->d_name);
                  snprintf(cart_label, sizeof(cart_label), "%s", path_remove_extension(cart_dirp->d_name));

                  vice_carts[j].value = strdup(cart_value);
                  vice_carts[j].label = strdup(cart_label);

                  option_defs_us[i].values[j].value = vice_carts[j].value;
                  option_defs_us[i].values[j].label = vice_carts[j].label;
                  ++j;
               }

               vice_carts[j].value = NULL;
               vice_carts[j].label = NULL;
            }
            closedir(cart_dir);
         }

         option_defs_us[i].values[j].value = NULL;
         option_defs_us[i].values[j].label = NULL;

         /* Info sublabel */
         char info[128] = {0};
         snprintf(info, sizeof(info), "Cartridge images go in 'system/vice/%s'.\nChanging while running resets the system!", machine_name);
         option_defs_us[i].info = strdup(info);
      }
      ++i;
   }

   struct retro_core_options_v2 options_us = {
      option_cats_us,
      option_defs_us
   };

   unsigned version = 0;
   if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
      version = 0;

   if (version >= 2)
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_v2_intl core_options_intl;

      core_options_intl.us    = &options_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = options_intl[language];

      libretro_supports_option_categories = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL,
            &core_options_intl);
#else
      libretro_supports_option_categories = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            &options_us);
#endif
   }
   else
   {
      size_t i, j;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_core_option_definition
            *option_v1_defs_us         = NULL;
#ifndef HAVE_NO_LANGEXTRA
      size_t num_options_intl          = 0;
      struct retro_core_option_v2_definition
            *option_defs_intl          = NULL;
      struct retro_core_option_definition
            *option_v1_defs_intl       = NULL;
      struct retro_core_options_intl
            core_options_v1_intl;
#endif
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine total number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      if (version >= 1)
      {
         /* Allocate US array */
         option_v1_defs_us = (struct retro_core_option_definition *)
               calloc(num_options + 1, sizeof(struct retro_core_option_definition));

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
            struct retro_core_option_value *option_values         = option_def_us->values;
            struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
            struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

            option_v1_def_us->key           = option_def_us->key;
            option_v1_def_us->desc          = option_def_us->desc;
            option_v1_def_us->info          = option_def_us->info;
            option_v1_def_us->default_value = option_def_us->default_value;

            /* Values must be copied individually... */
            while (option_values->value)
            {
               option_v1_values->value = option_values->value;
               option_v1_values->label = option_values->label;

               option_values++;
               option_v1_values++;
            }
         }

#ifndef HAVE_NO_LANGEXTRA
         if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
             (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH) &&
             options_intl[language])
            option_defs_intl = options_intl[language]->definitions;

         if (option_defs_intl)
         {
            /* Determine number of intl options */
            while (true)
            {
               if (option_defs_intl[num_options_intl].key)
                  num_options_intl++;
               else
                  break;
            }

            /* Allocate intl array */
            option_v1_defs_intl = (struct retro_core_option_definition *)
                  calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_intl array */
            for (i = 0; i < num_options_intl; i++)
            {
               struct retro_core_option_v2_definition *option_def_intl = &option_defs_intl[i];
               struct retro_core_option_value *option_values           = option_def_intl->values;
               struct retro_core_option_definition *option_v1_def_intl = &option_v1_defs_intl[i];
               struct retro_core_option_value *option_v1_values        = option_v1_def_intl->values;

               option_v1_def_intl->key           = option_def_intl->key;
               option_v1_def_intl->desc          = option_def_intl->desc;
               option_v1_def_intl->info          = option_def_intl->info;
               option_v1_def_intl->default_value = option_def_intl->default_value;

               /* Values must be copied individually... */
               while (option_values->value)
               {
                  option_v1_values->value = option_values->value;
                  option_v1_values->label = option_values->label;

                  option_values++;
                  option_v1_values++;
               }
            }
         }

         core_options_v1_intl.us    = option_v1_defs_us;
         core_options_v1_intl.local = option_v1_defs_intl;

         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_v1_intl);
#else
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
#endif
      }
      else
      {
         /* Allocate arrays */
         variables  = (struct retro_variable *)calloc(num_options + 1,
               sizeof(struct retro_variable));
         values_buf = (char **)calloc(num_options, sizeof(char *));

         if (!variables || !values_buf)
            goto error;

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            const char *key                        = option_defs_us[i].key;
            const char *desc                       = option_defs_us[i].desc;
            const char *default_value              = option_defs_us[i].default_value;
            struct retro_core_option_value *values = option_defs_us[i].values;
            size_t buf_len                         = 3;
            size_t default_index                   = 0;

            values_buf[i] = NULL;

            if (desc)
            {
               size_t num_values = 0;

               /* Determine number of values */
               while (true)
               {
                  if (values[num_values].value)
                  {
                     /* Check if this is the default value */
                     if (default_value)
                        if (strcmp(values[num_values].value, default_value) == 0)
                           default_index = num_values;

                     buf_len += strlen(values[num_values].value);
                     num_values++;
                  }
                  else
                     break;
               }

               /* Build values string */
               if (num_values > 0)
               {
                  buf_len += num_values - 1;
                  buf_len += strlen(desc);

                  values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                  if (!values_buf[i])
                     goto error;

                  strcpy(values_buf[i], desc);
                  strcat(values_buf[i], "; ");

                  /* Default value goes first */
                  strcat(values_buf[i], values[default_index].value);

                  /* Add remaining values */
                  for (j = 0; j < num_values; j++)
                  {
                     if (j != default_index)
                     {
                        strcat(values_buf[i], "|");
                        strcat(values_buf[i], values[j].value);
                     }
                  }
               }
            }

            variables[option_index].key   = key;
            variables[option_index].value = values_buf[i];
            option_index++;
         }

         /* Set variables */
         environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
      }

error:
      /* Clean up */

      if (option_v1_defs_us)
      {
         free(option_v1_defs_us);
         option_v1_defs_us = NULL;
      }

#ifndef HAVE_NO_LANGEXTRA
      if (option_v1_defs_intl)
      {
         free(option_v1_defs_intl);
         option_v1_defs_intl = NULL;
      }
#endif

      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

static const struct retro_controller_description joyport_controllers[] =
{
   { "RetroPad", RETRO_DEVICE_JOYPAD },
   { "Joystick", RETRO_DEVICE_VICE_JOYSTICK },
   { "Keyboard", RETRO_DEVICE_VICE_KEYBOARD },
   { "None", RETRO_DEVICE_NONE },
   { NULL, 0 }
};

static const struct retro_controller_description nonport_controllers[] =
{
   { "Keyboard", RETRO_DEVICE_VICE_KEYBOARD },
   { "None", RETRO_DEVICE_NONE },
   { NULL, 0 }
};

static void retro_set_inputs(void)
{
   unsigned i;

   const struct retro_controller_info ports[] =
   {
      { joyport_controllers, sizeof(joyport_controllers) / sizeof(joyport_controllers[0]) },
      { joyport_controllers, sizeof(joyport_controllers) / sizeof(joyport_controllers[0]) },
      { joyport_controllers, sizeof(joyport_controllers) / sizeof(joyport_controllers[0]) },
      { joyport_controllers, sizeof(joyport_controllers) / sizeof(joyport_controllers[0]) },
      { nonport_controllers, sizeof(nonport_controllers) / sizeof(nonport_controllers[0]) },
      { nonport_controllers, sizeof(nonport_controllers) / sizeof(nonport_controllers[0]) },
      { NULL, 0 }
   };

   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

   #define RETRO_DESCRIPTOR_BLOCK(_user)                                                                        \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },                                          \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },                                      \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },                                      \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },                                    \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B / Fire" },                                     \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },                                            \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },                                            \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },                                            \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },                                  \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },                                    \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L" },                                            \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R" },                                            \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2" },                                          \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2" },                                          \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3" },                                          \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3" },                                          \
   { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Left Analog X" },   \
   { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Left Analog Y" },   \
   { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Right Analog X" }, \
   { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Right Analog Y" }

   static struct retro_input_descriptor input_descriptors[] =
   {
      RETRO_DESCRIPTOR_BLOCK(0),
      RETRO_DESCRIPTOR_BLOCK(1),
      RETRO_DESCRIPTOR_BLOCK(2),
      RETRO_DESCRIPTOR_BLOCK(3),
      RETRO_DESCRIPTOR_BLOCK(4),
      RETRO_DESCRIPTOR_BLOCK(5),
      {0},
   };
   #undef RETRO_DESCRIPTOR_BLOCK

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &input_descriptors);

   /* Reset analog device types to RetroPad */
   for (i = 0; i < RETRO_DEVICES; i++)
      if (retro_devices[i] == RETRO_DEVICE_ANALOG)
         retro_devices[i] = RETRO_DEVICE_JOYPAD;
}

void retro_set_options_display(void)
{
   struct retro_core_option_display option_display;
   bool manual_crop = true;

   option_display.visible = (crop_id == CROP_MANUAL);
   manual_crop = option_display.visible;

   option_display.key = "vice_manual_crop_top";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_manual_crop_bottom";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_manual_crop_left";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_manual_crop_right";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   option_display.visible = !option_display.visible;
   option_display.key = "vice_crop_mode";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   option_display.visible = (crop_id == CROP_AUTO || crop_id == CROP_AUTO_DISABLE);
   option_display.key = "vice_crop_delay";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   /*** Options display ***/
   if (libretro_supports_option_categories)
   {
      option_display.visible = false;

      option_display.key = "vice_mapping_options_display";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

      option_display.key = "vice_video_options_display";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

      option_display.key = "vice_audio_options_display";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
      return;
   }

   /* Mapping options */
   option_display.visible = opt_mapping_options_display;

   option_display.key = "vice_mapper_up";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_down";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_left";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_right";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_select";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_start";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_b";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_a";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_y";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_x";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_l";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_r";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_l2";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_r2";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_l3";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_r3";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_lu";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_ld";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_ll";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_lr";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_ru";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_rd";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_rl";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_rr";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_vkbd";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_statusbar";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XVIC__)
   option_display.key = "vice_mapper_joyport_switch";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
   option_display.key = "vice_mapper_aspect_ratio_toggle";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
   option_display.key = "vice_mapper_crop_toggle";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_reset";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_warp_mode";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_turbo_fire_toggle";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_save_disk_toggle";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#if !defined(__XSCPU64__) && !defined(__X64DTV__)
   option_display.key = "vice_mapper_datasette_toggle_hotkeys";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_start";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_stop";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_rewind";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_forward";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_reset";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif

   /* Audio options */
   option_display.visible = opt_audio_options_display;

#if !defined(__X64DTV__)
   option_display.key = "vice_drive_sound_emulation";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
#if !defined(__XSCPU64__) && !defined(__X64DTV__)
   option_display.key = "vice_datasette_sound";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__) || defined(__XPET__) || defined(__XCBM2__)
   option_display.key = "vice_audio_leak_emulation";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__)
   option_display.key = "vice_sid_engine";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_sid_model";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_sid_extra";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_sampling";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_passband";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_gain";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_filterbias";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_8580filterbias";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
   option_display.key = "vice_sfx_sound_expander";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
   option_display.key = "vice_sound_sample_rate";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   /* Video options */
   option_display.visible = opt_video_options_display;

   option_display.key = "vice_vkbd_theme";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vkbd_transparency";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vkbd_dimming";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_statusbar";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_statusbar_startup";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_statusbar_messages";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_joyport_pointer_color";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_gfx_colors";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
   option_display.key = "vice_aspect_ratio";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
   option_display.key = "vice_crop";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   if (manual_crop)
   {
      option_display.key = "vice_manual_crop_top";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
      option_display.key = "vice_manual_crop_bottom";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
      option_display.key = "vice_manual_crop_left";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
      option_display.key = "vice_manual_crop_right";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   }
   else
   {
      option_display.key = "vice_crop_mode";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   }
   option_display.key = "vice_crop_delay";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#if defined(__XVIC__)
   option_display.key = "vice_vic20_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vic_filter";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vic_filter_oddline_phase";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vic_filter_oddline_offset";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vic_color_gamma";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vic_color_tint";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vic_color_saturation";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vic_color_contrast";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vic_color_brightness";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#elif defined(__XPLUS4__)
   option_display.key = "vice_plus4_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_ted_filter";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_ted_filter_oddline_phase";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_ted_filter_oddline_offset";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_ted_color_gamma";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_ted_color_tint";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_ted_color_saturation";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_ted_color_contrast";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_ted_color_brightness";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#elif defined(__XPET__)
   option_display.key = "vice_pet_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_crtc_filter";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#elif defined(__XCBM2__)
   option_display.key = "vice_cbm2_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_crtc_filter";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#else
   option_display.key = "vice_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_filter";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_filter_oddline_phase";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_filter_oddline_offset";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_color_gamma";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_color_tint";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_color_saturation";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_color_contrast";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_color_brightness";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
}

static bool retro_update_display(void)
{
   /* Core options */
   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

   return updated;
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   /* Must set these here for the dynamic cartridge option */
   retro_set_paths();

   retro_set_core_options();
   retro_set_inputs();

   bool support_no_game = true;
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &support_no_game);

   struct retro_led_interface led_interface;
   if (environ_cb(RETRO_ENVIRONMENT_GET_LED_INTERFACE, &led_interface))
      if (led_interface.set_led_state && !led_state_cb)
         led_state_cb = led_interface.set_led_state;

#ifdef USE_LIBRETRO_VFS
   struct retro_vfs_interface_info vfs_iface_info;
   vfs_iface_info.required_interface_version = 2;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
      filestream_vfs_init(&vfs_iface_info);
#endif
}

int log_resources_set_int(const char *name, int value)
{
   if (log_resource_set)
      log_cb(RETRO_LOG_INFO, "Set resource: %s => %d\n", name, value);
   return resources_set_int(name, value);
}

int log_resources_set_string(const char *name, const char *value)
{
   if (log_resource_set)
      log_cb(RETRO_LOG_INFO, "Set resource: %s => \"%s\"\n", name, value);
   return resources_set_string(name, value);
}

void set_variable(const char* key, const char* value)
{
   struct retro_variable var = {0};

   var.key   = strdup(key);
   var.value = strdup(value);
   if (environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var))
      log_cb(RETRO_LOG_INFO, "SET_VARIABLE: %s = \"%s\"\n", var.key, var.value);
}

char* get_variable(const char* key)
{
   struct retro_variable var = {0};

   var.key   = key;
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      return strdup(var.value);

   return NULL;
}

#define GET_VAR(x) \
   var.key   = "vice_" x; \
   var.value = NULL; \
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)

static void update_variables(void)
{
   struct retro_variable var = {0};

#ifdef RETRO_DEBUG
   log_cb(RETRO_LOG_INFO, "Updating variables, UI finalized = %d\n", retro_ui_finalized);
#endif

#if !defined(__XPET__) && !defined(__X64DTV__)
   GET_VAR("cartridge")
   {
      char cart_full[RETRO_PATH_MAX] = {0};

      if (!strcmp(var.value, "none"))
         snprintf(cart_full, sizeof(cart_full), "%s", "");
      else
         snprintf(cart_full, sizeof(cart_full), "%s%s%s%s%s",
               retro_system_data_directory, ARCHDEP_DIR_SEP_STR, machine_name, ARCHDEP_DIR_SEP_STR, var.value);

      if (retro_ui_finalized && strcmp(vice_opt.CartridgeFile, cart_full))
      {
         cartridge_unset_default();
         cartridge_detach_image(-1);

         if (strcmp(cart_full, ""))
#if defined(__XVIC__)
            cartridge_attach_image(vic20_autodetect_cartridge_type(cart_full), cart_full);
#elif defined(__XPLUS4__)
            cartridge_attach_image(CARTRIDGE_PLUS4_DETECT, cart_full);
#else
            cartridge_attach_image(0, cart_full);
#endif

         request_restart = true;
      }

      sprintf(vice_opt.CartridgeFile, "%s", cart_full);
   }
#endif

#if !defined(__X64DTV__)
   GET_VAR("autostart")
   {
      int autostartwarp = 0;

      if (!strcmp(var.value, "warp")) autostartwarp = 1;
      else                            autostartwarp = 0;

      if (!strcmp(var.value, "disabled")) opt_autostart = false;
      else                                opt_autostart = true;

      if (retro_ui_finalized)
      {
         if (vice_opt.AutostartWarp != autostartwarp)
            resources_set_int("AutostartWarp", autostartwarp);

         noautostart = (noautostart_locked) ? noautostart : !opt_autostart;
      }

      vice_opt.AutostartWarp = autostartwarp;
   }
#endif

#if !defined(__X64DTV__)
   GET_VAR("autoloadwarp")
   {
      opt_autoloadwarp = 0;

      if      (strstr(var.value, "disk"))      opt_autoloadwarp |= AUTOLOADWARP_DISK;
      else if (strstr(var.value, "tape"))      opt_autoloadwarp |= AUTOLOADWARP_TAPE;
      else                                     opt_autoloadwarp = AUTOLOADWARP_DISK | AUTOLOADWARP_TAPE;

      if      (strstr(var.value, "mute"))      opt_autoloadwarp |= AUTOLOADWARP_MUTE;
      else if (!strcmp(var.value, "disabled")) opt_autoloadwarp = 0;

      /* Silently restore drive sounds when autoloadwarp is disabled and DSE is enabled */
      if (retro_ui_finalized && vice_opt.DriveSoundEmulation && vice_opt.DriveTrueEmulation &&
          (!(opt_autoloadwarp & AUTOLOADWARP_DISK) && !(opt_autoloadwarp & AUTOLOADWARP_MUTE)))
         resources_set_int("DriveSoundEmulationVolume", vice_opt.DriveSoundEmulation);

      /* Also enable AutostartWarp for faster startup */
      if (opt_autoloadwarp)
      {
         vice_opt.AutostartWarp = 1;
         if (retro_ui_finalized)
            resources_set_int("AutostartWarp", vice_opt.AutostartWarp);
      }

#if !defined(__XSCPU64__)
      /* Silently restore tape sounds when autoloadwarp is disabled */
      if (retro_ui_finalized && vice_opt.DatasetteSound &&
          (!(opt_autoloadwarp & AUTOLOADWARP_TAPE) && !(opt_autoloadwarp & AUTOLOADWARP_MUTE)))
         resources_set_int("DatasetteSound", vice_opt.DatasetteSound);
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      /* Silently restore audio leak when autoloadwarp is disabled */
      if (retro_ui_finalized && vice_opt.AudioLeak &&
          (!opt_autoloadwarp || opt_autoloadwarp & AUTOLOADWARP_MUTE))
         resources_set_int(AUDIOLEAK_RESOURCE, vice_opt.AudioLeak);
#endif
   }
#endif

#if !defined(__X64DTV__)
   GET_VAR("floppy_multidrive")
   {
      if (!strcmp(var.value, "disabled")) opt_floppy_multidrive = false;
      else                                opt_floppy_multidrive = true;
   }

   GET_VAR("floppy_write_protection")
   {
      int readonly = 0;

      if (!strcmp(var.value, "disabled")) readonly = 0;
      else                                readonly = 1;

      if (retro_ui_finalized && vice_opt.AttachDevice8Readonly != readonly)
      {
         log_resources_set_int("AttachDevice8d0Readonly", readonly);
         log_resources_set_int("AttachDevice8d1Readonly", readonly);
      }

      vice_opt.AttachDevice8Readonly = readonly;
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
   GET_VAR("easyflash_write_protection")
   {
      int writecrt = 0;

      if (!strcmp(var.value, "disabled")) writecrt = 1;
      else                                writecrt = 0;

      if (retro_ui_finalized && vice_opt.EasyFlashWriteCRT != writecrt)
         log_resources_set_int("EasyFlashWriteCRT", writecrt);

      vice_opt.EasyFlashWriteCRT = writecrt;
   }
#endif

#if !defined(__X64DTV__)
   GET_VAR("work_disk")
   {
      int work_disk_type = opt_work_disk_type;
      int work_disk_unit = opt_work_disk_unit;

      if (!strcmp(var.value, "disabled"))    opt_work_disk_type = 0;
      else
      {
         if      (strstr(var.value, "_d64")) opt_work_disk_type = DISK_IMAGE_TYPE_D64;
         else if (strstr(var.value, "_d71")) opt_work_disk_type = DISK_IMAGE_TYPE_D71;
         else if (strstr(var.value, "_d81")) opt_work_disk_type = DISK_IMAGE_TYPE_D81;
         else if (strstr(var.value, "_fs"))  opt_work_disk_type = DISK_IMAGE_TYPE_FS;

         if      (strstr(var.value, "8_"))   opt_work_disk_unit = 8;
         else if (strstr(var.value, "9_"))   opt_work_disk_unit = 9;
      }

      if (work_disk_type != opt_work_disk_type || work_disk_unit != opt_work_disk_unit)
         request_update_work_disk = true;
   }

   GET_VAR("virtual_device_traps")
   {
      if (retro_ui_finalized)
      {
#if 1
         /* Printer only */
         if (!strcmp(var.value, "disabled") && vice_opt.VirtualDevices)
            log_resources_set_int("VirtualDevice4", 0);
         else if (!strcmp(var.value, "enabled") && !vice_opt.VirtualDevices)
            log_resources_set_int("VirtualDevice4", 1);
#else
         if (!strcmp(var.value, "disabled") && vice_opt.VirtualDevices)
         {
            log_resources_set_int("VirtualDevice8", 0);
            log_resources_set_int("VirtualDevice9", 0);
         }
         else if (!strcmp(var.value, "enabled") && !vice_opt.VirtualDevices)
         {
            log_resources_set_int("VirtualDevice8", 1);
            log_resources_set_int("VirtualDevice9", 1);
         }
#endif
      }

      if (!strcmp(var.value, "disabled")) vice_opt.VirtualDevices = 0;
      else                                vice_opt.VirtualDevices = 1;
   }
#endif

#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__) && !defined(__X64DTV__)
   GET_VAR("warp_boost")
   {
      if (!strcmp(var.value, "disabled")) opt_warp_boost = 0;
      else                                opt_warp_boost = 1;
   }
#endif

#if !defined(__X64DTV__)
   GET_VAR("drive_true_emulation")
   {
      if (retro_ui_finalized && !tde_locked)
      {
         if (!strcmp(var.value, "disabled") && vice_opt.DriveTrueEmulation)
         {
            log_resources_set_int("Drive8TrueEmulation", 0);
            log_resources_set_int("Drive9TrueEmulation", 0);
            log_resources_set_int("VirtualDevice8", 1);
            log_resources_set_int("VirtualDevice9", 1);

         }
         else if (!strcmp(var.value, "enabled") && !vice_opt.DriveTrueEmulation)
         {
            log_resources_set_int("Drive8TrueEmulation", 1);
            log_resources_set_int("Drive9TrueEmulation", 1);
            log_resources_set_int("VirtualDevice8", 0);
            log_resources_set_int("VirtualDevice9", 0);
         }
      }

      if (!tde_locked)
      {
         if (!strcmp(var.value, "disabled"))
            vice_opt.DriveTrueEmulation = 0;
         else
            vice_opt.DriveTrueEmulation = 1;
      }

      /* Silently restore sounds when TDE and DSE is enabled */
      if (retro_ui_finalized && vice_opt.DriveSoundEmulation && vice_opt.DriveTrueEmulation)
         resources_set_int("DriveSoundEmulationVolume", vice_opt.DriveSoundEmulation);
   }
#endif

#if !defined(__X64DTV__)
   GET_VAR("drive_sound_emulation")
   {
      int val = atoi(var.value) * 20;

      if (retro_ui_finalized && vice_opt.DriveSoundEmulation != val)
      {
         if (!strcmp(var.value, "disabled"))
         {
            log_resources_set_int("DriveSoundEmulation", 0);
            log_resources_set_int("DriveSoundEmulationVolume", 0);
         }
         else
         {
            log_resources_set_int("DriveSoundEmulation", 1);
            log_resources_set_int("DriveSoundEmulationVolume", val);
         }
      }

      vice_opt.DriveSoundEmulation = val;

      /* Silently mute sounds without TDE,
       * because motor sound will not stop if TDE is changed during motor spinning
       * and also with autoloadwarping, because warping is muted anyway */
      if (retro_ui_finalized && vice_opt.DriveSoundEmulation &&
            (!vice_opt.DriveTrueEmulation ||
            (opt_autoloadwarp & AUTOLOADWARP_DISK && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))))
         resources_set_int("DriveSoundEmulationVolume", 0);
   }
#endif

#if !defined(__XSCPU64__) && !defined(__X64DTV__)
   GET_VAR("datasette_sound")
   {
      int val = atoi(var.value);
      opt_datasette_sound_volume = val;

      if (retro_ui_finalized && vice_opt.DatasetteSound != val)
      {
         if (!strcmp(var.value, "disabled"))
            log_resources_set_int("DatasetteSound", 0);
         else
            log_resources_set_int("DatasetteSound", 1);
      }

      vice_opt.DatasetteSound = val;

      /* Silently mute sounds with autoloadwarping */
      if (retro_ui_finalized && vice_opt.DatasetteSound &&
            (opt_autoloadwarp & AUTOLOADWARP_TAPE && !(opt_autoloadwarp & AUTOLOADWARP_MUTE)))
         resources_set_int("DatasetteSound", 0);
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__) || defined(__XPET__) || defined(__XCBM2__)
   GET_VAR("audio_leak_emulation")
   {
      int audioleak = 0;
      opt_audio_leak_volume = atoi(var.value);

      if (!strcmp(var.value, "disabled")) audioleak = 0;
      else                                audioleak = 1;

      if (retro_ui_finalized && vice_opt.AudioLeak != audioleak)
         log_resources_set_int(AUDIOLEAK_RESOURCE, audioleak);

      vice_opt.AudioLeak = audioleak;

      /* Silently mute leak with autoloadwarping */
      if (retro_ui_finalized && vice_opt.AudioLeak &&
            (opt_autoloadwarp && !(opt_autoloadwarp & AUTOLOADWARP_MUTE)))
         resources_set_int(AUDIOLEAK_RESOURCE, 0);
   }
#endif

   GET_VAR("sound_sample_rate")
   {
      vice_opt.SoundSampleRate = atoi(var.value);
   }

#if defined(__XVIC__)
   GET_VAR("vic20_model")
   {
      int model = 0;
      bool opt_model_auto_prev = opt_model_auto;

      if (strstr(var.value, "auto")) opt_model_auto = true;
      else                           opt_model_auto = false;

      if      (!strcmp(var.value, "VIC20 PAL auto"))  model = VIC20MODEL_VIC20_PAL;
      else if (!strcmp(var.value, "VIC20 NTSC auto")) model = VIC20MODEL_VIC20_NTSC;
      else if (!strcmp(var.value, "VIC20 PAL"))       model = VIC20MODEL_VIC20_PAL;
      else if (!strcmp(var.value, "VIC20 NTSC"))      model = VIC20MODEL_VIC20_NTSC;
      else if (!strcmp(var.value, "VIC21"))           model = VIC20MODEL_VIC21;

      if (retro_ui_finalized && (vice_opt.Model != model || opt_model_auto != opt_model_auto_prev))
      {
         request_model_set = model;
         request_restart = true;
         /* Memory expansion needs to be reset to get updated */
         vice_opt.VIC20Memory = 0xff;
      }

      vice_opt.Model = model;
   }

   GET_VAR("vic20_memory_expansions")
   {
      int vic20mem = 0;

      if      (!strcmp(var.value, "none")) vic20mem = 0;
      else if (!strcmp(var.value, "3kB"))  vic20mem = 1;
      else if (!strcmp(var.value, "8kB"))  vic20mem = 2;
      else if (!strcmp(var.value, "16kB")) vic20mem = 3;
      else if (!strcmp(var.value, "24kB")) vic20mem = 4;
      else if (!strcmp(var.value, "35kB")) vic20mem = 5;
      else if (!strcmp(var.value, "auto")) vic20mem = -1;

      /* Super VIC uses memory blocks 1+2 by default */
      if (!vic20mem && vice_opt.Model == VIC20MODEL_VIC21)
         vic20mem = 3;

      if (retro_ui_finalized && vice_opt.VIC20Memory != vic20mem)
      {
         unsigned int vic_blocks = 0;
         switch (vic20mem)
         {
            case 1:
               vic_blocks |= VIC_BLK0;
               break;
            case 2:
               vic_blocks |= VIC_BLK1;
               break;
            case 3:
               vic_blocks |= VIC_BLK1;
               vic_blocks |= VIC_BLK2;
               break;
            case 4:
               vic_blocks |= VIC_BLK1;
               vic_blocks |= VIC_BLK2;
               vic_blocks |= VIC_BLK3;
               break;
            case 5:
               vic_blocks = VIC_BLK_ALL;
               break;
            default:
               break;
         }

         log_resources_set_int("RAMBlock0", (vic_blocks & VIC_BLK0) ? 1 : 0);
         log_resources_set_int("RAMBlock1", (vic_blocks & VIC_BLK1) ? 1 : 0);
         log_resources_set_int("RAMBlock2", (vic_blocks & VIC_BLK2) ? 1 : 0);
         log_resources_set_int("RAMBlock3", (vic_blocks & VIC_BLK3) ? 1 : 0);
         log_resources_set_int("RAMBlock5", (vic_blocks & VIC_BLK5) ? 1 : 0);
         request_restart = true;
      }

      vice_opt.VIC20Memory = vic20mem;
   }
#elif defined(__XPLUS4__)
   GET_VAR("plus4_model")
   {
      int model = 0;

      if      (!strcmp(var.value, "C16 PAL"))    model = PLUS4MODEL_C16_PAL;
      else if (!strcmp(var.value, "C16 NTSC"))   model = PLUS4MODEL_C16_NTSC;
      else if (!strcmp(var.value, "PLUS4 PAL"))  model = PLUS4MODEL_PLUS4_PAL;
      else if (!strcmp(var.value, "PLUS4 NTSC")) model = PLUS4MODEL_PLUS4_NTSC;
      else if (!strcmp(var.value, "V364 NTSC"))  model = PLUS4MODEL_V364_NTSC;
      else if (!strcmp(var.value, "232 NTSC"))   model = PLUS4MODEL_232_NTSC;

      if (retro_ui_finalized && vice_opt.Model != model)
      {
         request_model_set = model;
         request_restart = true;
      }

      vice_opt.Model = model;
   }
#elif defined(__X128__)
   GET_VAR("c128_model")
   {
      int model = 0;
      bool opt_model_auto_prev = opt_model_auto;

      if (strstr(var.value, "auto")) opt_model_auto = true;
      else                           opt_model_auto = false;

      if      (!strcmp(var.value, "C128 PAL auto"))      model = C128MODEL_C128_PAL;
      else if (!strcmp(var.value, "C128 NTSC auto"))     model = C128MODEL_C128_NTSC;
      else if (!strcmp(var.value, "C128 D PAL auto"))    model = C128MODEL_C128D_PAL;
      else if (!strcmp(var.value, "C128 D NTSC auto"))   model = C128MODEL_C128D_NTSC;
      else if (!strcmp(var.value, "C128 DCR PAL auto"))  model = C128MODEL_C128DCR_PAL;
      else if (!strcmp(var.value, "C128 DCR NTSC auto")) model = C128MODEL_C128DCR_NTSC;
      else if (!strcmp(var.value, "C128 PAL"))           model = C128MODEL_C128_PAL;
      else if (!strcmp(var.value, "C128 NTSC"))          model = C128MODEL_C128_NTSC;
      else if (!strcmp(var.value, "C128 D PAL"))         model = C128MODEL_C128D_PAL;
      else if (!strcmp(var.value, "C128 D NTSC"))        model = C128MODEL_C128D_NTSC;
      else if (!strcmp(var.value, "C128 DCR PAL"))       model = C128MODEL_C128DCR_PAL;
      else if (!strcmp(var.value, "C128 DCR NTSC"))      model = C128MODEL_C128DCR_NTSC;

      if (retro_ui_finalized && (vice_opt.Model != model || opt_model_auto != opt_model_auto_prev))
      {
         request_model_set = model;
         request_restart = true;
      }

      vice_opt.Model = model;
   }

   GET_VAR("c128_ram_expansion_unit")
   {
      int reusize = 0;

      if (strcmp(var.value, "none"))
         reusize = atoi(var.value);

      if (retro_ui_finalized && vice_opt.REUsize != reusize && opt_reu_allow)
      {
         if (!reusize)
            log_resources_set_int("REU", 0);
         else
         {
            log_resources_set_int("REUsize", reusize);
            log_resources_set_int("REU", 1);
         }
         request_restart = true;
      }

      vice_opt.REUsize = reusize;

      opt_reu_allow = reu_allow(full_path);
   }

   GET_VAR("c128_video_output")
   {
      int c128columnkey = 1;

      if      (!strcmp(var.value, "VICII")) c128columnkey = 1;
      else if (!strcmp(var.value, "VDC"))   c128columnkey = 0;

      if (retro_ui_finalized && vice_opt.C128ColumnKey != c128columnkey)
         log_resources_set_int("C128ColumnKey", c128columnkey);

      vice_opt.C128ColumnKey = c128columnkey;
   }

   GET_VAR("c128_vdc_ram")
   {
      int vdc64kb = 0;

      if      (!strcmp(var.value, "default")) vdc64kb = 0;
      else if (!strcmp(var.value, "64"))      vdc64kb = 1;

      if (retro_ui_finalized && vice_opt.VDC64KB != vdc64kb)
         log_resources_set_int("VDC64KB", vdc64kb);

      vice_opt.VDC64KB = vdc64kb;
   }

   GET_VAR("c128_go64")
   {
      int c128go64 = 0;

      if (!strcmp(var.value, "disabled")) c128go64 = 0;
      else                                c128go64 = 1;

      /* Force VIC-II with GO64 */
      if (c128go64)
      {
         vice_opt.C128ColumnKey = 1;
         if (retro_ui_finalized)
            log_resources_set_int("C128ColumnKey", 1);
      }

      if (retro_ui_finalized && vice_opt.Go64Mode != c128go64)
      {
         log_resources_set_int("Go64Mode", c128go64);
         request_restart = true;
      }
      vice_opt.Go64Mode = c128go64;
   }
#elif defined(__XPET__)
   GET_VAR("pet_model")
   {
      int model = 0;

      if      (!strcmp(var.value, "2001"))     model = PETMODEL_2001;
      else if (!strcmp(var.value, "3008"))     model = PETMODEL_3008;
      else if (!strcmp(var.value, "3016"))     model = PETMODEL_3016;
      else if (!strcmp(var.value, "3032"))     model = PETMODEL_3032;
      else if (!strcmp(var.value, "3032B"))    model = PETMODEL_3032B;
      else if (!strcmp(var.value, "4016"))     model = PETMODEL_4016;
      else if (!strcmp(var.value, "4032"))     model = PETMODEL_4032;
      else if (!strcmp(var.value, "4032B"))    model = PETMODEL_4032B;
      else if (!strcmp(var.value, "8032"))     model = PETMODEL_8032;
      else if (!strcmp(var.value, "8096"))     model = PETMODEL_8096;
      else if (!strcmp(var.value, "8296"))     model = PETMODEL_8296;
      else if (!strcmp(var.value, "SUPERPET")) model = PETMODEL_SUPERPET;
      
      if (retro_ui_finalized && vice_opt.Model != model)
      {
         request_model_set = model;
         request_restart = true;
         /* Keyboard layout refresh required. All models below 8032 except B models use graphics layout, others use business. */
         keyboard_init();
      }
      vice_opt.Model = model;
   }
#elif defined(__XCBM2__)
   GET_VAR("cbm2_model")
   {
      int model = 0;

      if      (!strcmp(var.value, "610 PAL"))      model = CBM2MODEL_610_PAL;
      else if (!strcmp(var.value, "610 NTSC"))     model = CBM2MODEL_610_NTSC;
      else if (!strcmp(var.value, "620 PAL"))      model = CBM2MODEL_620_PAL;
      else if (!strcmp(var.value, "620 NTSC"))     model = CBM2MODEL_620_NTSC;
      else if (!strcmp(var.value, "620PLUS PAL"))  model = CBM2MODEL_620PLUS_PAL;
      else if (!strcmp(var.value, "620PLUS NTSC")) model = CBM2MODEL_620PLUS_NTSC;
      else if (!strcmp(var.value, "710 NTSC"))     model = CBM2MODEL_710_NTSC;
      else if (!strcmp(var.value, "720 NTSC"))     model = CBM2MODEL_720_NTSC;
      else if (!strcmp(var.value, "720PLUS NTSC")) model = CBM2MODEL_720PLUS_NTSC;

      if (retro_ui_finalized && vice_opt.Model != model)
      {
         request_model_set = model;
         request_restart = true;
      }

      vice_opt.Model = model;
   }
#elif defined(__XCBM5x0__)
   GET_VAR("cbm5x0_model")
   {
      int model = 0;

      if      (!strcmp(var.value, "510 PAL"))  model = CBM2MODEL_510_PAL;
      else if (!strcmp(var.value, "510 NTSC")) model = CBM2MODEL_510_NTSC;

      if (retro_ui_finalized && vice_opt.Model != model)
      {
         request_model_set = model;
         request_restart = true;
      }

      vice_opt.Model = model;
   }
#elif defined(__X64DTV__)
   GET_VAR("c64dtv_model")
   {
      int model = 0;

      if      (!strcmp(var.value, "DTV2 PAL"))    model = DTVMODEL_V2_PAL;
      else if (!strcmp(var.value, "DTV2 NTSC"))   model = DTVMODEL_V2_NTSC;
      else if (!strcmp(var.value, "DTV3 PAL"))    model = DTVMODEL_V3_PAL;
      else if (!strcmp(var.value, "DTV3 NTSC"))   model = DTVMODEL_V3_NTSC;
      else if (!strcmp(var.value, "HUMMER NTSC")) model = DTVMODEL_HUMMER_NTSC;

      if (retro_ui_finalized && vice_opt.Model != model)
      {
         request_model_set = model;
         request_restart = true;
      }

      vice_opt.Model = model;
   }
#else
   GET_VAR("c64_model")
   {
      int model = 0;
      bool opt_model_auto_prev = opt_model_auto;

      if (strstr(var.value, "auto")) opt_model_auto = true;
      else                           opt_model_auto = false;

      if      (!strcmp(var.value, "C64 PAL auto"))   model = C64MODEL_C64_PAL;
      else if (!strcmp(var.value, "C64 NTSC auto"))  model = C64MODEL_C64_NTSC;
      else if (!strcmp(var.value, "C64C PAL auto"))  model = C64MODEL_C64C_PAL;
      else if (!strcmp(var.value, "C64C NTSC auto")) model = C64MODEL_C64C_NTSC;
      else if (!strcmp(var.value, "C64 PAL"))        model = C64MODEL_C64_PAL;
      else if (!strcmp(var.value, "C64 NTSC"))       model = C64MODEL_C64_NTSC;
      else if (!strcmp(var.value, "C64C PAL"))       model = C64MODEL_C64C_PAL;
      else if (!strcmp(var.value, "C64C NTSC"))      model = C64MODEL_C64C_NTSC;
      else if (!strcmp(var.value, "C64SX PAL"))      model = C64MODEL_C64SX_PAL;
      else if (!strcmp(var.value, "C64SX NTSC"))     model = C64MODEL_C64SX_NTSC;
      else if (!strcmp(var.value, "PET64 PAL"))      model = C64MODEL_PET64_PAL;
      else if (!strcmp(var.value, "PET64 NTSC"))     model = C64MODEL_PET64_NTSC;
      else if (!strcmp(var.value, "C64 GS PAL"))     model = C64MODEL_C64_GS;
      else if (!strcmp(var.value, "C64 JAP NTSC"))   model = C64MODEL_C64_JAP;
      else if (!strcmp(var.value, "C64 PAL N"))      model = C64MODEL_C64_PAL_N;
      else if (!strcmp(var.value, "C64 OLD PAL"))    model = C64MODEL_C64_OLD_PAL;
      else if (!strcmp(var.value, "C64 OLD NTSC"))   model = C64MODEL_C64_OLD_NTSC;

      if (retro_ui_finalized && (vice_opt.Model != model || opt_model_auto != opt_model_auto_prev))
      {
         request_model_set = model;
         request_restart = true;
      }

      vice_opt.Model = model;
   }

#if defined(__XSCPU64__)
   GET_VAR("supercpu_simm_size")
   {
      int simmsize = atoi(var.value);

      if (retro_ui_finalized && vice_opt.SIMMSize != simmsize)
      {
         log_resources_set_int("SIMMSize", simmsize);
         request_restart = true;
      }

      vice_opt.SIMMSize = simmsize;
   }
#else
   GET_VAR("ram_expansion_unit")
   {
      int reusize = 0;

      if (strcmp(var.value, "none"))
         reusize = atoi(var.value);

      if (retro_ui_finalized && vice_opt.REUsize != reusize && opt_reu_allow)
      {
         if (!reusize)
            log_resources_set_int("REU", 0);
         else
         {
            log_resources_set_int("REUsize", reusize);
            log_resources_set_int("REU", 1);
         }
         request_restart = true;
      }

      vice_opt.REUsize = reusize;

      opt_reu_allow = reu_allow(full_path);
   }
#endif
#endif

#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__)
   GET_VAR("sid_engine")
   {
      int sid_engine = SID_ENGINE_FASTSID;

      if      (!strcmp(var.value, "ReSID"))     sid_engine = SID_ENGINE_RESID;
#ifdef HAVE_RESID33
      else if (!strcmp(var.value, "ReSID-3.3")) sid_engine = SID_ENGINE_RESID33;
#endif
      else if (!strcmp(var.value, "ReSID-FP"))  sid_engine = SID_ENGINE_RESIDFP;

      if (retro_ui_finalized && vice_opt.SidEngine != sid_engine)
         log_resources_set_int("SidEngine", sid_engine);

      vice_opt.SidEngine = sid_engine;
   }

#if !defined(__X64DTV__)
   GET_VAR("sid_model")
   {
      int sid_model = SID_MODEL_6581;
      switch (vice_opt.Model)
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
         case C64MODEL_C64C_PAL:
         case C64MODEL_C64C_NTSC:
         case C64MODEL_C64_GS:
#elif defined(__X128__)
         case C128MODEL_C128DCR_PAL:
         case C128MODEL_C128DCR_NTSC:
#else
         case -1:
#endif
            sid_model = SID_MODEL_8580;
            break;
      }

      if      (!strcmp(var.value, "6581"))   sid_model = SID_MODEL_6581;
      else if (!strcmp(var.value, "8580"))   sid_model = SID_MODEL_8580;
      /* There is no digiboost for FastSID (and it's not needed either) */
      else if (!strcmp(var.value, "8580RD")) sid_model = (!vice_opt.SidEngine ? SID_MODEL_8580 : SID_MODEL_8580D);

      if (retro_ui_finalized && vice_opt.SidModel != sid_model)
         log_resources_set_int("SidModel", sid_model);

      vice_opt.SidModel = sid_model;
   }

   GET_VAR("sid_extra")
   {
      int sid_extra = atoi(var.value);
      if (strcmp(var.value, "disabled"))
         sid_extra = strtol(var.value, NULL, 16);

      if (retro_ui_finalized && vice_opt.SidExtra != sid_extra)
      {
         if (!sid_extra)
            log_resources_set_int("SidStereo", 0);
         else
         {
            log_resources_set_int("Sid2AddressStart", sid_extra);
            if (!vice_opt.SidExtra)
               log_resources_set_int("SidStereo", 1);
         }
      }

      vice_opt.SidExtra = sid_extra;
   }
#endif

   GET_VAR("resid_sampling")
   {
      int val = 0;

      if      (!strcmp(var.value, "fast"))            val = SID_RESID_SAMPLING_FAST;
      else if (!strcmp(var.value, "interpolation"))   val = SID_RESID_SAMPLING_INTERPOLATION;
      else if (!strcmp(var.value, "resampling"))      val = SID_RESID_SAMPLING_RESAMPLING;
      else if (!strcmp(var.value, "fast resampling")) val = SID_RESID_SAMPLING_FAST_RESAMPLING;

      if (retro_ui_finalized && vice_opt.SidResidSampling != val)
         log_resources_set_int("SidResidSampling", val);

      vice_opt.SidResidSampling = val;
   }

   GET_VAR("resid_passband")
   {
      int val = atoi(var.value);

      if (retro_ui_finalized && vice_opt.SidResidPassband != val)
      {
         log_resources_set_int("SidResidPassband", val);
         log_resources_set_int("SidResid8580Passband", val);
      }

      vice_opt.SidResidPassband = val;
   }

   GET_VAR("resid_gain")
   {
      int val = atoi(var.value);

      if (retro_ui_finalized && vice_opt.SidResidGain != val)
      {
         log_resources_set_int("SidResidGain", val);
         log_resources_set_int("SidResid8580Gain", val);
      }

      vice_opt.SidResidGain = val;
   }

   GET_VAR("resid_filterbias")
   {
      int val = atoi(var.value);

      if (retro_ui_finalized && vice_opt.SidResidFilterBias != val)
         log_resources_set_int("SidResidFilterBias", val);

      vice_opt.SidResidFilterBias = val;
   }

#if !defined(__X64DTV__)
   GET_VAR("resid_8580filterbias")
   {
      int val = atoi(var.value);

      if (retro_ui_finalized && vice_opt.SidResid8580FilterBias != val)
         log_resources_set_int("SidResid8580FilterBias", val);

      vice_opt.SidResid8580FilterBias = val;
   }
#endif
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__)
   GET_VAR("sfx_sound_expander")
   {
      int sfx_chip = atoi(var.value);

      if (retro_ui_finalized && vice_opt.SFXSoundExpanderChip != sfx_chip)
      {
         if (!sfx_chip)
            log_resources_set_int("SFXSoundExpander", 0);
         else
         {
            log_resources_set_int("SFXSoundExpanderChip", sfx_chip);
            if (!vice_opt.SFXSoundExpanderChip)
               log_resources_set_int("SFXSoundExpander", 1);
         }
      }

      vice_opt.SFXSoundExpanderChip = sfx_chip;
   }
#endif

   GET_VAR("crop")
   {
      if      (!strcmp(var.value, "disabled"))     crop_id = CROP_NONE;
      else if (!strcmp(var.value, "small"))        crop_id = CROP_SMALL;
      else if (!strcmp(var.value, "medium"))       crop_id = CROP_MEDIUM;
      else if (!strcmp(var.value, "maximum"))      crop_id = CROP_MAXIMUM;
      else if (!strcmp(var.value, "manual"))       crop_id = CROP_MANUAL;
      else if (!strcmp(var.value, "auto"))         crop_id = CROP_AUTO;
      else if (!strcmp(var.value, "auto_disable")) crop_id = CROP_AUTO_DISABLE;

      opt_crop_id = crop_id;
   }

   GET_VAR("crop_mode")
   {
      int crop_mode_id_prev = crop_mode_id;

      if      (!strcmp(var.value, "both"))       crop_mode_id = CROP_MODE_BOTH;
      else if (!strcmp(var.value, "vertical"))   crop_mode_id = CROP_MODE_VERTICAL;
      else if (!strcmp(var.value, "horizontal")) crop_mode_id = CROP_MODE_HORIZONTAL;
      else if (!strcmp(var.value, "16:9"))       crop_mode_id = CROP_MODE_16_9;
      else if (!strcmp(var.value, "16:10"))      crop_mode_id = CROP_MODE_16_10;
      else if (!strcmp(var.value, "4:3"))        crop_mode_id = CROP_MODE_4_3;
      else if (!strcmp(var.value, "5:4"))        crop_mode_id = CROP_MODE_5_4;

      /* Crop reset */
      if (crop_mode_id != crop_mode_id_prev)
         crop_id_prev = -1;
   }

   GET_VAR("crop_delay")
   {
      if (!strcmp(var.value, "disabled")) crop_delay = false;
      else                                crop_delay = true;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
   GET_VAR("aspect_ratio")
   {
      int opt_aspect_ratio_prev = opt_aspect_ratio;

      if      (!strcmp(var.value, "auto")) opt_aspect_ratio = 0;
      else if (!strcmp(var.value, "pal"))  opt_aspect_ratio = 1;
      else if (!strcmp(var.value, "ntsc")) opt_aspect_ratio = 2;
      else if (!strcmp(var.value, "raw"))  opt_aspect_ratio = 3;

      /* Revert if aspect ratio is locked */
      if (opt_aspect_ratio_locked)
         opt_aspect_ratio = opt_aspect_ratio_prev;

      /* Crop reset */
      if (opt_aspect_ratio != opt_aspect_ratio_prev)
         crop_id_prev = -1;
   }
#endif

   GET_VAR("manual_crop_top")
   {
      int manual_crop_top_prev = manual_crop_top;
      manual_crop_top = atoi(var.value);
      if (manual_crop_top != manual_crop_top_prev)
         crop_id_prev = -1;
   }

   GET_VAR("manual_crop_bottom")
   {
      int manual_crop_bottom_prev = manual_crop_bottom;
      manual_crop_bottom = atoi(var.value);
      if (manual_crop_bottom != manual_crop_bottom_prev)
         crop_id_prev = -1;
   }

   GET_VAR("manual_crop_left")
   {
      int manual_crop_left_prev = manual_crop_left;
      manual_crop_left = atoi(var.value);
      if (manual_crop_left != manual_crop_left_prev)
         crop_id_prev = -1;
   }

   GET_VAR("manual_crop_right")
   {
      int manual_crop_right_prev = manual_crop_right;
      manual_crop_right = atoi(var.value);
      if (manual_crop_right != manual_crop_right_prev)
         crop_id_prev = -1;
   }

   GET_VAR("gfx_colors")
   {
      /* Only allow screenmode change after restart */
      if (!pix_bytes_initialized)
      {
         if (!strcmp(var.value, "16bit"))      pix_bytes = 2;
         else if (!strcmp(var.value, "24bit")) pix_bytes = 4;
      }
   }

#if defined(__X128__)
   GET_VAR("vdc_filter")
   {
      int filter = strcmp(var.value, "disabled");
      int blur = -1;

      if (filter)
      {
         if      (strstr(var.value, "noblur"))  blur = 0;
         else if (strstr(var.value, "lowblur")) blur = 50;
         else if (strstr(var.value, "medblur")) blur = 250;
         else                                   blur = 500;
      }

      if (retro_ui_finalized && vice_opt.VDCFilter != blur)
      {
         log_resources_set_int("VDCFilter", filter);
         log_resources_set_int("VDCPALBlur", blur);
      }

      vice_opt.VDCFilter = blur;
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   GET_VAR("vicii_filter")
#elif defined(__XVIC__)
   GET_VAR("vic_filter")
#elif defined(__XPLUS4__)
   GET_VAR("ted_filter")
#elif defined(__XPET__) || defined(__XCBM2__)
   GET_VAR("crtc_filter")
#endif
   {
      int filter = strcmp(var.value, "disabled");
      int blur = -1;

      if (filter)
      {
         if      (strstr(var.value, "noblur"))  blur = 0;
         else if (strstr(var.value, "lowblur")) blur = 50;
         else if (strstr(var.value, "medblur")) blur = 250;
         else                                   blur = 500;
      }

      if (retro_ui_finalized && vice_opt.Filter != blur)
      {
#if defined(__XPET__) || defined(__XCBM2__)
         log_resources_set_int("CrtcFilter", filter);
         log_resources_set_int("CrtcPALBlur", blur);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDFilter", filter);
         log_resources_set_int("TEDPALBlur", blur);
#elif defined(__XVIC__)
         log_resources_set_int("VICFilter", filter);
         log_resources_set_int("VICPALBlur", blur);
#else
         log_resources_set_int("VICIIFilter", filter);
         log_resources_set_int("VICIIPALBlur", blur);
#endif
      }

      vice_opt.Filter = blur;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   GET_VAR("vicii_filter_oddline_phase")
#elif defined(__XVIC__)
   GET_VAR("vic_filter_oddline_phase")
#elif defined(__XPLUS4__)
   GET_VAR("ted_filter_oddline_phase")
#endif
   {
      int oddline_phase = atoi(var.value);

      if (retro_ui_finalized && vice_opt.FilterOddLinePhase != oddline_phase)
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIPALOddLinePhase", oddline_phase);
#elif defined(__XVIC__)
         log_resources_set_int("VICPALOddLinePhase", oddline_phase);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDPALOddLinePhase", oddline_phase);
#endif

      vice_opt.FilterOddLinePhase = oddline_phase;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   GET_VAR("vicii_filter_oddline_offset")
#elif defined(__XVIC__)
   GET_VAR("vic_filter_oddline_offset")
#elif defined(__XPLUS4__)
   GET_VAR("ted_filter_oddline_offset")
#endif
   {
      int oddline_offset = atoi(var.value);

      if (retro_ui_finalized && vice_opt.FilterOddLineOffset != oddline_offset)
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIPALOddLineOffset", oddline_offset);
#elif defined(__XVIC__)
         log_resources_set_int("VICPALOddLineOffset", oddline_offset);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDPALOddLineOffset", oddline_offset);
#endif

      vice_opt.FilterOddLineOffset = oddline_offset;
   }

#if defined(__XVIC__)
   GET_VAR("vic20_external_palette")
   {
      if (retro_ui_finalized && strcmp(var.value, vice_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("VICExternalPalette", 0);
         else
         {
            log_resources_set_int("VICExternalPalette", 1);
            log_resources_set_string("VICPaletteFile", var.value);
         }
      }

      sprintf(vice_opt.ExternalPalette, "%s", var.value);
   }
#elif defined(__XPLUS4__)
   GET_VAR("plus4_external_palette")
   {
      if (retro_ui_finalized && strcmp(var.value, vice_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("TEDExternalPalette", 0);
         else
         {
            log_resources_set_int("TEDExternalPalette", 1);
            log_resources_set_string("TEDPaletteFile", var.value);
         }
      }

      sprintf(vice_opt.ExternalPalette, "%s", var.value);
   }
#elif defined(__XPET__)
   GET_VAR("pet_external_palette")
   {
      if (retro_ui_finalized && strcmp(var.value, vice_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("CrtcExternalPalette", 0);
         else
         {
            log_resources_set_int("CrtcExternalPalette", 1);
            log_resources_set_string("CrtcPaletteFile", var.value);
         }
      }

      sprintf(vice_opt.ExternalPalette, "%s", var.value);
   }
#elif defined(__XCBM2__)
   GET_VAR("cbm2_external_palette")
   {
      if (retro_ui_finalized && strcmp(var.value, vice_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("CrtcExternalPalette", 0);
         else
         {
            log_resources_set_int("CrtcExternalPalette", 1);
            log_resources_set_string("CrtcPaletteFile", var.value);
         }
      }

      sprintf(vice_opt.ExternalPalette, "%s", var.value);
   }
#elif !defined(__X64DTV__)
   GET_VAR("external_palette")
   {
      if (retro_ui_finalized && strcmp(var.value, vice_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("VICIIExternalPalette", 0);
         else
         {
            log_resources_set_int("VICIIExternalPalette", 1);
            log_resources_set_string("VICIIPaletteFile", var.value);
         }
      }

      sprintf(vice_opt.ExternalPalette, "%s", var.value);
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   GET_VAR("vicii_color_gamma")
#elif defined(__XVIC__)
   GET_VAR("vic_color_gamma")
#elif defined(__XPLUS4__)
   GET_VAR("ted_color_gamma")
#endif
   {
      int color_gamma = atoi(var.value);

      /* Correct to new default gamma in 3.8 */
      color_gamma /= 2.8f;

      if (retro_ui_finalized && vice_opt.ColorGamma != color_gamma)
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIColorGamma", color_gamma);
#elif defined(__XVIC__)
         log_resources_set_int("VICColorGamma", color_gamma);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDColorGamma", color_gamma);
#endif

      vice_opt.ColorGamma = color_gamma;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   GET_VAR("vicii_color_tint")
#elif defined(__XVIC__)
   GET_VAR("vic_color_tint")
#elif defined(__XPLUS4__)
   GET_VAR("ted_color_tint")
#endif
   {
      int color_tint = atoi(var.value);

      if (retro_ui_finalized && vice_opt.ColorTint != color_tint)
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIColorTint", color_tint);
#elif defined(__XVIC__)
         log_resources_set_int("VICColorTint", color_tint);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDColorTint", color_tint);
#endif

      vice_opt.ColorTint = color_tint;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   GET_VAR("vicii_color_saturation")
#elif defined(__XVIC__)
   GET_VAR("vic_color_saturation")
#elif defined(__XPLUS4__)
   GET_VAR("ted_color_saturation")
#endif
   {
      int color_saturation = atoi(var.value);

      if (retro_ui_finalized && vice_opt.ColorSaturation != color_saturation)
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIColorSaturation", color_saturation);
#elif defined(__XVIC__)
         log_resources_set_int("VICColorSaturation", color_saturation);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDColorSaturation", color_saturation);
#endif

      vice_opt.ColorSaturation = color_saturation;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   GET_VAR("vicii_color_contrast")
#elif defined(__XVIC__)
   GET_VAR("vic_color_contrast")
#elif defined(__XPLUS4__)
   GET_VAR("ted_color_contrast")
#endif
   {
      int color_contrast = atoi(var.value);

      if (retro_ui_finalized && vice_opt.ColorContrast != color_contrast)
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIColorContrast", color_contrast);
#elif defined(__XVIC__)
         log_resources_set_int("VICColorContrast", color_contrast);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDColorContrast", color_contrast);
#endif

      vice_opt.ColorContrast = color_contrast;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   GET_VAR("vicii_color_brightness")
#elif defined(__XVIC__)
   GET_VAR("vic_color_brightness")
#elif defined(__XPLUS4__)
   GET_VAR("ted_color_brightness")
#endif
   {
      int color_brightness = atoi(var.value);

      if (retro_ui_finalized && vice_opt.ColorBrightness != color_brightness)
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIColorBrightness", color_brightness);
#elif defined(__XVIC__)
         log_resources_set_int("VICColorBrightness", color_brightness);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDColorBrightness", color_brightness);
#endif

      vice_opt.ColorBrightness = color_brightness;
   }
#endif

#if !defined(__XCBM5x0__)
   GET_VAR("userport_joytype")
   {
      int userportjoytype = -1;

      if      (!strcmp(var.value, "disabled")) userportjoytype = -1;
      else if (!strcmp(var.value, "CGA"))      userportjoytype = USERPORT_JOYSTICK_CGA;
      else if (!strcmp(var.value, "PET"))      userportjoytype = USERPORT_JOYSTICK_PET;
      else if (!strcmp(var.value, "Hummer"))   userportjoytype = USERPORT_JOYSTICK_HUMMER;
      else if (!strcmp(var.value, "OEM"))      userportjoytype = USERPORT_JOYSTICK_OEM;
      else if (!strcmp(var.value, "HIT"))      userportjoytype = USERPORT_JOYSTICK_HIT;
      else if (!strcmp(var.value, "Kingsoft")) userportjoytype = USERPORT_JOYSTICK_KINGSOFT;
      else if (!strcmp(var.value, "Starbyte")) userportjoytype = USERPORT_JOYSTICK_STARBYTE;
      else if (!strcmp(var.value, "Synergy"))  userportjoytype = USERPORT_JOYSTICK_SYNERGY;

      if (retro_ui_finalized && vice_opt.UserportJoyType != userportjoytype)
      {
         if (userportjoytype == -1)
            log_resources_set_int("UserportDevice", 0);
         else
            /* Keep backwards compatibility */
            log_resources_set_int("UserportDevice", userportjoytype + USERPORT_DEVICE_JOYSTICK_CGA);
      }

      vice_opt.UserportJoyType = userportjoytype;
   }
#else
   vice_opt.UserportJoyType = -1;
#endif

#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XVIC__)
   GET_VAR("joyport")
   {
      if      (!strcmp(var.value, "1") && !cur_port_locked) cur_port = 1;
      else if (!strcmp(var.value, "2") && !cur_port_locked) cur_port = 2;
   }
#endif

#if !defined(__XPET__) && !defined(__XCBM2__)
   GET_VAR("joyport_type")
   {
      opt_joyport_type = atoi(var.value);

      opt_joyport_paddles_split = true;
      if (!strcmp(var.value, "2R"))
         opt_joyport_paddles_split = false;

      /* Light guns/pens only possible in port 1 */
      if (opt_joyport_type > 10)
         cur_port = 1;
   }

   GET_VAR("joyport_pointer_color")
   {
      if      (!strcmp(var.value, "disabled")) opt_joyport_pointer_color = -1;
      else if (!strcmp(var.value, "black"))    opt_joyport_pointer_color = 0;
      else if (!strcmp(var.value, "white"))    opt_joyport_pointer_color = 1;
      else if (!strcmp(var.value, "red"))      opt_joyport_pointer_color = 2;
      else if (!strcmp(var.value, "green"))    opt_joyport_pointer_color = 3;
      else if (!strcmp(var.value, "blue"))     opt_joyport_pointer_color = 4;
      else if (!strcmp(var.value, "yellow"))   opt_joyport_pointer_color = 5;
      else if (!strcmp(var.value, "cyan"))     opt_joyport_pointer_color = 6;
      else if (!strcmp(var.value, "purple"))   opt_joyport_pointer_color = 7;
   }

   GET_VAR("analogmouse")
   {
      if      (!strcmp(var.value, "disabled")) opt_analogmouse = 0;
      else if (!strcmp(var.value, "left"))     opt_analogmouse = 1;
      else if (!strcmp(var.value, "right"))    opt_analogmouse = 2;
      else if (!strcmp(var.value, "both"))     opt_analogmouse = 3;
   }

   GET_VAR("analogmouse_deadzone")
   {
      opt_analogmouse_deadzone = atoi(var.value);
   }

   GET_VAR("analogmouse_speed")
   {
      opt_analogmouse_speed_left = atof(var.value);
   }

   GET_VAR("analogmouse_speed_right")
   {
      opt_analogmouse_speed_right = atof(var.value);
   }

   GET_VAR("dpadmouse_speed")
   {
      opt_dpadmouse_speed = atoi(var.value);
   }

   GET_VAR("mouse_speed")
   {
      opt_mouse_speed = atoi(var.value);
   }

   GET_VAR("retropad_options")
   {
      if      (!strcmp(var.value, "disabled"))    opt_retropad_options = RETROPAD_OPTIONS_DISABLED;
      else if (!strcmp(var.value, "rotate"))      opt_retropad_options = RETROPAD_OPTIONS_ROTATE;
      else if (!strcmp(var.value, "jump"))        opt_retropad_options = RETROPAD_OPTIONS_JUMP;
      else if (!strcmp(var.value, "rotate_jump")) opt_retropad_options = RETROPAD_OPTIONS_ROTATE_JUMP;
   }

   GET_VAR("keyrah_keypad_mappings")
   {
      if (!strcmp(var.value, "disabled")) opt_keyrah_keypad = false;
      else                                opt_keyrah_keypad = true;
   }

   GET_VAR("turbo_fire")
   {
      if (!turbo_fire_locked)
      {
         if (!strcmp(var.value, "disabled")) retro_turbo_fire = false;
         else                                retro_turbo_fire = true;
      }
   }

   GET_VAR("turbo_fire_button")
   {
      if      (!strcmp(var.value, "B"))  turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_B;
      else if (!strcmp(var.value, "A"))  turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_A;
      else if (!strcmp(var.value, "Y"))  turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_Y;
      else if (!strcmp(var.value, "X"))  turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_X;
      else if (!strcmp(var.value, "L"))  turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_L;
      else if (!strcmp(var.value, "R"))  turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_R;
      else if (!strcmp(var.value, "L2")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_L2;
      else if (!strcmp(var.value, "R2")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_R2;
   }

   GET_VAR("turbo_pulse")
   {
      turbo_pulse = atoi(var.value);
   }
#endif

#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XCBM5x0__)
   GET_VAR("keyboard_keymap")
   {
      int val = opt_keyboard_keymap;

      if      (!strcmp(var.value, "symbolic"))        opt_keyboard_keymap = KBD_INDEX_SYM;
      else if (!strcmp(var.value, "positional"))      opt_keyboard_keymap = KBD_INDEX_POS;
      else if (!strcmp(var.value, "symbolic-user"))   opt_keyboard_keymap = KBD_INDEX_USERSYM;
      else if (!strcmp(var.value, "positional-user")) opt_keyboard_keymap = KBD_INDEX_USERPOS;

      if (retro_ui_finalized && opt_keyboard_keymap != val)
         keyboard_init();
   }
#endif

   GET_VAR("physical_keyboard_pass_through")
   {
      if (!strcmp(var.value, "disabled")) opt_keyboard_pass_through = false;
      else                                opt_keyboard_pass_through = true;
   }

#if !defined(__X64DTV__)
   GET_VAR("reset")
   {
      if      (!strcmp(var.value, "autostart")) opt_reset_type = 0;
      else if (!strcmp(var.value, "soft"))      opt_reset_type = 1;
      else if (!strcmp(var.value, "hard"))      opt_reset_type = 2;
      else if (!strcmp(var.value, "freeze"))    opt_reset_type = 3;
   }
#endif

   GET_VAR("vkbd_theme")
   {
      if      (strstr(var.value, "auto"))    opt_vkbd_theme = 0;
      else if (strstr(var.value, "brown"))   opt_vkbd_theme = 1;
      else if (strstr(var.value, "beige"))   opt_vkbd_theme = 2;
      else if (strstr(var.value, "dark"))    opt_vkbd_theme = 3;
      else if (strstr(var.value, "light"))   opt_vkbd_theme = 4;

      if      (strstr(var.value, "outline")) opt_vkbd_theme |= 0x80;
   }

   GET_VAR("vkbd_transparency")
   {
      if      (!strcmp(var.value, "0%"))   opt_vkbd_alpha = GRAPH_ALPHA_100;
      else if (!strcmp(var.value, "25%"))  opt_vkbd_alpha = GRAPH_ALPHA_75;
      else if (!strcmp(var.value, "50%"))  opt_vkbd_alpha = GRAPH_ALPHA_50;
      else if (!strcmp(var.value, "75%"))  opt_vkbd_alpha = GRAPH_ALPHA_25;
      else if (!strcmp(var.value, "100%")) opt_vkbd_alpha = GRAPH_ALPHA_0;
   }

   GET_VAR("vkbd_dimming")
   {
      if      (!strcmp(var.value, "0%"))   opt_vkbd_dim_alpha = GRAPH_ALPHA_0;
      else if (!strcmp(var.value, "25%"))  opt_vkbd_dim_alpha = GRAPH_ALPHA_25;
      else if (!strcmp(var.value, "50%"))  opt_vkbd_dim_alpha = GRAPH_ALPHA_50;
      else if (!strcmp(var.value, "75%"))  opt_vkbd_dim_alpha = GRAPH_ALPHA_75;
      else if (!strcmp(var.value, "100%")) opt_vkbd_dim_alpha = GRAPH_ALPHA_100;
   }

   GET_VAR("statusbar")
   {
      opt_statusbar = 0;

      if (strstr(var.value, "top"))     opt_statusbar |= STATUSBAR_TOP;
      else                              opt_statusbar |= STATUSBAR_BOTTOM;

      if (strstr(var.value, "basic"))   opt_statusbar |= STATUSBAR_BASIC;
      if (strstr(var.value, "minimal")) opt_statusbar |= STATUSBAR_MINIMAL;
   }

   GET_VAR("statusbar_startup")
   {
      if (!retro_ui_finalized)
      {
         if (!strcmp(var.value, "enabled"))
            retro_statusbar = true;
         else
            retro_statusbar = false;
      }
   }

   GET_VAR("statusbar_messages")
   {
      if (!strcmp(var.value, "enabled"))
         opt_statusbar |= STATUSBAR_MESSAGES;
   }

   GET_VAR("mapping_options_display")
   {
      if (!strcmp(var.value, "disabled")) opt_mapping_options_display = 0;
      else                                opt_mapping_options_display = 1;
   }

   GET_VAR("audio_options_display")
   {
      if (!strcmp(var.value, "disabled")) opt_audio_options_display = 0;
      else                                opt_audio_options_display = 1;
   }

   GET_VAR("video_options_display")
   {
      if (!strcmp(var.value, "disabled")) opt_video_options_display = 0;
      else                                opt_video_options_display = 1;
   }

   GET_VAR("read_vicerc")
   {
      int opt_read_vicerc_prev = opt_read_vicerc;
      if (!strcmp(var.value, "disabled")) opt_read_vicerc = 0;
      else                                opt_read_vicerc = 1;

      if (retro_ui_finalized)
         request_reload_restart = (opt_read_vicerc != opt_read_vicerc_prev) ? true : request_reload_restart;
   }

#if defined(__XSCPU64__)
   GET_VAR("supercpu_speed_switch")
   {
      int speedswitch = 0;
      if (!strcmp(var.value, "enabled")) speedswitch = 1;

      if (retro_ui_finalized && vice_opt.SpeedSwitch != speedswitch)
         log_resources_set_int("SpeedSwitch", speedswitch);

      vice_opt.SpeedSwitch = speedswitch;
   }

   GET_VAR("supercpu_kernal")
   {
      int opt_supercpu_kernal_prev = opt_supercpu_kernal;
      opt_supercpu_kernal = atoi(var.value);

      if (retro_ui_finalized)
         request_reload_restart = (opt_supercpu_kernal != opt_supercpu_kernal_prev) ? true : request_reload_restart;
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
   GET_VAR("jiffydos")
   {
      int opt_jiffydos_prev = opt_jiffydos;
      if (!strcmp(var.value, "disabled")) opt_jiffydos = false;
      else                                opt_jiffydos = true;

      /* Forcefully disable JiffyDOS if TDE is disabled */
      if (!vice_opt.DriveTrueEmulation)
         opt_jiffydos_allow = false;

      if (!opt_jiffydos_allow)
         opt_jiffydos = false;

      if (retro_ui_finalized)
         request_reload_restart = (opt_jiffydos != opt_jiffydos_prev || request_restart) ? true : request_reload_restart;
   }
#endif

#if !defined(__X64DTV__)
   GET_VAR("printer")
   {
      if (!strcmp(var.value, "disabled")) vice_opt.Printer = 0;
      else                                vice_opt.Printer = 1;
   }
#endif

   /* Mapper */
   /* RetroPad */
   GET_VAR("mapper_up")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_UP] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_down")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_DOWN] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_left")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_LEFT] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_right")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_RIGHT] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_select")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_SELECT] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_start")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_START] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_b")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_B] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_a")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_A] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_y")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_Y] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_x")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_X] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_l")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_L] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_r")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_R] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_l2")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_L2] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_r2")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_R2] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_l3")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_L3] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_r3")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_R3] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_lr")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_LR] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_ll")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_LL] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_ld")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_LD] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_lu")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_LU] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_rr")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_RR] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_rl")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_RL] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_rd")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_RD] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_ru")
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_RU] = retro_keymap_id(var.value);
   }

   /* Hotkeys */
   GET_VAR("mapper_vkbd")
   {
      mapper_keys[RETRO_MAPPER_VKBD] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_statusbar")
   {
      mapper_keys[RETRO_MAPPER_STATUSBAR] = retro_keymap_id(var.value);
   }

#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XVIC__)
   GET_VAR("mapper_joyport_switch")
   {
      mapper_keys[RETRO_MAPPER_JOYPORT] = retro_keymap_id(var.value);
   }
#endif

   GET_VAR("mapper_reset")
   {
      mapper_keys[RETRO_MAPPER_RESET] = retro_keymap_id(var.value);
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
   GET_VAR("mapper_aspect_ratio_toggle")
   {
      mapper_keys[RETRO_MAPPER_ASPECT_RATIO] = retro_keymap_id(var.value);
   }
#endif

   GET_VAR("mapper_crop_toggle")
   {
      mapper_keys[RETRO_MAPPER_CROP] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_warp_mode")
   {
      mapper_keys[RETRO_MAPPER_WARP_MODE] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_turbo_fire_toggle")
   {
      mapper_keys[RETRO_MAPPER_TURBO_FIRE] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_save_disk_toggle")
   {
      mapper_keys[RETRO_MAPPER_SAVE_DISK] = retro_keymap_id(var.value);
   }

#if !defined(__XSCPU64__) && !defined(__X64DTV__)
   GET_VAR("datasette_hotkeys")
   {
      if (!strcmp(var.value, "disabled")) datasette_hotkeys = false;
      else                                datasette_hotkeys = true;
   }

   GET_VAR("mapper_datasette_toggle_hotkeys")
   {
      mapper_keys[RETRO_MAPPER_DATASETTE_HOTKEYS] = retro_keymap_id(var.value);
   }
   
   GET_VAR("mapper_datasette_stop")
   {
      mapper_keys[RETRO_MAPPER_DATASETTE_STOP] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_datasette_start")
   {
      mapper_keys[RETRO_MAPPER_DATASETTE_START] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_datasette_forward")
   {
      mapper_keys[RETRO_MAPPER_DATASETTE_FORWARD] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_datasette_rewind")
   {
      mapper_keys[RETRO_MAPPER_DATASETTE_REWIND] = retro_keymap_id(var.value);
   }

   GET_VAR("mapper_datasette_reset")
   {
      mapper_keys[RETRO_MAPPER_DATASETTE_RESET] = retro_keymap_id(var.value);
   }
#endif

   /* Hide/show core options */
   retro_set_options_display();
}

void emu_reset(int type)
{
   /* Changing opt_read_vicerc requires reloading */
   if (request_reload_restart)
      reload_restart();

   /* Disable warp */
   if (vsync_get_warp_mode())
      vsync_set_warp_mode(0);

   /* Reset Datasette or autostart from tape will fail */
   datasette_control(TAPEPORT_PORT_1, DATASETTE_CONTROL_RESET);

   /* Release keyboard keys and joystick switches */
   keyboard_clear_keymatrix();
   joystick_clear_all();

   /* Reset autoloadwarp audio ignore */
   audio_is_ignored = false;

   /* Reset tick */
   retro_now = 0;

   /* Reset autosys */
   autosys = opt_autostart;

   /* Follow core option type with -1 */
   type = (type == -1) ? opt_reset_type : type;
   switch (type)
   {
      case 0:
         /* Hard reset before autostart */
         machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);

         /* Check command line on autostart */
         if (dc->command || CMDFILE[0])
         {
            initcmdline_check_attach();
            break;
         }

         /* Build direct launch PRG */
         if (dc->load[dc->index])
         {
            free(autostartString);
            autostartString  = x_strdup(dc->files[dc->index]);
            path_remove_program(autostartString);
            autostartProgram = x_strdup(dc->load[dc->index]);
            charset_petconvstring((uint8_t *)autostartProgram, 0);
         }
         else
            autostartProgram = NULL;

         /* Allow autostarting with a different disk */
         if (dc->count > 1)
         {
            /* Only if first image has the same image type */
            if (dc_get_image_type(dc->files[0]) == dc_get_image_type(dc->files[dc->index]))
            {
               free(autostartString);
               autostartString = x_strdup(dc->files[dc->index]);
            }
         }
         if (autostartString != NULL && autostartString[0] != '\0' && !noautostart)
            autostart_autodetect(autostartString, autostartProgram, 0, AUTOSTART_MODE_RUN);

         /* Scan for save disk index */
         signed char save_disk_index = -1;
         for (unsigned index = 0; index < dc->count; index++)
            if (strstr(dc->labels[index], M3U_SAVEDISK_LABEL))
               save_disk_index = index;

         /* Scan for save disk 0, insert it silently if exists and not booting from floppy */
         if (save_disk_index > -1 && dc_get_image_type(dc->files[0]) != DC_IMAGE_TYPE_FLOPPY)
         {
            int index_cur = dc->index;

            dc->unit  = 8;
            dc->index = save_disk_index;

            /* Cycle disk tray */
            retro_disk_set_eject_state(true);
            retro_disk_set_eject_state(false);

            dc->index = index_cur;
         }
         break;
      case 1:
         machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);

         /* Allow restarting PRGs with RUN */
         if (autostartString != NULL && autostartString[0] != '\0' && strendswith(autostartString, "prg"))
            autostart_autodetect(autostartString, autostartProgram, 0, AUTOSTART_MODE_NONE);
         break;
      case 2:
         machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
         break;
      case 3:
         cartridge_trigger_freeze();
         break;
   }
}

/*****************************************************************************/
/* Disk Control */
bool retro_disk_set_eject_state(bool ejected)
{
   if (dc)
   {
      int unit = retro_disk_get_image_unit();

      if (dc->eject_state == ejected)
         return true;
      else
         dc->eject_state = ejected;

      if (!dc->files[dc->index])
         return false;

      if (path_is_valid(dc->files[dc->index]))
          display_current_image(((!dc->eject_state) ? dc->labels[dc->index] : ""), !dc->eject_state);

      if (dc->eject_state)
      {
         switch (unit)
         {
            case 0:
               cartridge_unset_default();
               cartridge_detach_image(-1);
               sound_volume_counter_reset();
               break;
            case 1:
               tape_image_detach(unit);
               break;
            default:
               file_system_detach_disk(unit, 0);
               break;
         }
      }
      else if (path_is_valid(dc->files[dc->index]))
      {
         switch (unit)
         {
            case 0:
#if defined(__XVIC__)
               cartridge_attach_image(vic20_autodetect_cartridge_type(dc->files[dc->index]), dc->files[dc->index]);
#elif defined(__XPLUS4__)
               cartridge_attach_image(CARTRIDGE_PLUS4_DETECT, dc->files[dc->index]);
#else
               cartridge_attach_image(unit, dc->files[dc->index]);
#endif
               /* PRGs must autostart on attach, cartridges reset anyway */
               if (strendswith(dc->files[dc->index], "prg"))
                  emu_reset(0);
               sound_volume_counter_reset();
               break;
            case 1:
               tape_image_attach(unit, dc->files[dc->index]);
               datasette_control(TAPEPORT_PORT_1, DATASETTE_CONTROL_START);
               break;
            default:
               sound_drive_mute = false;
               file_system_attach_disk(unit, 0, dc->files[dc->index]);
               autodetect_drivetype(unit);
               break;
         }
      }
   }

   return true;
}

bool retro_disk_get_eject_state(void)
{
   if (dc)
      return dc->eject_state;

   return true;
}

static unsigned retro_disk_get_image_index(void)
{
   if (dc)
      return dc->index;

   return 0;
}

bool retro_disk_set_image_index(unsigned index)
{
   if (dc)
   {
      if (index == dc->index)
         return true;

      if (dc->replace)
      {
         dc->replace = false;
         index = 0;
      }

      if (index < dc->count && dc->files[index])
      {
         dc->index = index;
         display_current_image(dc->labels[dc->index], false);
         return true;
      }
   }

   return false;
}

static unsigned retro_disk_get_num_images(void)
{
   if (dc)
      return dc->count;

   return 0;
}

static bool retro_disk_replace_image_index(unsigned index, const struct retro_game_info *info)
{
   if (dc)
   {
      if (info != NULL)
         dc_replace_file(dc, index, info->path);
      else
         dc_remove_file(dc, index);
      return true;
   }

   return false;
}

static bool retro_disk_add_image_index(void)
{
   if (dc)
   {
      if (dc->count <= DC_MAX_SIZE)
      {
         dc->files[dc->count]       = NULL;
         dc->labels[dc->count]      = NULL;
         dc->disk_labels[dc->count] = NULL;
         dc->load[dc->count]        = NULL;
         dc->types[dc->count]       = DC_IMAGE_TYPE_NONE;
         dc->count++;
         return true;
      }
   }

   return false;
}

static bool retro_disk_get_image_path(unsigned index, char *path, size_t len)
{
   if (len < 1)
      return false;

   if (dc)
   {
      if (index < dc->count)
      {
         if (!string_is_empty(dc->files[index]))
         {
            strlcpy(path, dc->files[index], len);
            return true;
         }
      }
   }

   return false;
}

static bool retro_disk_get_image_label(unsigned index, char *label, size_t len)
{
   if (len < 1)
      return false;

   if (dc)
   {
      if (index < dc->count)
      {
         if (!string_is_empty(dc->labels[index]))
         {
            strlcpy(label, dc->labels[index], len);
            return true;
         }
      }
   }

   return false;
}

static struct retro_disk_control_callback disk_interface = {
   retro_disk_set_eject_state,
   retro_disk_get_eject_state,
   retro_disk_get_image_index,
   retro_disk_set_image_index,
   retro_disk_get_num_images,
   retro_disk_replace_image_index,
   retro_disk_add_image_index,
};

static struct retro_disk_control_ext_callback disk_interface_ext = {
   retro_disk_set_eject_state,
   retro_disk_get_eject_state,
   retro_disk_get_image_index,
   retro_disk_set_image_index,
   retro_disk_get_num_images,
   retro_disk_replace_image_index,
   retro_disk_add_image_index,
   NULL, /* set_initial_image */
   retro_disk_get_image_path,
   retro_disk_get_image_label,
};

/*****************************************************************************/

void retro_reset(void)
{
   /* Reset DC index to first entry */
   if (dc)
   {
      dc->index = 0;
      retro_disk_set_eject_state(true);
      retro_disk_set_eject_state(false);
   }

   /* Trigger autostart-reset in retro_run() */
   request_restart = true;
}

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void retro_init(void)
{
   struct retro_log_callback log;
   log_cb = fallback_log;
   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;

   if (!environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
      perf_cb.get_time_usec = NULL;

   /* Clean ZIP temp */
   if (!string_is_empty(retro_temp_directory) && path_is_directory(retro_temp_directory))
      remove_recurse(retro_temp_directory);

   /* Use system directory for data files such as JiffyDOS and keymaps */
   if (     !string_is_empty(retro_system_data_directory)
         && retro_system_data_directory[0] != '.'
         && !path_is_directory(retro_system_data_directory))
   {
      log_cb(RETRO_LOG_INFO, "Mkdir system data directory: \"%s\"\n", retro_system_data_directory);
      archdep_mkdir(retro_system_data_directory, 0);
   }

   /* Disk Control interface */
   dc = dc_create();
   unsigned dci_version = 0;
   if (environ_cb(RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION, &dci_version) && (dci_version >= 1))
      environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE, &disk_interface_ext);
   else
      environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, &disk_interface);

   /* Keyboard callback */
   static struct retro_keyboard_callback keyboard_callback = {retro_keyboard_event};
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &keyboard_callback);

   /* Core option display callback */
   struct retro_core_options_update_display_callback update_display_callback = {retro_update_display};
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &update_display_callback);

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   if (environ_cb(RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE, NULL))
      libretro_supports_ff_override = true;

   bool achievements = true;
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);

   memset(retro_bmp, 0, sizeof(retro_bmp));
   init_output_audio_buffer(2048);

   retro_ui_finalized = false;
   update_variables();
}

void retro_deinit(void)
{
#if 0
   /* VICE shutdown
    * FIXME: Doing this will break static build reloads */
   machine_shutdown();
#endif

   /* Clean ZIP temp */
   if (!string_is_empty(retro_temp_directory) && path_is_directory(retro_temp_directory))
      remove_recurse(retro_temp_directory);

   /* Clean Disc Control context */
   if (dc)
      dc_free(dc);

   /* Clean dynamic cartridge info */
   free_vice_carts();

   /* Free buffers used by libretro-graph */
   libretro_graph_free();

   /* Free audio buffer */
   free_output_audio_buffer();

   /* 'Reset' troublesome static variables */
   pix_bytes_initialized = false;
   libretro_supports_bitmasks = false;
   libretro_supports_ff_override = false;
   libretro_supports_option_categories = false;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   if (port < RETRO_DEVICES)
      retro_devices[port] = device;
}

void retro_get_system_info(struct retro_system_info *info)
{
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   memset(info, 0, sizeof(*info));
   info->library_name     = "VICE " CORE_NAME;
   info->library_version  = PACKAGE_VERSION "" GIT_VERSION;
#if defined(__XVIC__)
   info->valid_extensions = "d64|d71|d80|d81|d82|g64|g41|x64|t64|tap|prg|p00|crt|bin|zip|7z|gz|d6z|d7z|d8z|g6z|g4z|x6z|cmd|m3u|vfl|vsf|nib|nbz|d2m|d4m|20|40|60|a0|b0|rom";
#else
   info->valid_extensions = "d64|d71|d80|d81|d82|g64|g41|x64|t64|tap|prg|p00|crt|bin|zip|7z|gz|d6z|d7z|d8z|g6z|g4z|x6z|cmd|m3u|vfl|vsf|nib|nbz|d2m|d4m|tcrt";
#endif
   info->need_fullpath    = true;
   info->block_extract    = true;
}

float retro_get_aspect_ratio(unsigned int width, unsigned int height, bool pixel_aspect)
{
   float ar;
   float par;
   int region = 0;
   region = retro_region;
   switch (opt_aspect_ratio)
   {
      case 1:
         region = RETRO_REGION_PAL;
         break;
      case 2:
         region = RETRO_REGION_NTSC;
         break;
      case 3:
         region = -1;
         par = 1;
         break;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   switch (region)
   {
      case RETRO_REGION_NTSC:
         par = 0.75000000f;
         break;
      case RETRO_REGION_PAL:
         par = 0.93650794f;
         break;
   }
#if defined(__X128__)
   if (c128_vdc)
   {
      switch (region)
      {
         case RETRO_REGION_NTSC:
            par = 0.76704375f / 2.0f;
            break;
         case RETRO_REGION_PAL:
            par = 0.92187500f / 2.0f;
            break;
      }
   }
#endif
   ar = ((float)width / (float)height) * par;
#elif defined(__XVIC__)
   switch (region)
   {
      case RETRO_REGION_NTSC:
         par = 1.50411479f / 2.0f;
         break;
      case RETRO_REGION_PAL:
         par = 1.66574035f / 2.0f;
         break;
   }
   ar = ((float)width / (float)height) * par;
#elif defined(__XPLUS4__)
   switch (region)
   {
      case RETRO_REGION_NTSC:
         par = 0.85760931f;
         break;
      case RETRO_REGION_PAL:
         par = 1.03743478f;
         break;
   }
   ar = ((float)width / (float)height) * par;
#elif defined(__XPET__) || defined(__XCBM2__)
   if (retrow > 384)
      par = 1.0f / 2.0f;
   else
      par = 1.0f;
   ar = ((float)width / (float)height) * par;
#else
   ar = (float)4 / (float)3;
#endif

   if (pixel_aspect)
      return par;
   if (opt_aspect_ratio == 3) /* 1:1 */
      return ((float)width / (float)height);
   return ar;
}

void update_geometry(int mode)
{
   struct retro_system_av_info system_av_info;
   bool update_av_info  = false;

   defaultw = retrow;
   defaulth = retroh;

#if defined(__X128__)
   /* Refresh VDC output state */
   is_vdc();
#endif

   switch (mode)
   {
      case 0:
         /* Crop mode init */
         crop_id_prev        = -1;
         retrow_crop         = retrow;
         retroh_crop         = retroh;
         retroXS_crop_offset = 0;
         retroYS_crop_offset = 0;
         retroXS_offset      = 0;
         retroYS_offset      = 0;
         retro_bmp_offset    = 0;

         system_av_info.geometry.base_width   = retrow;
         system_av_info.geometry.base_height  = retroh;
         system_av_info.geometry.aspect_ratio = retro_get_aspect_ratio(retrow, retroh, false);

         /* Update av_info when PAL/NTSC change occurs */
         if (retro_region != retro_get_region())
            update_av_info = true;

         /* Allow fall-through for cropping on the same run */

      case 1:
         if (crop_id == crop_id_prev)
            return;

         int crop_width            = 0;
         int crop_height           = 0;
         int crop_height_o         = 0;
         int crop_border_width     = 0;
         int crop_border_height    = 0;

         int retrow_crop_prev      = retrow_crop;
         int retroh_crop_prev      = retroh_crop;

         float crop_dar            = 0;
         float crop_par            = retro_get_aspect_ratio(0, 0, true);

         int crop_width_max        = CROP_WIDTH_MAX;
         int crop_height_max       = CROP_HEIGHT_MAX;

#if defined(__X128__)
         if (c128_vdc)
            crop_width_max         *= 2;
#elif defined(__XCBM2__)
         if (retrow > 384)
            crop_width_max         *= 2;
#elif defined(__XPET__)
         if (retrow > 384)
         {
            crop_width_max         *= 2;
            crop_height_max        += CROP_TOP_BORDER - 8;
         }
#endif

         switch (crop_id)
         {
            default:
               switch (crop_id)
               {
                  case CROP_SMALL:
                     crop_border_width  = 44;
                     crop_border_height = 36;
                     break;
                  case CROP_MEDIUM:
                     crop_border_width  = 22;
                     crop_border_height = 18;
                     break;
                  case CROP_MAXIMUM:
                     break;
                  case CROP_AUTO:
                  case CROP_AUTO_DISABLE:
                     crop_border_height = vice_raster.last_line - vice_raster.first_line - crop_height_max;

                     if (crop_border_height < 0)
                        crop_border_height = 0;
                     break;
               }

               crop_width    = retrow - crop_width_max  - crop_border_width;
               crop_height   = retroh - crop_height_max - crop_border_height;
               crop_height_o = crop_height;

               switch (crop_mode_id)
               {
                  case CROP_MODE_BOTH:
                     break;
                  case CROP_MODE_VERTICAL: /* Vertical disables horizontal crop */
                     crop_width = 0;
                     break;
                  case CROP_MODE_HORIZONTAL: /* Horizontal disables vertical crop */
                     crop_height = 0;
                     break;
                  case CROP_MODE_16_9:
                     crop_dar = (float)16/9;
                     break;
                  case CROP_MODE_16_10:
                     crop_dar = (float)16/10;
                     break;
                  case CROP_MODE_4_3:
                     crop_dar = (float)4/3;
                     break;
                  case CROP_MODE_5_4:
                     crop_dar = (float)5/4;
                     break;
               }

               if (crop_dar > 0)
               {
                  if (crop_mode_id > 4)
                     crop_height = retroh - crop_height_max - ((float)crop_border_height * (float)crop_dar / (float)crop_par);
                  crop_width = retrow - ((float)(retroh - crop_height) * (float)crop_dar / (float)crop_par);
                  if (retrow - crop_width <= crop_width_max)
                     crop_height = retroh - ((float)crop_width_max / (float)crop_dar * (float)crop_par);
               }

               if (retrow - crop_width < crop_width_max)
                  crop_width = retrow - crop_width_max;
               if (retroh - crop_height < crop_height_max)
                  crop_height = retroh - crop_height_max;

               if (crop_width < 0)
                  crop_width = 0;
               if (crop_height < 0)
                  crop_height = 0;

               retrow_crop = retrow - crop_width;
               retroh_crop = retroh - crop_height;

               retroXS_crop_offset  = (crop_width  > 1) ? (crop_width  / 2) : 0;
               retroYS_crop_offset  = (crop_height > 1) ? (crop_height / 2) : 0;
#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
               retroYS_crop_offset -= (retro_region == RETRO_REGION_PAL) ? 1 : 0;
#elif defined(__XVIC__)
               retroXS_crop_offset -= (retro_region == RETRO_REGION_PAL) ? 0 : -8;
               retroYS_crop_offset -= (retro_region == RETRO_REGION_PAL) ? 2 : 3;
#elif defined(__XPLUS4__)
               retroYS_crop_offset -= (retro_region == RETRO_REGION_PAL) ? 4 : 3;
#endif
               switch (crop_id)
               {
                  case CROP_AUTO:
                     /* Reset autocentering depending on mode */
                     if (crop_height > 0 && vice_raster.first_line >= 0)
                        retroYS_crop_offset = vice_raster.first_line + (crop_height - crop_height_o) / 2;
                     break;
               }

               /* No negative offsets */
               retroXS_crop_offset = (retroXS_crop_offset > 0) ? retroXS_crop_offset : 0;
               retroYS_crop_offset = (retroYS_crop_offset > 0) ? retroYS_crop_offset : 0;
#if 0
               printf("crop: dar=%f par=%f - x=%3d y=%3d, osx=%2d osy=%2d, f=%3d l=%3d = %3dx%3d = %f * %f = %f\n",
                     crop_dar, crop_par,
                     crop_width, crop_height,
                     retroXS_crop_offset, retroYS_crop_offset,
                     vice_raster.first_line, vice_raster.last_line,
                     retrow_crop, retroh_crop,
                     ((float)retrow_crop / (float)retroh_crop),
                     crop_par,
                     ((float)retrow_crop / (float)retroh_crop * crop_par));
#endif
               break;

            case CROP_MANUAL:
               crop_width          = manual_crop_left + manual_crop_right;
               crop_height         = manual_crop_top  + manual_crop_bottom;

               retrow_crop         = retrow - crop_width;
               retroh_crop         = retroh - crop_height;
               retroXS_crop_offset = manual_crop_left;
               retroYS_crop_offset = manual_crop_top;
               break;

            case CROP_NONE:
               retrow_crop         = retrow;
               retroh_crop         = retroh;
               retroXS_crop_offset = 0;
               retroYS_crop_offset = 0;
               break;
         }

         retroXS_offset   = retroXS_crop_offset;
         retroYS_offset   = retroYS_crop_offset;
         retro_bmp_offset = (retroXS_offset * (pix_bytes >> 1)) + (retroYS_offset * (retrow << (pix_bytes >> 2)));

         system_av_info.geometry.base_width   = retrow_crop;
         system_av_info.geometry.base_height  = retroh_crop;
         system_av_info.geometry.aspect_ratio = retro_get_aspect_ratio(retrow_crop, retroh_crop, false);
         break;
   }

   crop_id_prev = crop_id;

   if (runstate > RUNSTATE_FIRST_START)
   {
      if (update_av_info)
      {
         retro_get_system_av_info(&system_av_info);
         environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &system_av_info);
      }
      else
      {
         environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &system_av_info);
      }
   }
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   /* Remember region for av_info update */
   retro_region = retro_get_region();

   /* Reset crop for proper aspect ratio update after av_info change */
   if (crop_id)
      crop_id_prev = -1;

   info->geometry.base_width   = retrow;
   info->geometry.base_height  = retroh;
   info->geometry.max_width    = WINDOW_WIDTH;
   info->geometry.max_height   = WINDOW_HEIGHT;
   info->geometry.aspect_ratio = retro_get_aspect_ratio(retrow, retroh, false);
   info->timing.sample_rate    = sound_sample_rate_prev = vice_opt.SoundSampleRate;

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? C64_PAL_RFSH_PER_SEC : C64_NTSC_RFSH_PER_SEC;
#elif defined(__XSCPU64__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? SCPU64_PAL_RFSH_PER_SEC : SCPU64_NTSC_RFSH_PER_SEC;
#elif defined(__X128__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? C128_PAL_RFSH_PER_SEC : C128_NTSC_RFSH_PER_SEC;
#elif defined(__XPLUS4__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? PLUS4_PAL_RFSH_PER_SEC : PLUS4_NTSC_RFSH_PER_SEC;
#elif defined(__XVIC__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? VIC20_PAL_RFSH_PER_SEC : VIC20_NTSC_RFSH_PER_SEC;
#elif defined(__XCBM5x0__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? C500_PAL_RFSH_PER_SEC : C500_NTSC_RFSH_PER_SEC;
#elif defined(__XCBM2__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? C610_PAL_RFSH_PER_SEC : C610_NTSC_RFSH_PER_SEC;
#elif defined(__XPET__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? PET_PAL_RFSH_PER_SEC : PET_NTSC_RFSH_PER_SEC;
#endif
   info->timing.fps = retro_refresh;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

#define RETRO_AUDIO_BATCH

void retro_audio_queue(const int16_t *data, int32_t samples)
{
   if ((samples < 1) || !runstate)
      return;

#if ARCHDEP_SOUND_OUTPUT_MODE == SOUND_OUTPUT_STEREO
#ifdef RETRO_AUDIO_BATCH
   if (output_audio_buffer.capacity - output_audio_buffer.size < samples)
      ensure_output_audio_buffer_capacity((output_audio_buffer.capacity + samples) * 1.5);
   memcpy(output_audio_buffer.data + output_audio_buffer.size, data, samples * sizeof(*output_audio_buffer.data));
   output_audio_buffer.size += samples;
#else
   for (int x = 0; x < samples; x += 2) audio_cb(data[x], data[x + 1]);
#endif
#elif ARCHDEP_SOUND_OUTPUT_MODE == SOUND_OUTPUT_MONO
   for (int x = 0; x < samples; x++) audio_cb(data[x], data[x]);
#endif
}

void emu_model_set(int model)
{
   request_model_set = -1;
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__) || defined(__X128__) || defined(__XVIC__)
   if (opt_model_auto && opt_model_auto_locked)
      model = request_model_auto_set;
#endif

   if (model == request_model_prev)
      return;

   /* Suspend and mute sound while model is changing to avoid problems */
   sound_suspend();

#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
   c64model_set(model);
#elif defined(__X64DTV__)
   dtvmodel_set(model);
#elif defined(__X128__)
   c128model_set(model);
#elif defined(__XPLUS4__)
   plus4model_set(model);
#elif defined(__XVIC__)
   vic20model_set(model);
   vic20mem_set();
#elif defined(__XCBM5x0__)
   cbm2model_set(model);
#elif defined(__XCBM2__)
   cbm2model_set(model);
   /* PAL & NTSC use the same resolution, thus force-trigger av_info */
   defaulth = 0;
#elif defined(__XPET__)
   petmodel_set(model);
   defaulth = 0;
#endif

   sound_resume();
   sound_volume_counter_reset();

   request_model_prev = model;
   request_restart = true;
}

#define AUTOLOADWARP_TAPE_DEBUG 0

void retro_run(void)
{
   /* Core options */
   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

   if (retro_message)
   {
      struct retro_message msg;
      msg.msg = retro_message_msg;
      msg.frames = 500;
      environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
      retro_message = false;
   }

   if (runstate == RUNSTATE_FIRST_START)
   {
      /* This is only done once after loading the core from scratch and starting it */
      runstate = RUNSTATE_RUNNING;
   } 
   else if (runstate == RUNSTATE_LOADED_CONTENT)
   {
      /* Load content was called while core was already running, just do a reset with autostart */
      runstate = RUNSTATE_RUNNING;
      request_restart = true;
   }
   else if (runstate == RUNSTATE_RUNNING)
   {
      /* Set model */
      if (request_model_set > -1)
         emu_model_set(request_model_set);

      /* Update samplerate if changed by core option */
      if (sound_sample_rate_prev != vice_opt.SoundSampleRate)
      {
         sound_sample_rate_prev = vice_opt.SoundSampleRate;

         /* Ensure audio rendering is reinitialized on next use */
         sound_state_changed = true;
         retro_sound_keep_alive = false;
         sound_close();

         struct retro_system_av_info system_av_info;
         retro_get_system_av_info(&system_av_info);
         environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &system_av_info);
      }

      /* Update work disk */
      if (request_update_work_disk)
         update_work_disk();

#if defined(__XVIC__)
      /* Autorun "SYS x" command */
      if (autosys && retro_now > 300000)
      {
         autosys = false;
         vic20_autosys_run(full_path);
      }
#endif

      /* Delayed autoload for FileSystem, which isn't resolving "*"
       * as the first file available */
      if (autoloadrun)
      {
         char program[48];
         char command[64];

         autoloadrun = false;

         snprintf(program, sizeof(program), "%s", first_file_in_dir(full_path));
         if (!string_is_empty(program))
            charset_petconvstring((uint8_t *)program, 0);

         snprintf(command, sizeof(command), "LOAD\"%s\",8,1\r", program);
         if (!string_is_empty(command))
         {
            kbdbuf_feed(command);
            kbdbuf_feed("RUN:\r");
         }
      }

      /* Autoloadwarp audio detection */
      if (opt_autoloadwarp && !(opt_autoloadwarp & AUTOLOADWARP_MUTE))
         audio_is_playing = audio_playing();

      /* Frame-based tape autoloadwarping for fast audio detection */
      if (tape_enabled && (opt_autoloadwarp & AUTOLOADWARP_TAPE || vsync_get_warp_mode()) && !retro_warpmode)
      {
         bool audio = !(opt_autoloadwarp & AUTOLOADWARP_MUTE) && opt_autoloadwarp & AUTOLOADWARP_TAPE ? audio_is_playing : false;

         if (     !vsync_get_warp_mode()
               && !audio
               && tape_control == 1
               && tape_motor)
         {
            vsync_set_warp_mode(1);
#if AUTOLOADWARP_TAPE_DEBUG
            printf("Tape Warp  ON, counter:%3d control:%d motor:%d audio:%d\n", tape_counter, tape_control, tape_motor, audio);
#endif
         }
         else if (vsync_get_warp_mode()
               && tape_counter > tape_found_counter
               && (  audio
                  || tape_control != 1
                  || !tape_motor
                  || !(opt_autoloadwarp & AUTOLOADWARP_TAPE)))
         {
            vsync_set_warp_mode(0);
#if AUTOLOADWARP_TAPE_DEBUG
            printf("Tape Warp OFF, counter:%3d control:%d motor:%d audio:%d\n", tape_counter, tape_control, tape_motor, audio);
#endif
         }
      }

      /* Allow re-enabling warp during tape loading music playback by pressing Space */
      if (     opt_autoloadwarp & AUTOLOADWARP_TAPE
            && tape_enabled && tape_motor && tape_control == 1
            && !audio_is_ignored && audio_is_playing
            && !vsync_get_warp_mode()
            && retro_key_state_internal[RETROK_SPACE])
      {
         audio_is_ignored = true;
         statusbar_message_show(9, "%s", "Resuming warp...");
      }
      else if (opt_autoloadwarp & AUTOLOADWARP_TAPE
            && tape_enabled && !tape_motor
            && audio_is_ignored)
      {
         audio_ignore_count++;
         if (audio_ignore_count > 10)
         {
            audio_is_ignored = false;
            audio_ignore_count = 0;
         }
      }

#if defined(__X128__)
      /* VDC toggle key enforcing */
      {
         static unsigned column_key_state = 0;
         unsigned toggle_state = retro_key_state_internal[key_ctrl_column4080];

         if (toggle_state && toggle_state != column_key_state)
            set_vdc(!c128_vdc);

         column_key_state = toggle_state;
      }
#endif
   }

   /* Input poll */
   input_poll_cb();
   retro_poll_event();

   /* Main loop */
   while (retro_renderloop)
      maincpu_mainloop();
   retro_renderloop = 1;
   retro_now += 1000000 / retro_refresh;

   retro_sound_keep_alive = false;

   /* LED interface */
   if (led_state_cb)
      retro_led_interface();

   /* Force frontend fast-forward on when internal warp is on */
   {
      static int warp_mode_prev = 0;
      int warp_mode             = vsync_get_warp_mode();

      if (warp_mode != warp_mode_prev)
      {
         warp_mode_prev = warp_mode;
         retro_fastforwarding(warp_mode);
      }
   }

   /* Update geometry if model or crop mode changes */
   if ((defaultw == retrow && defaulth == retroh) && crop_id != crop_id_prev)
      update_geometry(1);
   else if (defaultw != retrow || defaulth != retroh)
      update_geometry(0);

   /* Virtual keyboard */
   /* Moved to retrodep video_canvas_refresh() in order to stop flashing during warping */

   /* Statusbar message timer */
   if (statusbar_message_timer > 0)
      statusbar_message_timer--;

   /* Forced statusbar messages */
   if ((!retro_statusbar && opt_statusbar & STATUSBAR_MESSAGES && statusbar_message_timer) || retro_statusbar)
      uistatusbar_draw();

   /* Set volume back to maximum after starting with mute, due to ReSID 6581 init pop */
   if (sound_volume_counter > 0)
   {
      sound_volume_counter--;
      if (sound_volume_counter == 0)
#if defined(__XPLUS4__)
         resources_set_int("SoundVolume", MASTER_VOLUME_MAX / 2);
#else
         resources_set_int("SoundVolume", MASTER_VOLUME_MAX);
#endif
   }

   /* Video output */
   video_cb(retro_bmp + retro_bmp_offset, retrow_crop, retroh_crop, retrow << (pix_bytes >> 1));

   /* Audio output */
   upload_output_audio_buffer();

   /* retro_reset() postponed here for proper JiffyDOS+vicerc core option refresh operation
    * Restart does nothing if done too early, therefore only allow it after the first frame */
   if (request_restart)
   {
      size_t retro_max = 20000;
      /* For some random reason virtual devices are not ready yet after
       * region based model change on startup, therefore has to postpone
       * reset for a longer period.. */
      if (!vice_opt.DriveTrueEmulation && opt_model_auto_locked)
         retro_max = 3000000;

      if (retro_now > retro_max)
      {
         request_restart = false;
         emu_reset(0);
      }
   }
}

bool retro_load_game(const struct retro_game_info *info)
{
   /* Pixel format */
   if (!pix_bytes_initialized)
   {
      pix_bytes_initialized = true;
      if (pix_bytes == 2)
      {
         enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
         if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
         {
            log_cb(RETRO_LOG_ERROR, "RGB565 is not supported.\n");
            environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
            return false;
         }
      }
      else if (pix_bytes == 4)
      {
         enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
         if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
         {
            pix_bytes = 2;
            log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported. Trying RGB565.\n");
            fmt = RETRO_PIXEL_FORMAT_RGB565;
            if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
            {
               log_cb(RETRO_LOG_INFO, "RGB565 is not supported.\n");
               environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
               return false;
            }
         }
      }
   }

   /* Content */
   if (info)
   {
      /* Special unicode chars won't work internally in VICE without conversion */
      char *local_path;
      local_path = utf8_to_local_string_alloc(info->path);
      if (local_path)
      {
         process_cmdline(local_path);
         free(local_path);
         local_path = NULL;
      }
      else
         return false;
   }
   else
      /* Empty cmdline processing required for VIC-20 core option cartridges on startup */
      process_cmdline("");

#if defined(__XPET__) || defined(__XCBM2__) || defined(__XVIC__)
   /* Joyport limit has to apply always */
   cur_port = 1;
   cur_port_locked = true;
#endif

   if (runstate == RUNSTATE_FIRST_START)
   {
      pre_main();

#if defined(__XVIC__)
      /* CMD launch sets model to UNKNOWN, and XVIC requires full resource reset only then,
       * otherwise autostart fails. X64 fails post-reset TDE & JiffyDOS toggles without.. */
      if (vice_opt.Model == 99)
#endif
         resources_set_defaults();

      reload_restart();
      update_geometry(0);
   }
   else if (runstate == RUNSTATE_RUNNING)
   {
      /* load game was called while core is already running */
      /* so we update runstate and do the deferred autostart_reset in retro_run */
      /* the autostart_reset has to be deferred because a bunch of other init stuff */
      /* is done between retro_load_game() and retro_run() at a higher level */
      runstate = RUNSTATE_LOADED_CONTENT;
   }

   struct retro_memory_descriptor memdesc[] = {
      {RETRO_MEMDESC_SYSTEM_RAM, mem_ram, 0, 0, 0, 0, mem_ram_size, NULL}
   };

   struct retro_memory_map mmap = {
      memdesc,
      sizeof(memdesc) / sizeof(memdesc[0])
   };

   environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &mmap);
   return true;
}

void retro_unload_game(void)
{
   /* Gzip savedisks */
   if (dc)
      dc_save_disk_compress(dc);

   cartridge_unset_default();
   cartridge_detach_image(-1);
   tape_image_detach_all();
   file_system_detach_disk_all();
   file_system_detach_disk_shutdown();

   if (dc)
      dc_reset(dc);

   free(autostartString);
   autostartString = NULL;
   free(autostartProgram);
   autostartProgram = NULL;

   retro_sound_keep_alive = false;
   cur_port_locked = false;
   opt_aspect_ratio_locked = false;
   noautostart_locked = false;
   tde_locked = false;
   request_model_set = -1;
   request_model_auto_set = -1;
   request_model_prev = -1;
   opt_model_auto = true;
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
   opt_jiffydos_allow = true;
#endif
}

unsigned retro_get_region(void)
{
   int machine_sync = 0;
   if (!retro_ui_finalized)
      return retro_region;

   resources_get_int("MachineVideoStandard", &machine_sync);
   switch (machine_sync)
   {
      default:
      case MACHINE_SYNC_PAL:
      case MACHINE_SYNC_PALN:
         return RETRO_REGION_PAL;
         break;
      case MACHINE_SYNC_NTSC:
      case MACHINE_SYNC_NTSCOLD:
         return RETRO_REGION_NTSC;
         break;
   }
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

static void dc_sync_index(void)
{
   unsigned dc_index;
   drive_t *drive = diskunit_context[0]->drives[0];
   if (drive == NULL || string_is_empty(dc_savestate_filename))
      return;

   if (!drive->GCR_image_loaded)
      return;

   for (dc_index = 0; dc_index < dc->count; dc_index++)
   {
      if (strcasestr(dc->files[dc_index], dc_savestate_filename) && dc->index != dc_index)
      {
         dc->index = dc_index;
         retro_disk_set_eject_state(true);
         retro_disk_set_eject_state(false);
      }
   }
}

/* CPU traps ensure we are never saving snapshots or loading them in the middle of a cpu instruction.
   Without this, savestate corruption occurs.
*/

static void save_trap(uint16_t addr, void *success)
{
   int save_disks;
   int drive_type;
   /* Only do 'save_disks' with the usual suspect which has disk swapping needs
    * It does not really save disk data, but filename instead,
    * for syncing disk index on state load */
   resources_get_int("Drive8Type", &drive_type);
   save_disks = (drive_type == DRIVE_TYPE_1541II && !tape_enabled) ? 1 : 0;

   /* params: stream, save_roms, save_disks, event_mode */
   if (machine_write_snapshot_to_stream(snapshot_stream, 0, save_disks, 0) >= 0)
      *((int *)success) = 1;
   else
      *((int *)success) = 0;
   save_trap_happened = 1;
}

static void load_trap(uint16_t addr, void *success)
{
   /* params: stream, event_mode */
   if (machine_read_snapshot_from_stream(snapshot_stream, 0) >= 0)
      *((int *)success) = 1;
   else
      *((int *)success) = 0;
   load_trap_happened = 1;
}

static void retro_unserialize_post(void)
{
   /* Disable warp */
   if (vsync_get_warp_mode())
      vsync_set_warp_mode(0);
   /* Reset LED status */
   vice_led_state[RETRO_LED_POWER] = vice_led_state[RETRO_LED_DRIVE] = vice_led_state[RETRO_LED_TAPE] = 0;
   /* Dismiss possible restart request */
   request_restart = false;
   /* Sync Disc Control index for D64 multidisks */
   dc_sync_index();
}

size_t retro_serialize_size(void)
{
   long snapshot_size = 0;
   if (retro_ui_finalized)
   {
      snapshot_stream = snapshot_memory_write_fopen(NULL, 0);
      int success = 0;
      interrupt_maincpu_trigger_trap(save_trap, (void *)&success);
      save_trap_happened = 0;
      while (!save_trap_happened)
         maincpu_mainloop();
      if (snapshot_stream != NULL)
      {
         if (success)
         {
            snapshot_fseek(snapshot_stream, 0, SEEK_END);
            snapshot_size = snapshot_ftell(snapshot_stream);
         }
         else
         {
            log_cb(RETRO_LOG_ERROR, "Failed to calculate snapshot size.\n");
         }
         snapshot_fclose(snapshot_stream);
         snapshot_stream = NULL;
      }
   }
   else
   {
      /* Guesstimation size for rewind init, because core is not running yet.
       * Size depends on the inserted disk format, thus */
      snapshot_size = mem_ram_size + (mem_ram_size * 0.5);

      /* Some non-EF cartridges allocate a ridiculous amount of memory,
       * and at this moment the only way to play safe is to overestimate, because
       * even a 64k sized cart (Badlands) will have a 592452 sized snapshot !? */
      if ((!string_is_empty(full_path) && strendswith(full_path, "crt")) ||
          (dc && dc->files[dc->index] && strendswith(dc->files[dc->index], "crt")))
         snapshot_size = 592452;
   }

   return snapshot_size;
}

bool retro_serialize(void *data_, size_t size)
{
   if (retro_ui_finalized)
   {
      snapshot_stream = snapshot_memory_write_fopen(data_, size);
      int success = 0;
      interrupt_maincpu_trigger_trap(save_trap, (void *)&success);
      save_trap_happened = 0;
      while (!save_trap_happened)
         maincpu_mainloop();
      if (snapshot_stream != NULL)
      {
         snapshot_fclose(snapshot_stream);
         snapshot_stream = NULL;
      }
      if (success)
      {
         return true;
      }
      log_cb(RETRO_LOG_ERROR, "Failed to serialize snapshot.\n");
   }
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   if (retro_ui_finalized)
   {
      /* Must stop autostart, or prg content will continue autostarting
       * after autosaved state is autoloaded on startup */
      autostart_reset();
      /* Don't stop and start audio on every frame while rewinding */
      retro_sound_keep_alive = true;
      snapshot_stream = snapshot_memory_read_fopen(data_, size);
      int success = 0;
      interrupt_maincpu_trigger_trap(load_trap, (void *)&success);
      load_trap_happened = 0;
      while (!load_trap_happened)
         maincpu_mainloop();
      if (snapshot_stream != NULL)
      {
         snapshot_fclose(snapshot_stream);
         snapshot_stream = NULL;
      }
      if (success)
      {
         retro_unserialize_post();
         return true;
      }
      log_cb(RETRO_LOG_ERROR, "Failed to unserialize snapshot.\n");
   }
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   if (id == RETRO_MEMORY_SYSTEM_RAM)
      return mem_ram;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   if (id == RETRO_MEMORY_SYSTEM_RAM)
      return mem_ram_size;
   return 0;
}

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}


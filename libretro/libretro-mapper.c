#include "libretro.h"
#include "libretro-core.h"
#include "libretro-mapper.h"
#include "libretro-vkbd.h"
#include "libretro-graph.h"

#include "archdep.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "vsync.h"
#include "mouse.h"
#include "resources.h"
#include "autostart.h"
#include "datasette.h"
#include "tapeport.h"
#include "kbd.h"
#include "keymap.h"
#include "mousedrv.h"
#include "cartridge.h"

/* Mouse speed flags */
#define MOUSE_SPEED_SLOWER 1
#define MOUSE_SPEED_FASTER 2
/* Mouse speed multipliers */
#define MOUSE_SPEED_SLOW 6
#define MOUSE_SPEED_FAST 2
/* Mouse D-Pad acceleration */
#define MOUSE_DPAD_ACCEL

extern unsigned int opt_mouse_speed;
extern void mouse_move2(float dx, float dy);
static void retro_mouse_move(unsigned j, int x, int y)
{
   if (j + 1 == cur_port)
   {
      mouse_move(
         (x * (float)opt_mouse_speed / 100.0f),
         (y * (float)opt_mouse_speed / 100.0f)
      );
   }
   else
   {
      mouse_move2(
         (x * (float)opt_mouse_speed / 100.0f),
         (y * (float)opt_mouse_speed / 100.0f)
      );
   }
}

/* Core flags */
int mapper_keys[RETRO_MAPPER_LAST] = {0};
static long mapper_keys_pressed_time = 0;
static int mapper_flag[RETRO_DEVICES][128] = {0};
bool retro_capslock = false;
#if defined(__X128__)
bool c128_capslock = false;
#endif
unsigned int retro_warpmode = 0;

unsigned int cur_port = 2;
static int cur_port_prev = -1;
bool cur_port_locked = false;
unsigned int mouse_value[2 + 1] = {0};
unsigned int mouse_speed[4] = {0};

bool retro_statusbar = 0;
extern unsigned char statusbar_text[RETRO_PATH_MAX];
extern float retro_refresh;

uint8_t retro_mouse_discard = 0;
unsigned retro_key_state[RETROK_LAST] = {0};
unsigned retro_key_state_internal[RETROK_LAST] = {0};
unsigned retro_key_event_state[RETROK_LAST] = {0};
int16_t joypad_bits[RETRO_DEVICES];
int16_t joypad_axis[RETRO_DEVICES][RETRO_AXIS_MAX];
extern bool libretro_supports_bitmasks;
extern bool libretro_ff_enabled;
extern void retro_fastforwarding(bool);
extern dc_storage *dc;
extern long vkbd_mapping_active;
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__) || defined(__X128__) || defined(__XVIC__)
extern int mem_cartridge_type;
#else
int mem_cartridge_type = CARTRIDGE_NONE;
#endif

/* Core options */
extern unsigned int opt_joyport_type;
static int opt_joyport_type_prev = -1;
extern int opt_joyport_pointer_color;
extern unsigned int opt_dpadmouse_speed;
extern unsigned int opt_analogmouse;
extern unsigned int opt_analogmouse_deadzone;
extern float opt_analogmouse_speed_left;
extern float opt_analogmouse_speed_right;
bool datasette_hotkeys = false;

extern unsigned int opt_reset_type;
extern unsigned int opt_crop_id;
extern bool opt_keyboard_pass_through;
extern unsigned int opt_aspect_ratio;
extern bool opt_aspect_ratio_locked;

bool retro_turbo_fire = false;
bool turbo_fire_locked = false;
unsigned int turbo_fire_button = 0;
unsigned int turbo_pulse = 6;
unsigned int turbo_state[RETRO_DEVICES] = {0};
unsigned int turbo_toggle[RETRO_DEVICES] = {0};

int retro_ui_get_pointer_state(int *px, int *py, unsigned int *pbuttons)
{
   if (retro_vkbd)
      return 0;

   *pbuttons = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);

   *px = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
   *py = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
   *px = (int)((*px + 0x7fff) * retrow_crop / 0xffff + retroXS_offset);
   *py = (int)((*py + 0x7fff) * retroh_crop / 0xffff + retroYS_offset);

   if (opt_joyport_pointer_color > -1)
   {
      unsigned pointer_color = 0;
      unsigned pointer_white = RGBc(255, 255, 255);
      switch (opt_joyport_pointer_color)
      {
         case 0: pointer_color = RGBc(  0,   0,   0); break; /* Black */
         case 1: pointer_color = RGBc(255, 255, 255); break; /* White */
         case 2: pointer_color = RGBc(255,   0,   0); break; /* Red */
         case 3: pointer_color = RGBc(  0, 255,   0); break; /* Green */
         case 4: pointer_color = RGBc(  0,   0, 255); break; /* Blue */
         case 5: pointer_color = RGBc(255, 255,   0); break; /* Yellow */
         case 6: pointer_color = RGBc(  0, 255, 255); break; /* Cyan */
         case 7: pointer_color = RGBc(255,   0, 255); break; /* Purple */
      }

      draw_hline(*px - 3, *py, 3, 1, pointer_color);
      draw_hline(*px - 2, *py, 1, 1, pointer_white);

      draw_hline(*px + 1, *py, 3, 1, pointer_color);
      draw_hline(*px + 2, *py, 1, 1, pointer_white);

      draw_vline(*px, *py - 3, 1, 3, pointer_color);
      draw_vline(*px, *py - 2, 1, 1, pointer_white);

      draw_vline(*px, *py + 1, 1, 3, pointer_color);
      draw_vline(*px, *py + 2, 1, 1, pointer_white);
   }

   return 1;
}

int retro_keymap_id(const char *val)
{
   int i = 0;
   while (retro_keys[i].id < RETROK_LAST)
   {
      if (!strcmp(retro_keys[i].value, val))
         return retro_keys[i].id;
      i++;
   }
   return 0;
}

char *retro_keymap_value(const int id)
{
   int i = 0;
   while (retro_keys[i].id < RETROK_LAST)
   {
      if (retro_keys[i].id == id)
         return retro_keys[i].value;
      i++;
   }
   return 0;
}

char *retro_keymap_label(const int id)
{
   int i = 0;
   while (retro_keys[i].id < RETROK_LAST)
   {
      if (retro_keys[i].id == id)
         return retro_keys[i].label;
      i++;
   }
   return 0;
}

void statusbar_message_show(signed char icon, const char *format, ...)
{
   unsigned char statusbar_temp[RETRO_PATH_MAX] = {0};
   va_list args;
   va_start(args, format);

   if (!icon)
      icon = ' ';

   vsnprintf(statusbar_temp, sizeof(statusbar_temp), format, args);
   snprintf(statusbar_text, sizeof(statusbar_text), "%c %-98s", (icon | 0x80), statusbar_temp);
   va_end(args);

   statusbar_message_timer = 2 * retro_refresh;
}

void emu_function(int function)
{
   char tmp_str[20] = {0};

   switch (function)
   {
      case EMU_VKBD:
         toggle_vkbd();
         return;
      case EMU_STATUSBAR:
         retro_statusbar = (retro_statusbar) ? 0 : 1;
         resources_set_int("SDLStatusbar", retro_statusbar);
         break;
      case EMU_JOYPORT:
#if defined(__XPET__) || defined(__XCBM2__) || defined(__XVIC__)
         break;
#endif
         cur_port++;
         unsigned max_port = (vice_opt.UserportJoyType != -1) ? 4 : 2;
         if (cur_port > max_port)
            cur_port = 1;

         /* Lock current port */
         cur_port_locked = true;
         /* Statusbar notification */
         statusbar_message_show(9, "%s %d", "Port", cur_port);
         /* Clear mapper flags to prevent possible sticky keys */
         memset(mapper_flag, 0, sizeof(mapper_flag));
         break;
      case EMU_RESET:
         /* Cart freeze requires a cart */
         if (opt_reset_type == 3 && mem_cartridge_type == CARTRIDGE_NONE)
            break;
         emu_reset(-1);
         /* Statusbar notification */
         switch (opt_reset_type)
         {
            default:
            case 0: snprintf(tmp_str, sizeof(tmp_str), "Autostart"); break;
            case 1: snprintf(tmp_str, sizeof(tmp_str), "Soft reset"); break;
            case 2: snprintf(tmp_str, sizeof(tmp_str), "Hard reset"); break;
            case 3: snprintf(tmp_str, sizeof(tmp_str), "Freeze"); break;
         }
         statusbar_message_show(4, "%s", tmp_str);
         break;
      case EMU_FREEZE:
         /* Cart freeze requires a cart */
         if (mem_cartridge_type == CARTRIDGE_NONE)
            break;
         emu_reset(3);
         /* Statusbar notification */
         statusbar_message_show(4, "%s", "Freeze");
         break;
      case EMU_ASPECT_RATIO:
         if (opt_aspect_ratio == 0)
            opt_aspect_ratio = (retro_region == RETRO_REGION_NTSC) ? 2 : 1;
         opt_aspect_ratio++;
         if (opt_aspect_ratio > 3)
            opt_aspect_ratio = 1;
         /* Reset crop */
         crop_id_prev = -1;
         /* Lock aspect ratio */
         opt_aspect_ratio_locked = true;
         /* Statusbar notification */
         statusbar_message_show(5, "%s %s",
               "Pixel Aspect",
               (opt_aspect_ratio == 1) ? "PAL" : (opt_aspect_ratio == 2) ? "NTSC" : "1:1");
         break;
      case EMU_CROP:
         if (crop_id == 0 && opt_crop_id == 0)
            break;
         if (crop_id > 0)
            crop_id = 0;
         else if (crop_id == 0)
            crop_id = opt_crop_id;
         /* Statusbar notification */
         statusbar_message_show(5, "%s %s",
               "Crop",
               (crop_id) ? "ON" : "OFF");
         break;
      case EMU_TURBO_FIRE:
         retro_turbo_fire = !retro_turbo_fire;
         /* Lock turbo fire */
         turbo_fire_locked = true;
         /* Statusbar notification */
         statusbar_message_show(9, "%s %s",
               "Turbo Fire",
               (retro_turbo_fire) ? "ON" : "OFF");
         break;
      case EMU_SAVE_DISK:
         dc_save_disk_toggle(dc, false, true);
         break;
      case EMU_WARP_MODE:
         retro_warpmode = (retro_warpmode) ? 0 : 1;
         vsync_set_warp_mode(retro_warpmode);
         break;
      case EMU_DATASETTE_HOTKEYS:
#if defined(__X64DTV__) || defined(__XSCPU64__)
         break;
#endif
         datasette_hotkeys = !datasette_hotkeys;
         /* Statusbar notification */
         statusbar_message_show(24, "%s %s",
               "Datasette Hotkeys",
               (datasette_hotkeys) ? "ON" : "OFF");
         break;

      case EMU_DATASETTE_STOP:
#if defined(__X64DTV__) || defined(__XSCPU64__)
         break;
#endif
         if (!tape_enabled)
            break;
         datasette_control(TAPEPORT_PORT_1, DATASETTE_CONTROL_STOP);
         /* Statusbar notification */
         statusbar_message_show(23, "%s",
               "Datasette STOP");
         break;
      case EMU_DATASETTE_START:
#if defined(__X64DTV__) || defined(__XSCPU64__)
         break;
#endif
         if (!tape_enabled)
            break;
         datasette_control(TAPEPORT_PORT_1, DATASETTE_CONTROL_START);
         /* Statusbar notification */
         statusbar_message_show(20, "%s",
               "Datasette PLAY");
         break;
      case EMU_DATASETTE_FORWARD:
#if defined(__X64DTV__) || defined(__XSCPU64__)
         break;
#endif
         if (!tape_enabled)
            break;
         datasette_control(TAPEPORT_PORT_1, DATASETTE_CONTROL_FORWARD);
         /* Statusbar notification */
         statusbar_message_show(22, "%s",
               "Datasette F.FWD");
         break;
      case EMU_DATASETTE_REWIND:
#if defined(__X64DTV__) || defined(__XSCPU64__)
         break;
#endif
         if (!tape_enabled)
            break;
         datasette_control(TAPEPORT_PORT_1, DATASETTE_CONTROL_REWIND);
         /* Statusbar notification */
         statusbar_message_show(21, "%s",
               "Datasette REWIND");
         break;
      case EMU_DATASETTE_RESET:
#if defined(__X64DTV__) || defined(__XSCPU64__)
         break;
#endif
         if (!tape_enabled)
            break;
         datasette_control(TAPEPORT_PORT_1, DATASETTE_CONTROL_RESET);
         /* Statusbar notification */
         statusbar_message_show(19, "%s",
               "Datasette RESET");
         break;
   } 
}

static int adjust_analog_deadzone(int analog_axis, int analog_deadzone, int analog_mag)
{
   float max   = 32768.0f;
   float range = max - analog_deadzone;
   float norm  = (analog_mag - analog_deadzone) / range;
   float scale = norm / analog_mag * max;
   return analog_axis * scale;
}

static int process_analogmouse(int analog_axis, int analog_deadzone, int analog_mag, float mouse_speed, float mouse_multiplier, float *sub_pixel_remainder)
{
   int mouse_axis = 0;

   if (analog_axis)
   {
      int analog_adjusted = adjust_analog_deadzone(analog_axis, analog_deadzone, analog_mag);
      float delta = analog_adjusted * 10.0f * mouse_speed / (32768.0f / mouse_multiplier);
      mouse_axis = delta + *sub_pixel_remainder;

      if (fabs(delta) < 0.1f)
         *sub_pixel_remainder = 0;
      else
         *sub_pixel_remainder += delta - mouse_axis;

      if (fabs(*sub_pixel_remainder) > 1.0f)
         *sub_pixel_remainder = 0;
   }
   else
      *sub_pixel_remainder = 0;

   return mouse_axis;
}

void retro_key_up(int key)
{
   retro_key_state_internal[key] = 0;

   /* Prevent LShift keyup if ShiftLock is on */
   if (key == RETROK_LSHIFT)
   {
      if (!retro_capslock)
         kbd_handle_keyup(key);
   }
   else 
      kbd_handle_keyup(key);
}

void retro_key_down(int key)
{
   retro_key_state_internal[key] = 1;

   /* CapsLock / ShiftLock */
   if (key == RETROK_CAPSLOCK)
   {
      if (retro_capslock)
         kbd_handle_keyup(RETROK_LSHIFT);
      else
         kbd_handle_keydown(RETROK_LSHIFT);
      retro_capslock = !retro_capslock;
   }
#if defined(__X128__)
   else if (key == key_ctrl_caps)
   {
      c128_capslock = !c128_capslock;
      kbd_handle_keydown(key);
   }
#endif
   else
      kbd_handle_keydown(key);
}

void process_key(uint8_t disable_keys)
{
   uint16_t i = 0;

   for (i = RETROK_BACKSPACE; i < RETROK_LAST; i++)
   {
      if (disable_keys == 1 && (i == RETROK_UP || i == RETROK_DOWN || i == RETROK_LEFT || i == RETROK_RIGHT)
       || disable_keys == 2)
         retro_key_event_state[i] = 0;

      if (retro_key_event_state[i] && !retro_key_state[i])
      {
         /* Skip keydown if VKBD is active */
         if (retro_vkbd && i != RETROK_CAPSLOCK)
            continue;

         retro_key_state[i] = 1;
         retro_key_down(i);
      }
      else if (!retro_key_event_state[i] && retro_key_state[i])
      {
         retro_key_state[i] = 0;
         retro_key_up(i);
      }
   }
}

void retro_keyboard_event(bool down, unsigned code,
      uint32_t character, uint16_t mod)
{
   switch (code)
   {
      case RETROK_UNKNOWN:
      case RETROK_PAUSE:
         return;
   }
   retro_key_event_state[code] = down;
}

void update_input(uint8_t disable_keys)
{
   /* RETRO  B  Y  SL ST UP DN LT RT A  X  L   R   L2  R2  L3  R3  LR  LL  LD  LU  RR  RL  RD  RU
    * INDEX  0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19  20  21  22  23
    */

   unsigned int i = 0, j = 0, mk = 0;
   int LX = 0, LY = 0, RX = 0, RY = 0;
   const int threshold = 20000;
   long now = retro_ticks() / 1000;

   static int jbt[2][RETRO_DEVICE_ID_JOYPAD_LAST] = {0};
   static int kbt[EMU_FUNCTION_COUNT] = {0};
    
   /* Iterate hotkeys, skip Datasette hotkeys if Datasette hotkeys are disabled or if VKBD is on */
   int i_last = (datasette_hotkeys && !retro_vkbd) ? RETRO_MAPPER_DATASETTE_RESET : RETRO_MAPPER_DATASETTE_HOTKEYS;
   i_last -= RETRO_DEVICE_ID_JOYPAD_LAST;

   for (i = 0; i <= i_last; i++)
   {
      /* Skip RetroPad mappings from mapper_keys */
      mk = i + RETRO_DEVICE_ID_JOYPAD_LAST;

      /* Key down */
      if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, mapper_keys[mk]) && !kbt[i] && mapper_keys[mk])
      {
         kbt[i] = 1;
         switch (mk)
         {
            case RETRO_MAPPER_VKBD:
               mapper_keys_pressed_time = now; /* Decide on release */
               break;
            case RETRO_MAPPER_STATUSBAR:
               mapper_keys_pressed_time = now; /* Decide on release */
               break;
            case RETRO_MAPPER_JOYPORT:
               emu_function(EMU_JOYPORT);
               break;
            case RETRO_MAPPER_RESET:
               emu_function(EMU_RESET);
               break;
            case RETRO_MAPPER_ASPECT_RATIO:
               emu_function(EMU_ASPECT_RATIO);
               break;
            case RETRO_MAPPER_CROP:
               emu_function(EMU_CROP);
               break;
            case RETRO_MAPPER_WARP_MODE:
               emu_function(EMU_WARP_MODE);
               break;
            case RETRO_MAPPER_TURBO_FIRE:
               emu_function(EMU_TURBO_FIRE);
               break;
            case RETRO_MAPPER_SAVE_DISK:
               emu_function(EMU_SAVE_DISK);
               break;
            case RETRO_MAPPER_DATASETTE_HOTKEYS:
               emu_function(EMU_DATASETTE_HOTKEYS);
               break;

            case RETRO_MAPPER_DATASETTE_STOP:
               emu_function(EMU_DATASETTE_STOP);
               break;
            case RETRO_MAPPER_DATASETTE_START:
               emu_function(EMU_DATASETTE_START);
               break;
            case RETRO_MAPPER_DATASETTE_FORWARD:
               emu_function(EMU_DATASETTE_FORWARD);
               break;
            case RETRO_MAPPER_DATASETTE_REWIND:
               emu_function(EMU_DATASETTE_REWIND);
               break;
            case RETRO_MAPPER_DATASETTE_RESET:
               emu_function(EMU_DATASETTE_RESET);
               break;
         }
      }
      /* Key up */
      else if (!input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, mapper_keys[mk]) && kbt[i] && mapper_keys[mk])
      {
         kbt[i] = 0;
         switch (mk)
         {
            case RETRO_MAPPER_VKBD:
               if (now - mapper_keys_pressed_time > LONG_PRESS && libretro_ff_enabled)
                  retro_fastforwarding(false);
               else if (now - mapper_keys_pressed_time < SHORT_PRESS)
                  emu_function(EMU_VKBD);
               else
                  emu_function(EMU_STATUSBAR);
               mapper_keys_pressed_time = 0;
               break;
            case RETRO_MAPPER_STATUSBAR:
               if (now - mapper_keys_pressed_time > LONG_PRESS && libretro_ff_enabled)
                  retro_fastforwarding(false);
               else if (now - mapper_keys_pressed_time < SHORT_PRESS)
                  emu_function(EMU_STATUSBAR);
               else
                  emu_function(EMU_VKBD);
               mapper_keys_pressed_time = 0;
               break;
            case RETRO_MAPPER_WARP_MODE:
               emu_function(EMU_WARP_MODE);
               break;
         }
      }
      else if (mapper_keys_pressed_time)
      {
         if (now - mapper_keys_pressed_time > LONG_PRESS && !libretro_ff_enabled)
            retro_fastforwarding(true);
      }
   }

   /* The check for kbt[i] here prevents the hotkey from generating C64 key events */
   /* retro_vkbd check is now in process_key() to allow certain keys while retro_vkbd */
   for (i = 0; i < (sizeof(kbt)/sizeof(kbt[0])); i++)
   {
      if (kbt[i])
      {
         disable_keys = 2;
         break;
      }
   }

   process_key(disable_keys);

   /* RetroPad hotkeys for ports 1 & 2 */
   for (j = 0; j < 2; j++)
   {
      if (vkbd_mapping_active)
         continue;

      if (retro_devices[j] == RETRO_DEVICE_JOYPAD)
      {
         LX = joypad_axis[j][AXIS_LX];
         LY = joypad_axis[j][AXIS_LY];
         RX = joypad_axis[j][AXIS_RX];
         RY = joypad_axis[j][AXIS_RY];

         /* No analog remappings with non-joysticks and stick overrides */
         if (opt_joyport_type > 1)
         {
            if (opt_analogmouse == 1 || opt_analogmouse == 3)
               LX = LY = 0;

            if (opt_analogmouse == 2 || opt_analogmouse == 3)
               RX = RY = 0;
         }

         for (i = 0; i < RETRO_DEVICE_ID_JOYPAD_LAST; i++)
         {
            int just_pressed  = 0;
            int just_released = 0;
            if (i < 16) /* Remappable RetroPad buttons and directions */
            {
               /* Skip VKBD buttons if VKBD is visible and buttons
                * are mapped to keyboard keys, but allow release */
               if (retro_vkbd)
               {
                  switch (i)
                  {
                     case RETRO_DEVICE_ID_JOYPAD_UP:
                     case RETRO_DEVICE_ID_JOYPAD_DOWN:
                     case RETRO_DEVICE_ID_JOYPAD_LEFT:
                     case RETRO_DEVICE_ID_JOYPAD_RIGHT:
                     case RETRO_DEVICE_ID_JOYPAD_B:
                     case RETRO_DEVICE_ID_JOYPAD_Y:
                     case RETRO_DEVICE_ID_JOYPAD_A:
                     case RETRO_DEVICE_ID_JOYPAD_X:
                     case RETRO_DEVICE_ID_JOYPAD_START:
                        if (mapper_keys[i] >= 0 && !jbt[j][i])
                           continue;
                        break;
                  }
               }

               /* No mappings if button = turbo fire in joystick mode */
               if (retro_turbo_fire && i == turbo_fire_button && opt_joyport_type == 1)
                  continue;

               if ((joypad_bits[j] & (1 << i)) && !jbt[j][i])
                  just_pressed = 1;
               else if (!(joypad_bits[j] & (1 << i)) && jbt[j][i])
                  just_released = 1;
            }
            else if (i >= 16) /* Remappable RetroPad analog stick directions */
            {
               switch (i)
               {
                  case RETRO_DEVICE_ID_JOYPAD_LR:
                     if (LX > threshold && !jbt[j][i]) just_pressed = 1;
                     else if (LX < threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_LL:
                     if (LX < -threshold && !jbt[j][i]) just_pressed = 1;
                     else if (LX > -threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_LD:
                     if (LY > threshold && !jbt[j][i]) just_pressed = 1;
                     else if (LY < threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_LU:
                     if (LY < -threshold && !jbt[j][i]) just_pressed = 1;
                     else if (LY > -threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_RR:
                     if (RX > threshold && !jbt[j][i]) just_pressed = 1;
                     else if (RX < threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_RL:
                     if (RX < -threshold && !jbt[j][i]) just_pressed = 1;
                     else if (RX > -threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_RD:
                     if (RY > threshold && !jbt[j][i]) just_pressed = 1;
                     else if (RY < threshold && jbt[j][i]) just_released = 1;
                     break;
                  case RETRO_DEVICE_ID_JOYPAD_RU:
                     if (RY < -threshold && !jbt[j][i]) just_pressed = 1;
                     else if (RY > -threshold && jbt[j][i]) just_released = 1;
                     break;
                  default:
                     break;
               }
            }

            if (just_pressed)
            {
               jbt[j][i] = 1;
               if (!mapper_keys[i]) /* Unmapped, e.g. set to "---" in core options */
                  continue;

               if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_VKBD])
                  emu_function(EMU_VKBD);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_STATUSBAR])
                  emu_function(EMU_STATUSBAR);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_JOYPORT])
                  emu_function(EMU_JOYPORT);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_RESET])
                  emu_function(EMU_RESET);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_ASPECT_RATIO])
                  emu_function(EMU_ASPECT_RATIO);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_CROP])
                  emu_function(EMU_CROP);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_WARP_MODE])
                  emu_function(EMU_WARP_MODE);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_TURBO_FIRE])
                  emu_function(EMU_TURBO_FIRE);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_SAVE_DISK])
                  emu_function(EMU_SAVE_DISK);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_HOTKEYS])
                  emu_function(EMU_DATASETTE_HOTKEYS);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_STOP])
                  emu_function(EMU_DATASETTE_STOP);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_START])
                  emu_function(EMU_DATASETTE_START);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_FORWARD])
                  emu_function(EMU_DATASETTE_FORWARD);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_REWIND])
                  emu_function(EMU_DATASETTE_REWIND);
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_RESET])
                  emu_function(EMU_DATASETTE_RESET);
               else if (mapper_keys[i] == MOUSE_SLOWER)
                  mouse_speed[j] |= MOUSE_SPEED_SLOWER;
               else if (mapper_keys[i] == MOUSE_FASTER)
                  mouse_speed[j] |= MOUSE_SPEED_FASTER;
               else if (mapper_keys[i] == JOYSTICK_FIRE)
                  mapper_flag[cur_port][JOYPAD_FIRE] = 1;
               else if (mapper_keys[i] == JOYSTICK_FIRE2)
                  mapper_flag[cur_port][JOYPAD_FIRE2] = 1;
               else if (mapper_keys[i] == JOYSTICK_FIRE3)
                  mapper_flag[cur_port][JOYPAD_FIRE3] = 1;
               else if (mapper_keys[i] == JOYSTICK_UP)
                  mapper_flag[cur_port][JOYPAD_N] = 1;
               else if (mapper_keys[i] == JOYSTICK_DOWN)
                  mapper_flag[cur_port][JOYPAD_S] = 1;
               else if (mapper_keys[i] == JOYSTICK_LEFT)
                  mapper_flag[cur_port][JOYPAD_W] = 1;
               else if (mapper_keys[i] == JOYSTICK_RIGHT)
                  mapper_flag[cur_port][JOYPAD_E] = 1;
               else if (mapper_keys[i] == OTHERJOY_FIRE)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_FIRE] = 1;
               else if (mapper_keys[i] == OTHERJOY_UP)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_N] = 1;
               else if (mapper_keys[i] == OTHERJOY_DOWN)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_S] = 1;
               else if (mapper_keys[i] == OTHERJOY_LEFT)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_W] = 1;
               else if (mapper_keys[i] == OTHERJOY_RIGHT)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_E] = 1;
               else if (mapper_keys[i] == TOGGLE_VKBD)
                  mapper_keys_pressed_time = now; /* Decide on release */
               else if (mapper_keys[i] == TOGGLE_STATUSBAR)
                  mapper_keys_pressed_time = now; /* Decide on release */
               else if (mapper_keys[i] == SWITCH_JOYPORT)
                  emu_function(EMU_JOYPORT);
               else
                  retro_key_down(mapper_keys[i]);
            }
            else if (just_released)
            {
               jbt[j][i] = 0;
               if (!mapper_keys[i]) /* Unmapped, e.g. set to "---" in core options */
                  continue;

               if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_VKBD])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_STATUSBAR])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_JOYPORT])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_RESET])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_ASPECT_RATIO])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_CROP])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_WARP_MODE])
                  emu_function(EMU_WARP_MODE);
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_TURBO_FIRE])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_SAVE_DISK])
                  ; /* nop */
               else if (mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_HOTKEYS])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_STOP])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_START])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_FORWARD])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_REWIND])
                  ; /* nop */
               else if (datasette_hotkeys && mapper_keys[i] == mapper_keys[RETRO_MAPPER_DATASETTE_RESET])
                  ; /* nop */
               else if (mapper_keys[i] == MOUSE_SLOWER)
                  mouse_speed[j] &= ~MOUSE_SPEED_SLOWER;
               else if (mapper_keys[i] == MOUSE_FASTER)
                  mouse_speed[j] &= ~MOUSE_SPEED_FASTER;
               else if (mapper_keys[i] == JOYSTICK_FIRE)
                  mapper_flag[cur_port][JOYPAD_FIRE] = 0;
               else if (mapper_keys[i] == JOYSTICK_FIRE2)
                  mapper_flag[cur_port][JOYPAD_FIRE2] = 0;
               else if (mapper_keys[i] == JOYSTICK_FIRE3)
                  mapper_flag[cur_port][JOYPAD_FIRE3] = 0;
               else if (mapper_keys[i] == JOYSTICK_UP)
                  mapper_flag[cur_port][JOYPAD_N] = 0;
               else if (mapper_keys[i] == JOYSTICK_DOWN)
                  mapper_flag[cur_port][JOYPAD_S] = 0;
               else if (mapper_keys[i] == JOYSTICK_LEFT)
                  mapper_flag[cur_port][JOYPAD_W] = 0;
               else if (mapper_keys[i] == JOYSTICK_RIGHT)
                  mapper_flag[cur_port][JOYPAD_E] = 0;
               else if (mapper_keys[i] == OTHERJOY_FIRE)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_FIRE] = 0;
               else if (mapper_keys[i] == OTHERJOY_UP)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_N] = 0;
               else if (mapper_keys[i] == OTHERJOY_DOWN)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_S] = 0;
               else if (mapper_keys[i] == OTHERJOY_LEFT)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_W] = 0;
               else if (mapper_keys[i] == OTHERJOY_RIGHT)
                  mapper_flag[(cur_port == 2) ? 1 : 2][JOYPAD_E] = 0;
               else if (mapper_keys[i] == TOGGLE_VKBD)
               {
                  if (now - mapper_keys_pressed_time > LONG_PRESS && libretro_ff_enabled)
                     retro_fastforwarding(false);
                  else if (now - mapper_keys_pressed_time < SHORT_PRESS)
                     emu_function(EMU_VKBD);
                  else
                     emu_function(EMU_STATUSBAR);
                  mapper_keys_pressed_time = 0;
               }
               else if (mapper_keys[i] == TOGGLE_STATUSBAR)
               {
                  if (now - mapper_keys_pressed_time > LONG_PRESS && libretro_ff_enabled)
                     retro_fastforwarding(false);
                  else if (now - mapper_keys_pressed_time < SHORT_PRESS)
                     emu_function(EMU_STATUSBAR);
                  else
                     emu_function(EMU_VKBD);
                  mapper_keys_pressed_time = 0;
               }
               else if (mapper_keys[i] == SWITCH_JOYPORT)
                  ; /* nop */
               else
                  retro_key_up(mapper_keys[i]);
            }
            else if (mapper_keys_pressed_time)
            {
               if (now - mapper_keys_pressed_time > LONG_PRESS && !libretro_ff_enabled)
                  retro_fastforwarding(true);
            }
         } /* for i */
      } /* if retro_devices[0]==joypad */
   } /* for j */

   /* Virtual keyboard for ports 1 & 2 */
   input_vkbd();
}

int process_keyboard_pass_through()
{
   unsigned process = 0;

   /* Defaults */
   int fire1_button = RETRO_DEVICE_ID_JOYPAD_B;
   int fire2_button = RETRO_DEVICE_ID_JOYPAD_A;
   int jump_button  = -1;

   /* Fire buttons */
   switch (opt_retropad_options)
   {
      case RETROPAD_OPTIONS_ROTATE:
      case RETROPAD_OPTIONS_ROTATE_JUMP:
         fire1_button = RETRO_DEVICE_ID_JOYPAD_Y;
         fire2_button = RETRO_DEVICE_ID_JOYPAD_B;
         break;
   }

   /* Jump button */
   switch (opt_retropad_options)
   {
      case RETROPAD_OPTIONS_JUMP:
         jump_button  = RETRO_DEVICE_ID_JOYPAD_A;
         fire2_button = -1;
         break;
      case RETROPAD_OPTIONS_ROTATE_JUMP:
         jump_button  = RETRO_DEVICE_ID_JOYPAD_B;
         fire2_button = -1;
         break;
   }

   /* Null only with RetroPad */
   if (retro_devices[0] == RETRO_DEVICE_JOYPAD)
   {
      if (mapper_keys[fire1_button] || (retro_turbo_fire && fire1_button == turbo_fire_button))
         fire1_button = -1;
      if (mapper_keys[fire2_button] || (retro_turbo_fire && fire2_button == turbo_fire_button))
         fire2_button = -1;
      if (mapper_keys[jump_button] || (retro_turbo_fire && jump_button == turbo_fire_button))
         jump_button  = -1;
   }

   /* Prevent RetroPad from generating keyboard key presses when RetroPad is controlled with keyboard */
   switch (retro_devices[0])
   {
      case RETRO_DEVICE_JOYPAD:
         if ((fire1_button > -1 && (joypad_bits[0] & (1 << fire1_button))) ||
             (fire2_button > -1 && (joypad_bits[0] & (1 << fire2_button))) ||
             (jump_button > -1  && (joypad_bits[0] & (1 << jump_button))) ||
             (retro_turbo_fire  && (joypad_bits[0] & (1 << turbo_fire_button))) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_B))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_B]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_Y))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_Y]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_A))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_A]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_X))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_X]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L2))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L2]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R2))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R2]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L3))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L3]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R3))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R3]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT)) && mapper_keys[RETRO_DEVICE_ID_JOYPAD_SELECT]) ||
             ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_START))  && mapper_keys[RETRO_DEVICE_ID_JOYPAD_START]))
            process = 2; /* Skip all keyboard input */
         else
         if ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
            process = 1; /* Skip cursor keys */
         break;

      case RETRO_DEVICE_VICE_JOYSTICK:
         if ((fire1_button > -1 && (joypad_bits[0] & (1 << fire1_button))) ||
             (fire2_button > -1 && (joypad_bits[0] & (1 << fire2_button))) ||
             (jump_button > -1  && (joypad_bits[0] & (1 << jump_button))))
            process = 2; /* Skip all keyboard input */
         else
         if ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
             (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
            process = 1; /* Skip cursor keys */
         break;
   }

   /* Fire button */
   fire1_button = RETRO_DEVICE_ID_JOYPAD_B;
   fire2_button = RETRO_DEVICE_ID_JOYPAD_A;
   switch (opt_retropad_options)
   {
      case RETROPAD_OPTIONS_ROTATE:
      case RETROPAD_OPTIONS_ROTATE_JUMP:
         fire1_button = RETRO_DEVICE_ID_JOYPAD_Y;
         fire2_button = RETRO_DEVICE_ID_JOYPAD_B;
         break;
   }

   /* Jump button */
   jump_button = -1;
   switch (opt_retropad_options)
   {
      case RETROPAD_OPTIONS_JUMP:
         jump_button  = RETRO_DEVICE_ID_JOYPAD_A;
         fire2_button = -1;
         break;
      case RETROPAD_OPTIONS_ROTATE_JUMP:
         jump_button  = RETRO_DEVICE_ID_JOYPAD_B;
         fire2_button = -1;
         break;
   }

   /* Null only with RetroPad */
   if (retro_devices[1] == RETRO_DEVICE_JOYPAD)
   {
      if (mapper_keys[fire1_button] || (retro_turbo_fire && fire1_button == turbo_fire_button))
         fire1_button = -1;
      if (mapper_keys[fire2_button] || (retro_turbo_fire && fire2_button == turbo_fire_button))
         fire2_button = -1;
      if (mapper_keys[jump_button] || (retro_turbo_fire && jump_button == turbo_fire_button))
         jump_button  = -1;
   }

   switch (retro_devices[1])
   {
      case RETRO_DEVICE_JOYPAD:
         if ((fire1_button > -1 && (joypad_bits[1] & (1 << fire1_button))) ||
             (fire2_button > -1 && (joypad_bits[1] & (1 << fire2_button))) ||
             (jump_button > -1  && (joypad_bits[1] & (1 << jump_button))) ||
             (retro_turbo_fire  && (joypad_bits[1] & (1 << turbo_fire_button))) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_B))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_B]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_Y))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_Y]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_A))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_A]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_X))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_X]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R))      && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L2))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L2]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R2))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R2]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L3))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_L3]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R3))     && mapper_keys[RETRO_DEVICE_ID_JOYPAD_R3]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT)) && mapper_keys[RETRO_DEVICE_ID_JOYPAD_SELECT]) ||
             ((joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_START))  && mapper_keys[RETRO_DEVICE_ID_JOYPAD_START]) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
            process = 2; /* Skip all keyboard input */
         break;

      case RETRO_DEVICE_VICE_JOYSTICK:
         if ((fire1_button > -1 && (joypad_bits[1] & (1 << fire1_button))) ||
             (fire2_button > -1 && (joypad_bits[1] & (1 << fire2_button))) ||
             (jump_button > -1  && (joypad_bits[1] & (1 << jump_button))) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
              (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
            process = 2; /* Skip all keyboard input */
         break;
   }

   return process;
}

void retro_poll_event()
{
   uint8_t i, j;
   for (j = 0; j < RETRO_DEVICES; j++)
   {
      if (libretro_supports_bitmasks)
         joypad_bits[j] = input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
      else
      {
         joypad_bits[j] = 0;
         for (i = 0; i < RETRO_DEVICE_ID_JOYPAD_LR; i++)
            joypad_bits[j] |= input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
      }
   }

   /* Analog sticks for first two controllers */
   for (j = 0; j < 2; j++)
   {
      joypad_axis[j][AXIS_LX] = input_state_cb(j, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
      joypad_axis[j][AXIS_LY] = input_state_cb(j, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
      joypad_axis[j][AXIS_RX] = input_state_cb(j, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
      joypad_axis[j][AXIS_RY] = input_state_cb(j, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
   }

   /* Keyboard pass-through */
   uint8_t disable_keys = 0;
   if (!opt_keyboard_pass_through)
      disable_keys = process_keyboard_pass_through();
   update_input(disable_keys);

   /* Joystick port iteration */
   uint8_t retro_port;
   uint8_t max_port = (vice_opt.UserportJoyType != -1) ? 4 : 2;
   for (retro_port = 0; retro_port < max_port; retro_port++)
   {
      if (retro_devices[retro_port] == RETRO_DEVICE_VICE_JOYSTICK ||
          retro_devices[retro_port] == RETRO_DEVICE_JOYPAD)
      {
         int8_t vice_port  = cur_port;
         uint8_t joy_value = 0;

         if (vice_opt.UserportJoyType != -1)
         {
            /* With userport adapter: Next retro port controls next joyport */
            vice_port = cur_port + retro_port;
            vice_port = (vice_port > max_port) ? (vice_port - max_port) : vice_port;
         }
         else
         {
            /* Without userport adapter: Second port controls opposite joyport */
            if (retro_port == 1)
               vice_port = (cur_port == 2) ? 1 : 2;
         }

         /* No same port joystick movements with non-joysticks */
         if (opt_joyport_type > 1 && vice_port == cur_port)
            continue;
         /* No joystick movements with paddles */
         if (opt_joyport_type == 2)
            continue;

         joy_value = get_joystick_value(vice_port - 1);

         /* Up */
         if (((joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_UP) && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_UP])
         &&  !(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP9)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP3)
                  )
                  ||
                  (vice_port == cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP8)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP2)
                  )
               )
            )
         || (mapper_flag[vice_port][JOYPAD_N])
         )
            joy_value |= (!retro_vkbd) ? JOYPAD_N : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
              && (!mapper_flag[vice_port][JOYPAD_N]))
            joy_value &= ~JOYPAD_N;

         /* Down */
         if (((joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN) && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_DOWN])
         &&  !(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP3)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP9)
                  )
                  ||
                  (vice_port == cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP2)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP8)
                  )
               )
            )
         || (mapper_flag[vice_port][JOYPAD_S])
         )
            joy_value |= (!retro_vkbd) ? JOYPAD_S : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
              && (!mapper_flag[vice_port][JOYPAD_S]))
            joy_value &= ~JOYPAD_S;

         /* Left */
         if (((joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT) && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_LEFT])
         &&  !(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP7)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP1)
                  )
                  ||
                  (vice_port == cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP4)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP6)
                  )
               )
            )
         || (mapper_flag[vice_port][JOYPAD_W])
         )
            joy_value |= (!retro_vkbd) ? JOYPAD_W : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
              && (!mapper_flag[vice_port][JOYPAD_W]))
            joy_value &= ~JOYPAD_W;

         /* Right */
         if (((joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT) && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_RIGHT])
         &&  !(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP1)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP7)
                  )
                  ||
                  (vice_port == cur_port
                  &&  input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP6)
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP4)
                  )
               )
            )
         || (mapper_flag[vice_port][JOYPAD_E])
         )
            joy_value |= (!retro_vkbd) ? JOYPAD_E : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
              && (!mapper_flag[vice_port][JOYPAD_E]))
            joy_value &= ~JOYPAD_E;

         /* Fire buttons */
         int fire1_button = RETRO_DEVICE_ID_JOYPAD_B;
         int fire2_button = RETRO_DEVICE_ID_JOYPAD_A;
         int jump_button  = -1;

         switch (opt_retropad_options)
         {
            case RETROPAD_OPTIONS_ROTATE:
            case RETROPAD_OPTIONS_ROTATE_JUMP:
               fire1_button = RETRO_DEVICE_ID_JOYPAD_Y;
               fire2_button = RETRO_DEVICE_ID_JOYPAD_B;
               break;
         }

         /* Jump button */
         switch (opt_retropad_options)
         {
            case RETROPAD_OPTIONS_JUMP:
               jump_button  = RETRO_DEVICE_ID_JOYPAD_A;
               fire2_button = -1;
               break;
            case RETROPAD_OPTIONS_ROTATE_JUMP:
               jump_button  = RETRO_DEVICE_ID_JOYPAD_B;
               fire2_button = -1;
               break;
         }

         /* Null only with RetroPad */
         if (retro_devices[retro_port] == RETRO_DEVICE_JOYPAD)
         {
            if (mapper_keys[fire1_button] || (retro_turbo_fire && fire1_button == turbo_fire_button))
               fire1_button = -1;
            if (mapper_keys[fire2_button] || (retro_turbo_fire && fire2_button == turbo_fire_button))
               fire2_button = -1;
            if (mapper_keys[jump_button] || (retro_turbo_fire && jump_button == turbo_fire_button))
               jump_button  = -1;
         }

         /* Fire button */
         if ((fire1_button > -1 && (joypad_bits[retro_port] & (1 << fire1_button)))
         || (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port && input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP0))
                  ||
                  (vice_port == cur_port && input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP5))
               )
            )
         || (mapper_flag[vice_port][JOYPAD_FIRE])
         )
            joy_value |= (!retro_vkbd) ? JOYPAD_FIRE : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << fire1_button))
              && (!mapper_flag[vice_port][JOYPAD_FIRE]))
            joy_value &= ~JOYPAD_FIRE;

         /* 2nd fire button */
         if ((fire2_button > -1 && (joypad_bits[retro_port] & (1 << fire2_button)))
         || (mapper_flag[vice_port][JOYPAD_FIRE2])
         )
            joy_value |= (!retro_vkbd) ? JOYPAD_FIRE2 : joy_value;
         else if (!(joypad_bits[retro_port] & (1 << fire2_button))
              && (!mapper_flag[vice_port][JOYPAD_FIRE2]))
            joy_value &= ~JOYPAD_FIRE2;

         /* Jump button */
         if (jump_button > -1 && (joypad_bits[retro_port] & (1 << jump_button)))
         {
            joy_value |= (!retro_vkbd) ? JOYPAD_N : joy_value;
            joy_value &= ~JOYPAD_S;
         }
         else if (!(joypad_bits[retro_port] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
         && (opt_keyrah_keypad && vice_port < 3
            && (
                  (vice_port != cur_port
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP9)
                  )
                  &&
                  (vice_port == cur_port
                  && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_KP8)
                  )
               )
            )
         )
            joy_value &= ~JOYPAD_N;

         /* 3rd fire button, only via mapper flag */
         if (vice_port < 3 && mapper_flag[vice_port][JOYPAD_FIRE3])
            joy_value |= (!retro_vkbd) ? JOYPAD_FIRE3 : joy_value;
         else if (vice_port < 3 && !mapper_flag[vice_port][JOYPAD_FIRE3])
            joy_value &= ~JOYPAD_FIRE3;

         /* Turbo fire */
         if (retro_devices[retro_port] == RETRO_DEVICE_JOYPAD
          && retro_turbo_fire
          && !(joypad_bits[retro_port] & (1 << fire1_button))
          && !(joypad_bits[retro_port] & (1 << fire2_button)))
         {
            if ((joypad_bits[retro_port] & (1 << turbo_fire_button)))
            {
               if (turbo_state[vice_port])
               {
                  if ((turbo_toggle[vice_port]) == (turbo_pulse))
                     turbo_toggle[vice_port] = 1;
                  else
                     turbo_toggle[vice_port]++;

                  if (turbo_toggle[vice_port] > (turbo_pulse / 2))
                  {
                     joy_value &= ~JOYPAD_FIRE;
                     mapper_flag[vice_port][JOYPAD_FIRE] = 0;
                  }
                  else
                  {
                     joy_value |= (!retro_vkbd) ? JOYPAD_FIRE : joy_value;
                     mapper_flag[vice_port][JOYPAD_FIRE] = 1;
                  }
               }
               else
               {
                  turbo_state[vice_port] = 1;
                  joy_value |= (!retro_vkbd) ? JOYPAD_FIRE : joy_value;
                  mapper_flag[vice_port][JOYPAD_FIRE] = 1;
               }
            }
            else
            {
               turbo_state[vice_port] = 0;
               turbo_toggle[vice_port] = 0;
               mapper_flag[vice_port][JOYPAD_FIRE] = 0;
            }
         }

         joystick_set_value_absolute(vice_port - 1, joy_value);

#if 0
         if (vice_port == 2)
            printf("Joy %d: %2d, fire1=%d fire2=%d jump=%d, turbo=%d %d %d\n", vice_port, joy_value, fire1_button, fire2_button, jump_button, turbo_fire_button, turbo_state[vice_port], turbo_toggle[vice_port]);
#endif
      }
   }

   /* Default to joysticks, set both ports */
   if (opt_joyport_type == 1)
   {
      if (opt_joyport_type_prev != opt_joyport_type)
      {
         opt_joyport_type_prev = opt_joyport_type;
         resources_set_int("JoyPort1Device", opt_joyport_type);
#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XVIC__)
         resources_set_int("JoyPort2Device", opt_joyport_type);
#endif
      }
   }
   /* Other than a joystick, set only cur_port */
   else if (opt_joyport_type > 1 && cur_port < 3)
   {
      if (opt_joyport_type_prev != opt_joyport_type || cur_port_prev != cur_port)
      {
         opt_joyport_type_prev = opt_joyport_type;
         cur_port_prev = cur_port;

#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XVIC__)
         if (cur_port == 2)
         {
            resources_set_int("JoyPort1Device", 1);
            resources_set_int("JoyPort2Device", opt_joyport_type);
         }
         else
         {
            resources_set_int("JoyPort2Device", 1);
            resources_set_int("JoyPort1Device", opt_joyport_type);
         }
#else
         resources_set_int("JoyPort1Device", opt_joyport_type);
#endif
      }

      int8_t j = cur_port - 1;
      int8_t retro_j = 0;
      static float mouse_multiplier = 1;
      static int dpadmouse_speed[4] = {0};
      static int dpadmouse_pressed[4] = {0};
#ifdef MOUSE_DPAD_ACCEL
      long now = 0;
      now = retro_ticks() / 1000;
#endif

      int16_t retro_mouse_x[4] = {0}, retro_mouse_y[4] = {0};
      int16_t retro_mouse_l[4] = {0}, retro_mouse_r[4] = {0}, retro_mouse_m[4] = {0};
      static uint8_t retro_mouse_l_state[4] = {0}, retro_mouse_r_state[4] = {0}, retro_mouse_m_state[4] = {0};

      /* keep track of analog mouse motion with high precision, to allow fine-grained speed changes */
      static float sub_pixel_remainder_leftstick_x[4] = {0, 0};
      static float sub_pixel_remainder_leftstick_y[4] = {0, 0};
      static float sub_pixel_remainder_rightstick_x[4] = {0, 0};
      static float sub_pixel_remainder_rightstick_y[4] = {0, 0};

      int analog_stick[2] = {0};
      int analog_deadzone = opt_analogmouse_deadzone * 32768.0f / 100.0f;

      /* Paddles (opt_joyport_type = 2) share the same joyport, but are meant for 2 players.
       * Therefore treat RetroPort0 vertical axis as RetroPort1 horizontal axis, and second fire as RetroPort1 fire.
       * Bypassed VICE limitation of one mouse when using paddles for maximum 4 paddles. */
      unsigned retro_j_max = 2;
      if (opt_joyport_type == 2)
         retro_j_max = 4;

      /* Joypad and mouse control only when VKBD is closed */
      if (!retro_vkbd)
      {
         for (retro_j = 0; retro_j < retro_j_max; retro_j++)
         {
            /* Button rotation only with joypad, and buttons only when no keyboard mappings */
            uint8_t button_l = RETRO_DEVICE_ID_JOYPAD_B;
            uint8_t button_r = RETRO_DEVICE_ID_JOYPAD_A;

            if (retro_devices[retro_j] == RETRO_DEVICE_JOYPAD
                  && (opt_retropad_options == RETROPAD_OPTIONS_ROTATE || opt_retropad_options == RETROPAD_OPTIONS_ROTATE_JUMP))
            {
               button_l = RETRO_DEVICE_ID_JOYPAD_Y;
               button_r = RETRO_DEVICE_ID_JOYPAD_B;
            }

            retro_mouse_l[retro_j] = !mapper_keys[button_l] && (joypad_bits[retro_j] & (1 << button_l)) ? 1 : 0;
            retro_mouse_r[retro_j] = !mapper_keys[button_r] && (joypad_bits[retro_j] & (1 << button_r)) ? 1 : 0;

            /* Real mouse buttons */
            if (!retro_mouse_l[retro_j] && !retro_mouse_r[retro_j] && !retro_mouse_m[retro_j])
            {
               retro_mouse_l[retro_j] = input_state_cb(retro_j, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
               retro_mouse_r[retro_j] = input_state_cb(retro_j, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
               retro_mouse_m[retro_j] = input_state_cb(retro_j, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE);
            }
         }

         if (retro_mouse_discard)
         {
            retro_mouse_discard--;
            retro_mouse_l[0] = 0;
         }

         /* Joypad movement */
         for (retro_j = 0; retro_j < retro_j_max; retro_j++)
         {
            /* No digital mouse if any D-Pad buttons are pressing keyboard keys */
            if (     mapper_keys[RETRO_DEVICE_ID_JOYPAD_UP]
                  || mapper_keys[RETRO_DEVICE_ID_JOYPAD_DOWN]
                  || mapper_keys[RETRO_DEVICE_ID_JOYPAD_LEFT]
                  || mapper_keys[RETRO_DEVICE_ID_JOYPAD_RIGHT])
               continue;

            /* Digital mouse speed modifiers */
            if (!dpadmouse_pressed[retro_j])
#ifdef MOUSE_DPAD_ACCEL
               dpadmouse_speed[retro_j] = opt_dpadmouse_speed / 4;
#else
               dpadmouse_speed[retro_j] = opt_dpadmouse_speed;
#endif

            if (mouse_speed[retro_j] & MOUSE_SPEED_FASTER)
               dpadmouse_speed[retro_j] = dpadmouse_speed[retro_j] + 2;
            if (mouse_speed[retro_j] & MOUSE_SPEED_SLOWER)
               dpadmouse_speed[retro_j] = dpadmouse_speed[retro_j] - 4;

#ifdef MOUSE_DPAD_ACCEL
            /* Digital mouse acceleration */
            if (dpadmouse_pressed[retro_j] && (now - dpadmouse_pressed[retro_j] > 200))
            {
               dpadmouse_speed[retro_j]++;
               dpadmouse_pressed[retro_j] = now;
            }
#endif

            /* Digital mouse speed limits */
            if (dpadmouse_speed[retro_j] < 1)
               dpadmouse_speed[retro_j] = 1;
            if (dpadmouse_speed[retro_j] > opt_dpadmouse_speed && !(mouse_speed[j] & MOUSE_SPEED_FASTER))
               dpadmouse_speed[retro_j] = opt_dpadmouse_speed;
            if (dpadmouse_speed[retro_j] > 20)
               dpadmouse_speed[retro_j] = 20;

            if (joypad_bits[retro_j] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
               retro_mouse_x[retro_j] += dpadmouse_speed[retro_j];
            else if (joypad_bits[retro_j] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
               retro_mouse_x[retro_j] -= dpadmouse_speed[retro_j];

            if (opt_joyport_type != 2)
            {
               if (joypad_bits[retro_j] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
                  retro_mouse_y[retro_j] += dpadmouse_speed[retro_j];
               else if (joypad_bits[retro_j] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
                  retro_mouse_y[retro_j] -= dpadmouse_speed[retro_j];
            }

#ifdef MOUSE_DPAD_ACCEL
            /* Acceleration timestamps */
            if ((retro_mouse_x[retro_j] != 0 || retro_mouse_y[retro_j] != 0) && !dpadmouse_pressed[retro_j])
               dpadmouse_pressed[retro_j] = now;
            else if ((retro_mouse_x[retro_j] == 0 && retro_mouse_y[retro_j] == 0) && dpadmouse_pressed[retro_j])
               dpadmouse_pressed[retro_j] = 0;
#endif
         }
      }

      for (retro_j = 0; retro_j < retro_j_max; retro_j++)
      {
         /* Analog sticks depending on core options */
         if (  (opt_analogmouse == 1 || opt_analogmouse == 3)
            || (  !mapper_keys[RETRO_DEVICE_ID_JOYPAD_LR]
               && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_LL]
               && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_LD]
               && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_LU])
            )
         {
            /* Left analog movement */
            analog_stick[0] = joypad_axis[retro_j][AXIS_LX];
            analog_stick[1] = joypad_axis[retro_j][AXIS_LY];

            float analog_mag = sqrt((analog_stick[0] * analog_stick[0]) + (analog_stick[1] * analog_stick[1]));
            if (analog_mag <= analog_deadzone)
               analog_stick[0] = analog_stick[1] = 0;

            /* Analog stick speed modifiers */
            mouse_multiplier = 1;
            if (mouse_speed[retro_j] & MOUSE_SPEED_FASTER)
               mouse_multiplier = mouse_multiplier * MOUSE_SPEED_FAST;
            if (mouse_speed[retro_j] & MOUSE_SPEED_SLOWER)
               mouse_multiplier = mouse_multiplier / MOUSE_SPEED_SLOW;

            retro_mouse_x[retro_j] += process_analogmouse(analog_stick[0], analog_deadzone, analog_mag, opt_analogmouse_speed_left, mouse_multiplier, &(sub_pixel_remainder_leftstick_x[retro_j]));
            retro_mouse_y[retro_j] += process_analogmouse(analog_stick[1], analog_deadzone, analog_mag, opt_analogmouse_speed_left, mouse_multiplier, &(sub_pixel_remainder_leftstick_y[retro_j]));
         }

         if (  (opt_analogmouse == 2 || opt_analogmouse == 3)
            || (  !mapper_keys[RETRO_DEVICE_ID_JOYPAD_RR]
               && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_RL]
               && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_RD]
               && !mapper_keys[RETRO_DEVICE_ID_JOYPAD_RU])
            )
         {
            /* Right analog movement */
            analog_stick[0] = joypad_axis[retro_j][AXIS_RX];
            analog_stick[1] = joypad_axis[retro_j][AXIS_RY];

            float analog_mag = sqrt((analog_stick[0] * analog_stick[0]) + (analog_stick[1] * analog_stick[1]));
            if (analog_mag <= analog_deadzone)
               analog_stick[0] = analog_stick[1] = 0;

            /* Analog stick speed modifiers */
            mouse_multiplier = 1;
            if (mouse_speed[retro_j] & MOUSE_SPEED_FASTER)
               mouse_multiplier = mouse_multiplier * MOUSE_SPEED_FAST;
            if (mouse_speed[retro_j] & MOUSE_SPEED_SLOWER)
               mouse_multiplier = mouse_multiplier / MOUSE_SPEED_SLOW;

            retro_mouse_x[retro_j] += process_analogmouse(analog_stick[0], analog_deadzone, analog_mag, opt_analogmouse_speed_right, mouse_multiplier, &(sub_pixel_remainder_rightstick_x[retro_j]));
            retro_mouse_y[retro_j] += process_analogmouse(analog_stick[1], analog_deadzone, analog_mag, opt_analogmouse_speed_right, mouse_multiplier, &(sub_pixel_remainder_rightstick_y[retro_j]));
         }
      }

      if (!retro_vkbd)
      {
         /* Real mouse movement only without VKBD */
         for (retro_j = 0; retro_j < retro_j_max; retro_j++)
         {
            if (!retro_mouse_x[retro_j] && !retro_mouse_y[retro_j])
            {
               retro_mouse_x[retro_j] = input_state_cb(retro_j, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
               retro_mouse_y[retro_j] = input_state_cb(retro_j, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
            }
         }
      }

      /* Ports 1 & 2 to VICE */
      for (retro_j = 0; retro_j < 2; retro_j++)
      {
         int vice_j  = 0;
         int mouse_x = 0;
         int mouse_y = 0;
         int mouse_l = 0;
         int mouse_r = 0;
         int mouse_m = 0;

         j = retro_j + (cur_port - 1);
         j = (j > 1) ? 0 : j;
         vice_j = j + 1;
         vice_j = (vice_j > 2) ? 0 : vice_j;

         if (opt_joyport_type == 2)
         {
            if (retro_j == 0)
            {
               mouse_x = retro_mouse_x[retro_j];
               mouse_y = -retro_mouse_x[retro_j + 1];
               mouse_l = retro_mouse_l[retro_j];
               mouse_r = retro_mouse_l[retro_j + 1];
            }
            else if (retro_j == 1)
            {
               mouse_x = retro_mouse_x[retro_j + 1];
               mouse_y = -retro_mouse_x[retro_j + 2];
               mouse_l = retro_mouse_l[retro_j + 1];
               mouse_r = retro_mouse_l[retro_j + 2];
            }
         }
         else
         {
            mouse_x = retro_mouse_x[retro_j];
            mouse_y = retro_mouse_y[retro_j];
            mouse_l = retro_mouse_l[retro_j];
            mouse_r = retro_mouse_r[retro_j];
            mouse_m = retro_mouse_m[retro_j];
         }

         if (!mouse_l && !mouse_r && !mouse_m && !mouse_x && !mouse_y)
            mouse_value[vice_j] = 0;

         /* Mouse buttons */
         /* Left */
         if (mouse_l && !retro_mouse_l_state[retro_j])
         {
            retro_mouse_l_state[retro_j] = mouse_l;
            mouse_value[vice_j] |= JOYPAD_FIRE;
            if (opt_joyport_type == 2 && retro_j == 1)
               joystick_set_value_or(vice_j - 1, JOYPAD_W);
            else
               mouse_button(0, mouse_l);
         }
         else if (!mouse_l && retro_mouse_l_state[retro_j])
         {
            retro_mouse_l_state[retro_j] = mouse_l;
            mouse_value[vice_j] &= ~JOYPAD_FIRE;
            if (opt_joyport_type == 2 && retro_j == 1)
               joystick_set_value_and(vice_j - 1, ~JOYPAD_W);
            else
               mouse_button(0, mouse_l);
         }

         /* Right */
         if (mouse_r && !retro_mouse_r_state[retro_j])
         {
            retro_mouse_r_state[retro_j] = mouse_r;
            mouse_value[vice_j] |= JOYPAD_FIRE2;
            if (opt_joyport_type == 2 && retro_j == 1)
               joystick_set_value_or(vice_j - 1, JOYPAD_E);
            else
               mouse_button(1, mouse_r);
         }
         else if (!mouse_r && retro_mouse_r_state[retro_j])
         {
            retro_mouse_r_state[retro_j] = mouse_r;
            mouse_value[vice_j] &= ~JOYPAD_FIRE2;
            if (opt_joyport_type == 2 && retro_j == 1)
               joystick_set_value_and(vice_j - 1, ~JOYPAD_E);
            else
               mouse_button(1, mouse_r);
         }

         /* Middle */
         if (mouse_m && !retro_mouse_m_state[retro_j])
         {
            retro_mouse_m_state[retro_j] = mouse_m;
            mouse_value[vice_j] |= JOYPAD_FIRE3;
            mouse_button(2, mouse_m);
         }
         else if (!mouse_m && retro_mouse_m_state[retro_j])
         {
            retro_mouse_m_state[retro_j] = mouse_m;
            mouse_value[vice_j] &= ~JOYPAD_FIRE3;
            mouse_button(2, mouse_m);
         }

         /* Movement */
         if (mouse_x || mouse_y)
         {
            if (mouse_y < 0 && !(mouse_value[vice_j] & JOYPAD_N))
               mouse_value[vice_j] |= JOYPAD_N;
            else if (mouse_y > -1 && mouse_value[vice_j] & JOYPAD_N)
               mouse_value[vice_j] &= ~JOYPAD_N;

            if (mouse_y > 0 && !(mouse_value[vice_j] & JOYPAD_S))
               mouse_value[vice_j] |= JOYPAD_S;
            else if (mouse_y < 1 && mouse_value[vice_j] & JOYPAD_S)
               mouse_value[vice_j] &= ~JOYPAD_S;

            if (mouse_x < 0 && !(mouse_value[vice_j] & JOYPAD_W))
               mouse_value[vice_j] |= JOYPAD_W;
            else if (mouse_x > -1 && mouse_value[vice_j] & JOYPAD_W)
               mouse_value[vice_j] &= ~JOYPAD_W;

            if (mouse_x > 0 && !(mouse_value[vice_j] & JOYPAD_E))
               mouse_value[vice_j] |= JOYPAD_E;
            else if (mouse_x < 1 && mouse_value[vice_j] & JOYPAD_E)
               mouse_value[vice_j] &= ~JOYPAD_E;

            retro_mouse_move(j, mouse_x, mouse_y);
         }
      }
   }
}

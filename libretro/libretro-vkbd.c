#include "libretro-core.h"
#include "libretro-graph.h"
#include "libretro-vkbd.h"
#include "libretro-mapper.h"

#include "kbd.h"
#include "keyboard.h"
#include "keymap.h"

bool retro_vkbd = false;
static bool retro_vkbd_page = false;
static bool retro_vkbd_transparent = true;
signed char retro_vkbd_ready = 0;
static int vkflag[10] = {0};

#ifdef __X128__
extern int c128_vdc;
extern bool c128_capslock;
#endif

#define POINTER_DEBUG 0
#if POINTER_DEBUG
static int pointer_x = 0;
static int pointer_y = 0;
#endif
static int last_pointer_x = 0;
static int last_pointer_y = 0;

/* VKBD starting point: 10x3 == f7 */
static int vkey_pos_x = 10;
#ifdef __X128__
static int vkey_pos_y = 4;
#else
static int vkey_pos_y = 3;
#endif
static int vkbd_x_min = 0;
static int vkbd_x_max = 0;
static int vkbd_y_min = 0;
static int vkbd_y_max = 0;

long vkbd_mapping_active = 0;
static int vkbd_mapping_key = 0;
static bool vkbd_mapping_activated = false;

/* VKBD_MIN_HOLDING_TIME: Hold a direction longer than this and automatic movement sets in */
/* VKBD_MOVE_DELAY: Delay between automatic movement from button to button */
#define VKBD_MIN_HOLDING_TIME 200
#define VKBD_MOVE_DELAY 50
static bool let_go_of_direction = true;
static long last_move_time = 0;
static long last_press_time = 0;

/* VKBD_STICKY_HOLDING_TIME: Button press longer than this triggers sticky key */
#define VKBD_STICKY_HOLDING_TIME 1000
static bool let_go_of_button = true;
static long last_press_time_button = 0;
static int vkey_pressed = -1;
static int vkey_sticky  = -1;
static int vkey_sticky1 = -1;
static int vkey_sticky2 = -1;

static long last_vkey_pressed_time = 0;
static int last_vkey_pressed = -1;
static int vkey_sticky1_release = 0;
static int vkey_sticky2_release = 0;

typedef struct
{
   char normal[10];
   char shift[10];
   int value;
} retro_vkeys;

static const retro_vkeys vkeys[VKBDX * VKBDY * 2] =
{
#ifdef __X128__

   /* C128 extra */
   { "ESC","ESC",RETROK_F1 },
   { "TAB","TAB",RETROK_F2 },
   { "ALT","ALT",RETROK_F3 },
   { "CAPS\1LOCK","CAPS\1LOCK",RETROK_F4 },
   { {}   ,{}   ,-1 },
   { "HELP","HELP",RETROK_F5 },
   { "LINE\1FEED","LINE\1FEED",RETROK_F6 },
   { "4080\1DSPL","4080\1DSPL",RETROK_F7 },
   { "NO\1SCRL","NO\1SCRL",RETROK_F8 },
   { {}   ,{}   ,-1 },
   { {15} ,{15} ,VKBD_NUMPAD },

   /* 0 */
   { "1"  ,"!"  ,RETROK_1 },
   { "2"  ,"\"" ,RETROK_2 },
   { "3"  ,"#"  ,RETROK_3 },
   { "4"  ,"$"  ,RETROK_4 },
   { "5"  ,"%"  ,RETROK_5 },
   { "6"  ,"&"  ,RETROK_6 },
   { "7"  ,"'"  ,RETROK_7 },
   { "8"  ,"("  ,RETROK_8 },
   { "9"  ,")"  ,RETROK_9 },
   { "0"  ,"0"  ,RETROK_0 },
   { "f1" ,"f2" ,RETROK_F9 },

   /* 11 */
   { "Q"  ,"Q"  ,RETROK_q },
   { "W"  ,"W"  ,RETROK_w },
   { "E"  ,"E"  ,RETROK_e },
   { "R"  ,"R"  ,RETROK_r },
   { "T"  ,"T"  ,RETROK_t },
   { "Y"  ,"Y"  ,RETROK_y },
   { "U"  ,"U"  ,RETROK_u },
   { "I"  ,"I"  ,RETROK_i },
   { "O"  ,"O"  ,RETROK_o },
   { "P"  ,"P"  ,RETROK_p },
   { "f3" ,"f4" ,RETROK_F10 },

   /* 22 */
   { "A"  ,"A"  ,RETROK_a },
   { "S"  ,"S"  ,RETROK_s },
   { "D"  ,"D"  ,RETROK_d },
   { "F"  ,"F"  ,RETROK_f },
   { "G"  ,"G"  ,RETROK_g },
   { "H"  ,"H"  ,RETROK_h },
   { "J"  ,"J"  ,RETROK_j },
   { "K"  ,"K"  ,RETROK_k },
   { "L"  ,"L"  ,RETROK_l },
   { {31} ,{31} ,RETROK_INSERT }, /* £ */
   { "f5" ,"f6" ,RETROK_F11 },

   /* 33 */
   { "Z"  ,"Z"  ,RETROK_z },
   { "X"  ,"X"  ,RETROK_x },
   { "C"  ,"C"  ,RETROK_c },
   { "V"  ,"V"  ,RETROK_v },
   { "B"  ,"B"  ,RETROK_b },
   { "N"  ,"N"  ,RETROK_n },
   { "M"  ,"M"  ,RETROK_m },
   { ","  ,"<"  ,RETROK_COMMA },
   { "."  ,">"  ,RETROK_PERIOD },
   { "/"  ,"?"  ,RETROK_SLASH },
   { "f7" ,"f8" ,RETROK_F12 },

   /* 44 */
   { {25} ,{25} ,RETROK_BACKQUOTE }, /* Left arrow */
   { "CTRL","CTRL",RETROK_TAB },
   { "+"  ,"+"  ,RETROK_MINUS },
   { "-"  ,"-"  ,RETROK_EQUALS },
   { "@"  ,"@"  ,RETROK_LEFTBRACKET },
   { "*"  ,"*"  ,RETROK_RIGHTBRACKET },
   { {26} ,{6}  ,RETROK_DELETE }, /* Up arrow / Pi */
   { ":"  ,"["  ,RETROK_SEMICOLON },
   { ";"  ,"]"  ,RETROK_QUOTE },
   { "="  ,"="  ,RETROK_BACKSLASH },
   { "STBR","SVDS",VKBD_STATUSBAR_SAVEDISK }, /* Statusbar / Save disk */

   /* 55 */
   { {24} ,{24} ,RETROK_LCTRL },
   { "RUN\1STOP","RUN\1STOP",RETROK_ESCAPE },
   { "SHFT\1LOCK","SHFT\1LOCK",VKBD_SHIFTLOCK },
   { {'S','H','F','T','\1',27},{'S','H','F','T','\1',27},RETROK_LSHIFT },
   { {'S','H','F','T','\1',' ',' ',' ',29},{'S','H','F','T','\1',' ',' ',' ',29},RETROK_RSHIFT },
   { "RSTR","RSTR",RETROK_PAGEUP },
   { "CLR\1HOME","CLR\1HOME",RETROK_HOME },
   { "INST\1DEL","INST\1DEL",RETROK_BACKSPACE },
   { {30} ,{30} ,RETROK_UP },
   { {16} ,{16} ,RETROK_RETURN },
   { "JOYP","ASPR",VKBD_JOYPORT_ASPECT }, /* Switch joyport / Toggle aspect ratio */

   /* 66 */
   { {17} ,{17} ,VKBD_RESET },
   { {19} ,{19} ,VKBD_DATASETTE_RESET },
   { {20} ,{20} ,VKBD_DATASETTE_START },
   { {21} ,{21} ,VKBD_DATASETTE_RWD },
   { {22} ,{22} ,VKBD_DATASETTE_FWD },
   { {23} ,{23} ,VKBD_DATASETTE_STOP },
   { {18} ,{18} ,RETROK_SPACE },
   { {27} ,{27} ,RETROK_LEFT },
   { {28} ,{28} ,RETROK_DOWN },
   { {29} ,{29} ,RETROK_RIGHT },
   { "TRBF","CROP",VKBD_TURBO_CROP }, /* Toggle turbo fire / Toggle crop mode */

   /* C128 extra */
   { "ESC","ESC",RETROK_F1 },
   { "TAB","TAB",RETROK_F2 },
   { "ALT","ALT",RETROK_F3 },
   { "CAPS\1LOCK","CAPS\1LOCK",RETROK_F4 },
   { {}   ,{}   ,-1 },
   { "HELP","HELP",RETROK_F5 },
   { "LINE\1FEED","LINE\1FEED",RETROK_F6 },
   { "4080\1DSPL","4080\1DSPL",RETROK_F7 },
   { "NO\1SCRL","NO\1SCRL",RETROK_F8 },
   { {}   ,{}   ,-1 },
   { {15} ,{15} ,VKBD_NUMPAD },

   /* 0 */
   { {15,'1'},{15,'1'},RETROK_KP1 },
   { {15,'2'},{15,'2'},RETROK_KP2 },
   { {15,'3'},{15,'3'},RETROK_KP3 },
   { {15,'4'},{15,'4'},RETROK_KP4 },
   { {15,'5'},{15,'5'},RETROK_KP5 },
   { {15,'6'},{15,'6'},RETROK_KP6 },
   { {15,'7'},{15,'7'},RETROK_KP7 },
   { {15,'8'},{15,'8'},RETROK_KP8 },
   { {15,'9'},{15,'9'},RETROK_KP9 },
   { {15,'0'},{15,'0'},RETROK_KP0 },
   { "f1" ,"f2" ,RETROK_F9 },

   /* 11 */
   { "Q"  ,"Q"  ,RETROK_q },
   { "W"  ,"W"  ,RETROK_w },
   { "E"  ,"E"  ,RETROK_e },
   { "R"  ,"R"  ,RETROK_r },
   { "T"  ,"T"  ,RETROK_t },
   { "Y"  ,"Y"  ,RETROK_y },
   { "U"  ,"U"  ,RETROK_u },
   { "I"  ,"I"  ,RETROK_i },
   { "O"  ,"O"  ,RETROK_o },
   { "P"  ,"P"  ,RETROK_p },
   { "f3" ,"f4" ,RETROK_F10 },

   /* 22 */
   { "A"  ,"A"  ,RETROK_a },
   { "S"  ,"S"  ,RETROK_s },
   { "D"  ,"D"  ,RETROK_d },
   { "F"  ,"F"  ,RETROK_f },
   { "G"  ,"G"  ,RETROK_g },
   { "H"  ,"H"  ,RETROK_h },
   { "J"  ,"J"  ,RETROK_j },
   { "K"  ,"K"  ,RETROK_k },
   { "L"  ,"L"  ,RETROK_l },
   { {31} ,{31} ,RETROK_INSERT }, /* £ */
   { "f5" ,"f6" ,RETROK_F11 },

   /* 33 */
   { "Z"  ,"Z"  ,RETROK_z },
   { "X"  ,"X"  ,RETROK_x },
   { "C"  ,"C"  ,RETROK_c },
   { "V"  ,"V"  ,RETROK_v },
   { "B"  ,"B"  ,RETROK_b },
   { "N"  ,"N"  ,RETROK_n },
   { "M"  ,"M"  ,RETROK_m },
   { ","  ,"<"  ,RETROK_COMMA },
   { {15,'.'},{15,'.'},RETROK_KP_PERIOD },
   { "/"  ,"?"  ,RETROK_SLASH },
   { "f7" ,"f8" ,RETROK_F12 },

   /* 44 */
   { {25} ,{25} ,RETROK_BACKQUOTE }, /* Left arrow */
   { "CTRL","CTRL",RETROK_TAB },
   { {15,'+'},{15,'+'},RETROK_KP_PLUS },
   { {15,'-'},{15,'-'},RETROK_KP_MINUS },
   { "@"  ,"@"  ,RETROK_LEFTBRACKET },
   { "*"  ,"*"  ,RETROK_RIGHTBRACKET },
   { {26} ,{6}  ,RETROK_DELETE }, /* Up arrow / Pi */
   { ":"  ,"["  ,RETROK_SEMICOLON },
   { ";"  ,"]"  ,RETROK_QUOTE },
   { "="  ,"="  ,RETROK_BACKSLASH },
   { "STBR","SVDS",VKBD_STATUSBAR_SAVEDISK }, /* Statusbar / Save disk */

   /* 55 */
   { {24} ,{24} ,RETROK_LCTRL },
   { "RUN\1STOP","RUN\1STOP",RETROK_ESCAPE },
   { "SHFT\1LOCK","SHFT\1LOCK",VKBD_SHIFTLOCK },
   { {'S','H','F','T','\1',27},{'S','H','F','T','\1',27},RETROK_LSHIFT },
   { {'S','H','F','T','\1',' ',' ',' ',29},{'S','H','F','T','\1',' ',' ',' ',29},RETROK_RSHIFT },
   { "RSTR","RSTR",RETROK_PAGEUP },
   { "CLR\1HOME","CLR\1HOME",RETROK_HOME },
   { "INST\1DEL","INST\1DEL",RETROK_BACKSPACE },
   { {30} ,{30} ,RETROK_UP },
   { {15,16},{15,16},RETROK_KP_ENTER },
   { "JOYP","ASPR",VKBD_JOYPORT_ASPECT }, /* Switch joyport / Toggle aspect ratio */

   /* 66 */
   { {17} ,{17} ,VKBD_RESET },
   { {19} ,{19} ,VKBD_DATASETTE_RESET },
   { {20} ,{20} ,VKBD_DATASETTE_START },
   { {21} ,{21} ,VKBD_DATASETTE_RWD },
   { {22} ,{22} ,VKBD_DATASETTE_FWD },
   { {23} ,{23} ,VKBD_DATASETTE_STOP },
   { {18} ,{18} ,RETROK_SPACE },
   { {27} ,{27} ,RETROK_LEFT },
   { {28} ,{28} ,RETROK_DOWN },
   { {29} ,{29} ,RETROK_RIGHT },
   { "TRBF","CROP",VKBD_TURBO_CROP }, /* Toggle turbo fire / Toggle crop mode */

#else

   /* 0 */
   { "1"  ,"!"  ,RETROK_1 },
   { "2"  ,"\"" ,RETROK_2 },
   { "3"  ,"#"  ,RETROK_3 },
   { "4"  ,"$"  ,RETROK_4 },
   { "5"  ,"%"  ,RETROK_5 },
   { "6"  ,"&"  ,RETROK_6 },
   { "7"  ,"'"  ,RETROK_7 },
   { "8"  ,"("  ,RETROK_8 },
   { "9"  ,")"  ,RETROK_9 },
#ifdef __XPLUS4__
   { "0"  ,{26} ,RETROK_0 },
#else
   { "0"  ,"0"  ,RETROK_0 },
#endif
#ifdef __XPLUS4__
   { "F1" ,"F4" ,RETROK_F1 },
#else
   { "f1" ,"f2" ,RETROK_F1 },
#endif

   /* 11 */
   { "Q"  ,"Q"  ,RETROK_q },
   { "W"  ,"W"  ,RETROK_w },
   { "E"  ,"E"  ,RETROK_e },
   { "R"  ,"R"  ,RETROK_r },
   { "T"  ,"T"  ,RETROK_t },
   { "Y"  ,"Y"  ,RETROK_y },
   { "U"  ,"U"  ,RETROK_u },
   { "I"  ,"I"  ,RETROK_i },
   { "O"  ,"O"  ,RETROK_o },
   { "P"  ,"P"  ,RETROK_p },
#ifdef __XPLUS4__
   { "F2" ,"F5" ,RETROK_F2 },
#else
   { "f3" ,"f4" ,RETROK_F3 },
#endif

   /* 22 */
   { "A"  ,"A"  ,RETROK_a },
   { "S"  ,"S"  ,RETROK_s },
   { "D"  ,"D"  ,RETROK_d },
   { "F"  ,"F"  ,RETROK_f },
   { "G"  ,"G"  ,RETROK_g },
   { "H"  ,"H"  ,RETROK_h },
   { "J"  ,"J"  ,RETROK_j },
   { "K"  ,"K"  ,RETROK_k },
   { "L"  ,"L"  ,RETROK_l },
   { {31} ,{31} ,RETROK_INSERT }, /* £ */
#ifdef __XPLUS4__
   { "F3" ,"F6" ,RETROK_F3 },
#else
   { "f5" ,"f6" ,RETROK_F5 },
#endif

   /* 33 */
   { "Z"  ,"Z"  ,RETROK_z },
   { "X"  ,"X"  ,RETROK_x },
   { "C"  ,"C"  ,RETROK_c },
   { "V"  ,"V"  ,RETROK_v },
   { "B"  ,"B"  ,RETROK_b },
   { "N"  ,"N"  ,RETROK_n },
   { "M"  ,"M"  ,RETROK_m },
   { ","  ,"<"  ,RETROK_COMMA },
   { "."  ,">"  ,RETROK_PERIOD },
   { "/"  ,"?"  ,RETROK_SLASH },
#ifdef __XPLUS4__
   { "HELP","F7" ,RETROK_F8 },
#else
   { "f7" ,"f8" ,RETROK_F7 },
#endif

   /* 44 */
#ifdef __XPLUS4__
   { "ESC","ESC",RETROK_BACKQUOTE },
#else
   { {25} ,{25} ,RETROK_BACKQUOTE }, /* Left arrow */
#endif
   { "CTRL","CTRL",RETROK_TAB },
   { "+"  ,"+"  ,RETROK_MINUS },
   { "-"  ,"-"  ,RETROK_EQUALS },
   { "@"  ,"@"  ,RETROK_LEFTBRACKET },
   { "*"  ,"*"  ,RETROK_RIGHTBRACKET },
   { {26} ,{6}  ,RETROK_DELETE }, /* Up arrow / Pi */
   { ":"  ,"["  ,RETROK_SEMICOLON },
   { ";"  ,"]"  ,RETROK_QUOTE },
   { "="  ,"="  ,RETROK_BACKSLASH },
   { "STBR","SVDS",VKBD_STATUSBAR_SAVEDISK }, /* Statusbar / Save disk */

   /* 55 */
   { {24} ,{24} ,RETROK_LCTRL },
   { "RUN\1STOP","RUN\1STOP",RETROK_ESCAPE },
   { "SHFT\1LOCK","SHFT\1LOCK",VKBD_SHIFTLOCK },
   { {'S','H','F','T','\1',27},{'S','H','F','T','\1',27},RETROK_LSHIFT },
   { {'S','H','F','T','\1',' ',' ',' ',29},{'S','H','F','T','\1',' ',' ',' ',29},RETROK_RSHIFT },
   { "RSTR","RSTR",RETROK_PAGEUP },
   { "CLR\1HOME","CLR\1HOME",RETROK_HOME },
   { "INST\1DEL","INST\1DEL",RETROK_BACKSPACE },
   { {30} ,{30} ,RETROK_UP },
   { {16} ,{16} ,RETROK_RETURN },
   { "JOYP","ASPR",VKBD_JOYPORT_ASPECT }, /* Switch joyport / Toggle aspect ratio */

   /* 66 */
   { {17} ,{17} ,VKBD_RESET },
   { {19} ,{19} ,VKBD_DATASETTE_RESET },
   { {20} ,{20} ,VKBD_DATASETTE_START },
   { {21} ,{21} ,VKBD_DATASETTE_RWD },
   { {22} ,{22} ,VKBD_DATASETTE_FWD },
   { {23} ,{23} ,VKBD_DATASETTE_STOP },
   { {18} ,{18} ,RETROK_SPACE },
   { {27} ,{27} ,RETROK_LEFT },
   { {28} ,{28} ,RETROK_DOWN },
   { {29} ,{29} ,RETROK_RIGHT },
   { "TRBF","CROP",VKBD_TURBO_CROP }, /* Toggle turbo fire / Toggle crop mode */

#endif
};

static int BKG_PADDING(const char* str)
{
   unsigned font_width_default = (retrow >= 704) ? 12 : 6;
   unsigned len = strlen(str);
   unsigned padding = 0;
   unsigned i = 0;

   if (strchr(str, '\1'))
      len = 4;

   for (i = 0; i < len; i++)
   {
      unsigned font_width;

      if (str[i] >= 'a' && str[i] <= 'z')
         font_width = font_width_default - 2;
      else
         font_width = font_width_default;

      padding -= (font_width / 2);
   }

   if (retrow > 704)
      padding--;

   return padding;
}

/* Alternate color keys */
static const int vkbd_alt_keys[] =
{
      RETROK_F1, RETROK_F3, RETROK_F5, RETROK_F7,
#if defined(__XPLUS4__)
      RETROK_F2, RETROK_F8,
#elif defined(__X128__)
      RETROK_F2, RETROK_F4, RETROK_F6, RETROK_F8,
      RETROK_F9, RETROK_F10, RETROK_F11, RETROK_F12,
#endif
};
static const int vkbd_alt_keys_len = sizeof(vkbd_alt_keys) / sizeof(vkbd_alt_keys[0]);

/* Extra color keys */
static const int vkbd_extra_keys[] =
{
   VKBD_NUMPAD, VKBD_RESET, VKBD_STATUSBAR_SAVEDISK, VKBD_JOYPORT_ASPECT, VKBD_TURBO_CROP
};
static const int vkbd_extra_keys_len = sizeof(vkbd_extra_keys) / sizeof(vkbd_extra_keys[0]);

/* Datasette color keys */
static const int vkbd_datasette_keys[] =
{
   VKBD_DATASETTE_STOP, VKBD_DATASETTE_START, VKBD_DATASETTE_FWD, VKBD_DATASETTE_RWD, VKBD_DATASETTE_RESET
};
static const int vkbd_datasette_keys_len = sizeof(vkbd_datasette_keys) / sizeof(vkbd_datasette_keys[0]);

void print_vkbd(void)
{
   libretro_graph_alpha_t ALPHA      = opt_vkbd_alpha;
   libretro_graph_alpha_t BKG_ALPHA  = ALPHA;
   libretro_graph_alpha_t BRD_ALPHA  = opt_vkbd_dim_alpha;
   long now                          = retro_ticks() / 1000;
   bool shifted                      = false;
   bool text_outline                 = false;
   int page                          = (retro_vkbd_page) ? VKBDX * VKBDY : 0;
   int x                             = 0;
   int y                             = 0;

   int XKEY                          = 0;
   int YKEY                          = 0;
   int XTEXT                         = 0;
   int YTEXT                         = 0;
   int XOFFSET                       = 0;
   int XPADDING                      = 0;
   int YOFFSET                       = 0;
   int YPADDING                      = 0;
   int XKEYSPACING                   = 1;
   int YKEYSPACING                   = 1;

   int BKG_PADDING_X_DEFAULT         = -3;
   int BKG_PADDING_Y_DEFAULT         = -3;
   int BKG_PADDING_X                 = 0;
   int BKG_PADDING_Y                 = 0;
   int BKG_COLOR                     = 0;
   int BKG_COLOR_NORMAL              = 0;
   int BKG_COLOR_ALT                 = 0;
   int BKG_COLOR_EXTRA               = 0;
   int BKG_COLOR_TAPE                = 0;
   int BKG_COLOR_SEL                 = 0;
   int BKG_COLOR_ACTIVE              = 0;
   int BRD_COLOR                     = 0;

   int FONT_MAX                      = 10;
   int FONT_WIDTH                    = 1;
   int FONT_HEIGHT                   = 1;
   int FONT_COLOR                    = 0;
   int FONT_COLOR_NORMAL             = 0;
   int FONT_COLOR_SEL                = 0;

   char string[11]                   = {0};
   unsigned theme                    = opt_vkbd_theme;
   if (theme & 0x80)
   {
      text_outline = true;
      theme &= ~0x80;
   }

   if (!theme)
   {
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
      unsigned model = vice_opt.Model;
      if (request_model_set > -1 && request_model_set != model)
         model = request_model_set;

      switch (model)
      {
         case C64MODEL_C64C_PAL:
         case C64MODEL_C64C_NTSC:
         case C64MODEL_C64_GS:
            theme = 2;
            break;
         default:
            theme = 1;
            break;
      }
#elif defined(__XVIC__)
      theme = 1;
#elif defined(__XPLUS4__) || defined(__X128__)
      theme = 2;
#else
      theme = 3;
#endif
   }

   switch (theme)
   {
      default:
      case 1: /* C64 brown */
         BKG_COLOR_NORMAL  = (pix_bytes == 4) ? COLOR_BROWN_32 : COLOR_BROWN_16;
         BKG_COLOR_ALT     = (pix_bytes == 4) ? COLOR_BROWNGRAY_32 : COLOR_BROWNGRAY_16;
         BKG_COLOR_EXTRA   = (pix_bytes == 4) ? COLOR_BROWNLITE_32 : COLOR_BROWNLITE_16;
         BKG_COLOR_TAPE    = (pix_bytes == 4) ? COLOR_TAPE_32 : COLOR_TAPE_16;
         BKG_COLOR_SEL     = (pix_bytes == 4) ? COLOR_180_32 : COLOR_180_16;
         BKG_COLOR_ACTIVE  = (pix_bytes == 4) ? COLOR_BROWNDARK_32 : COLOR_BROWNDARK_16;
         FONT_COLOR_NORMAL = (pix_bytes == 4) ? COLOR_WHITE_32 : COLOR_WHITE_16;
         FONT_COLOR_SEL    = (pix_bytes == 4) ? COLOR_BLACK_32 : COLOR_BLACK_16;
         break;

      case 3: /* Dark */
         BKG_COLOR_NORMAL  = (pix_bytes == 4) ? COLOR_32_32 : COLOR_32_16;
         BKG_COLOR_ALT     = (pix_bytes == 4) ? COLOR_64_32 : COLOR_64_16;
         BKG_COLOR_EXTRA   = (pix_bytes == 4) ? COLOR_16_32 : COLOR_16_16;
         BKG_COLOR_TAPE    = (pix_bytes == 4) ? COLOR_64_32 : COLOR_64_16;
         BKG_COLOR_SEL     = (pix_bytes == 4) ? COLOR_180_32 : COLOR_180_16;
         BKG_COLOR_ACTIVE  = (pix_bytes == 4) ? COLOR_10_32 : COLOR_10_16;
         FONT_COLOR_NORMAL = (pix_bytes == 4) ? COLOR_WHITE_32 : COLOR_WHITE_16;
         FONT_COLOR_SEL    = (pix_bytes == 4) ? COLOR_BLACK_32 : COLOR_BLACK_16;
         break;

      case 2: /* C64C beige */
         BKG_COLOR_NORMAL  = (pix_bytes == 4) ? COLOR_BEIGE_32 : COLOR_BEIGE_16;
         BKG_COLOR_ALT     = (pix_bytes == 4) ? COLOR_BEIGEDARK_32 : COLOR_BEIGEDARK_16;
         BKG_COLOR_EXTRA   = (pix_bytes == 4) ? COLOR_100_32 : COLOR_100_16;
         BKG_COLOR_TAPE    = (pix_bytes == 4) ? COLOR_TAPE_32 : COLOR_TAPE_16;
         BKG_COLOR_SEL     = (pix_bytes == 4) ? COLOR_40_32 : COLOR_40_16;
         BKG_COLOR_ACTIVE  = (pix_bytes == 4) ? COLOR_250_32 : COLOR_250_16;
         FONT_COLOR_NORMAL = (pix_bytes == 4) ? COLOR_BLACK_32 : COLOR_BLACK_16;
         FONT_COLOR_SEL    = (pix_bytes == 4) ? COLOR_WHITE_32 : COLOR_WHITE_16;
         break;

      case 4: /* Light */
         BKG_COLOR_NORMAL  = (pix_bytes == 4) ? COLOR_220_32 : COLOR_220_16;
         BKG_COLOR_ALT     = (pix_bytes == 4) ? COLOR_160_32 : COLOR_160_16;
         BKG_COLOR_EXTRA   = (pix_bytes == 4) ? COLOR_100_32 : COLOR_100_16;
         BKG_COLOR_TAPE    = (pix_bytes == 4) ? COLOR_160_32 : COLOR_160_16;
         BKG_COLOR_SEL     = (pix_bytes == 4) ? COLOR_40_32 : COLOR_40_16;
         BKG_COLOR_ACTIVE  = (pix_bytes == 4) ? COLOR_250_32 : COLOR_250_16;
         FONT_COLOR_NORMAL = (pix_bytes == 4) ? COLOR_BLACK_32 : COLOR_BLACK_16;
         FONT_COLOR_SEL    = (pix_bytes == 4) ? COLOR_WHITE_32 : COLOR_WHITE_16;
         break;
   }

   BRD_COLOR = (pix_bytes == 4) ? COLOR_10_32 : COLOR_10_16;

   memset(graphed, 0, sizeof(graphed));

#if defined(__XVIC__)
   /* VIC */
   XOFFSET  = 0;
   YOFFSET  = 0;
   XPADDING = 116;

   if (retro_region == RETRO_REGION_NTSC)
   {
      XOFFSET  = 8;
      YOFFSET  = -1;
      XPADDING = 68;
   }
#elif defined(__XCBM2__) || defined(__XPET__)
   /* CRTC */
   XOFFSET  = 0;
   YOFFSET  = 1;
   XPADDING = 74;

   if (retrow >= 704)
      FONT_WIDTH *= 2;
#elif defined(__XPLUS4__)
   /* TED */
   XOFFSET  = 0;
   XPADDING = 74;
#else
   /* VIC-II */
   XOFFSET  = 0;
   YOFFSET  = 1;
   XPADDING = 74;
#if defined(__X128__)
   /* Difference due to VKBD height difference */
   if (retro_region == RETRO_REGION_NTSC)
      YOFFSET  = 2;

   /* VDC */
   if (retrow >= 704)
   {
      XOFFSET     = 2;
      XPADDING    = 120;
      XPADDING   *= 2;
      FONT_WIDTH *= 2;

      if (retro_region == RETRO_REGION_NTSC || retroh == 240)
         YOFFSET  = 7;
   }
#endif
#endif

   /* Vertical centering calculations */
   {
      int crop_top_border = (retroh - CROP_HEIGHT_MAX) / 2;

      /* Bonus 10 for statusbar */
      YPADDING = (crop_top_border + 10) * 2;

      /* Vertical centering offset required if both borders are not used */
      if (retroYS_offset && retroYS_offset < crop_top_border)
      {
         int leftover = retroh - retroh_crop - crop_top_border - retroYS_offset;
         if (leftover > 0)
            YOFFSET -= (crop_top_border - retroYS_offset) / 2;
      }
   }

   int XSIDE     = (retrow - XPADDING) / VKBDX;
   int YSIDE     = (retroh - YPADDING) / VKBDY;

   int XBASEKEY  = (XPADDING > 0) ? (XPADDING / 2) : 0;
   int YBASEKEY  = (YPADDING > 0) ? (YPADDING / 2) : 0;

   int XBASETEXT = XBASEKEY + (XSIDE / 2);
   int YBASETEXT = YBASEKEY + (YSIDE / 2);

   int x_gap = 0;
   int y_gap = 0;

   int vkbd_x_gap_pad = VKBDX_GAP_PAD * FONT_WIDTH;
   int vkbd_y_gap_pad = VKBDY_GAP_PAD * FONT_HEIGHT;

   XOFFSET -= (vkbd_x_gap_pad / 2);
   YOFFSET -= (vkbd_y_gap_pad / 2);

   /* Coordinates */
   vkbd_x_min = XOFFSET + XBASEKEY + XKEYSPACING;
   vkbd_x_max = XOFFSET - XBASEKEY - XKEYSPACING + retrow;
   vkbd_y_min = YOFFSET + YBASEKEY + YKEYSPACING;
   vkbd_y_max = YOFFSET + YBASEKEY + (YSIDE * VKBDY);

   vkbd_x_max += vkbd_x_gap_pad;
   vkbd_y_max += vkbd_y_gap_pad;

   /* Opacity */
   BKG_ALPHA = (retro_vkbd_transparent) ? ALPHA : GRAPH_ALPHA_100;

   /* Key label shifted */
   shifted = false;
   if (retro_capslock || vkey_sticky1 == RETROK_LSHIFT || vkey_sticky2 == RETROK_LSHIFT
                      || vkey_sticky1 == RETROK_RSHIFT || vkey_sticky2 == RETROK_RSHIFT)
      shifted = true;
   if (vkflag[RETRO_DEVICE_ID_JOYPAD_B] && (vkey_pressed == RETROK_LSHIFT || vkey_pressed == RETROK_RSHIFT))
      shifted = true;
   if (retro_key_state_internal[RETROK_LSHIFT] || retro_key_state_internal[RETROK_RSHIFT])
      shifted = true;

   /* Key layout */
   for (x = 0; x < VKBDX; x++)
   {
      x_gap = 0;
      if (VKBDX_GAP_POS && VKBDX_GAP_PAD && x >= VKBDX_GAP_POS)
         x_gap = vkbd_x_gap_pad;

      for (y = 0; y < VKBDY; y++)
      {
         y_gap = 0;
         if (VKBDY_GAP_POS && VKBDY_GAP_PAD && y >= VKBDY_GAP_POS)
            y_gap = vkbd_y_gap_pad;

         /* Default key color */
         BKG_COLOR = BKG_COLOR_NORMAL;
         BKG_ALPHA = (retro_vkbd_transparent) ? ALPHA : GRAPH_ALPHA_100;

         /* Reset key color */
         if (vkeys[(y * VKBDX) + x].value == VKBD_RESET)
            BKG_COLOR = (pix_bytes == 4) ? COLOR_RED_32 : COLOR_RED_16;
         else
         {
            /* Alternate key color */
            for (int alt_key = 0; alt_key < vkbd_alt_keys_len; ++alt_key)
                if (vkbd_alt_keys[alt_key] == vkeys[(y * VKBDX) + x + page].value)
                    BKG_COLOR = BKG_COLOR_ALT;

            /* Extra key color */
            for (int extra_key = 0; extra_key < vkbd_extra_keys_len; ++extra_key)
                if (vkbd_extra_keys[extra_key] == vkeys[(y * VKBDX) + x + page].value)
                    BKG_COLOR = BKG_COLOR_EXTRA;

            /* Datasette key color */
            for (int datasette_key = 0; datasette_key < vkbd_datasette_keys_len; ++datasette_key)
                if (vkbd_datasette_keys[datasette_key] == vkeys[(y * VKBDX) + x + page].value)
                    BKG_COLOR = BKG_COLOR_TAPE;
         }

         /* Default font color */
         FONT_COLOR = FONT_COLOR_NORMAL;

         /* Pressed keys via mapper */
         int current_key = vkeys[(y * VKBDX) + x + page].value;
         current_key = (current_key > 0) ? current_key : 0;

         /* Sticky + CapsLock + pressed colors */
         if ( (vkey_sticky1 == vkeys[(y * VKBDX) + x + page].value
          ||   vkey_sticky2 == vkeys[(y * VKBDX) + x + page].value
          ||(retro_capslock && vkeys[(y * VKBDX) + x + page].value == VKBD_SHIFTLOCK)
          ||(retro_key_state_internal[current_key])
#ifdef __X128__
          ||(c128_vdc && vkeys[(y * VKBDX) + x + page].value == key_ctrl_column4080) /* 40/80 display toggle */
          ||(c128_capslock && vkeys[(y * VKBDX) + x + page].value == key_ctrl_caps) /* Caps Lock toggle */
#endif
          ||(vkflag[RETRO_DEVICE_ID_JOYPAD_START] && vkeys[(y * VKBDX) + x + page].value == RETROK_RETURN)
          ||(vkflag[RETRO_DEVICE_ID_JOYPAD_X]     && vkeys[(y * VKBDX) + x + page].value == RETROK_SPACE)
          ||(tape_enabled && tape_control == 1 && vkeys[(y * VKBDX) + x + page].value == VKBD_DATASETTE_START)
          ||(tape_enabled && tape_control == 2 && vkeys[(y * VKBDX) + x + page].value == VKBD_DATASETTE_FWD)
          ||(tape_enabled && tape_control == 3 && vkeys[(y * VKBDX) + x + page].value == VKBD_DATASETTE_RWD))
          && BKG_COLOR != BKG_COLOR_EXTRA && vkeys[(y * VKBDX) + x + page].value != VKBD_RESET)
         {
            FONT_COLOR = FONT_COLOR_NORMAL;
            BKG_COLOR  = BKG_COLOR_ACTIVE;

            switch (BKG_ALPHA)
            {
               case GRAPH_ALPHA_0:
               case GRAPH_ALPHA_25:
               case GRAPH_ALPHA_50:
                  BKG_ALPHA = GRAPH_ALPHA_75;
                  break;
               case GRAPH_ALPHA_75:
               case GRAPH_ALPHA_100:
               default:
                  /* Do nothing */
                  break;
            }
         }

         /* Key string */
         if (tape_enabled && vkeys[(y * VKBDX) + x + page].value == VKBD_DATASETTE_RESET)
            snprintf(string, sizeof(string), "%03d", tape_counter);
         else
            snprintf(string, sizeof(string), "%s",
               (!shifted) ? vkeys[(y * VKBDX) + x + page].normal
                          : vkeys[(y * VKBDX) + x + page].shift);

         /* Key centering */
         BKG_PADDING_X = BKG_PADDING_X_DEFAULT;
         BKG_PADDING_Y = BKG_PADDING_Y_DEFAULT;
         if (tape_enabled && vkeys[(y * VKBDX) + x + page].value == VKBD_DATASETTE_RESET)
            BKG_PADDING_X = BKG_PADDING("000");
         else
            BKG_PADDING_X = BKG_PADDING(string);

         if (strchr(string, '\1'))
            BKG_PADDING_Y = -6;

         /* Key positions */
         XKEY  = x_gap + XOFFSET + XBASEKEY + (x * XSIDE);
         XTEXT = x_gap + XOFFSET + XBASETEXT + BKG_PADDING_X + (x * XSIDE);
         YKEY  = y_gap + YOFFSET + YBASEKEY + (y * YSIDE);
         YTEXT = y_gap + YOFFSET + YBASETEXT + BKG_PADDING_Y + (y * YSIDE);

         /* Empty key */
         if (vkeys[(y * VKBDX) + x].value == -1)
         {
            /* Key background */
            draw_fbox(XKEY+XKEYSPACING, YKEY+YKEYSPACING,
                      XSIDE-XKEYSPACING, YSIDE-YKEYSPACING,
                      0, BRD_ALPHA);
         }
         /* Not selected key */
         else if (((vkey_pos_y * VKBDX) + vkey_pos_x + page) != ((y * VKBDX) + x + page))
         {
            int FONT_ALPHA = BKG_ALPHA;
            FONT_ALPHA = (FONT_ALPHA < GRAPH_ALPHA_25) ? GRAPH_ALPHA_25 : FONT_ALPHA;
            FONT_ALPHA = (FONT_ALPHA > GRAPH_ALPHA_75) ? GRAPH_ALPHA_75 : FONT_ALPHA;

            /* Key background */
            draw_fbox(XKEY+XKEYSPACING, YKEY+YKEYSPACING,
                      XSIDE-XKEYSPACING, YSIDE-YKEYSPACING,
                      BKG_COLOR, BKG_ALPHA);

            /* Key text */
            draw_text(XTEXT, YTEXT, FONT_COLOR, BKG_COLOR, FONT_ALPHA,
                      (text_outline) ? GRAPH_BG_OUTLINE : GRAPH_BG_SHADOW, FONT_WIDTH, FONT_HEIGHT, FONT_MAX,
                      string);
         }

         /* Key border */
         draw_box(XKEY+XKEYSPACING-FONT_WIDTH, YKEY+YKEYSPACING-FONT_HEIGHT,
                  XSIDE-XKEYSPACING+FONT_WIDTH, YSIDE-YKEYSPACING+FONT_HEIGHT,
                  FONT_WIDTH, FONT_HEIGHT,
                  0, BRD_ALPHA);
      }
   }

   /* Opacity */
   BKG_ALPHA = (retro_vkbd_transparent)
         ? ((BKG_ALPHA == GRAPH_ALPHA_100) ? GRAPH_ALPHA_100 : GRAPH_ALPHA_75)
         : GRAPH_ALPHA_100;

   /* Selected key */
   int current_key_pos = (vkey_pos_y * VKBDX) + vkey_pos_x + page;
   int current_key_val = vkeys[current_key_pos].value;

   /* Pressed key color */
   if (vkflag[RETRO_DEVICE_ID_JOYPAD_B] && (current_key_val == vkey_sticky1 ||
                                            current_key_val == vkey_sticky2))
      ; /* no-op */
   else if (vkflag[RETRO_DEVICE_ID_JOYPAD_B] || retro_key_state_internal[(current_key_val > 0) ? current_key_val : 0])
      BKG_COLOR_SEL = BKG_COLOR_ACTIVE;
   else
      FONT_COLOR = FONT_COLOR_SEL;

   /* Selected key string */
   snprintf(string, sizeof(string), "%s",
         (!shifted) ? vkeys[current_key_pos].normal
                    : vkeys[current_key_pos].shift);

   /* Reset key */
   if (current_key_val == VKBD_RESET)
   {
      signed char reset_counter = 0;
      if (last_vkey_pressed_time < now && last_vkey_pressed != -1)
         reset_counter = (VKBD_STICKY_HOLDING_TIME - (now - last_vkey_pressed_time)) / 100;
      if (reset_counter < 0)
         reset_counter = 0;

      if (last_vkey_pressed != -1 && reset_counter == 0)
      {
         FONT_COLOR    = (pix_bytes == 4) ? COLOR_WHITE_32 : COLOR_WHITE_16;
         BKG_COLOR_SEL = (pix_bytes == 4) ? COLOR_RED_32 : COLOR_RED_16;
      }

      if (reset_counter > 0)
         snprintf(string, sizeof(string), "%1d", reset_counter);
   }

   /* Quick mapping */
   {
      if (vkbd_mapping_active && vkbd_mapping_key && (now / 100) % 2 == 0 ||
          (now - last_vkey_pressed_time > SHORT_PRESS && vkflag[RETRO_DEVICE_ID_JOYPAD_Y]))
      {
         FONT_COLOR    = (pix_bytes == 4) ? COLOR_WHITE_32 : COLOR_WHITE_16;
         BKG_COLOR_SEL = (pix_bytes == 4) ? COLOR_GREEN_32 : COLOR_GREEN_16;
      }

      if (vkbd_mapping_active && !vkbd_mapping_key && (now / 100) % 2 == 0 ||
          (now - last_vkey_pressed_time > LONG_PRESS * 2 && vkflag[RETRO_DEVICE_ID_JOYPAD_Y]))
      {
         FONT_COLOR    = (pix_bytes == 4) ? COLOR_WHITE_32 : COLOR_WHITE_16;
         BKG_COLOR_SEL = (pix_bytes == 4) ? COLOR_RED_32 : COLOR_RED_16;
      }
   }

   /* Key centering */
   BKG_PADDING_X = BKG_PADDING_X_DEFAULT;
   BKG_PADDING_Y = BKG_PADDING_Y_DEFAULT;
   BKG_PADDING_X = BKG_PADDING(string);

   if (strchr(string, '\1'))
      BKG_PADDING_Y = -6;

   x_gap = 0;
   if (VKBDX_GAP_POS && VKBDX_GAP_PAD && vkey_pos_x >= VKBDX_GAP_POS)
      x_gap = vkbd_x_gap_pad;
   y_gap = 0;
   if (VKBDY_GAP_POS && VKBDY_GAP_PAD && vkey_pos_y >= VKBDY_GAP_POS)
      y_gap = vkbd_y_gap_pad;

   /* Selected key position */
   XKEY  = x_gap + XOFFSET + XBASEKEY + (vkey_pos_x * XSIDE);
   XTEXT = x_gap + XOFFSET + XBASETEXT + BKG_PADDING_X + (vkey_pos_x * XSIDE);
   YKEY  = y_gap + YOFFSET + YBASEKEY + (vkey_pos_y * YSIDE);
   YTEXT = y_gap + YOFFSET + YBASETEXT + BKG_PADDING_Y + (vkey_pos_y * YSIDE);

   /* Selected key background */
   draw_fbox(XKEY+XKEYSPACING, YKEY+YKEYSPACING,
             XSIDE-XKEYSPACING, YSIDE-YKEYSPACING,
             BKG_COLOR_SEL, BKG_ALPHA);

   /* Selected key text */
   draw_text(XTEXT, YTEXT, FONT_COLOR, 0, GRAPH_ALPHA_100,
             GRAPH_BG_NONE, FONT_WIDTH, FONT_HEIGHT, FONT_MAX,
             string);

   if (BRD_ALPHA == GRAPH_ALPHA_0)
      return;

   /* Gap backgrounds */
   if (VKBDX_GAP_POS)
      draw_fbox(XOFFSET+XBASEKEY+(VKBDX_GAP_POS * XSIDE)+FONT_WIDTH, vkbd_y_min-FONT_HEIGHT,
                vkbd_x_gap_pad-FONT_WIDTH, vkbd_y_max-vkbd_y_min+(FONT_HEIGHT * 2),
                0, BRD_ALPHA);

   if (VKBDY_GAP_POS)
   {
      draw_fbox(vkbd_x_min-FONT_WIDTH, YOFFSET+YBASEKEY+(VKBDY_GAP_POS * YSIDE)+FONT_HEIGHT,
                vkbd_x_max-vkbd_x_min+FONT_WIDTH-XSIDE-VKBDX_GAP_PAD, vkbd_y_gap_pad-FONT_HEIGHT,
                0, BRD_ALPHA);
#if 0
      draw_fbox(XOFFSET+XBASEKEY+(VKBDX_GAP_POS * XSIDE)+vkbd_x_gap_pad, YOFFSET+YBASEKEY+(VKBDY_GAP_POS * YSIDE)+FONT_HEIGHT,
                XSIDE+FONT_WIDTH, vkbd_y_gap_pad-FONT_HEIGHT,
                0, BRD_ALPHA);
#endif
   }

   /* Corner dimming */
   {
      unsigned corner_y_min = 0;
      unsigned corner_y_max = 0;

      /* Top */
      corner_y_min = 0;
      corner_y_max = vkbd_y_min - YKEYSPACING;
      draw_fbox(0, corner_y_min,
                retrow, corner_y_max,
                0, BRD_ALPHA);

      /* Bottom */
      corner_y_min = vkbd_y_max + YKEYSPACING;
      corner_y_max = retroh - vkbd_y_max - YKEYSPACING;
      draw_fbox(0, corner_y_min,
                retrow, corner_y_max,
                0, BRD_ALPHA);

      /* Left + Right */
      corner_y_min = vkbd_y_min - YKEYSPACING;
      corner_y_max = vkbd_y_max - vkbd_y_min + (YKEYSPACING * 2);
      draw_fbox(0, corner_y_min,
                vkbd_x_min - XKEYSPACING, corner_y_max,
                0, BRD_ALPHA);
      draw_fbox(vkbd_x_max, corner_y_min,
                retrow - vkbd_x_max , corner_y_max,
                0, BRD_ALPHA);
   }

#if POINTER_DEBUG
   draw_hline(pointer_x, pointer_y, 1, 1, RGBc(255, 0, 255));
#endif
}

static void input_vkbd_sticky(void)
{
   if (vkey_sticky && last_vkey_pressed != -1 && last_vkey_pressed > 0)
   {
      if (vkey_sticky1 > -1 && vkey_sticky1 != last_vkey_pressed)
      {
         if (vkey_sticky2 > -1 && vkey_sticky2 != last_vkey_pressed)
            retro_key_up(vkey_sticky2);
         vkey_sticky2 = last_vkey_pressed;
      }
      else
         vkey_sticky1 = last_vkey_pressed;
   }

   /* Keyup only after button is up */
   if (last_vkey_pressed != -1 && !vkflag[RETRO_DEVICE_ID_JOYPAD_B])
   {
      if (vkey_pressed == -1 && last_vkey_pressed >= 0 && last_vkey_pressed != vkey_sticky1 && last_vkey_pressed != vkey_sticky2)
         retro_key_up(last_vkey_pressed);

      last_vkey_pressed = -1;
   }

   if (vkey_sticky1_release)
   {
      vkey_sticky1_release = 0;
      vkey_sticky1 = -1;
   }
   if (vkey_sticky2_release)
   {
      vkey_sticky2_release = 0;
      vkey_sticky2 = -1;
   }
}

static void convert_vkbd_to_mapper(int *vkbd_mapping_key, char **var_value)
{
   bool shifted = retro_capslock
         || retro_key_state_internal[RETROK_LSHIFT]
         || retro_key_state_internal[RETROK_RSHIFT];

   /* Convert VKBD special keys to mapper special keys */
   switch (*vkbd_mapping_key)
   {
      case VKBD_STATUSBAR_SAVEDISK:
         if (shifted)
         {
            *var_value = get_variable("vice_mapper_save_disk_toggle");
            *vkbd_mapping_key = 0;
         }
         else
         {
            *var_value = strdup("TOGGLE_STATUSBAR");
            *vkbd_mapping_key = TOGGLE_STATUSBAR;
         }
         break;
      case VKBD_JOYPORT_ASPECT:
         if (shifted)
         {
            *var_value = get_variable("vice_mapper_aspect_ratio_toggle");
            *vkbd_mapping_key = 0;
         }
         else
         {
            *var_value = strdup("SWITCH_JOYPORT");
            *vkbd_mapping_key = SWITCH_JOYPORT;
         }
         break;
      case VKBD_TURBO_CROP:
         if (shifted)
         {
            *var_value = get_variable("vice_mapper_crop_toggle");
            *vkbd_mapping_key = 0;
         }
         else
         {
            *var_value = get_variable("vice_mapper_turbo_fire_toggle");
            *vkbd_mapping_key = 0;
         }
         break;
      case VKBD_RESET:
         *var_value = get_variable("vice_mapper_reset");
         *vkbd_mapping_key = 0;
         break;
      case VKBD_SHIFTLOCK:
         *var_value = strdup("RETROK_CAPSLOCK");
         *vkbd_mapping_key = RETROK_CAPSLOCK;
         break;
      case VKBD_DATASETTE_STOP:
         *var_value = get_variable("vice_mapper_datasette_stop");
         *vkbd_mapping_key = 0;
         break;
      case VKBD_DATASETTE_START:
         *var_value = get_variable("vice_mapper_datasette_start");
         *vkbd_mapping_key = 0;
         break;
      case VKBD_DATASETTE_FWD:
         *var_value = get_variable("vice_mapper_datasette_forward");
         *vkbd_mapping_key = 0;
         break;
      case VKBD_DATASETTE_RWD:
         *var_value = get_variable("vice_mapper_datasette_rewind");
         *vkbd_mapping_key = 0;
         break;
      case VKBD_DATASETTE_RESET:
         *var_value = get_variable("vice_mapper_datasette_reset");
         *vkbd_mapping_key = 0;
         break;
      case VKBD_NUMPAD:
         *var_value = strdup("");
         *vkbd_mapping_key = 0;
         break;
      default:
         *var_value = strdup(retro_keymap_value(*vkbd_mapping_key));
         break;
   }
}

static bool input_vkbd_mapper(void)
{
   long now                 = retro_ticks() / 1000;
   static int joypad_button = -1;
   const int threshold      = 20000;

   if (!vkbd_mapping_active)
      return false;

   /* Init */
   if (vkbd_mapping_activated)
   {
      int converted_key = vkbd_mapping_key;
      char *converted_label;

      converted_label        = '\0';
      vkbd_mapping_activated = false;

      convert_vkbd_to_mapper(&converted_key, &converted_label);
      if (string_is_empty(converted_label))
      {
         statusbar_message_show(9, "Unable to map!");

         free(converted_label);
         converted_label = NULL;
         goto reset;
      }

      statusbar_message_show(9, "Press button for \"%s\"",
            (converted_key) ? retro_keymap_label(converted_key) : converted_label);

      free(converted_label);
      converted_label = NULL;
   }
   /* Waiting for input */
   else if (joypad_button < 0)
   {
      unsigned i = 0, j = 0;

      int duration                 = now - vkbd_mapping_active;
      int max_duration             = 3000;
      int duration_sec             = (max_duration + 1000 - duration) / 1000;
      static int duration_sec_prev = 0;

      for (j = 0; j < 2; j++)
      {
         for (i = 0; i < RETRO_DEVICE_ID_JOYPAD_LAST; i++)
         {
            switch (i)
            {
               case RETRO_DEVICE_ID_JOYPAD_LR:
                  if (joypad_axis[j][AXIS_LX] > threshold)
                     joypad_button = i;
                  break;
               case RETRO_DEVICE_ID_JOYPAD_LL:
                  if (joypad_axis[j][AXIS_LX] < -threshold)
                     joypad_button = i;
                  break;
               case RETRO_DEVICE_ID_JOYPAD_LD:
                  if (joypad_axis[j][AXIS_LY] > threshold)
                     joypad_button = i;
                  break;
               case RETRO_DEVICE_ID_JOYPAD_LU:
                  if (joypad_axis[j][AXIS_LY] < -threshold)
                     joypad_button = i;
                  break;

               case RETRO_DEVICE_ID_JOYPAD_RR:
                  if (joypad_axis[j][AXIS_RX] > threshold)
                     joypad_button = i;
                  break;
               case RETRO_DEVICE_ID_JOYPAD_RL:
                  if (joypad_axis[j][AXIS_RX] < -threshold)
                     joypad_button = i;
                  break;
               case RETRO_DEVICE_ID_JOYPAD_RD:
                  if (joypad_axis[j][AXIS_RY] > threshold)
                     joypad_button = i;
                  break;
               case RETRO_DEVICE_ID_JOYPAD_RU:
                  if (joypad_axis[j][AXIS_RY] < -threshold)
                     joypad_button = i;
                  break;

               default:
                  if (joypad_bits[j] & (1 << i))
                     joypad_button = i;
                  break;
            }
         }
      }

      if (duration_sec != duration_sec_prev)
      {
         duration_sec_prev = duration_sec;
         if (duration_sec < 3)
            statusbar_message_show(9, "Press button for %d..", duration_sec);
      }

      /* Don't wait forever */
      if (duration > max_duration)
         goto reset;
   }
   /* Process selection after all buttons are released */
   else if (joypad_button > -1 && !joypad_bits[0] && !joypad_bits[1] &&
         abs(joypad_axis[0][AXIS_LX]) < threshold && abs(joypad_axis[0][AXIS_LY]) < threshold &&
         abs(joypad_axis[0][AXIS_RX]) < threshold && abs(joypad_axis[0][AXIS_RY]) < threshold &&
         abs(joypad_axis[1][AXIS_LX]) < threshold && abs(joypad_axis[1][AXIS_LY]) < threshold &&
         abs(joypad_axis[1][AXIS_RX]) < threshold && abs(joypad_axis[1][AXIS_RY]) < threshold)
   {
      char var_key[64];
      char prefix[32];
      char *var_value;

      var_value = '\0';
      snprintf(prefix, sizeof(prefix), "%s", "vice_mapper_");

      /* Build core option key */
      switch (joypad_button)
      {
         case RETRO_DEVICE_ID_JOYPAD_UP: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "up"); break;
         case RETRO_DEVICE_ID_JOYPAD_DOWN: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "down"); break;
         case RETRO_DEVICE_ID_JOYPAD_LEFT: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "left"); break;
         case RETRO_DEVICE_ID_JOYPAD_RIGHT: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "right"); break;
         case RETRO_DEVICE_ID_JOYPAD_B: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "b"); break;
         case RETRO_DEVICE_ID_JOYPAD_Y: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "y"); break;
         case RETRO_DEVICE_ID_JOYPAD_A: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "a"); break;
         case RETRO_DEVICE_ID_JOYPAD_X: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "x"); break;
         case RETRO_DEVICE_ID_JOYPAD_L: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "l"); break;
         case RETRO_DEVICE_ID_JOYPAD_R: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "r"); break;
         case RETRO_DEVICE_ID_JOYPAD_L2: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "l2"); break;
         case RETRO_DEVICE_ID_JOYPAD_R2: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "r2"); break;
         case RETRO_DEVICE_ID_JOYPAD_L3: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "l3"); break;
         case RETRO_DEVICE_ID_JOYPAD_R3: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "r3"); break;
         case RETRO_DEVICE_ID_JOYPAD_LU: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "lu"); break;
         case RETRO_DEVICE_ID_JOYPAD_LD: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "ld"); break;
         case RETRO_DEVICE_ID_JOYPAD_LL: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "ll"); break;
         case RETRO_DEVICE_ID_JOYPAD_LR: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "lr"); break;
         case RETRO_DEVICE_ID_JOYPAD_RU: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "ru"); break;
         case RETRO_DEVICE_ID_JOYPAD_RD: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "rd"); break;
         case RETRO_DEVICE_ID_JOYPAD_RL: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "rl"); break;
         case RETRO_DEVICE_ID_JOYPAD_RR: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "rr"); break;
         case RETRO_DEVICE_ID_JOYPAD_SELECT: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "select"); break;
         case RETRO_DEVICE_ID_JOYPAD_START: snprintf(var_key, sizeof(var_key), "%s%s", prefix, "start"); break;
      }

      /* Prevent locked-up states */
      if (   (joypad_button == RETRO_DEVICE_ID_JOYPAD_Y && vkbd_mapping_key < 0) ||
            !strcmp(get_variable(var_key), "TOGGLE_VKBD"))
      {
         statusbar_message_show(9, "Illegal mapping combo!");

         free(var_value);
         var_value = NULL;
         goto reset;
      }

      convert_vkbd_to_mapper(&vkbd_mapping_key, &var_value);
      set_variable(var_key, var_value);

      statusbar_message_show(9, "RetroPad %s = \"%s\"",
            string_to_upper(&var_key[12]),
            (vkbd_mapping_key) ? retro_keymap_label(vkbd_mapping_key) : var_value);

      free(var_value);
      var_value = NULL;
      goto reset;
   }

   return true;

reset:
   vkbd_mapping_active = 0;
   vkbd_mapping_key    = -1;
   joypad_button       = -1;
   retro_vkbd_ready    = -2;

   return true;
}

void toggle_vkbd(void)
{
   /* No toggling while key is pressed */
   if (vkflag[RETRO_DEVICE_ID_JOYPAD_B])
      return;
   retro_vkbd = !retro_vkbd;
   /* Reset VKBD input readiness */
   retro_vkbd_ready = -2;
   /* Release VKBD controllable joypads */
   memset(joypad_bits, 0, 2*sizeof(joypad_bits[0]));
#ifdef ANDROID
   /* Discard mouse input for x frames, thanks to Android
    * input driver doing ghost mouse clicks.. */
   if (!retro_vkbd)
      retro_mouse_discard = 20;
#endif
}

void input_vkbd(void)
{
   long now  = 0;
   uint8_t i = 0;

   input_vkbd_sticky();

   if (input_vkbd_mapper())
      return;

   if (!retro_vkbd)
      return;

   /* Wait for all inputs to be released */
   if (retro_vkbd_ready < 1)
   {
      if ((retro_vkbd_ready == 0 && !joypad_bits[0] && !joypad_bits[1]) ||
           retro_vkbd_ready < 0)
         retro_vkbd_ready++;
      return;
   }

   now = retro_ticks() / 1000;

   /* Absolute pointer */
   int p_x = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
   int p_y = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

   if (p_x != 0 && p_y != 0 && (p_x != last_pointer_x || p_y != last_pointer_y))
   {
      int px = (int)((p_x + 0x7fff) * retrow_crop / 0xffff + retroXS_offset);
      int py = (int)((p_y + 0x7fff) * retroh_crop / 0xffff + retroYS_offset);

      last_pointer_x = p_x;
      last_pointer_y = p_y;

      if (px >= vkbd_x_min && px <= vkbd_x_max && py >= vkbd_y_min && py <= vkbd_y_max)
      {
         float vkey_width = (float)(vkbd_x_max - vkbd_x_min - VKBDX_GAP_PAD) / VKBDX;
         vkey_pos_x = ((px - vkbd_x_min) / vkey_width);
         if (VKBDX_GAP_POS && vkey_pos_x >= VKBDX_GAP_POS)
            vkey_pos_x = ((px - vkbd_x_min + VKBDX_GAP_PAD) / vkey_width);

         float vkey_height = (float)(vkbd_y_max - vkbd_y_min - VKBDY_GAP_PAD) / VKBDY;
         vkey_pos_y = ((py - vkbd_y_min) / vkey_height);
         if (VKBDY_GAP_POS && vkey_pos_y >= VKBDY_GAP_POS)
            vkey_pos_y = ((py - vkbd_y_min - VKBDY_GAP_PAD) / vkey_height);

         vkey_pos_x = (vkey_pos_x < 0) ? 0 : vkey_pos_x;
         vkey_pos_x = (vkey_pos_x > VKBDX - 1) ? VKBDX - 1 : vkey_pos_x;
         vkey_pos_y = (vkey_pos_y < 0) ? 0 : vkey_pos_y;
         vkey_pos_y = (vkey_pos_y > VKBDY - 1) ? VKBDY - 1 : vkey_pos_y;
      }
#if POINTER_DEBUG
      pointer_x = px;
      pointer_y = py;
      printf("px:%d py:%d (%d,%d) vkey:%dx%d\n", p_x, p_y, px, py, vkey_pos_x, vkey_pos_y);
#endif
   }

   /* Press Return, RetroPad Start */
   i = RETRO_DEVICE_ID_JOYPAD_START;
   if (!vkflag[i] && mapper_keys[i] >= 0 && ((joypad_bits[0] & (1 << i)) ||
                                             (joypad_bits[1] & (1 << i)))
                                         && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN))
   {
      vkflag[i] = 1;
      retro_key_down(RETROK_RETURN);
   }
   else
   if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                     !(joypad_bits[1] & (1 << i))))
   {
      vkflag[i] = 0;
      retro_key_up(RETROK_RETURN);
   }

   /* Toggle ShiftLock / Quick mapper, RetroPad Y */
   i = RETRO_DEVICE_ID_JOYPAD_Y;
   if (!vkflag[i] && mapper_keys[i] >= 0 && ((joypad_bits[0] & (1 << i)) ||
                                             (joypad_bits[1] & (1 << i))))
   {
      vkflag[i] = 1;
      vkey_pressed = vkeys[(vkey_pos_y * VKBDX) + vkey_pos_x + ((retro_vkbd_page) ? VKBDX * VKBDY : 0)].value;
      vkbd_mapping_key = vkey_pressed;

      last_vkey_pressed = vkey_pressed;
      last_vkey_pressed_time = now;
   }
   else
   if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                     !(joypad_bits[1] & (1 << i))))
   {
      vkflag[i] = 0;

      if (now - last_vkey_pressed_time < SHORT_PRESS)
      {
         retro_key_down(RETROK_CAPSLOCK);
         retro_key_up(RETROK_CAPSLOCK);
      }
      else
      {
         vkbd_mapping_active = now;
         vkbd_mapping_activated = true;

         /* Clear button with very long press */
         if (now - last_vkey_pressed_time > LONG_PRESS * 2)
            vkbd_mapping_key = 0;
      }

      vkey_pressed = -1;
   }

   /* Press Space, RetroPad X */
   i = RETRO_DEVICE_ID_JOYPAD_X;
   if (!vkflag[i] && mapper_keys[i] >= 0 && ((joypad_bits[0] & (1 << i)) ||
                                             (joypad_bits[1] & (1 << i))))
   {
      vkflag[i] = 1;
      retro_key_down(RETROK_SPACE);
   }
   else
   if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                     !(joypad_bits[1] & (1 << i))))
   {
      vkflag[i] = 0;
      retro_key_up(RETROK_SPACE);
   }

   /* Toggle transparency, RetroPad A */
   i = RETRO_DEVICE_ID_JOYPAD_A;
   if (!vkflag[i] && mapper_keys[i] >= 0 && ((joypad_bits[0] & (1 << i)) ||
                                             (joypad_bits[1] & (1 << i))))
   {
      vkflag[i] = 1;
      retro_vkbd_transparent = !retro_vkbd_transparent;
   }
   else
   if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                     !(joypad_bits[1] & (1 << i))))
   {
      vkflag[i] = 0;
   }

   /* Key press, RetroPad B joyports 1+2 / Keyboard Enter / Pointer */
   i = RETRO_DEVICE_ID_JOYPAD_B;
   if (!vkflag[i] && ((joypad_bits[0] & (1 << i)) ||
                      (joypad_bits[1] & (1 << i)) ||
                      input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN) ||
                      input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED)))
   {
      vkflag[i] = 1;
      vkey_pressed = vkeys[(vkey_pos_y * VKBDX) + vkey_pos_x + ((retro_vkbd_page) ? VKBDX * VKBDY : 0)].value;

      if (vkey_pressed != -1 && last_vkey_pressed == -1)
      {
         bool shifted = retro_capslock
               || retro_key_state_internal[RETROK_LSHIFT]
               || retro_key_state_internal[RETROK_RSHIFT];

         switch (vkey_pressed)
         {
            case VKBD_NUMPAD:
               retro_vkbd_page = !retro_vkbd_page;
               break;
            case VKBD_RESET: /* Reset on release */
               break;
            case VKBD_STATUSBAR_SAVEDISK:
               if (shifted)
                  emu_function(EMU_SAVE_DISK);
               else
                  emu_function(EMU_STATUSBAR);
               break;
            case VKBD_JOYPORT_ASPECT:
               if (shifted)
                  emu_function(EMU_ASPECT_RATIO);
               else
                  emu_function(EMU_JOYPORT);
               break;
            case VKBD_TURBO_CROP:
               if (shifted)
                  emu_function(EMU_CROP);
               else
                  emu_function(EMU_TURBO_FIRE);
               break;
            case VKBD_SHIFTLOCK:
               retro_key_down(RETROK_CAPSLOCK);
               retro_key_up(RETROK_CAPSLOCK);
               break;

            case VKBD_DATASETTE_STOP:
               emu_function(EMU_DATASETTE_STOP);
               break;
            case VKBD_DATASETTE_START:
               emu_function(EMU_DATASETTE_START);
               break;
            case VKBD_DATASETTE_FWD:
               emu_function(EMU_DATASETTE_FORWARD);
               break;
            case VKBD_DATASETTE_RWD:
               emu_function(EMU_DATASETTE_REWIND);
               break;
            case VKBD_DATASETTE_RESET:
               emu_function(EMU_DATASETTE_RESET);
               break;

            default:
               if (vkey_pressed == vkey_sticky1)
                  vkey_sticky1_release = 1;
               if (vkey_pressed == vkey_sticky2)
                  vkey_sticky2_release = 1;

               if (vkey_pressed != vkey_sticky1 && vkey_pressed != vkey_sticky2)
                  retro_key_down(vkey_pressed);
               break;
         }
      }

      last_vkey_pressed = vkey_pressed;
      last_vkey_pressed_time = now;
   }
   else
   if (vkflag[i] && (!(joypad_bits[0] & (1 << i)) &&
                     !(joypad_bits[1] & (1 << i)) &&
                     !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN) &&
                     !input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED)))
   {
      vkey_pressed = -1;
      vkflag[i] = 0;

      if (last_vkey_pressed == VKBD_RESET)
      {
         /* Reset on long press */
         if (now - last_vkey_pressed_time > VKBD_STICKY_HOLDING_TIME)
            emu_function(EMU_RESET);
         /* Freeze on short press */
         else
            emu_function(EMU_FREEZE);
      }
   }

   if (vkflag[i] && vkey_pressed > 0)
   {
      if (let_go_of_button)
         last_press_time_button = now;
      if (now - last_press_time_button > VKBD_STICKY_HOLDING_TIME)
         vkey_sticky = 1;
      let_go_of_button = false;
   }
   else
   {
      let_go_of_button = true;
      vkey_sticky = 0;
   }

   /* Allow directions when key is not pressed */
   if (!vkflag[RETRO_DEVICE_ID_JOYPAD_B])
   {
      if (!vkflag[RETRO_DEVICE_ID_JOYPAD_UP] && ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
                                                 (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) ||
                                                 input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP)))
         vkflag[RETRO_DEVICE_ID_JOYPAD_UP] = 1;
      else
      if (vkflag[RETRO_DEVICE_ID_JOYPAD_UP] && (!(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) &&
                                                !(joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) &&
                                                !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP)))
         vkflag[RETRO_DEVICE_ID_JOYPAD_UP] = 0;

      if (!vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] && ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
                                                   (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) ||
                                                   input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN)))
         vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] = 1;
      else
      if (vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] && (!(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) &&
                                                  !(joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) &&
                                                  !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN)))
         vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] = 0;

      if (!vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] && ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
                                                   (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) ||
                                                   input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT)))
         vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] = 1;
      else
      if (vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] && (!(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) &&
                                                  !(joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) &&
                                                  !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT)))
         vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] = 0;

      if (!vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] && ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ||
                                                    (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) ||
                                                    input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT)))
         vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] = 1;
      else
      if (vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] && (!(joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) &&
                                                   !(joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) &&
                                                   !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT)))
         vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] = 0;
   }
   else /* Release all directions when key is pressed */
   {
      vkflag[RETRO_DEVICE_ID_JOYPAD_UP]    = 0;
      vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN]  = 0;
      vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT]  = 0;
      vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT] = 0;
   }

   if (vkflag[RETRO_DEVICE_ID_JOYPAD_UP] ||
       vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN] ||
       vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT] ||
       vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT])
   {
      if (let_go_of_direction)
         /* just pressing down */
         last_press_time = now;

      if ((now - last_press_time > VKBD_MIN_HOLDING_TIME
        && now - last_move_time > VKBD_MOVE_DELAY)
        || let_go_of_direction)
      {
         bool searching = true;
         last_move_time = now;

         /* Handle skipping of empty keys */
         while (searching)
         {
            if (vkflag[RETRO_DEVICE_ID_JOYPAD_UP])
               vkey_pos_y -= 1;
            else if (vkflag[RETRO_DEVICE_ID_JOYPAD_DOWN])
               vkey_pos_y += 1;

            if (vkflag[RETRO_DEVICE_ID_JOYPAD_LEFT])
               vkey_pos_x -= 1;
            else if (vkflag[RETRO_DEVICE_ID_JOYPAD_RIGHT])
               vkey_pos_x += 1;

            if (vkey_pos_x < 0)
               vkey_pos_x = VKBDX - 1;
            else if (vkey_pos_x > VKBDX - 1)
               vkey_pos_x = 0;
            if (vkey_pos_y < 0)
               vkey_pos_y = VKBDY - 1;
            else if (vkey_pos_y > VKBDY - 1)
               vkey_pos_y = 0;

            searching = (vkeys[(vkey_pos_y * VKBDX) + vkey_pos_x + ((retro_vkbd_page) ? VKBDX * VKBDY : 0)].value == -1);
         }
      }
      let_go_of_direction = false;
   }
   else
      let_go_of_direction = true;

   if (vkey_pos_x < 0)
      vkey_pos_x = VKBDX - 1;
   else if (vkey_pos_x > VKBDX - 1)
      vkey_pos_x = 0;
   if (vkey_pos_y < 0)
      vkey_pos_y = VKBDY - 1;
   else if (vkey_pos_y > VKBDY - 1)
      vkey_pos_y = 0;

#if 0
   printf("vkey:%d sticky:%d sticky1:%d sticky2:%d, now:%d last:%d\n", vkey_pressed, vkey_sticky, vkey_sticky1, vkey_sticky2, now, last_press_time_button);
#endif
}

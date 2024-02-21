#ifndef LIBRETRO_VKBD_H
#define LIBRETRO_VKBD_H

#include "libretro-graph.h"

extern bool retro_vkbd;
extern bool retro_capslock;
extern signed char retro_vkbd_ready;

extern void print_vkbd(void);
extern void input_vkbd(void);
extern void toggle_vkbd(void);

extern unsigned int opt_vkbd_theme;
extern libretro_graph_alpha_t opt_vkbd_alpha;
extern libretro_graph_alpha_t opt_vkbd_dim_alpha;

#define VKBDX 11
#ifdef __X128__
#define VKBDY 8
#else
#define VKBDY 7
#endif

#define VKBDX_GAP_POS 10
#define VKBDY_GAP_POS 0
#define VKBDX_GAP_PAD 4
#define VKBDY_GAP_PAD 0

#define VKBD_NUMPAD             -2
#define VKBD_RESET              -3
#define VKBD_STATUSBAR_SAVEDISK -4
#define VKBD_JOYPORT_ASPECT     -5
#define VKBD_TURBO_CROP         -6
#define VKBD_SHIFTLOCK          -10

#define VKBD_DATASETTE_STOP     -11
#define VKBD_DATASETTE_START    -12
#define VKBD_DATASETTE_FWD      -13
#define VKBD_DATASETTE_RWD      -14
#define VKBD_DATASETTE_RESET    -15

#endif /* LIBRETRO_VKBD_H */

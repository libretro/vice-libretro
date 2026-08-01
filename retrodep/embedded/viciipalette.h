#include "vicii_c64hq_vpl.h"
#include "vicii_c64s_vpl.h"
#include "vicii_ccs64_vpl.h"
#include "vicii_cjam_vpl.h"
#include "vicii_colodore_vpl.h"
#include "vicii_community_colors_vpl.h"
#include "vicii_deekay_vpl.h"
#include "vicii_frodo_vpl.h"
#include "vicii_godot_vpl.h"
#include "vicii_lemon64_vpl.h"
#include "vicii_palette_vpl.h"
#include "vicii_palette_6569R1_v1r_vpl.h"
#include "vicii_palette_6569R5_v1r_vpl.h"
#include "vicii_palette_8565R2_v1r_vpl.h"
#include "vicii_palette_C64_amber_vpl.h"
#include "vicii_palette_C64_cyan_vpl.h"
#include "vicii_palette_C64_green_vpl.h"
#include "vicii_pc64_vpl.h"
#include "vicii_pepto_ntsc_vpl.h"
#include "vicii_pepto_ntsc_sony_vpl.h"
#include "vicii_pepto_pal_vpl.h"
#include "vicii_pepto_palold_vpl.h"
#include "vicii_pixcen_vpl.h"
#include "vicii_ptoing_vpl.h"
#include "vicii_rgb_vpl.h"
#include "vicii_the64_vpl.h"
#include "vicii_ultimate64_vpl.h"
#include "vicii_vice_vpl.h"

#define VICII_PALETTE_FILES \
    { "c64hq", "c64hq.vpl", 16, vicii_c64hq_vpl }, \
    { "c64s", "c64s.vpl", 16, vicii_c64s_vpl  }, \
    { "ccs64", "ccs64.vpl", 16, vicii_ccs64_vpl }, \
    { "cjam", "cjam.vpl", 16, vicii_cjam_vpl }, \
    { "colodore", "colodore.vpl", 16, vicii_colodore_vpl }, \
    { "community-colors", "community-colors.vpl", 16, vicii_community_colors_vpl }, \
    { "deekay", "deekay.vpl", 16, vicii_deekay_vpl }, \
    { "frodo", "frodo.vpl", 16, vicii_frodo_vpl }, \
    { "godot", "godot.vpl", 16, vicii_godot_vpl }, \
    { "lemon64", "lemon64.vpl", 16, vicii_lemon64_vpl }, \
    { "palette", "palette.vpl", 16, vicii_palette_vpl }, \
    { "palette_6569R1_v1r", "palette_6569R1_v1r.vpl", 16, vicii_palette_6569R1_v1r_vpl }, \
    { "palette_6569R5_v1r", "palette_6569R5_v1r.vpl", 16, vicii_palette_6569R5_v1r_vpl }, \
    { "palette_8565R2_v1r", "palette_8565R2_v1r.vpl", 16, vicii_palette_8565R2_v1r_vpl }, \
    { "palette_C64_amber", "palette_C64_amber.vpl", 16, vicii_palette_C64_amber_vpl }, \
    { "palette_C64_cyan", "palette_C64_cyan.vpl", 16, vicii_palette_C64_cyan_vpl }, \
    { "palette_C64_green", "palette_C64_green.vpl", 16, vicii_palette_C64_green_vpl }, \
    { "pc64", "pc64.vpl", 16, vicii_pc64_vpl }, \
    { "pepto-ntsc", "pepto-ntsc.vpl", 16, vicii_pepto_ntsc_vpl }, \
    { "pepto-ntsc-sony", "pepto-ntsc-sony.vpl", 16, vicii_pepto_ntsc_sony_vpl }, \
    { "pepto-pal", "pepto-pal.vpl", 16, vicii_pepto_pal_vpl }, \
    { "pepto-palold", "pepto-palold.vpl", 16, vicii_pepto_palold_vpl }, \
    { "pixcen", "pixcen.vpl", 16, vicii_pixcen_vpl }, \
    { "ptoing", "ptoing.vpl", 16, vicii_ptoing_vpl }, \
    { "rgb", "rgb.vpl", 16, vicii_rgb_vpl }, \
    { "the64", "the64.vpl", 16, vicii_the64_vpl }, \
    { "ultimate64", "ultimate64.vpl", 16, vicii_ultimate64_vpl }, \
    { "vice", "vice.vpl", 16, vicii_vice_vpl }, \


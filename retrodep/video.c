#include "vice.h"
#include "interrupt.h"
#include "cmdline.h"
#include "video.h"
#include "videoarch.h"
#include "palette.h"
#include "viewport.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "ui.h"
#include "vsync.h"
#include "raster.h"
#include "sound.h"
#include "machine.h"
#include "resources.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libretro-core.h"
#include "libretro-vkbd.h"

#if defined(__X128__)
extern int c128_vdc;
#endif

int machine_ui_done = 0;
int num_screens = 0;

static const cmdline_option_t cmdline_options[] = {
     { NULL }
};

int video_init_cmdline_options(void)
{
   return cmdline_register_options(cmdline_options);
}

int video_arch_cmdline_options_init(void)
{
   return cmdline_register_options(cmdline_options);
}

void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas)
{
}

video_canvas_t *video_canvas_create(video_canvas_t *canvas, 
      unsigned int *width, unsigned int *height, int mapped)
{
   switch (canvas->index)
   {
      default:
      case 0:
         /* VIC-II, VIC etc */
         canvas->videoconfig->rendermode = VIDEO_RENDER_PAL_NTSC_1X1;
         break;
      case 1:
         /* VDC */
         canvas->videoconfig->rendermode = VIDEO_RENDER_RGBI_1X1;
         break;
   }

   canvas->depth = 8 * pix_bytes;
   video_canvas_set_palette(canvas, canvas->palette);
   return canvas;
}

void video_canvas_destroy(struct video_canvas_s *canvas)
{
}

void video_arch_canvas_init(struct video_canvas_s *canvas)
{
   canvas->index = num_screens;
   num_screens++;
}

char video_canvas_can_resize(video_canvas_t *canvas)
{
   return 1;
}

int video_canvas_set_palette(struct video_canvas_s *canvas,
                             struct palette_s *palette)
{
   unsigned int i, col = 0;
   video_render_color_tables_t *color_tables = &canvas->videoconfig->color_tables;

   if (!canvas || !palette) {
      return 0; /* No palette, nothing to do */
   }

   canvas->palette = palette;

   for (i = 0; i < palette->num_entries; i++) {
      if (pix_bytes == 2)
         col = RGB565(palette->entries[i].red, palette->entries[i].green, palette->entries[i].blue);
      else
         col = palette->entries[i].red << 16 | palette->entries[i].green << 8 | palette->entries[i].blue;

      video_render_setphysicalcolor(canvas->videoconfig, i, col, canvas->depth);
   }

   for (i = 0; i < 256; i++) {
      if (pix_bytes == 2)
         video_render_setrawrgb(color_tables, i, RGB565(i, 0, 0), RGB565(0, i, 0), RGB565(0, 0, i));
      else
         video_render_setrawrgb(color_tables, i, i << 16, i << 8, i);
   }
   video_render_initraw(canvas->videoconfig);

   return 0;
}

static void video_canvas_crop(struct video_canvas_s *canvas)
{
   unsigned i                  = 0;
   unsigned j                  = 0;
   unsigned color_diff         = 0;
   unsigned crop_height_max    = CROP_HEIGHT_MAX;
   unsigned crop_top_border    = CROP_TOP_BORDER;
   unsigned crop_bottom_border = CROP_TOP_BORDER + CROP_HEIGHT_MAX;
   unsigned crop_left_border   = CROP_LEFT_BORDER;
   unsigned crop_pad           = 10;
   unsigned crop_counter       = (crop_delay) ? 3 : 0;

   if (!retrow_crop || !retroh_crop)
      return;

#if defined(__X128__)
   if (canvas->videoconfig->rendermode == VIDEO_RENDER_RGBI_1X1)
   {
      crop_top_border     = CROP_VDC_TOP_BORDER;
      crop_left_border    = CROP_VDC_LEFT_BORDER;

      vice_raster.blanked = 0;
   }
#elif defined(__XPET__)
   if (retrow > 384)
      crop_top_border -= 13;
#elif defined(__XCBM2__)
   if (retroh == 366)
   {
      crop_top_border -= 25;
      crop_height_max += 150;
   }
#elif defined(__XPLUS4__)
   if (retroh == 242)
      crop_top_border = CROP_TOP_BORDER_NTSC;
#elif defined(__XVIC__)
   if (retroh == 234)
      crop_top_border = CROP_TOP_BORDER_NTSC;

   vice_raster.blanked = 0;
#endif

   /* Reset to maximum crop */
   vice_raster.first_line = (!crop_id_prev) ? 0 : crop_top_border;
   vice_raster.last_line  = (!crop_id_prev) ? retroh : vice_raster.first_line + crop_height_max;

   switch (crop_id)
   {
      case CROP_AUTO:
#if !defined(__X64__)
      /* Accurate VIC-II requires different method for Auto-Disable */
      case CROP_AUTO_DISABLE:
#endif
         /* Pixel color per row must change, and border colors must
          * differ in order to count as a show-worthy row, otherwise
          * loaders with flashing borders would count as hits */
         color_diff         = 1500 * pix_bytes;
         crop_bottom_border = crop_top_border + crop_height_max;

         /* Top border, start from top */
         for (i = 0; i < crop_top_border && !vice_raster.blanked; i++)
         {
            unsigned row      = i * (retrow << (pix_bytes >> 2));
            unsigned color    = row + (crop_left_border + crop_pad) * (pix_bytes >> 1);
            unsigned lb_color = row + crop_pad * (pix_bytes >> 1);
            unsigned rb_color = row + (retrow - crop_left_border) * (pix_bytes >> 1);
            unsigned found    = 0;

            for (j = crop_left_border + crop_pad; j < retrow - crop_left_border - crop_pad; j++)
            {
               unsigned pixel = row + j * (pix_bytes >> 1);

               if (abs(retro_bmp[pixel] - retro_bmp[color]) > color_diff)
                  found++;

               if (     found
                     && retro_bmp[lb_color] != retro_bmp[pixel]
                     && retro_bmp[rb_color] != retro_bmp[pixel])
               {
#if 0
                  printf("%s: FRST %3d %3d, %3d %d %d\n", __func__, i, j, found, retro_bmp[color], retro_bmp[pixel]);
#endif
                  vice_raster.first_line = i;
                  break;
               }
            }

            if (vice_raster.first_line < crop_top_border)
               break;
         }

#if defined(__X64__) || defined(__X64SC__) || defined(__X64DTV__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         /* Allow bottom border upwards a few rows if top border is not used much.
          * For oddly shifted cases: Alien Syndrome, Out Run Europa */
         if (vice_raster.first_line > 20)
            crop_bottom_border -= 5;
#endif

#if defined(__X128__)
         if (c128_vdc)
         {
            if (vice_raster.first_line < crop_top_border)
            {
               vice_raster.last_line -= crop_top_border - vice_raster.first_line;
               crop_bottom_border    -= crop_top_border - vice_raster.first_line;
            }
         }
#endif

         /* Bottom border, start from bottom, almost */
         for (i = retroh - 2; i > crop_bottom_border && !vice_raster.blanked; i--)
         {
            unsigned row      = i * (retrow << (pix_bytes >> 2));
            unsigned color    = row + (crop_left_border + crop_pad) * (pix_bytes >> 1);
            unsigned lb_color = row + crop_pad * (pix_bytes >> 1);
            unsigned rb_color = row + (retrow - crop_left_border) * (pix_bytes >> 1);
            unsigned found    = 0;

            for (j = crop_left_border + crop_pad; j < retrow - crop_left_border - crop_pad; j++)
            {
               unsigned pixel = row + j * (pix_bytes >> 1);

               if (abs(retro_bmp[pixel] - retro_bmp[color]) > color_diff)
                  found++;

               if (     found
                     && retro_bmp[lb_color] != retro_bmp[pixel]
                     && retro_bmp[rb_color] != retro_bmp[pixel]
                  )
               {
#if 0
                  printf("%s: LAST %3d %3d, %3d %d %d\n", __func__, i, j, found, retro_bmp[color], retro_bmp[pixel]);
#endif
                  vice_raster.last_line = i + 1;
                  break;
               }
            }

            if (vice_raster.last_line > crop_top_border + crop_height_max)
               break;
         }

#if defined(__X128__)
         if (c128_vdc)
         {
            /* Center vertically if the whole area is not used */
            if (vice_raster.first_line < crop_top_border)
            {
               if (i == crop_bottom_border)
               {
                  int center = (vice_raster.last_line - i + 1) / 2;
                  if (center > 1)
                  {
                     vice_raster.first_line -= center;
                     vice_raster.last_line  -= center;
                  }
               }
            }
         }
#endif

         /* Align the resulting screen height to even number */
         if ((vice_raster.last_line - vice_raster.first_line) % 2)
            vice_raster.last_line++;

         /* Auto-Disable for x64sc */
         if (     crop_id == CROP_AUTO_DISABLE
               && (  vice_raster.first_line != crop_top_border
                  || vice_raster.last_line  != vice_raster.first_line + crop_height_max))
         {
            vice_raster.first_line = 0;
            vice_raster.last_line  = retroh;
         }

         /* Result pondering with stabilization period */
         if (vice_raster.first_line != vice_raster.first_line_prev ||
             vice_raster.last_line  != vice_raster.last_line_prev)
         {
            vice_raster.counter = 0;

            /* Require a line step bigger than one */
            if (abs(vice_raster.first_line_active - vice_raster.first_line) > 1)
               vice_raster.first_line_maybe = vice_raster.first_line;
            if (abs(vice_raster.last_line_active - vice_raster.last_line) > 1)
               vice_raster.last_line_maybe  = vice_raster.last_line;
         }

         if (      (  vice_raster.first_line == vice_raster.first_line_maybe
                   || vice_raster.last_line  == vice_raster.last_line_maybe)
                && (  vice_raster.first_line != vice_raster.first_line_active
                   || vice_raster.last_line  != vice_raster.last_line_active))
         {
            vice_raster.counter++;
         }

         if (vice_raster.counter > crop_counter)
         {
            if (     (retroh_crop != vice_raster.last_line - vice_raster.first_line)
                  || vice_raster.first_line != vice_raster.first_line_active
                  || vice_raster.last_line  != vice_raster.last_line_active)
               crop_id_prev               = -1;

            vice_raster.counter           = 0;
            vice_raster.first_line_active = vice_raster.first_line;
            vice_raster.last_line_active  = vice_raster.last_line;
         }

#if 0
         printf("%s: %d, first=%d active=%d maybe=%d prev=%d, last=%d active=%d maybe=%d prev=%d\n", __func__,
               vice_raster.counter,
               vice_raster.first_line, vice_raster.first_line_active, vice_raster.first_line_maybe, vice_raster.first_line_prev,
               vice_raster.last_line, vice_raster.last_line_active, vice_raster.last_line_maybe, vice_raster.last_line_prev);
#endif
         break;

#if defined(__X64__)
      /* Quick line 'blanking' only works with fast core */
      case CROP_AUTO_DISABLE:
         crop_counter = (crop_delay) ? 1 : 0;

         if (!vice_raster.blanked)
         {
            vice_raster.first_line = 0;
            vice_raster.last_line  = retroh;
         }

         /* Result pondering with stabilization period */
         if (vice_raster.first_line != vice_raster.first_line_prev ||
             vice_raster.last_line  != vice_raster.last_line_prev)
         {
            vice_raster.counter          = 0;
            vice_raster.first_line_maybe = vice_raster.first_line;
            vice_raster.last_line_maybe  = vice_raster.last_line;
         }

         if (      (  vice_raster.first_line  == vice_raster.first_line_maybe
                   || vice_raster.last_line   == vice_raster.last_line_maybe)
                && (  vice_raster.first_line != vice_raster.first_line_active
                   || vice_raster.last_line  != vice_raster.last_line_active))
         {
            vice_raster.counter++;
         }

         if (vice_raster.counter > crop_counter)
         {
            if (retroh_crop != vice_raster.last_line - vice_raster.first_line)
               crop_id_prev               = -1;

            vice_raster.counter           = 0;
            vice_raster.first_line_active = vice_raster.first_line;
            vice_raster.last_line_active  = vice_raster.last_line;
         }
         break;
#endif

      default:
         break;
   }

   vice_raster.first_line_prev = vice_raster.first_line;
   vice_raster.last_line_prev  = vice_raster.last_line;
   vice_raster.blanked         = 0;
}

void video_canvas_refresh(struct video_canvas_s *canvas,
      unsigned int xs, unsigned int ys,
      unsigned int xi, unsigned int yi,
      unsigned int w, unsigned int h)
{
#ifdef RETRO_DEBUG
   printf("XS:%d YS:%d XI:%d YI:%d W:%d H:%d\n",xs,ys,xi,yi,w,h);
#endif

   video_canvas_render(
         canvas, (uint8_t *)&retro_bmp,
         retrow, retroh,
         retroXS, retroYS,
         0, 0, /*xi, yi,*/
         retrow * pix_bytes
   );

   /* Automatic crop */
   if (crop_id >= CROP_AUTO)
      video_canvas_crop(canvas);

   /* Virtual keyboard */
   if (retro_vkbd)
      print_vkbd();
}

int video_init()
{
   return 0;
}

void video_shutdown()
{
}

int video_arch_resources_init()
{
   return 0;
}

void video_arch_resources_shutdown()
{
}

void fullscreen_capability(struct cap_fullscreen_s *cap_fullscreen)
{
}

#if defined(__X128__)
int video_arch_get_active_chip(void)
{
   return c128_vdc ? VIDEO_CHIP_VDC : VIDEO_CHIP_VICII;
}
#endif

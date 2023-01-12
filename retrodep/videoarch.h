#ifndef VICE_VIDEOARCH_H
#define VICE_VIDEOARCH_H

#include "archdep.h"
#include "video.h"

typedef struct video_canvas_s {
    /** \brief Nonzero if it is safe to access other members of the
     *         structure. */
    unsigned int initialized;

    /** \brief Nonzero if the structure has been fully realized. */
    unsigned int created;

    /** \brief Index of the canvas, needed for x128 and xcbm2 */
    int index;

    /** \brief Depth of the canvas in bpp */
    unsigned int depth;

    /** \brief Rendering configuration as seen by the emulator
     *         core. */
    struct video_render_config_s *videoconfig;

    /** \brief Drawing buffer as seen by the emulator core. */
    struct draw_buffer_s *draw_buffer;

    /** \brief Display window as seen by the emulator core. */
    struct viewport_s *viewport;

    /** \brief Machine screen geometry as seen by the emulator
     *         core. */
    struct geometry_s *geometry;

    /** \brief Color palette for translating display results into
     *         window colors. */
    struct palette_s *palette;

    /** \brief Methods for managing the draw buffer when the core
     *         rasterizer handles it. */
    struct video_draw_buffer_callback_s *video_draw_buffer_callback;

    /** \brief Used to limit frame rate under warp. */
    tick_t warp_next_render_tick;
    
    int crt_type;
} video_canvas_t;

typedef struct vice_renderer_backend_s {
} vice_renderer_backend_t;

#endif

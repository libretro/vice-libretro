/*
 * soundretro.c - Simple forwarding sound device for libretro
 *
 */

#ifdef __LIBRETRO__

#include "vice.h"
#include "sound.h"

#include "libretro-core.h"
extern void retro_audio_render(const int16_t *data, size_t frames);

static int retro_sound_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    *speed = vice_opt.SoundSampleRate;
#if 0
    printf("speed:%d fragsize:%d fragnr:%d channels:%d\n", *speed, *fragsize, *fragnr, *channels);
#endif
    return 0;
}

static int retro_write(SWORD *pbuf, size_t nr)
{
#if 0
    printf("pbuf:%d nr:%d\n", *pbuf, nr);
#endif
    retro_audio_render(pbuf, nr);
    return 0;
}

static int retro_flush(char *state)
{
    return 0;
}

static sound_device_t retro_device =
{
    "retro",
    retro_sound_init,
    retro_write,
    NULL,
    NULL,/*retro_flush,*/
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    2
};

int sound_init_retro_device(void)
{
    return sound_register_device(&retro_device);
}

#endif

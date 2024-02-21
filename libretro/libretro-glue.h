#ifndef LIBRETRO_GLUE_H
#define LIBRETRO_GLUE_H
#include "libretro-dc.h"

/* zlib */
#include "deps/libz/zlib.h"
#include "deps/libz/unzip.h"
void gz_compress(const char *in, const char *out);
void gz_uncompress(const char *in, const char *out);
void zip_uncompress(char *in, char *out, char *lastfile);

/* 7z */
#include "deps/7zip/7z.h"
#include "deps/7zip/7zBuf.h"
#include "deps/7zip/7zCrc.h"
#include "deps/7zip/7zFile.h"
#include "deps/7zip/7zTypes.h"
void sevenzip_uncompress(char *in, char *out, char *lastfile);

/* NIBTOOLS */
int nib_convert(char *in, char *out);

/* Misc */
int qstrcmp(const void *a, const void *b);
int retro_remove(const char *path);
void remove_recurse(const char *path);
void m3u_scan_recurse(const char *path, zip_m3u_t *list);

/* String helpers functions */
char* trimwhitespace(char *str);
char* strleft(const char* str, int len);
char* strright(const char* str, int len);
bool strstartswith(const char* str, const char* start);
bool strendswith(const char* str, const char* end);
void path_join(char* out, const char* basedir, const char* filename);
char *path_remove_program(char *path);
char *first_file_in_dir(char *path);

/* VICE helpers */
extern int log_resources_set_int(const char *name, int value);
extern int log_resources_set_string(const char *name, const char *value);

#endif /* LIBRETRO_GLUE_H */

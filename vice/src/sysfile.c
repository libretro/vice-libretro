/*
 * sysfile.c - Simple locator for VICE system files.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "findpath.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "sysfile.h"
#include "util.h"

/* #define DBGSYSFILE */

#ifdef DBGSYSFILE
#define DBG(x)  log_printf x
#else
#define DBG(x)
#endif

#ifdef __LIBRETRO__
#ifdef USE_EMBEDDED
#include "embedded.h"
#endif
#endif

/* Resources.  */

static char *default_path = NULL;
static char *system_path = NULL;
static char *expanded_system_path = NULL;

static int set_system_path(const char *val, void *param)
{
    char *tmp_path, *tmp_path_save, *p, *s, *current_dir;

    util_string_set(&system_path, val);

    lib_free(expanded_system_path);
    expanded_system_path = NULL; /* will subsequently be replaced */

    tmp_path_save = util_subst(system_path, "$$", default_path); /* malloc'd */

    current_dir = archdep_current_dir();

    tmp_path = tmp_path_save; /* tmp_path points into tmp_path_save */
    for (;;) {
        p = strstr(tmp_path, ARCHDEP_FINDPATH_SEPARATOR_STRING);

        if (p != NULL) {
            *p = 0;
        }
        if (!archdep_path_is_relative(tmp_path)) {
            /* absolute path */
            if (expanded_system_path == NULL) {
                s = util_concat(tmp_path, NULL); /* concat allocs a new str. */
            } else {
                s = util_concat(expanded_system_path,
                                ARCHDEP_FINDPATH_SEPARATOR_STRING,
                                tmp_path, NULL );
            }
        } else { /* relative path */
            if (expanded_system_path == NULL) {
                s = util_concat(current_dir,
                                ARCHDEP_DIR_SEP_STR,
                                tmp_path, NULL );
            } else {
                s = util_concat(expanded_system_path,
                                ARCHDEP_FINDPATH_SEPARATOR_STRING,
                                current_dir,
                                ARCHDEP_DIR_SEP_STR,
                                tmp_path, NULL );
            }
        }
        lib_free(expanded_system_path);
        expanded_system_path = s;

        if (p == NULL) {
            break;
        }

        tmp_path = p + strlen(ARCHDEP_FINDPATH_SEPARATOR_STRING);
    }

    lib_free(current_dir);
    lib_free(tmp_path_save);

    DBG(("set_system_path -> expanded_system_path:'%s'", expanded_system_path));

    return 0;
}

const char *get_system_path(void)
{
    return expanded_system_path;
}

static const resource_string_t resources_string[] = {
    { "Directory", "$$", RES_EVENT_NO, NULL,
      &system_path, set_system_path, NULL },
    RESOURCE_STRING_LIST_END
};

/* Command-line options.  */

static const cmdline_option_t cmdline_options[] =
{
    { "-directory", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Directory", NULL,
      "<Path>", "Define search path to locate system files" },
    CMDLINE_LIST_END
};

/* ------------------------------------------------------------------------- */

int sysfile_init(const char *emu_id)
{
    default_path = archdep_default_sysfile_pathlist(emu_id);
    DBG(("sysfile_init(%s) -> default_path:'%s'", emu_id, default_path));
    /* HACK: set the default value early, so the systemfile locater also works
             in early startup */
    set_system_path("$$", NULL);
    return 0;
}

void sysfile_shutdown(void)
{
    lib_free(default_path);
    lib_free(expanded_system_path);
}

int sysfile_resources_init(void)
{
    return resources_register_string(resources_string);
}

void sysfile_resources_shutdown(void)
{
    lib_free(system_path);
}

int sysfile_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* Locate a system file called `name' by using the search path in
   `Directory', checking that the file can be accesses in mode `mode', and
   return an open stdio stream for that file.  If `complete_path_return' is
   not NULL, `*complete_path_return' points to a malloced string with the
   complete path if the file was found or is NULL if not.  */
FILE *sysfile_open(const char *name, const char *subpath, char **complete_path_return,
                   const char *open_mode)
{
    char *p = NULL;
    FILE *f;

    if (name == NULL || *name == '\0') {
        log_error(LOG_DEFAULT, "Missing name for system file.");
        return NULL;
    }

    /*
     * name      - filename or command we are looking for in the resulting path
     * expanded_system_path  - list of search path(es), separated by target specific separator
     * subpath  - path tail component, will be appended to the resulting path
     */
    p = findpath(name, expanded_system_path, subpath, ARCHDEP_ACCESS_R_OK);

    if (p == NULL) {
        if (complete_path_return != NULL) {
            *complete_path_return = NULL;
        }
        return NULL;
    } else {
        unsigned int isdir = 0;

        /* make sure we're not opening a directory */
        archdep_stat(p, NULL, &isdir);
        if (isdir) {
            log_error(LOG_DEFAULT, "'%s' is a directory, not a file.", p);
            if (complete_path_return != NULL) {
                *complete_path_return = NULL;
            }
            return NULL;
        }

        f = fopen(p, open_mode);

        if (f == NULL || complete_path_return == NULL) {
            lib_free(p);
            p = NULL;
        }
        if (complete_path_return != NULL) {
            *complete_path_return = p;
        }
        return f;
    }
}

/* As `sysfile_open', but do not open the file.  Just return 0 if the file is
   found and is readable, or -1 if an error occurs.  */
int sysfile_locate(const char *name, const char *subpath, char **complete_path_return)
{
    FILE *f = sysfile_open(name, subpath, complete_path_return, MODE_READ);

    if (f != NULL) {
        fclose(f);
        return 0;
    } else {
        return -1;
    }
}

/* ------------------------------------------------------------------------- */

/*
 * If minsize >= 0, and the file is smaller than maxsize, load the data
 * into the end of the memory range.
 * If minsize < 0, load it at the start.
 */
int sysfile_load(const char *name, const char *subpath, uint8_t *dest, int minsize, int maxsize)
{
    FILE *fp = NULL;
    size_t rsize = 0;
    off_t tmpsize;
    char *complete_path = NULL;
    int load_at_end;

#ifdef USE_EMBEDDED
    if ((rsize = embedded_check_file(name, dest, minsize, maxsize)) != 0) {
        return (int)rsize;
    }
#endif

    fp = sysfile_open(name, subpath, &complete_path, MODE_READ);

    if (fp == NULL) {
        /* Try to open the file from the current directory. */
        const char working_dir_prefix[3] = {
            '.', ARCHDEP_DIR_SEP_CHR, '\0'
        };
        char *local_name = NULL;

        local_name = util_concat(working_dir_prefix, name, NULL);
        fp = sysfile_open((const char *)local_name, subpath, &complete_path, MODE_READ);
        lib_free(local_name);
        local_name = NULL;

        if (fp == NULL) {
            goto fail;
        }
    }

    log_message(LOG_DEFAULT, "Loading system file `%s'.", complete_path);

    tmpsize = archdep_file_size(fp);
    if (tmpsize < 0) {
        log_message(LOG_DEFAULT, "Failed to determine size of '%s'.", complete_path);
        goto fail;
    }
    rsize = (size_t)tmpsize;
    if (minsize < 0) {
        minsize = -minsize;
        load_at_end = 0;
    } else {
        load_at_end = 1;
    }

    if (rsize < ((size_t)minsize)) {
        log_error(LOG_DEFAULT, "ROM %s: short file.", complete_path);
        goto fail;
    }
    if (rsize == ((size_t)maxsize + 2)) {
        log_warning(LOG_DEFAULT,
                    "ROM `%s': two bytes too large - removing assumed "
                    "start address.", complete_path);
        if (fread((char *)dest, 1, 2, fp) < 2) {
            goto fail;
        }
        rsize -= 2;
    }
    if (load_at_end && rsize < ((size_t)maxsize)) {
        dest += maxsize - rsize;
    } else if (rsize > ((size_t)maxsize)) {
        log_warning(LOG_DEFAULT,
                    "ROM `%s': long file (%"PRI_SIZE_T"), discarding end (%"PRI_SIZE_T" bytes).",
                    complete_path, rsize, rsize - maxsize);
        rsize = maxsize;
    }
    if ((rsize = fread((char *)dest, 1, rsize, fp)) < ((size_t)minsize)) {
        goto fail;
    }

    fclose(fp);
    lib_free(complete_path);
    return (int)rsize;  /* return ok */

fail:
    lib_free(complete_path);
    return -1;
}

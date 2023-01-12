#include "libretro-core.h"
#include "encodings/utf.h"
#include "streams/file_stream.h"

extern char retro_temp_directory[RETRO_PATH_MAX];
extern retro_log_printf_t log_cb;

#include "archdep.h"

/* Misc */
int sensible_strcmp(char *a, char *b)
{
   int i;
   for (i = 0; a[i] == b[i]; i++)
      if (a[i] == '\0')
         return 0;
   /* Replace " " (32) with "/" (47) when comparing for more natural sorting, such as:
    * 1. Spy vs Spy                                   1. Last Ninja, The
    * 2. Spy vs Spy II - The Island Caper     and     2. Last Ninja 2
    * 3. Spy vs Spy III - Arctic Antics               3. Last Ninja 3
    * Because "/" (47) is bigger than "," (44) and "." (46), and it is not used in filenames */
   if (a[i] == 32)
      return (47 < (unsigned char)b[i]) ? -1 : 1;
   if (b[i] == 32)
      return ((unsigned char)a[i] < 47) ? -1 : 1;
   return ((unsigned char)a[i] < (unsigned char)b[i]) ? -1 : 1;
}

int qstrcmp(const void *a, const void *b)
{
   char *pa = (char *)a;
   char *pb = (char *)b;
   return sensible_strcmp(pa, pb);
}

void remove_recurse(const char *path)
{
   struct dirent *dirp;
   char filename[RETRO_PATH_MAX];
   DIR *dir = opendir(path);
   if (dir == NULL)
      return;

   while ((dirp = readdir(dir)) != NULL)
   {
      if (dirp->d_name[0] == '.')
         continue;

      sprintf(filename, "%s%s%s", path, ARCHDEP_DIR_SEP_STR, dirp->d_name);
      log_cb(RETRO_LOG_INFO, "Clean: %s\n", filename);

      if (path_is_directory(filename))
         remove_recurse(filename);
      else
         remove(filename);
   }

   closedir(dir);

   /* Leave the root directory for RAM disk usage */
   if (strcmp(retro_temp_directory, path))
      archdep_rmdir(path);
}

void m3u_scan_recurse(const char *path, zip_m3u_t *list)
{
   DIR *zip_dir;
   struct dirent *zip_dirp;
   char *zip_lastfile = {0};

   zip_dir = opendir(path);

   while ((zip_dirp = readdir(zip_dir)) != NULL)
   {
      char zip_fullpath[RETRO_PATH_MAX] = {0};

      if (zip_dirp->d_name[0] == '.' || strendswith(zip_dirp->d_name, ".m3u") || list->mode > 1)
         continue;

      path_join(zip_fullpath, retro_temp_directory, zip_dirp->d_name);
      if (path_is_directory(zip_fullpath))
      {
         m3u_scan_recurse(zip_fullpath, list);
         continue;
      }

      path_join(zip_fullpath, path, zip_dirp->d_name);
      if (!strcmp(path, retro_temp_directory))
         zip_lastfile = local_to_utf8_string_alloc(zip_dirp->d_name);
      else
         zip_lastfile = local_to_utf8_string_alloc(zip_fullpath);

      /* Multi file mode, generate playlist */
      if (dc_get_image_type(zip_dirp->d_name) == DC_IMAGE_TYPE_FLOPPY
       || dc_get_image_type(zip_dirp->d_name) == DC_IMAGE_TYPE_TAPE
       || dc_get_image_type(zip_dirp->d_name) == DC_IMAGE_TYPE_MEM
      )
      {
         list->mode = 1;
         list->num++;
         snprintf(list->list[list->num-1], RETRO_PATH_MAX, "%s", zip_lastfile);
      }
   }
   closedir(zip_dir);

   if (zip_lastfile)
      free(zip_lastfile);
   zip_lastfile = NULL;
}

void path_join(char* out, const char* basedir, const char* filename)
{
   snprintf(out, RETRO_PATH_MAX, "%s%s%s", basedir, ARCHDEP_DIR_SEP_STR, filename);
}

/* Note: This function returns a pointer to a substring_left of the original string.
 * If the given string was allocated dynamically, the caller must not overwrite
 * that pointer with the returned value, since the original pointer must be
 * deallocated using the same allocator with which it was allocated. The return
 * value must NOT be deallocated using free() etc. */
char* trimwhitespace(char *str)
{
   char *end;

   /* Trim leading space */
   while (isspace((unsigned char)*str)) str++;

   if (*str == 0) /* All spaces? */
      return str;

   /* Trim trailing space */
   end = str + strlen(str) - 1;
   while (end > str && isspace((unsigned char)*end)) end--;

   /* Write new null terminator character */
   end[1] = '\0';

   return str;
}

/* Returns a substring of 'str' that contains the 'len' leftmost characters of 'str'. */
char* strleft(const char* str, int len)
{
   char* result = calloc(len + 1, sizeof(char));
   strncpy(result, str, len);
   return result;
}

/* Returns a substring of 'str' that contains the 'len' rightmost characters of 'str'. */
char* strright(const char* str, int len)
{
   int pos = strlen(str) - len;
   char* result = calloc(len + 1, sizeof(char));
   strncpy(result, str + pos, len);
   return result;
}

/* Returns true if 'str' starts with 'start' */
bool strstartswith(const char* str, const char* start)
{
   if (strlen(str) >= strlen(start))
      if (!strncasecmp(str, start, strlen(start)))
         return true;

   return false;
}

/* Returns true if 'str' ends with 'end' */
bool strendswith(const char* str, const char* end)
{
   if (strlen(str) >= strlen(end))
      if (!strcasecmp((char*)&str[strlen(str)-strlen(end)], end))
         return true;

   return false;
}

/* Removes ':PRG' */
char *path_remove_program(char *path)
{
   char *last = !string_is_empty(path)
      ? (char*)strrchr(path_basename(path), ':') : NULL;
   if (!last)
      return NULL;
   if (*last)
      *last = '\0';
   return path;
}

char *first_file_in_dir(char *path)
{
   DIR *path_dir;
   struct dirent *path_dirp;

   path_dir = opendir(path);
   char *path_lastfile = {0};
   while ((path_dirp = readdir(path_dir)) != NULL && string_is_empty(path_lastfile))
   {
      if (path_dirp->d_name[0] == '.')
         continue;

      path_lastfile = local_to_utf8_string_alloc(path_dirp->d_name);
   }
   closedir(path_dir);

   return path_lastfile;
}

/* zlib */
void zip_uncompress(char *in, char *out, char *lastfile)
{
   uLong i;
   unz_global_info gi;

   unzFile uf           = NULL;
   char *in_local       = NULL;
   const char* password = NULL;
   int size_buf         = 8192;
   int err;

   in_local             = utf8_to_local_string_alloc(in);
   uf                   = unzOpen(in_local);

   free(in_local);
   in_local = NULL;

   err = unzGetGlobalInfo (uf, &gi);

   for (i = 0; i < gi.number_entry; i++)
   {
      char filename_inzip[256];
      char filename_withpath[512];
      char* filename_withoutpath;
      char* p;
      unz_file_info file_info;
      FILE *fout = NULL;
      void* buf;

      filename_inzip[0]    = '\0';
      filename_withpath[0] = '\0';

      buf = (void*)malloc(size_buf);
      if (buf == NULL)
      {
         log_cb(RETRO_LOG_ERROR, "Unzip: Error allocating memory\n");
         return;
      }

      err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
      snprintf(filename_withpath, sizeof(filename_withpath), "%s%s%s", out, ARCHDEP_DIR_SEP_STR, filename_inzip);
      if ((dc_get_image_type(filename_inzip) == DC_IMAGE_TYPE_FLOPPY ||
           dc_get_image_type(filename_inzip) == DC_IMAGE_TYPE_TAPE) && lastfile != NULL)
            snprintf(lastfile, RETRO_PATH_MAX, "%s", filename_inzip);

      p = filename_withoutpath = filename_inzip;
      while ((*p) != '\0')
      {
         if (((*p) == '/') || ((*p) == '\\'))
            filename_withoutpath = p + 1;
         p++;
      }

      if ((*filename_withoutpath) == '\0')
      {
         log_cb(RETRO_LOG_INFO, "Mkdir: %s\n", filename_withpath);
         path_mkdir(filename_withpath);
      }
      else if (!path_is_valid(filename_withpath))
      {
         char* write_filename;
         unsigned skip = 0;
         unsigned x    = 0;

         write_filename = local_to_utf8_string_alloc(filename_withpath);

         /* Replace non-ascii chars with underscore */
         for (x = 128; x < 256; x++)
            string_replace_all_chars(write_filename, x, '_');

         err = unzOpenCurrentFilePassword(uf, password);
         if (err != UNZ_OK)
            log_cb(RETRO_LOG_ERROR, "Unzip: Error %d with zipfile in unzOpenCurrentFilePassword: %s\n", err, write_filename);

         if ((skip == 0) && (err == UNZ_OK))
         {
            fout = fopen(write_filename, "wb");
            if (fout == NULL)
               log_cb(RETRO_LOG_ERROR, "Unzip: Error opening %s\n", write_filename);
         }

         if (fout != NULL)
         {
            log_cb(RETRO_LOG_INFO, "Unzip: %s\n", write_filename);

            do
            {
               err = unzReadCurrentFile(uf, buf, size_buf);
               if (err < 0)
               {
                  log_cb(RETRO_LOG_ERROR, "Unzip: Error %d with zipfile in unzReadCurrentFile\n", err);
                  break;
               }
               if (err > 0)
               {
                  if (!fwrite(buf, err, 1, fout))
                  {
                     log_cb(RETRO_LOG_ERROR, "Unzip: Error writing extracted file %s\n", write_filename);
                     err = UNZ_ERRNO;
                     break;
                  }
               }
            }
            while (err > 0);
            if (fout)
               fclose(fout);
         }

         free(write_filename);
         write_filename = NULL;

         if (err == UNZ_OK)
         {
            err = unzCloseCurrentFile(uf);
            if (err != UNZ_OK)
               log_cb(RETRO_LOG_ERROR, "Unzip: Error %d with zipfile in unzCloseCurrentFile\n", err);
         }
         else
            unzCloseCurrentFile(uf);
      }

      free(buf);

      if ((i + 1) < gi.number_entry)
      {
         err = unzGoToNextFile(uf);
         if (err != UNZ_OK)
         {
            log_cb(RETRO_LOG_ERROR, "Unzip: Error %d with zipfile in unzGoToNextFile\n", err);
            break;
         }
      }
   }

   if (uf)
   {
      unzCloseCurrentFile(uf);
      unzClose(uf);
      uf = NULL;
   }
}

/* 7zip */
#ifdef HAVE_7ZIP
#define SEVENZIP_LOOKTOREAD_BUF_SIZE (1 << 14)
struct sevenzip_context_t
{
   uint8_t *output;
   CFileInStream archiveStream;
   CLookToRead2 lookStream;
   ISzAlloc allocImp;
   ISzAlloc allocTempImp;
   CSzArEx db;
   size_t temp_size;
   uint32_t block_index;
   uint32_t parse_index;
   uint32_t decompress_index;
   uint32_t packIndex;
};

static void *sevenzip_stream_alloc_impl(ISzAllocPtr p, size_t size)
{
   if (size == 0)
      return 0;
   return malloc(size);
}

static void sevenzip_stream_free_impl(ISzAllocPtr p, void *address)
{
   (void)p;

   if (address)
      free(address);
}

static void *sevenzip_stream_alloc_tmp_impl(ISzAllocPtr p, size_t size)
{
   (void)p;
   if (size == 0)
      return 0;
   return malloc(size);
}

void sevenzip_uncompress(char *in, char *out, char *lastfile)
{
   CFileInStream archiveStream;
   CLookToRead2 lookStream;
   ISzAlloc allocImp;
   ISzAlloc allocTempImp;
   CSzArEx db;
   uint8_t *output      = 0;
   int64_t outsize      = -1;

   /*These are the allocation routines.
    * Currently using the non-standard 7zip choices. */
   allocImp.Alloc       = sevenzip_stream_alloc_impl;
   allocImp.Free        = sevenzip_stream_free_impl;
   allocTempImp.Alloc   = sevenzip_stream_alloc_tmp_impl;
   allocTempImp.Free    = sevenzip_stream_free_impl;

   lookStream.bufSize   = SEVENZIP_LOOKTOREAD_BUF_SIZE * sizeof(Byte);
   lookStream.buf       = (Byte*)malloc(lookStream.bufSize);

   if (!lookStream.buf)
      lookStream.bufSize = 0;

#if defined(_WIN32) && defined(USE_WINDOWS_FILE) && !defined(LEGACY_WIN32)
   if (!string_is_empty(in))
   {
      wchar_t *pathW = utf8_to_utf16_string_alloc(in);

      if (pathW)
      {
         /* Could not open 7zip archive? */
         if (InFile_OpenW(&archiveStream.file, pathW))
         {
            free(pathW);
            return;
         }

         free(pathW);
      }
   }
#else
   /* Could not open 7zip archive? */
   if (InFile_Open(&archiveStream.file, in))
      return;
#endif

   FileInStream_CreateVTable(&archiveStream);
   LookToRead2_CreateVTable(&lookStream, false);
   lookStream.realStream = &archiveStream.vt;
   LookToRead2_Init(&lookStream);
   CrcGenerateTable();

   memset(&db, 0, sizeof(db));

   SzArEx_Init(&db);

   if (SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp) == SZ_OK)
   {
      uint32_t i;
      uint16_t *temp       = NULL;
      size_t temp_size     = 0;
      uint32_t block_index = 0xFFFFFFFF;
      SRes res             = SZ_OK;
      size_t output_size   = 0;

      for (i = 0; i < db.NumFiles; i++)
      {
         size_t j;
         size_t len;
         char infile[RETRO_PATH_MAX];
         size_t offset                = 0;
         size_t outSizeProcessed      = 0;
         unsigned x;

         len = SzArEx_GetFileNameUtf16(&db, i, NULL);

         if (len > temp_size)
         {
            if (temp)
               free(temp);
            temp_size = len;
            temp = (uint16_t *)malloc(temp_size * sizeof(temp[0]));

            if (temp == 0)
            {
               res = SZ_ERROR_MEM;
               break;
            }
         }

         SzArEx_GetFileNameUtf16(&db, i, temp);
         res       = SZ_ERROR_FAIL;
         infile[0] = '\0';

         if (!temp)
            break;

         res = utf16_to_char_string(temp, infile, sizeof(infile))
               ? SZ_OK : SZ_ERROR_FAIL;

         /* C LZMA SDK does not support chunked extraction - see here:
          * sourceforge.net/p/sevenzip/discussion/45798/thread/6fb59aaf/
          * */
         res = SzArEx_Extract(&db, &lookStream.vt, i, &block_index,
               &output, &output_size, &offset, &outSizeProcessed,
               &allocImp, &allocTempImp);

         if (res != SZ_OK)
            break; /* This goes to the error section. */

         outsize = (int64_t)outSizeProcessed;

         char output_path[RETRO_PATH_MAX] = {0};
         snprintf(output_path, RETRO_PATH_MAX, "%s%s%s", out, ARCHDEP_DIR_SEP_STR, infile);
         if (dc_get_image_type(output_path) == DC_IMAGE_TYPE_FLOPPY && lastfile != NULL)
            snprintf(lastfile, RETRO_PATH_MAX, "%s", path_basename(output_path));

#if 0
         /* Replace non-ascii chars with underscore */
         for (x = 128; x < 256; x++)
            string_replace_all_chars(output_path, x, '_');
#endif

         for (j = 0; output_path[j] != 0; j++)
         {
            if (output_path[j] == '/')
            {
               output_path[j] = 0;
               path_mkdir((const char *)output_path);
               output_path[j] = ARCHDEP_DIR_SEP_CHR;
            }
         }

         const void *ptr = (const void*)(output + offset);

         if (path_is_valid(output_path))
            continue;
         else if (SzArEx_IsDir(&db, i))
         {
            path_mkdir((const char *)temp);
            log_cb(RETRO_LOG_INFO, "Mkdir: %s\n", output_path);
            continue;
         }

         if (filestream_write_file(output_path, ptr, outsize))
         {
            res = SZ_OK;
            log_cb(RETRO_LOG_INFO, "Un7ip: %s\n", output_path);
         }
         else
         {
            res = SZ_ERROR_FAIL;
            log_cb(RETRO_LOG_ERROR, "Un7ip: Error writing extracted file %s\n", output_path);
         }
      }

      if (temp)
         free(temp);
      IAlloc_Free(&allocImp, output);

      if (res == SZ_ERROR_UNSUPPORTED)
         log_cb(RETRO_LOG_ERROR, "Un7ip: Decoder doesn't support this archive\n");
      else if (res == SZ_ERROR_MEM)
         log_cb(RETRO_LOG_ERROR, "Un7ip: Can not allocate memory\n");
      else if (res == SZ_ERROR_CRC)
         log_cb(RETRO_LOG_ERROR, "Un7ip: CRC error\n");
   }

   SzArEx_Free(&db, &allocImp);
   File_Close(&archiveStream.file);
}
#else
void sevenzip_uncompress(char *in, char *out, char *lastfile)
{
}
#endif

/* NIBTOOLS */
typedef unsigned char __u_char;
#include "deps/nibtools/nibtools.h"

#include "deps/nibtools/gcr.c"
#include "deps/nibtools/prot.c"
#include "deps/nibtools/crc.c"
#include "deps/nibtools/bitshifter.c"
#include "deps/nibtools/lz.c"

#undef printf
#define printf(format, ...) log_cb(RETRO_LOG_INFO, format, __VA_ARGS__)
char log_output[512] = {0};

BYTE compressed_buffer[(MAX_HALFTRACKS_1541 + 2) * NIB_TRACK_LENGTH];
BYTE file_buffer[(MAX_HALFTRACKS_1541 + 2) * NIB_TRACK_LENGTH];
BYTE track_buffer[(MAX_HALFTRACKS_1541 + 2) * NIB_TRACK_LENGTH];
BYTE track_density[MAX_HALFTRACKS_1541 + 2];
BYTE track_alignment[MAX_HALFTRACKS_1541 + 2];
size_t track_length[MAX_HALFTRACKS_1541 + 2];
int file_buffer_size;
int start_track, end_track, track_inc;
int reduce_sync, reduce_badgcr, reduce_gap;
int fix_gcr, align, force_align;
int gap_match_length;
int cap_min_ignore;
int skip_halftracks;
int verbose;
int rpm_real;
int auto_capacity_adjust;
int skew;
int align_disk;
int ihs;
int mode;
int unformat_passes;
int capacity_margin;
int align_delay;
int increase_sync = 0;
int presync = 0;
BYTE fillbyte = 0xfe;
BYTE drive = 8;
char * cbm_adapter = "";
int use_floppycode_srq = 0;
int override_srq = 0;
int extra_capacity_margin=5;
int sync_align_buffer=0;
int fattrack=0;
int track_match=0;
int old_g64=0;
int read_killer=1;
int backwards=0;

/* Skip halftrack compression, fixes "Ski or Die", but breaks "Ultimate Wizard"
 * unless density is taken into consideration */
bool skip_compress_halftrack = true;
unsigned int skip_compress_halftrack_density = 2;

int write_dword(FILE * fd, unsigned int * buf, int num)
{
	int i;
	BYTE *tmpbuf;

	tmpbuf = malloc(num);

	for (i = 0; i < (num / 4); i++)
	{
		tmpbuf[i * 4] = buf[i] & 0xff;
		tmpbuf[i * 4 + 1] = (buf[i] >> 8) & 0xff;
		tmpbuf[i * 4 + 2] = (buf[i] >> 16) & 0xff;
		tmpbuf[i * 4 + 3] = (buf[i] >> 24) & 0xff;
	}

	if (fwrite(tmpbuf, num, 1, fd) < 1)
	{
		free(tmpbuf);
		return -1;
	}
	free(tmpbuf);
	return 0;
}

int compare_extension(unsigned char * filename, unsigned char * extension)
{
	char *dot;

	dot = strrchr((const char *)filename, '.');
	if (dot == NULL)
		return (0);

	for (++dot; *dot != '\0'; dot++, extension++)
		if (tolower(*dot) != tolower(*extension))
			return (0);

	if (*extension == '\0')
		return (1);
	else
		return (0);
}

int load_file(char *filename, BYTE *file_buffer)
{
	int size;
	FILE *fpin;

	if (verbose) printf("Loading \"%s\"...\n", filename);

	if ((fpin = fopen(filename, "rb")) == NULL)
	{
		printf("Couldn't open input file %s!\n", filename);
		return 0;
	}

	fseek(fpin, 0, SEEK_END);
	size = ftell(fpin);
	rewind(fpin);

	if (fread(file_buffer, size, 1, fpin) != 1) {
		printf("%s", "unable to read file\n");
		return 0;
	}

	if (verbose) printf("Successfully loaded %d bytes.\n", size);
	fclose(fpin);
	return size;
}

int read_nib(BYTE *file_buffer, int file_buffer_size, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
	int track, t_index=0, h_index=0;

	if (verbose) printf("%s", "Parsing NIB data...\n");

	if (memcmp(file_buffer, "MNIB-1541-RAW", 13) != 0)
	{
		printf("%s", "Not valid NIB data!\n");
		return 1;
	}
	else if (verbose) printf("NIB file version %d\n", file_buffer[13]);

	while(file_buffer[0x10+h_index])
	{
		track = file_buffer[0x10+h_index];
		track_density[track] = (BYTE)(file_buffer[0x10 + h_index + 1]);
		track_density[track] %= BM_MATCH;  	 /* discard unused BM_MATCH mark */

		memcpy(track_buffer + (track * NIB_TRACK_LENGTH),
			file_buffer + (t_index * NIB_TRACK_LENGTH) + 0x100,
			NIB_TRACK_LENGTH);

		h_index+=2;
		t_index++;
	}
	if (verbose) printf("Successfully parsed NIB data for %d tracks\n", t_index);
	return 1;
}

int align_tracks(BYTE *track_buffer, BYTE *track_density, size_t *track_length, BYTE *track_alignment)
{
	int track;
	BYTE nibdata[NIB_TRACK_LENGTH];

	memset(nibdata, 0, sizeof(nibdata));
	if (verbose) printf("%s", "Aligning tracks...\n");

	for (track = 1; track <= 84; track ++)
	{
		memcpy(nibdata, track_buffer+(track*NIB_TRACK_LENGTH), NIB_TRACK_LENGTH);
		memset(track_buffer + (track * NIB_TRACK_LENGTH), 0x00, NIB_TRACK_LENGTH);

		/* process track cycle */
		track_length[track] = extract_GCR_track(
			track_buffer + (track * NIB_TRACK_LENGTH),
			nibdata,
			&track_alignment[track],
			track/2,
			capacity_min[track_density[track]&3],
			capacity_max[track_density[track]&3]
		);

		/* output some specs */
		if((verbose)&&(track_length[track]>0))
		{
			log_output[0] = '\0';
			snprintf(log_output + strlen(log_output), sizeof(log_output), "%4.1f: ", (float) track/2);
			if(track_density[track] & BM_NO_SYNC) strcat(log_output, "NOSYNC:");
			if(track_density[track] & BM_FF_TRACK) strcat(log_output, "KILLER:");
			snprintf(log_output + strlen(log_output), sizeof(log_output), "(%d:", track_density[track]&3);
			snprintf(log_output + strlen(log_output), sizeof(log_output), "%d) ", track_length[track]);
			snprintf(log_output + strlen(log_output), sizeof(log_output), "[align=%s]", alignments[track_alignment[track]]);
			printf("%s\n", log_output);
		}
	}
	return 1;
}

int write_g64(char *filename, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
	/* writes contents of buffers into G64 file, with header and density information */

	/* when writing a G64 file, we don't care about the limitations of drive hardware
		However, VICE (previous to version 2.2) ignored the G64 header and hardcodes 7928 as the largest
		track size, and also requires it to be 84 tracks no matter if they're used or not.
	*/

	#define OLD_G64_TRACK_MAXLEN 8192
	DWORD G64_TRACK_MAXLEN=7928;
	BYTE header[12];
	DWORD gcr_track_p[MAX_HALFTRACKS_1541] = {0};
	DWORD gcr_speed_p[MAX_HALFTRACKS_1541] = {0};
	/*BYTE gcr_track[G64_TRACK_MAXLEN + 2]; */
	BYTE gcr_track[NIB_TRACK_LENGTH + 2];
	size_t track_len, badgcr;
	/*size_t skewbytes=0; */
	int index=0, track, added_sync=0, addsyncloops;
	FILE * fpout;
	BYTE buffer[NIB_TRACK_LENGTH];
	size_t raw_track_size[4] = { 6250, 6666, 7142, 7692 };
	/*char errorstring[0x1000]; */

#if 0
	printf("%s", "Writing G64 file...\n");
#else
	log_cb(RETRO_LOG_INFO, "->G64: %s\n", filename);
#endif

	fpout = fopen(filename, "wb");
	if (fpout == NULL)
	{
		printf("Cannot open G64 image %s.\n", filename);
		return 0;
	}

	/* determine max track size (VICE still can't handle) */
#if 0
	for (index= 0; index < MAX_HALFTRACKS_1541; index += track_inc)
	{
		if(track_length[index+2] > G64_TRACK_MAXLEN)
			G64_TRACK_MAXLEN = track_length[index+2];
	}
#endif
	if (verbose) printf("G64 Track Length = %d\n", G64_TRACK_MAXLEN);

	/* Create G64 header */
	strcpy((char *) header, "GCR-1541");
	header[8] = 0;	/* G64 version */
	header[9] = MAX_HALFTRACKS_1541; /* Number of Halftracks  (VICE <2.2 can't handle non-84 track images) */
	/*header[9] = (unsigned char)end_track; */
	header[10] = (BYTE) (G64_TRACK_MAXLEN % 256);	/* Size of each stored track */
	header[11] = (BYTE) (G64_TRACK_MAXLEN / 256);

	if (fwrite(header, sizeof(header), 1, fpout) != 1)
	{
		printf("%s", "Cannot write G64 header.\n");
		return 0;
	}

	/* Create track and speed tables */
	for (track = 0; track < MAX_HALFTRACKS_1541; track +=track_inc)
	{
		/* calculate track positions and speed zone data */
		if((!old_g64)&&(!track_length[track+2])) continue;

		gcr_track_p[track] = 0xc + (MAX_TRACKS_1541 * 16) + (index++ * (G64_TRACK_MAXLEN + 2));
		gcr_speed_p[track] = track_density[track+2]&3;
	}

	/* write headers */
	if (write_dword(fpout, gcr_track_p, sizeof(gcr_track_p)) < 0)
	{
		printf("%s", "Cannot write track header.\n");
		return 0;
	}

	if (write_dword(fpout, gcr_speed_p, sizeof(gcr_speed_p)) < 0)
	{
		printf("%s", "Cannot write speed header.\n");
		return 0;
	}

	/* shuffle raw GCR between formats */
	for (track = 2; track <= MAX_HALFTRACKS_1541+1; track +=track_inc)
	{
		log_output[0] = '\0';

		fillbyte = track_buffer[(track * NIB_TRACK_LENGTH) + track_length[track] - 1];
		memset(buffer, fillbyte, sizeof(buffer));

		track_len = track_length[track];
		if(track_len>G64_TRACK_MAXLEN) track_len=G64_TRACK_MAXLEN;

		if((!old_g64)&&(!track_len)) continue;

		memcpy(buffer, track_buffer + (track * NIB_TRACK_LENGTH), track_len);

		/* user display */
		if (verbose)
		{
			snprintf(log_output + strlen(log_output), sizeof(log_output), "%4.1f: (", (float)track/2);
			snprintf(log_output + strlen(log_output), sizeof(log_output), "%d", track_density[track]&3);
			if ((track_density[track]&3) != speed_map[track/2]) strcat(log_output, "!");
			snprintf(log_output + strlen(log_output), sizeof(log_output), ":%d) ", track_length[track]);
			if (track_density[track] & BM_NO_SYNC) strcat(log_output, "NOSYNC ");
			if (track_density[track] & BM_FF_TRACK) strcat(log_output, "KILLER ");
		}

		/* process/compress GCR data */
		if(increase_sync)
		{
			for(addsyncloops=0;addsyncloops<increase_sync;addsyncloops++)
			{
				added_sync = lengthen_sync(buffer, track_len, G64_TRACK_MAXLEN);
				track_len += added_sync;
				if (verbose) snprintf(log_output + strlen(log_output), sizeof(log_output), "[+sync:%d] ", added_sync);
			}
		}

		badgcr = check_bad_gcr(buffer, track_len);
		if (verbose>1) snprintf(log_output + strlen(log_output), sizeof(log_output), "(weak:%d) ", badgcr);

		if(rpm_real)
		{
			/*capacity[speed_map[track/2]] = raw_track_size[speed_map[track/2]]; */
			switch (track_density[track])
			{
				case 0:
					capacity[speed_map[track/2]] = (size_t)(DENSITY0/rpm_real);
					break;
				case 1:
					capacity[speed_map[track/2]] = (size_t)(DENSITY1/rpm_real);
					break;
				case 2:
					capacity[speed_map[track/2]] = (size_t)(DENSITY2/rpm_real);
					break;
				case 3:
					capacity[speed_map[track/2]] = (size_t)(DENSITY3/rpm_real);
					break;
			}

			if(capacity[speed_map[track/2]] > G64_TRACK_MAXLEN)
				capacity[speed_map[track/2]] = G64_TRACK_MAXLEN;

			if(track_len > capacity[speed_map[track/2]])
				track_len = compress_halftrack(track, buffer, track_density[track], track_len);
			if (verbose) snprintf(log_output + strlen(log_output), sizeof(log_output), "(%d) ", track_len);
		}
		else
		{
			capacity[speed_map[track/2]] = G64_TRACK_MAXLEN;
			track_len = compress_halftrack(track, buffer, track_density[track], track_len);
		}
		if (verbose>1) snprintf(log_output + strlen(log_output), sizeof(log_output), "(fill:$%.2x)", fillbyte);

		gcr_track[0] = (BYTE) (track_len % 256);
		gcr_track[1] = (BYTE) (track_len / 256);

		/* apply skew, if specified */
#if 0
		if(skew)
		{
			skewbytes = skew * (capacity[track_density[track]&3] / 200);
			if(skewbytes > track_len)
				skewbytes = skewbytes - track_len;
			snprintf(log_output + strlen(log_output), sizeof(log_output)," {skew=%d} ", skewbytes);
		}
		memcpy(gcr_track+2, buffer+skewbytes, track_len-skewbytes);
		memcpy(gcr_track+2+track_len-skewbytes, buffer, skewbytes);
#endif
		memcpy(gcr_track+2, buffer, track_len);

		if (verbose) printf("%s\n", log_output);

		if (fwrite(gcr_track, (G64_TRACK_MAXLEN + 2), 1, fpout) != 1)
		{
			printf("%s", "Cannot write G64 track data.\n");
			return 0;
		}
	}
	fclose(fpout);
	if (verbose) printf("%s", "Successfully saved G64 file\n");
	return 1;
}

size_t compress_halftrack(int halftrack, BYTE *track_buffer, BYTE density, size_t length)
{
	size_t orglen;
	BYTE gcrdata[NIB_TRACK_LENGTH];

	if (skip_compress_halftrack && skip_compress_halftrack_density == density)
		return length;

	/* copy to spare buffer */
	memcpy(gcrdata, track_buffer, NIB_TRACK_LENGTH);
	memset(track_buffer, 0, NIB_TRACK_LENGTH);

	/* process and compress track data (if needed) */
	if (length > 0)
	{
		/* If our track contains sync, we reduce to a minimum of 32 bits
		   less is too short for some loaders including CBM, but only 10 bits are technically required */
		orglen = length;
		if ( (length > (capacity[density&3])) && (!(density & BM_NO_SYNC)) &&
			(reduce_map[halftrack/2] & REDUCE_SYNC) )
		{
			/* reduce sync marks within the track */
			length = reduce_runs(gcrdata, length, capacity[density&3], reduce_sync, 0xff);
			if (verbose) snprintf(log_output + strlen(log_output), sizeof(log_output), "(sync-%d)", orglen - length);
		}

		/* reduce bad GCR runs */
		orglen = length;
		if ( (length > (capacity[density&3])) &&
			(reduce_map[halftrack/2] & REDUCE_BAD) )
		{
			length = reduce_runs(gcrdata, length, capacity[density&3], 0, 0x00);
			if (verbose) snprintf(log_output + strlen(log_output), sizeof(log_output), "(badgcr-%d)", orglen - length);
		}

		/* reduce sector gaps -  they occur at the end of every sector and vary from 4-19 bytes, typically  */
		orglen = length;
		if ( (length > (capacity[density&3])) &&
			(reduce_map[halftrack/2] & REDUCE_GAP) )
		{
			length = reduce_gaps(gcrdata, length, capacity[density & 3]);
			if (verbose) snprintf(log_output + strlen(log_output), sizeof(log_output), "(gap-%d)", orglen - length);
		}

		/* still not small enough, we have to truncate the end (reduce tail) */
		orglen = length;
		if (length > capacity[density&3])
		{
			length = capacity[density&3];
			if (verbose) snprintf(log_output + strlen(log_output), sizeof(log_output), "(trunc-%d)", orglen - length);
		}
	}

	/* if track is empty (unformatted) fill with '0' bytes to simulate */
	if ( (!length) && (density & BM_NO_SYNC))
	{
		memset(gcrdata, 0, NIB_TRACK_LENGTH);
		length = NIB_TRACK_LENGTH;
	}

	/* write processed track buffer */
	memcpy(track_buffer, gcrdata, length);
	return length;
}

int nib_convert(char *in, char *out)
{
	char inname[256], outname[256];
	int t;

	start_track = 1 * 2;
	end_track = 42 * 2;
	track_inc = 1;
	fix_gcr = 1;
	reduce_sync = 4;
	skip_halftracks = 0;
	align = ALIGN_NONE;
	force_align = ALIGN_NONE;
	gap_match_length = 7;
	cap_min_ignore = 0;
	verbose = 0;
	rpm_real = 295;

	/* default is to reduce sync */
	memset(reduce_map, REDUCE_SYNC, MAX_TRACKS_1541+1);
	/*memset(track_length, 0, MAX_TRACKS_1541+1);*/
	for(t=0; t<MAX_TRACKS_1541+1; t++)
		track_length[t] = NIB_TRACK_LENGTH; /* I do not recall why this was done, but left at MAX */

	/* clear heap buffers */
	memset(compressed_buffer, 0x00, sizeof(compressed_buffer));
	memset(file_buffer, 0x00, sizeof(file_buffer));
	memset(track_buffer, 0x00, sizeof(track_buffer));

	snprintf(inname, sizeof(inname), "%s", local_to_utf8_string_alloc(in));
	snprintf(outname, sizeof(outname), "%s", local_to_utf8_string_alloc(out));

	/* convert */
	if (compare_extension((unsigned char *)inname, (unsigned char *)"NIB"))
	{
		if(!(file_buffer_size = load_file(inname, file_buffer))) return 0;
		if(!(read_nib(file_buffer, file_buffer_size, track_buffer, track_density, track_length))) return 0;
		if( (compare_extension((unsigned char *)outname, (unsigned char *)"G64")) || (compare_extension((unsigned char *)outname, (unsigned char *)"D64")) )
			align_tracks(track_buffer, track_density, track_length, track_alignment);
		search_fat_tracks(track_buffer, track_density, track_length);
	}
	else if (compare_extension((unsigned char *)inname, (unsigned char *)"NBZ"))
	{
		if(!(file_buffer_size = load_file(inname, compressed_buffer))) return 0;
		if(!(file_buffer_size = LZ_Uncompress(compressed_buffer, file_buffer, file_buffer_size))) return 0;
		if(!(read_nib(file_buffer, file_buffer_size, track_buffer, track_density, track_length))) return 0;
		if( (compare_extension((unsigned char *)outname, (unsigned char *)"G64")) || (compare_extension((unsigned char *)outname, (unsigned char *)"D64")) )
			align_tracks(track_buffer, track_density, track_length, track_alignment);
		search_fat_tracks(track_buffer, track_density, track_length);
	}

	if (compare_extension((unsigned char *)outname, (unsigned char *)"G64"))
	{
		if(skip_halftracks) track_inc = 2;
		if(!(write_g64(outname, track_buffer, track_density, track_length))) return 0;
	}

	return 1;
}

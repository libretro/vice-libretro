#ifndef SNAPSHOT_STREAM_H
#define SNAPSHOT_STREAM_H

#include "types.h"

#define SNAPSHOT_MACHINE_NAME_LEN       16
#define SNAPSHOT_MODULE_NAME_LEN        16

#define SNAPSHOT_NO_ERROR                         0
#define SNAPSHOT_WRITE_EOF_ERROR                  1
#define SNAPSHOT_WRITE_BYTE_ARRAY_ERROR           2
#define SNAPSHOT_READ_EOF_ERROR                   3
#define SNAPSHOT_READ_BYTE_ARRAY_ERROR            4
#define SNAPSHOT_ILLEGAL_STRING_LENGTH_ERROR      5
#define SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR         6
#define SNAPSHOT_ILLEGAL_OFFSET_ERROR             7
#define SNAPSHOT_FIRST_MODULE_NOT_FOUND_ERROR     8
#define SNAPSHOT_MODULE_HEADER_READ_ERROR         9
#define SNAPSHOT_MODULE_NOT_FOUND_ERROR          10
#define SNAPSHOT_MODULE_CLOSE_ERROR              11
#define SNAPSHOT_MODULE_SKIP_ERROR               12
#define SNAPSHOT_CANNOT_CREATE_SNAPSHOT_ERROR    13
#define SNAPSHOT_CANNOT_WRITE_MAGIC_STRING_ERROR 14
#define SNAPSHOT_CANNOT_WRITE_VERSION_ERROR      15
#define SNAPSHOT_CANNOT_WRITE_MACHINE_NAME_ERROR 16
#define SNAPSHOT_CANNOT_OPEN_FOR_READ_ERROR      17
#define SNAPSHOT_MAGIC_STRING_MISMATCH_ERROR     18
#define SNAPSHOT_CANNOT_READ_VERSION_ERROR       19
#define SNAPSHOT_CANNOT_READ_MACHINE_NAME_ERROR  20
#define SNAPSHOT_MACHINE_MISMATCH_ERROR          21
#define SNAPSHOT_READ_CLOSE_EOF_ERROR            22
#define SNAPSHOT_WRITE_CLOSE_EOF_ERROR           23
#define SNAPSHOT_MODULE_HIGHER_VERSION           24
#define SNAPSHOT_MODULE_INCOMPATIBLE             25
#define SNAPSHOT_CANNOT_WRITE_SNAPSHOT           26
#define SNAPSHOT_CANNOT_READ_SNAPSHOT            27
#define SNAPSHOT_MODULE_NOT_IMPLEMENTED          28
#define SNAPSHOT_ATA_IMAGE_FILENAME_MISMATCH     29
#define SNAPSHOT_VICII_MODEL_MISMATCH            30

#ifndef VICE_RASTER_SNAPSHOT_H
typedef struct snapshot_module_s snapshot_module_t;
#endif
typedef struct snapshot_s snapshot_t;

void snapshot_display_error(void);

int snapshot_module_write_byte(snapshot_module_t *m, uint8_t data);
int snapshot_module_write_word(snapshot_module_t *m, uint16_t data);
int snapshot_module_write_dword(snapshot_module_t *m, uint32_t data);
int snapshot_module_write_qword(snapshot_module_t *m, uint64_t data);
int snapshot_module_write_double(snapshot_module_t *m, double db);
int snapshot_module_write_padded_string(snapshot_module_t *m, const char *s, uint8_t pad_char, int len);
int snapshot_module_write_byte_array(snapshot_module_t *m, const uint8_t *data, unsigned int num);
int snapshot_module_write_word_array(snapshot_module_t *m, const uint16_t *data, unsigned int num);
int snapshot_module_write_dword_array(snapshot_module_t *m, const uint32_t *data, unsigned int num);
int snapshot_module_write_string(snapshot_module_t *m, const char *s);

int snapshot_module_read_byte(snapshot_module_t *m, uint8_t *b_return);
int snapshot_module_read_word(snapshot_module_t *m, uint16_t *w_return);
int snapshot_module_read_dword(snapshot_module_t *m, uint32_t *dw_return);
int snapshot_module_read_qword(snapshot_module_t *m, uint64_t *qw_return);
int snapshot_module_read_double(snapshot_module_t *m, double *db_return);
int snapshot_module_read_byte_array(snapshot_module_t *m, uint8_t *b_return, unsigned int num);
int snapshot_module_read_word_array(snapshot_module_t *m, uint16_t *w_return, unsigned int num);
int snapshot_module_read_dword_array(snapshot_module_t *m, uint32_t *dw_return, unsigned int num);
int snapshot_module_read_string(snapshot_module_t *m, char **s);
int snapshot_module_read_byte_into_int(snapshot_module_t *m, int *value_return);
int snapshot_module_read_byte_into_uint(snapshot_module_t *m, unsigned int *value_return);
int snapshot_module_read_word_into_int(snapshot_module_t *m, int *value_return);
int snapshot_module_read_word_into_uint(snapshot_module_t *m, unsigned int *value_return);
int snapshot_module_read_dword_into_ulong(snapshot_module_t *m, unsigned long *value_return);
int snapshot_module_read_dword_into_int(snapshot_module_t *m, int *value_return);
int snapshot_module_read_dword_into_uint(snapshot_module_t *m, unsigned int *value_return);
int snapshot_module_read_qword_into_int64(snapshot_module_t *m, int64_t *value_return);

#define SMW_B        snapshot_module_write_byte
#define SMW_W        snapshot_module_write_word
#define SMW_DW       snapshot_module_write_dword
#define SMW_QW       snapshot_module_write_qword
#define SMW_DB       snapshot_module_write_double
#define SMW_PSTR     snapshot_module_write_padded_string
#define SMW_BA       snapshot_module_write_byte_array
#define SMW_WA       snapshot_module_write_word_array
#define SMW_DWA      snapshot_module_write_dword_array
#define SMW_STR      snapshot_module_write_string
#define SMW_CLOCK    snapshot_module_write_qword
#define SMR_B        snapshot_module_read_byte
#define SMR_W        snapshot_module_read_word
#define SMR_DW       snapshot_module_read_dword
#define SMR_QW       snapshot_module_read_qword
#define SMR_DB       snapshot_module_read_double
#define SMR_BA       snapshot_module_read_byte_array
#define SMR_WA       snapshot_module_read_word_array
#define SMR_DWA      snapshot_module_read_dword_array
#define SMR_STR      snapshot_module_read_string
#define SMR_B_INT    snapshot_module_read_byte_into_int
#define SMR_B_UINT   snapshot_module_read_byte_into_uint
#define SMR_W_INT    snapshot_module_read_word_into_int
#define SMR_W_UINT   snapshot_module_read_word_into_uint
#define SMR_DW_UL    snapshot_module_read_dword_into_ulong
#define SMR_DW_INT   snapshot_module_read_dword_into_int
#define SMR_DW_UINT  snapshot_module_read_dword_into_uint
#define SMR_CLOCK    snapshot_module_read_qword

snapshot_module_t *snapshot_module_create(snapshot_t *s, const char *name, uint8_t major_version, uint8_t minor_version);
snapshot_module_t *snapshot_module_open(snapshot_t *s, const char *name, uint8_t *major_version_return, uint8_t *minor_version_return);
int snapshot_module_close(snapshot_module_t *m);

snapshot_t *snapshot_create(const char *filename, uint8_t major_version, uint8_t minor_version, const char *snapshot_machine_name);
snapshot_t *snapshot_open(const char *filename, uint8_t *major_version_return, uint8_t *minor_version_return, const char *snapshot_machine_name);
int snapshot_close(snapshot_t *s);

void snapshot_set_error(int error);
int snapshot_get_error(void);

int snapshot_version_is_equal(uint8_t major_version, uint8_t minor_version, uint8_t major_version_required, uint8_t minor_version_required);
int snapshot_version_is_bigger(uint8_t major_version, uint8_t minor_version, uint8_t major_version_required, uint8_t minor_version_required);
int snapshot_version_is_smaller(uint8_t major_version, uint8_t minor_version, uint8_t major_version_required, uint8_t minor_version_required);

/* Stream */

struct snapshot_stream_s;
typedef struct snapshot_stream_s snapshot_stream_t;

int machine_write_snapshot_to_stream(struct snapshot_stream_s *stream, int save_roms, int save_disks, int event_mode);
int machine_read_snapshot_from_stream(struct snapshot_stream_s *stream, int event_mode);
int snapshot_free(snapshot_t *s);

snapshot_t *snapshot_create_from_stream(snapshot_stream_t *f, uint8_t major_version, uint8_t minor_version, const char *snapshot_machine_name);
snapshot_t *snapshot_open_from_stream(snapshot_stream_t *f, uint8_t *major_version_return, uint8_t *minor_version_return, const char *snapshot_machine_name);

snapshot_stream_t* snapshot_file_read_fopen(const char* pathname);
snapshot_stream_t* snapshot_file_write_fopen(const char* pathname);

snapshot_stream_t* snapshot_memory_read_fopen(const void* buffer, size_t buffer_size);
snapshot_stream_t* snapshot_memory_write_fopen(void* buffer, size_t buffer_size);

size_t snapshot_read(snapshot_stream_t* f, void* ptr, size_t size);
size_t snapshot_write(snapshot_stream_t* f, const void* ptr, size_t size);
int snapshot_fseek(snapshot_stream_t *f, long offset, int whence);
long snapshot_ftell(snapshot_stream_t *f);

int snapshot_fclose(snapshot_stream_t *f);
int snapshot_fclose_erase(snapshot_stream_t *f);

#endif

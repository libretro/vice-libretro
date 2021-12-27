#ifndef _SYSCONFIG_H
#define _SYSCONFIG_H

#ifndef __cplusplus

#ifdef USE_LIBRETRO_VFS
#include <streams/file_stream_transforms.h>
#define HAVE_FSEEKO
#undef fputs
#define fputs(string, stream) rfprintf(stream, "%s", string)
#undef clearerr
#define clearerr(stream)
#undef setbuf
#define setbuf(stream, buffer)
#undef stderr
#define stderr 0
#undef stdout
#define stdout 0
#undef rewind
#define rewind filestream_rewind
#undef fseek
#define fseek rfseek
#undef ftell
#define ftell rftell
#undef fseeko
#define fseeko rfseek
#undef ftello
#define ftello rftell
#endif /* USE_LIBRETRO_VFS */

#endif /* __cplusplus */

#endif /* _SYSCONFIG_H */

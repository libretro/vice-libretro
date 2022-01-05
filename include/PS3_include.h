#include <sys/sys_time.h>
#include <sys/timer.h>
#include <sys/types.h>
#include <sys/time.h>
#include <cmath>

#ifndef PS3_HEADER
#define PS3_HEADER

#define F_OK	0
#define W_OK	0
#define R_OK	0
#define X_OK	0

#ifndef S_IREAD
#define S_IREAD S_IRUSR
#endif
#ifndef S_IWRITE
#define S_IWRITE S_IWUSR
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_IFBLK
#define S_IFBLK		(-1)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m)	(((m)&S_IFBLK)==S_IFBLK)
#endif

#ifndef S_IFCHR
#define S_IFCHR		(-1)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m)	(((m)&S_IFCHR)==S_IFCHR)
#endif

#define chdir(...) 0
#define isatty(...) 0
#define currentpath "/dev_hdd0/game/RETROARCH/USRDIR"
#define tmppath "/dev_hdd0/game/RETROARCH/USRDIR/cores/savefiles"
#define getwd(buffer) (strcpy(buffer, currentpath)) ? (buffer) : (NULL)
#define tmpnam(...) (tmppath)
#define getenv(...) (NULL)
#ifdef __cplusplus
#define exp(xval) ::std::exp(xval)
#define fabs(xval) ::std::fabs(xval)
#define sqrt(xval) ::std::sqrt(xval)
#undef log10
#define log10(xval) ::std::log10(xval)
#undef log
#define log(xval) ::std::log(xval)
#define ceil(xval) ::std::ceil(xval)
#undef sin
#define sin(xval) ::std::sin(xval)
#define pow(xval, yval) ::std::pow(xval, yval)
#endif
int access(const char *fpath, int mode);

#endif

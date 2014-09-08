#ifndef COMPAT_H__
#define COMPAT_H__

#include <string.h>

#ifndef HAVE_STRLCPY
size_t strlcpy(char* dst, const char* src, size_t size);
#endif

#ifndef HAVE_STRLCAT
size_t strlcat(char* dst, const char* src, size_t size);
#endif

#endif // COMPAT_H__

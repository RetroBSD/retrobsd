/*
 * ISO C Standard:  7.15  Variable arguments  <stdarg.h>
 */
#ifndef _STDARG_H
#define _STDARG_H

/* Define __gnuc_va_list.  */


#ifdef __GNUC__
#   ifndef __GNUC_VA_LIST
#       define __GNUC_VA_LIST
        typedef __builtin_va_list __gnuc_va_list;
#   endif
#   define va_start(ap, last)	__builtin_va_start((ap), last)
#endif
#ifdef __PCC__
#   define va_start(ap, last)   __builtin_stdarg_start((ap), last)
#endif
#define va_arg(ap, type)        __builtin_va_arg((ap), type)
#define va_end(ap)              __builtin_va_end((ap))
#define va_copy(dest, src)      __builtin_va_copy((dest), (src))


#ifndef _VA_LIST
#define _VA_LIST
#ifdef __GNUC__
    typedef __builtin_va_list va_list;
#endif
#ifdef __SMALLER_C__
    typedef int *va_list;
#endif
#endif

#endif /* not _STDARG_H */

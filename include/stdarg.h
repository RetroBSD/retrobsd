/*
 * ISO C Standard:  7.15  Variable arguments  <stdarg.h>
 */
#ifndef _STDARG_H
#define _STDARG_H

/*
 * Define va_start, va_arg, va_end, va_copy.
 */
#if defined(__clang__)          /* Clang/LLVM */
# define va_start(ap, last)     __builtin_va_start((ap), last)
# define va_arg(ap, type)       __builtin_va_arg((ap), type)
# define va_end(ap)             __builtin_va_end((ap))
# define va_copy(dest, src)     __builtin_va_copy((dest), (src))
    typedef __builtin_va_list va_list;

#elif defined(__GNUC__)         /* Gnu C */
# define va_start(ap, last)     __builtin_va_start((ap), last)
# define va_arg(ap, type)       __builtin_va_arg((ap), type)
# define va_end(ap)             __builtin_va_end((ap))
# define va_copy(dest, src)     __builtin_va_copy((dest), (src))
    typedef __builtin_va_list va_list;

#elif defined(__PCC__)          /* PCC */
# define va_start(ap, last)     __builtin_stdarg_start((ap), last)
# define va_arg(ap, type)       __builtin_va_arg((ap), type)
# define va_end(ap)             __builtin_va_end((ap))
# define va_copy(dest, src)     __builtin_va_copy((dest), (src))
    typedef __builtin_va_list va_list;

#else                           /* SmallerC, LCC */
# define va_start(ap, last)     (ap = ((char*)&(last) + \
        (((sizeof(last) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))))
# define va_arg(ap, type)       ((type*)(ap += \
        sizeof(type) == sizeof(int) ? sizeof(type) : \
        (-(int)(ap) & (sizeof(type) - 1)) + sizeof(type)))[-1]
# define va_end(ap)
# define va_copy(dest, src)     (dest = (src))
    typedef char *va_list;
#endif

#endif /* not _STDARG_H */

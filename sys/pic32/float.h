#ifndef _MACHINE_FLOAT_H_
#define _MACHINE_FLOAT_H_

/* Characteristics of IEEE floating types */

#define FLT_RADIX       2
#define FLT_ROUNDS      1

/*
 * These values provide information pertaining to the float type.
 */
#define FLT_EPSILON     1.19209290E-07F /* b**(1-p) */
#define FLT_MAX         3.40282347E+38F /* (1-b**(-p))*b**emax */
#define FLT_MIN         1.17549435E-38F /* b**(emin-1) */
#define FLT_DIG         6               /* floor((p-1)*log10(b))+(b == 10) */
#define FLT_MANT_DIG    24              /* p */
#define FLT_MAX_10_EXP  38              /* floor(log10((1-b**(-p))*b**emax)) */
#define FLT_MAX_EXP     128             /* emax */
#define FLT_MIN_10_EXP  (-37)           /* ceil(log10(b**(emin-1))) */
#define FLT_MIN_EXP     (-125)          /* emin */

/*
 * These values provide information pertaining to the double type
 * The values are dependant upon the presence of the -fno-short-double
 * compiler option.
 */
#define DBL_EPSILON     FLT_EPSILON
#define DBL_MAX         FLT_MAX
#define DBL_MIN         FLT_MIN
#define DBL_DIG         FLT_DIG
#define DBL_MANT_DIG    FLT_MANT_DIG
#define DBL_MAX_10_EXP  FLT_MAX_10_EXP
#define DBL_MAX_EXP     FLT_MAX_EXP
#define DBL_MIN_10_EXP  FLT_MIN_10_EXP
#define DBL_MIN_EXP     FLT_MIN_EXP

/*
 * These values provide information pertaining to the long double type.
 */
#define LDBL_EPSILON    2.2204460492503131E-16
#define LDBL_MAX        1.7976931348623157E+308
#define LDBL_MIN        2.2250738585072014E-308
#define LDBL_DIG        15
#define LDBL_MANT_DIG   53
#define LDBL_MAX_10_EXP 308
#define LDBL_MAX_EXP    1024
#define LDBL_MIN_10_EXP (-307)
#define LDBL_MIN_EXP    (-1021)

#endif /* _MACHINE_FLOAT_H_ */

typedef union        // LITTLE ENDIAN
{
  double value;
  struct
  {
    unsigned long lsw;
    unsigned long msw;
  } parts;
} ieee_double_shape_type;

/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS(ix0,ix1,d)            \
do {                        \
  ieee_double_shape_type ew_u;               \
  ew_u.value = (d);                  \
  (ix0) = ew_u.parts.msw;               \
  (ix1) = ew_u.parts.lsw;               \
} while (0)

/* Get the more significant 32 bit int from a double.  */

#define GET_HIGH_WORD(i,d)               \
do {                        \
  ieee_double_shape_type gh_u;               \
  gh_u.value = (d);                  \
  (i) = gh_u.parts.msw;                  \
} while (0)

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS(d,ix0,ix1)               \
do {                        \
  ieee_double_shape_type iw_u;               \
  iw_u.parts.msw = (ix0);               \
  iw_u.parts.lsw = (ix1);               \
  (d) = iw_u.value;                  \
} while (0)

#define SET_HIGH_WORD(d,v)                                      \
do {                                                            \
    ieee_double_shape_type sh_u;                                  \
    sh_u.value = (d);                                             \
    sh_u.parts.msw = (v);                                         \
    (d) = sh_u.value;                                             \
} while (0)


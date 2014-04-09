/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS(high,low,d) \
        high = *(unsigned long long*) &d; \
        low  = (*(unsigned long long*) &d) >> 32


/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS(d,high,low) \
        *(unsigned long long*) &(x) = (unsigned long long) (high) << 32 | (low)


typedef union
{
  double value;
  struct
  {
    uint32_t lsw;
    uint32_t msw;
  } parts;
} ieee_double_shape_type;


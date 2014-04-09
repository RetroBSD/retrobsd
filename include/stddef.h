
/* Offset of member MEMBER in a struct of type TYPE. */

#if defined(__GNUC__) && __GNUC__ > 3
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE*)0)->MEMBER)
#endif

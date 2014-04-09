#include "c.h"

extern Interface symbolic64IR;
extern Interface symbolicIR;
extern Interface bytecodeIR;
extern Interface nullIR;

extern Interface alphaIR;
extern Interface mipsebIR;
extern Interface mipselIR;
extern Interface sparcIR;
extern Interface solarisIR;
extern Interface x86IR;
extern Interface x86linuxIR;

Binding bindings[] = {
#ifdef TARGET_ALPHA
        { "alpha-osf",            &alphaIR },
#endif
#ifdef TARGET_MIPS
        { "mips-el",              &mipselIR },
        { "mips-eb",              &mipsebIR },
#endif
#ifdef TARGET_SPARC
        { "sparc-sun",            &sparcIR },
#endif
#ifdef TARGET_SOLARIS
        { "sparc-solaris",        &solarisIR },
#endif
#ifdef TARGET_X86
        { "x86-win32",            &x86IR },
        { "x86-linux",            &x86linuxIR },
#endif
        { "symbolic64",           &symbolic64IR },
        { "symbolic",             &symbolicIR },
        { "bytecode",             &bytecodeIR },
        { "null",                 &nullIR },

	{ NULL, NULL },
};

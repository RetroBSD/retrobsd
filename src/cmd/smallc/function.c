/*
 * File function.c: 2.1 (83/03/20,16:02:04)
 */

#include <stdio.h>
#include "defs.h"
#include "data.h"

int argtop;

#define ARGOFFSET 20        /* for MIPS32 */

/**
 * begin a function
 * called from "parse", this routine tries to make a function out
 * of what follows
 * modified version.  p.l. woods
 */
newfunc() {
    char n[NAMESIZE];
    int idx, type;
    fexitlab = getlabel();

    if (!symname(n)) {
        error("illegal function or declaration");
        kill();
        return;
    }
    if (idx = findglb(n)) {
        if (symbol_table[idx].identity != FUNCTION)
            multidef(n);
        else if (symbol_table[idx].offset == FUNCTION)
            multidef(n);
        else
            symbol_table[idx].offset = FUNCTION;
    } else
        add_global(n, FUNCTION, CINT, FUNCTION, PUBLIC, 1);

    if (!match("("))
        error("missing open paren");

    output_string(n);
    output_label_terminator();
    newline();
    local_table_index = NUMBER_OF_GLOBALS; //locptr = STARTLOC;
    argstk = 0;

    // ANSI style argument declaration
    if (doAnsiArguments() == 0) {
        // K&R style argument declaration
        while (! match(")")) {
            if (symname(n)) {
                if (findloc(n))
                    multidef(n);
                else {
                    add_local(n, 0, 0, ARGOFFSET + argstk, AUTO, 1);
                    argstk = argstk + INTSIZE;
                }
            } else {
                error("illegal argument name");
                junk();
            }
            blanks();
            if (!streq(line + lptr, ")")) {
                if (!match(","))
                    error("expected comma");
            }
            if (endst())
                break;
        }
        stkp = 0;
        argtop = argstk;
        while (argstk) {
            if (type = get_type()) {
                getarg(type);
                need_semicolon();
            } else {
                error("wrong number args");
                break;
            }
        }
    }
    fentry(argtop);
    statement(YES);
    print_label(fexitlab);
    output_label_terminator();
    newline();
    gen_modify_stack(0);
    gen_ret();
    stkp = 0;
    local_table_index = NUMBER_OF_GLOBALS; //locptr = STARTLOC;
}

/**
 * declare argument types
 * called from "newfunc", this routine adds an entry in the local
 * symbol table for each named argument
 * completely rewritten version.  p.l. woods
 * @param t argument type (char, int)
 * @return
 */
getarg(int t) {
    int j, legalname, argptr;
    char n[NAMESIZE];

    for (;;) {
        if (argstk == 0)
            return;

        if (match("*"))
            j = POINTER;
        else
            j = VARIABLE;

        if (! (legalname = symname(n)))
            illname();

        if (match("[")) {
            while (inbyte() != ']')
                if (endst())
                    break;
                j = POINTER;
        }
        if (legalname) {
            if (argptr = findloc(n)) {
                symbol_table[argptr].identity = j;
                symbol_table[argptr].type = t;
            } else
                error("expecting argument name");
        }
        argstk = argstk - INTSIZE;
        if (endst())
            return;
        if (! match(","))
            error("expected comma");
    }
}

doAnsiArguments() {
    int type;
    type = get_type();
    if (type == 0) {
        return 0; // no type detected, revert back to K&R style
    }
    argstk = 0;
    for (;;)
    {
        if (type) {
            doLocalAnsiArgument(type);
        } else {
            error("wrong number args");
            break;
        }
        if (match(",")) {
            type = get_type();
            continue;
        }
        if (match(")")) {
            break;
        }
    }
    argtop = argstk;
}

doLocalAnsiArgument(int type) {
    char symbol_name[NAMESIZE];
    int identity, argptr, ptr;

    if (match("*")) {
        identity = POINTER;
    } else {
        identity = VARIABLE;
    }
    if (symname(symbol_name)) {
        if (findloc(symbol_name)) {
            multidef(symbol_name);
        } else {
            argptr = add_local (symbol_name, identity, type, ARGOFFSET + argstk, AUTO, 1);
            argstk += INTSIZE;
        }
    } else {
        error("illegal argument name");
        junk();
    }
    if (match("[")) {
        while (inbyte() != ']') {
            if (endst()) {
                break;
            }
        }
        identity = POINTER;
        symbol_table[argptr].identity = identity;
    }
}

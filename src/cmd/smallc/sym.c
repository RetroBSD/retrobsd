/*
 * File sym.c: 2.1 (83/03/20,16:02:19)
 */

#include <stdio.h>
#include "defs.h"
#include "data.h"

#define LOCALOFFSET 16      /* for MIPS32 */

/**
 * declare a static variable
 * @param type
 * @param storage
 * @return
 */
declare_global(int type, int storage) {
        int     k, j, count;
        char    sname[NAMESIZE];

        for (;;) {
                for (;;) {
                        if (endst ())
                                return;
                        k = 1;
                        if (match ("*"))
                                j = POINTER;
                        else
                                j = VARIABLE;
                        if (!symname (sname))
                                illname ();
                        if (findglb (sname))
                                multidef (sname);
                        count = 1;
                        if (match ("[")) {
                                k = needsub ();
                                count = k;
                                //if (k || storage == EXTERN)
                                        j = ARRAY;
                                //else
                                //        j = POINTER;
                        }
                        j = initials(sname, type, j, k);
                        add_global (sname, j, type, (k == 0 ? -1 : k), storage, count);
                        break;
                }
                if (!match (","))
                        return;
        }
}

/**
 * declare local variables
 * works just like "declglb", but modifies machine stack and adds
 * symbol table entry with appropriate stack offset to find it again
 * @param typ
 * @param stclass
 */
declare_local (typ, stclass)
int     typ, stclass;
{
        int     k, j, count;
        char    sname[NAMESIZE];

        for (;;) {
                for (;;) {
                        if (endst ())
                                return;
                        if (match ("*"))
                                j = POINTER;
                        else
                                j = VARIABLE;
                        if (!symname (sname))
                                illname ();
                        if (findloc (sname))
                                multidef (sname);
                        count = 1;
                        if (match ("[")) {
                                k = needsub ();
                                count = k;
                                if (k) {
                                        j = ARRAY;
                                        if (typ & CINT)
                                                k = k * INTSIZE;
                                } else {
                                        j = POINTER;
                                        k = INTSIZE;
                                }
                        } else
                                if ((typ & CCHAR) && (j != POINTER))
                                        k = 1;
                                else
                                        k = INTSIZE;
                        if (stclass != LSTATIC) {
                                k = galign(k);
                                stkp = gen_modify_stack (stkp - k);
                                add_local (sname, j, typ, stkp + LOCALOFFSET, AUTO, count);
                        } else
                                add_local( sname, j, typ, k, LSTATIC, count);
                        break;
                }
                if (!match (","))
                        return;
        }
}

/**
 * initialize global objects
 * @param symbol_name
 * @param size char 1 or integer 2
 * @param identity
 * @param dim
 * @return 1 if variable is initialized
 */
int initials(char *symbol_name, int size, int identity, int dim) {
    int dim_unknown = 0;
    litptr = 0;
    if(dim == 0) { // allow for xx[] = {..}; declaration
        dim_unknown = 1;
    }
    if (!(size & CCHAR) && !(size & CINT)) {
        error("unsupported storage size");
    }
    if(match("=")) {
        // an array
        if(match("{")) {
            while((dim > 0) || (dim_unknown)) {
                if (init(symbol_name, size, identity, &dim) && dim_unknown) {
                    dim_unknown++;
                }
                if(match(",") == 0) {
                    break;
                }
            }
            needbrack("}");
            if(--dim_unknown == 0)
                identity = POINTER;
        // single constant
        } else {
            init(symbol_name, size, identity, &dim);
        }
    }
    return identity;
}

/**
 * evaluate one initializer, add data to table
 * @param size
 * @param ident
 * @param dim
 * @return
 */
init(char *symbol_name, int size, int ident, int *dim) {
    int value, number_of_chars;
    if(ident == POINTER) {
        error("cannot assign to pointer");
    }
    if(quoted_string(&value)) {
        if((ident == VARIABLE) || (size != 1))
            error("found string: must assign to char pointer or array");
        number_of_chars = litptr - value;
        *dim = *dim - number_of_chars;
        while (number_of_chars > 0) {
            add_data(symbol_name, CCHAR, litq[value++]);
            number_of_chars = number_of_chars - 1;
        }
    } else if (number(&value)) {
        add_data(symbol_name, CINT, value);
        *dim = *dim - 1;
    } else if(quoted_char(&value)) {
        add_data(symbol_name, CCHAR, value);
        *dim = *dim - 1;
    } else {
        return 0;
    }
    return 1;
}

/**
 * get required array size. [xx]
 * @return array size
 */
needsub () {
        int     num[1];

        if (match ("]"))
                return (0);
        if (!number (num)) {
                error ("must be constant");
                num[0] = 1;
        }
        if (num[0] < 0) {
                error ("negative size illegal");
                num[0] = (-num[0]);
        }
        needbrack ("]");
        return (num[0]);
}

/**
 * search global table for given symbol name
 * @param sname
 * @return table index
 */
int findglb (char *sname) {
        int idx;

        idx = 1;
        while (idx < global_table_index) {
                if (astreq (sname, symbol_table[idx].name, NAMEMAX))
                        return (idx);
                idx++;
        }
        return (0);
}

/**
 * search local table for given symbol name
 * @param sname
 * @return table index
 */
int findloc (char *sname) {
        int idx;

        idx = local_table_index;
        while (idx >= NUMBER_OF_GLOBALS) {
                idx--;
                if (astreq (sname, symbol_table[idx].name, NAMEMAX))
                        return (idx);
        }
        return (0);
}

/**
 * add new symbol to global table
 * @param sname
 * @param identity
 * @param type
 * @param value
 * @param storage
 * @return new index
 */
int add_global (char *sname, int identity, int type, int offset, int storage, int count)
{
        symbol_t *symbol;
        char *buffer_ptr;

        current_symbol_table_idx = findglb (sname);
        if (current_symbol_table_idx != 0) {
                return (current_symbol_table_idx);
        }
        if (global_table_index >= NUMBER_OF_GLOBALS) {
                error ("global symbol table overflow");
                return (0);
        }
        current_symbol_table_idx = global_table_index;
        symbol = &symbol_table[current_symbol_table_idx];
        buffer_ptr = symbol->name;
        while (alphanumeric(*buffer_ptr++ = *sname++));
        symbol->identity = identity;
        symbol->type = type;
        symbol->storage = storage;
        symbol->count = count;
        symbol->offset = offset;
        global_table_index++;
        return (current_symbol_table_idx);
}

/**
 * add new symbol to local table
 * @param sname
 * @param identity
 * @param type
 * @param value
 * @param storage_class
 * @return
 */
int add_local (char *sname, int identity, int type, int offset, int storage_class, int count) {
        int k;
        symbol_t *symbol;
        char *buffer_ptr;
//printf("local - symbol: %s offset: %d count: %d\n", sname, offset, count);

        if (current_symbol_table_idx = findloc (sname)) {
                return (current_symbol_table_idx);
        }
        if (local_table_index >= NUMBER_OF_GLOBALS + NUMBER_OF_LOCALS) {
                error ("local symbol table overflow");
                return (0);
        }
        current_symbol_table_idx = local_table_index;
        symbol = &symbol_table[current_symbol_table_idx];
        buffer_ptr = symbol->name;
        while (alphanumeric(*buffer_ptr++ = *sname++));
        symbol->identity = identity;
        symbol->type = type;
        symbol->storage = storage_class;
        symbol->count = count;
        if (storage_class == LSTATIC) {
                data_segment_gdata();
                print_label(k = getlabel());
                output_label_terminator();
                gen_def_storage();
                output_number(offset);
                newline();
                code_segment_gtext();
                offset = k;
        }
        symbol->offset = offset;
        local_table_index++;
        return (current_symbol_table_idx);
}

/*
 *      test if next input string is legal symbol name
 *
 */
symname(char *sname) {
    int k;

    blanks();
    if (!alpha (ch ()))
        return (0);
    k = 0;
    while (alphanumeric(ch ()))
        sname[k++] = gch ();
    sname[k] = 0;
    return (1);
}

illname() {
    error ("illegal symbol name");
}

/**
 * print error message
 * @param symbol_name
 * @return
 */
multidef (char *symbol_name) {
    error ("already defined");
    gen_comment ();
    output_string (symbol_name);
    newline ();
}

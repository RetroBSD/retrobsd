/*
 * File main.c: 2.7 (84/11/28,10:14:56)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "defs.h"
#include "data.h"

main(int argc, char *argv[])
{
    char *param = NULL;
    char *infile = NULL;
    char *outfile = NULL;
    int i;
    ctext = 0;
    errs = 0;
    for (i=1; i<argc; i++) {
        param = argv[i];
        if (*param != '-')
            break;
        while (*++param) {
            switch (*param) {
                case 't':       // output c source as asm comments
                    ctext = 1;
                    break;
                case 'v':       // verbose mode
                    verbose = 1;
                    break;
                default:
                    usage();
            }
        }
    }

    if (i == argc && isatty(0))
        usage();

    if (i < argc)
        infile = argv[i];

    if (i+1 < argc)
        outfile = argv[i+1];

    compile(infile, outfile);
    exit(errs != 0);
}

/**
 * compile one file if filename is NULL redirect do to stdin/stdout
 * @param file filename
 * @return
 */
compile(char *infile, char *outfile)
{
    global_table_index = 1;
    local_table_index = NUMBER_OF_GLOBALS;
    loop_table_index = 0;
    inclsp = 0;
    swstp = 0;
    litptr = 0;
    stkp = 0;
    errcnt = 0;
    ncmp = 0;
    lastst = 0;
    cmode = 1;
    glbflag = 1;
    nxtlab = 0;
    litlab = getlabel();
    //add_global("memory", ARRAY, CCHAR, 0, EXTERN);
    //add_global("stack", ARRAY, CCHAR, 0, EXTERN);
    rglobal_table_index = global_table_index;
    //add_global("etext", ARRAY, CCHAR, 0, EXTERN);
    //add_global("edata", ARRAY, CCHAR, 0, EXTERN);
    //initmac();
    create_initials();

    if (infile == NULL) {
        input = stdin;
    } else {
        /* open input file */
        input = fopen (infile, "r");
        if (! input) {
                printf ("%s: Cannot read\n", infile);
                return;
        }
    }

    if (outfile == NULL) {
        output = stdout;
    } else {
        /* open output file */
        output = fopen (outfile, "w");
        if (! output) {
                printf ("%s: Cannot write\n", outfile);
                return;
        }
    }

    // compiler body
    header();
    code_segment_gtext();
    parse();
    fclose(input);
    data_segment_gdata();
    dumplits();
    dumpglbs();
    errorsummary();
    trailer();
    fclose(output);
    errs = errs || errfile;
}

frontend_version()
{
    output_string("\tFront End (2.7,84/11/28)");
}

/**
 * prints usage
 * @return exits the execution
 */
usage()
{
    fputs("Usage:\n", stderr);
    fputs("  smallc [-t] [infile [outfile]]\n", stderr);
    fputs("Options:\n", stderr);
    fputs("  -t      Output C source as asm comments\n", stderr);
    fputs("  -v      Verbose messages\n", stderr);
    exit(1);
}

/**
 * "asm" pseudo-statement
 * enters mode where assembly language statements are passed
 * intact through parser
 */
doasm ()
{
        cmode = 0;
        for (;;) {
                readline ();
                if (match ("__endasm__"))
                        break;
                if (feof (input))
                        break;
                output_string (line);
                newline ();
        }
        kill ();
        cmode = 1;
}

/**
 * process all input text
 * at this level, only static declarations, defines, includes,
 * and function definitions are legal.
 */
parse()
{
    while (!feof(input)) {
        if (amatch("extern", 6)) {
            dodcls(EXTERN);
        } else if (amatch("static", 6)) {
            dodcls(STATIC);
        } else if (dodcls(PUBLIC)) {
            ;
        } else if (match("__asm__")) {
            doasm();
        } else if (match("#")) {
            kill();
        } else {
            newfunc();
        }
        blanks();
    }
}

/**
 * parse top level declarations
 * @param stclass
 * @return
 */
dodcls(int stclass)
{
    int type;
    blanks();
    if (type = get_type()) {
        declare_global(type, stclass);
    } else if (stclass == PUBLIC) {
        return (0);
    } else {
        declare_global(CINT, stclass);
    }
    need_semicolon();
    return (1);
}

/**
 * dump the literal pool
 */
dumplits()
{
    int j, k;

    if (litptr == 0)
        return;
    print_label(litlab);
    output_label_terminator();
    k = 0;
    while (k < litptr) {
        gen_def_byte();
        j = 8;
        while (j--) {
            output_number(litq[k++]);
            if ((j == 0) | (k >= litptr)) {
                newline();
                break;
            }
            output_byte(',');
        }
    }
}

/**
 * dump all static variables
 */
dumpglbs()
{
    int dim, i, list_size, line_count, value;

    if (!glbflag)
        return;
    current_symbol_table_idx = rglobal_table_index;
    while (current_symbol_table_idx < global_table_index) {
        symbol_t *symbol = &symbol_table[current_symbol_table_idx];
        if (symbol->identity != FUNCTION) {
            ppubext(symbol);
            if (symbol->storage != EXTERN) {
                if ((symbol->type & CINT) || (symbol->identity == POINTER))
                    gen_align_word();
                output_string(symbol->name);
                output_label_terminator();
                dim = symbol->offset;
                list_size = 0;
                line_count = 0;
                if (find_symbol(symbol->name)) { // has initials
                    list_size = get_size(symbol->name);
                    if (dim == -1) {
                        dim = list_size;
                    }
                }
                for (i=0; i<dim; i++) {
                    if (line_count % 10 == 0) {
                        newline();
                        if ((symbol->type & CINT) || (symbol->identity == POINTER)) {
                            gen_def_word();
                        } else {
                            gen_def_byte();
                        }
                    }
                    if (i < list_size) {
                        // dump data
                        value = get_item_at(symbol->name, i);
                        output_number(value);
                    } else {
                        // dump zero, no more data available
                        output_number(0);
                    }
                    line_count++;
                    if (line_count % 10 == 0) {
                        line_count = 0;
                    } else {
                        if (i < dim-1) {
                            output_byte( ',' );
                        }
                    }
                }
                newline();
            }
        } else {
            fpubext(symbol);
        }
        current_symbol_table_idx++;
    }
}

/*
 * report errors
 */
errorsummary()
{
    if (ncmp)
        error("missing closing bracket");
    gen_comment();
    newline();
    gen_comment();
    output_with_tab("");
    output_decimal(errcnt);
    if (errcnt) errfile = YES;
    output_string(" error(s) in compilation");
    newline();
    gen_comment();
    output_with_tab("literal pool: ");
    output_decimal(litptr);
    newline();
    gen_comment();
    output_with_tab("global pool: ");
    output_decimal(global_table_index - rglobal_table_index);
    newline();
    if (errcnt > 0)
        printf("Compilation failed: %d error(s)\n", errcnt);
}

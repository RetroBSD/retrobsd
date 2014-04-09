/*
 * File stmt.c: 2.1 (83/03/20,16:02:17)
 */

#include <stdio.h>
#include "defs.h"
#include "data.h"

/**
 * statement parser
 * called whenever syntax requires a statement.  this routine
 * performs that statement and returns a number telling which one
 * @param func func is true if we require a "function_statement", which
 * must be compound, and must contain "statement_list" (even if
 * "declaration_list" is omitted)
 * @return statement type
 */
statement (int func) {
        if ((ch () == 0) & feof (input))
                return (0);
        lastst = 0;
        if (func)
                if (match ("{")) {
                        do_compound (YES);
                        return (lastst);
                } else
                        error ("function requires compound statement");
        if (match ("{"))
                do_compound (NO);
        else
                do_statement ();
        return (lastst);
}

/**
 * declaration
 */
statement_declare() {
        if (amatch("register", 8))
                do_local_declares(DEFAUTO);
        else if (amatch("auto", 4))
                do_local_declares(DEFAUTO);
        else if (amatch("static", 6))
                do_local_declares(LSTATIC);
        else if (do_local_declares(AUTO)) ;
        else
                return (NO);
        return (YES);
}

/**
 * local declarations
 * @param stclass
 * @return
 */
do_local_declares(int stclass) {
        int type = 0;
        blanks();
        if (type = get_type()) {
            declare_local(type, stclass);
        } else if (stclass == LSTATIC || stclass == DEFAUTO) {
            declare_local(CINT, stclass);
        } else {
            return(0);
        }
        need_semicolon();
        return(1);
}

/**
 * non-declaration statement
 */
do_statement () {
        if (amatch ("if", 2)) {
                doif ();
                lastst = STIF;
        } else if (amatch ("while", 5)) {
                dowhile ();
                lastst = STWHILE;
        } else if (amatch ("switch", 6)) {
                doswitch ();
                lastst = STSWITCH;
        } else if (amatch ("do", 2)) {
                dodo ();
                need_semicolon ();
                lastst = STDO;
        } else if (amatch ("for", 3)) {
                dofor ();
                lastst = STFOR;
        } else if (amatch ("return", 6)) {
                doreturn ();
                need_semicolon ();
                lastst = STRETURN;
        } else if (amatch ("break", 5)) {
                dobreak ();
                need_semicolon ();
                lastst = STBREAK;
        } else if (amatch ("continue", 8)) {
                docont ();
                need_semicolon ();
                lastst = STCONT;
        } else if (match (";"))
                ;
        else if (amatch ("case", 4)) {
                docase ();
                lastst = statement (NO);
        } else if (amatch ("default", 7)) {
                dodefault ();
                lastst = statement (NO);
        } else if (match ("__asm__")) {
                doasm ();
                lastst = STASM;
        } else if (match("#")) {
                kill();
        } else if (match ("{"))
                do_compound (NO);
        else {
                expression (YES);
/*              if (match (":")) {
                        dolabel ();
                        lastst = statement (NO);
                } else {
*/                      need_semicolon ();
                        lastst = STEXP;
/*              }
*/      }
}

/**
 * compound statement
 * allow any number of statements to fall between "{" and "}"
 * 'func' is true if we are in a "function_statement", which
 * must contain "statement_list"
 */
do_compound(int func) {
        int     decls;

        decls = YES;
        ncmp++;
        while (!match ("}")) {
                if (feof (input))
                        return;
                if (decls) {
                        if (!statement_declare ())
                                decls = NO;
                } else
                        do_statement ();
        }
        ncmp--;
}

/**
 * "if" statement
 */
doif() {
        int     fstkp, flab1, flab2;
        int     flev;

        flev = local_table_index;
        fstkp = stkp;
        flab1 = getlabel ();
        test (flab1, FALSE);
        statement (NO);
        stkp = gen_modify_stack (fstkp);
        local_table_index = flev;
        if (!amatch ("else", 4)) {
                generate_label (flab1);
                return;
        }
        gen_jump (flab2 = getlabel ());
        generate_label (flab1);
        statement (NO);
        stkp = gen_modify_stack (fstkp);
        local_table_index = flev;
        generate_label (flab2);
}

/**
 * "while" statement
 */
dowhile() {
        loop_t loop;

        loop.symbol_idx = local_table_index;
        loop.stack_pointer = stkp;
        loop.type = WSWHILE;
        loop.test_label = getlabel ();
        loop.exit_label = getlabel ();
        addloop (&loop);
        generate_label (loop.test_label);
        test (loop.exit_label, FALSE);
        statement (NO);
        gen_jump (loop.test_label);
        generate_label (loop.exit_label);
        local_table_index = loop.symbol_idx;
        stkp = gen_modify_stack (loop.stack_pointer);
        delloop ();
}

/**
 * "do" statement
 */
dodo() {
        loop_t loop;

        loop.symbol_idx = local_table_index;
        loop.stack_pointer = stkp;
        loop.type = WSDO;
        loop.body_label = getlabel ();
        loop.test_label = getlabel ();
        loop.exit_label = getlabel ();
        addloop (&loop);
        generate_label (loop.body_label);
        statement (NO);
        if (!match ("while")) {
                error ("missing while");
                return;
        }
        generate_label (loop.test_label);
        test (loop.body_label, TRUE);
        generate_label (loop.exit_label);
        local_table_index = loop.symbol_idx;
        stkp = gen_modify_stack (loop.stack_pointer);
        delloop ();
}

/**
 * "for" statement
 */
dofor() {
        loop_t loop;
        loop_t *p;

        loop.symbol_idx = local_table_index;
        loop.stack_pointer = stkp;
        loop.type = WSFOR;
        loop.test_label = getlabel ();
        loop.cont_label = getlabel ();
        loop.body_label = getlabel ();
        loop.exit_label = getlabel ();
        addloop (&loop);
        p = readloop ();
        needbrack ("(");
        if (!match (";")) {
                expression (YES);
                need_semicolon ();
        }
        generate_label (p->test_label);
        if (!match (";")) {
                expression (YES);
                gen_test_jump (p->body_label, TRUE);
                gen_jump (p->exit_label);
                need_semicolon ();
        } else
                p->test_label = p->body_label;
        generate_label (p->cont_label);
        if (!match (")")) {
                expression (YES);
                needbrack (")");
                gen_jump (p->test_label);
        } else
                p->cont_label = p->test_label;
        generate_label (p->body_label);
        statement (NO);
        gen_jump (p->cont_label);
        generate_label (p->exit_label);
        local_table_index = p->symbol_idx;
        stkp = gen_modify_stack (p->stack_pointer);
        delloop ();
}

/**
 * "switch" statement
 */
doswitch() {
        loop_t loop;
        loop_t *ptr;

        loop.symbol_idx = local_table_index;
        loop.stack_pointer = stkp;
        loop.type = WSSWITCH;
        loop.test_label = swstp;
        loop.body_label = getlabel ();
        loop.cont_label = loop.exit_label = getlabel ();
        addloop (&loop);
        gen_immediate_a ();
        print_label (loop.body_label);
        newline ();
        gen_push ();
        needbrack ("(");
        expression (YES);
        needbrack (")");
        stkp = stkp + INTSIZE;  // '?case' will adjust the stack
        gen_jump_case ();
        statement (NO);
        ptr = readswitch ();
        gen_jump (ptr->exit_label);
        dumpsw (ptr);
        generate_label (ptr->exit_label);
        local_table_index = ptr->symbol_idx;
        stkp = gen_modify_stack (ptr->stack_pointer);
        swstp = ptr->test_label;
        delloop ();
}

/**
 * "case" label
 */
docase() {
        int     val;

        val = 0;
        if (readswitch ()) {
                if (!number (&val))
                        if (!quoted_char (&val))
                                error ("bad case label");
                addcase (val);
                if (!match (":"))
                        error ("missing colon");
        } else
                error ("no active switch");
}

/**
 * "default" label
 */
dodefault() {
        loop_t *ptr;
        int        lab;

        if (ptr = readswitch ()) {
                ptr->cont_label = lab = getlabel ();
                generate_label (lab);
                if (!match (":"))
                        error ("missing colon");
        } else
                error ("no active switch");
}

/**
 * "return" statement
 */
doreturn() {
        if (endst () == 0)
                expression (YES);
        gen_jump(fexitlab);
}

/**
 * "break" statement
 */
dobreak() {
        loop_t *ptr;

        if ((ptr = readloop ()) == 0)
                return;
        gen_modify_stack (ptr->stack_pointer);
        gen_jump (ptr->exit_label);
}

/**
 * "continue" statement
 */
docont() {
        loop_t *ptr;

        if ((ptr = findloop ()) == 0)
                return;
        gen_modify_stack (ptr->stack_pointer);
        if (ptr->type == WSFOR)
                gen_jump (ptr->cont_label);
        else
                gen_jump (ptr->test_label);
}

/**
 * dump switch table
 */
dumpsw(loop_t *loop) {
        int     i,j;

        data_segment_gdata ();
        generate_label (loop->body_label);
        if (loop->test_label != swstp) {
                j = loop->test_label;
                while (j < swstp) {
                        gen_def_word ();
                        i = 4;
                        while (i--) {
                                output_number (swstcase[j]);
                                output_byte (',');
                                print_label (swstlab[j++]);
                                if ((i == 0) | (j >= swstp)) {
                                        newline ();
                                        break;
                                }
                                output_byte (',');
                        }
                }
        }
        gen_def_word ();
        print_label (loop->cont_label);
        output_string (",0");
        newline ();
        code_segment_gtext ();
}

/*      File lex.c: 2.1 (83/03/20,16:02:09) */
/*% cc -O -c %
 *
 */

#include <stdio.h>
#include "defs.h"
#include "data.h"

/**
 * test if given character is alpha
 * @param c
 * @return
 */
alpha(char c) {
    if (c > 127)
            return 0;
    return (((c >= 'a') && (c <= 'z')) ||
            ((c >= 'A') && (c <= 'Z')) ||
            (c == '_'));
}

/**
 * test if given character is numeric
 * @param c
 * @return
 */
numeric(char c) {
    if (c > 127)
            return 0;
    return ((c >= '0') && (c <= '9'));
}

/**
 * test if given character is alphanumeric
 * @param c
 * @return
 */
alphanumeric(char c) {
    return ((alpha (c)) || (numeric (c)));
}

/**
 * semicolon enforcer
 * called whenever syntax requires a semicolon
 */
need_semicolon() {
    if (!match (";"))
        error ("missing semicolon");
}

junk() {
    if (alphanumeric (inbyte ()))
        while (alphanumeric (ch ()))
            gch ();
    else
        while (alphanumeric (ch ())) {
            if (ch () == 0)
                break;
            gch ();
        }
    blanks ();
}

endst() {
    blanks ();
    return ((streq (line + lptr, ";") | (ch () == 0)));
}

/**
 * enforces bracket
 * @param str
 * @return
 */
needbrack(char *str) {
    if (!match (str)) {
        error ("missing bracket");
        gen_comment ();
        output_string (str);
        newline ();
    }
}

/**
 *
 * @param str1
 * @return
 */
sstreq(str1) char *str1; {
    return (streq(line + lptr, str1));
}

/**
 * indicates whether or not the current substring in the source line matches a
 * literal string
 * accepts the address of the current character in the source
 * line and the address of the a literal string, and returns the substring length
 * if a match occurs and zero otherwise
 * @param str1 address1
 * @param str2 address2
 * @return
 */
streq(char str1[], char str2[]) {
    int k;
    k = 0;
    while (str2[k]) {
        if ((str1[k] != str2[k]))
            return (0);
        k++;
    }
    return (k);
}

/**
 * compares two string both must be zero ended, otherwise no match found
 * ensures that the entire token is examined
 * @param str1
 * @param str2
 * @param len
 * @return
 */
astreq (char str1[], char str2[], int len) {
    int k;
    k = 0;
    while (k < len) {
        if ((str1[k] != str2[k]))
            break;
        if (str1[k] == 0)
            break;
        if (str2[k] == 0)
            break;
        k++;
    }
    if (alphanumeric (str1[k]))
        return (0);
    if (alphanumeric (str2[k]))
        return (0);
    return (k);
}

/**
 * looks for a match between a literal string and the current token in
 * the input line. It skips over the token and returns true if a match occurs
 * otherwise it retains the current position in the input line and returns false
 * there is no verification that all of the token was matched
 * @param lit
 * @return
 */
match (char *lit) {
    int k;
    blanks();
    if (k = streq (line + lptr, lit)) {
        lptr = lptr + k;
        return (1);
    }
    return (0);
}

/**
 * compares two string both must be zero ended, otherwise no match found
 * advances line pointer only if match found
 * it assumes that an alphanumeric (including underscore) comparison
 * is being made and guarantees that all of the token in the source line is
 * scanned in the process
 * @param lit
 * @param len
 * @return
 */
amatch(char *lit, int len) {
    int k;

    blanks();
    if (k = astreq (line + lptr, lit, len)) {
        lptr = lptr + k;
        while (alphanumeric (ch ()))
            inbyte ();
        return (1);
    }
    return (0);
}

blanks() {
    for (;;) {
        while (ch () == 0) {
            readline ();
            if (feof (input))
                break;
        }
        if (ch () == ' ')
            gch ();
        else if (ch () == 9)
            gch ();
        else
            return;
    }
}

/**
 * returns declaration type
 * @return CCHAR, CINT, UCHAR, UINT
 */
int get_type() {
    if (amatch ("register", 8)) {
        if (amatch("char", 4))
            return CCHAR;
        else if (amatch ("int", 3))
            return CINT;
        else
            return CINT;
    } else if(amatch("unsigned", 8)) {
        if (amatch("char", 4)) {
            return UCHAR;
        } else if (amatch("int", 3)) {
            return UINT;
        }
    } else if(amatch("signed", 8)) {
        if (amatch("char", 4)) {
            return CCHAR;
        } else if (amatch("int", 3)) {
            return CINT;
        }
    } else if (amatch ("char", 4)) {
        return CCHAR;
    } else if (amatch ("int", 3)) {
        return CINT;
    }
    return 0;
}

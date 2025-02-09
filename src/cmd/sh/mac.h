/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */

#define TRUE (-1)
#define FALSE 0
#define LOBYTE 0377
#define STRIP 0177
#define QUOTE 0200

#define EOF 0
#define NL '\n'
#define SP ' '
#define LQ '`'
#define RQ '\''
#define MINUS '-'
#define COLON ':'
#define TAB '\t'

#define blank() prc(SP)
#define tab() prc(TAB)
#define newline() prc(NL)

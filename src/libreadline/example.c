#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "readline.h"
#include "history.h"

int main(int argc, char **argv) {
    char *line;
    char *prgname = argv[0];

    /* Parse options, with --multiline we enable multi line editing. */
    while(argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv,"--multiline")) {
            readline_set_multiline(1);
            printf("Multi-line mode enabled.\n");
        } else if (!strcmp(*argv,"--keycodes")) {
            readline_print_keycodes();
            exit(0);
        } else {
            fprintf(stderr, "Usage: %s [--multiline] [--keycodes]\n", prgname);
            exit(1);
        }
    }

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    add_history("history.txt"); /* Load the history at startup */

    /* Now this is the main loop of the typical readline-based application.
     * The call to readline() will block as long as the user types something
     * and presses enter.
     *
     * The typed string is returned as a malloc() allocated string by
     * readline, so the user needs to free() it. */
    while((line = readline("hello> ")) != NULL) {
        /* Do something with the string. */
        if (line[0] != '\0' && line[0] != '/') {
            printf("echo: '%s'\n", line);
            add_history(line); /* Add to the history. */
            write_history("history.txt"); /* Save the history on disk. */
        } else if (!strncmp(line,"/historylen",11)) {
            /* The "/historylen" command will change the history len. */
            int len = atoi(line+11);
            history_set_length(len);
        } else if (line[0] == '/') {
            printf("Unrecognized command: %s\n", line);
        }
        free(line);
    }
    return 0;
}

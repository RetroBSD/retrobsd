/*
 *  topological sort
 *  input is sequence of pairs of items (blank-free strings)
 *  nonidentical pair is a directed edge in graph
 *  identical pair merely indicates presence of node
 *  output is ordered list of items consistent with
 *  the partial ordering specified by the graph
 */
#include <stdio.h>
#include <stdlib.h>

/*  the nodelist always has an empty element at the end to
 *  make it easy to grow in natural order
 *  states of the "live" field:
 */
#define DEAD 0    /* already printed*/
#define LIVE 1    /* not yet printed*/
#define VISITED 2 /*used only in findloop()*/

/*  a predecessor list tells all the immediate
 *  predecessors of a given node
 */
struct predlist {
    struct predlist *nextpred;
    struct nodelist *pred;
};

struct nodelist {
    struct nodelist *nextnode;
    struct predlist *inedges;
    char *name;
    int live;
} firstnode = { NULL, NULL, NULL, DEAD };

struct nodelist *nindex();
struct nodelist *findloop();
struct nodelist *mark();
char *empty = "";

static void error(char *s, char *t);
static int present(struct nodelist *i, struct nodelist *j);
static int anypred(struct nodelist *i);
static int cmp(char *s, char *t);
static void note(char *s, char *t);

/*  the first for loop reads in the graph,
 *  the second prints out the ordering
 */
int main(int argc, char **argv)
{
    struct predlist *t;
    FILE *input = stdin;
    struct nodelist *i, *j;
    int x;
    char precedes[50], follows[50];

    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (input == NULL)
            error("cannot open ", argv[1]);
    }
    for (;;) {
        x = fscanf(input, "%s%s", precedes, follows);
        if (x == EOF)
            break;
        if (x != 2)
            error("odd data", empty);
        i = nindex(precedes);
        j = nindex(follows);
        if (i == j || present(i, j))
            continue;
        t = (struct predlist *)malloc(sizeof(struct predlist));
        t->nextpred = j->inedges;
        t->pred = i;
        j->inedges = t;
    }
    for (;;) {
        x = 0; /*anything LIVE on this sweep?*/
        for (i = &firstnode; i->nextnode != NULL; i = i->nextnode) {
            if (i->live == LIVE) {
                x = 1;
                if (!anypred(i))
                    break;
            }
        }
        if (x == 0)
            break;
        if (i->nextnode == NULL)
            i = findloop();
        printf("%s\n", i->name);
        i->live = DEAD;
    }
}

/*  is i present on j's predecessor list?
 */
int present(struct nodelist *i, struct nodelist *j)
{
    struct predlist *t;

    for (t = j->inedges; t != NULL; t = t->nextpred)
        if (t->pred == i)
            return (1);
    return (0);
}

/*  is there any live predecessor for i?
 */
int anypred(struct nodelist *i)
{
    struct predlist *t;

    for (t = i->inedges; t != NULL; t = t->nextpred)
        if (t->pred->live == LIVE)
            return (1);
    return (0);
}

/*  turn a string into a node pointer
 */
struct nodelist *nindex(char *s)
{
    struct nodelist *i;
    char *t;

    for (i = &firstnode; i->nextnode != NULL; i = i->nextnode)
        if (cmp(s, i->name))
            return (i);
    for (t = s; *t; t++)
        ;
    t = malloc((unsigned)(t + 1 - s));
    i->nextnode = (struct nodelist *)malloc(sizeof(struct nodelist));
    if (i->nextnode == NULL || t == NULL)
        error("too many items", empty);
    i->name = t;
    i->live = LIVE;
    i->nextnode->nextnode = NULL;
    i->nextnode->inedges = NULL;
    i->nextnode->live = DEAD;
    while ((*t++ = *s++))
        ;
    return (i);
}

int cmp(char *s, char *t)
{
    while (*s == *t) {
        if (*s == 0)
            return (1);
        s++;
        t++;
    }
    return (0);
}

void error(char *s, char *t)
{
    note(s, t);
    exit(1);
}

void note(char *s, char *t)
{
    fprintf(stderr, "tsort: %s%s\n", s, t);
}

/*  given that there is a cycle, find some
 *  node in it
 */
struct nodelist *findloop()
{
    struct nodelist *i, *j;

    for (i = &firstnode; i->nextnode != NULL; i = i->nextnode)
        if (i->live == LIVE)
            break;
    note("cycle in data", empty);
    i = mark(i);
    if (i == NULL)
        error("program error", empty);
    for (j = &firstnode; j->nextnode != NULL; j = j->nextnode)
        if (j->live == VISITED)
            j->live = LIVE;
    return (i);
}

/*  depth-first search of LIVE predecessors
 *  to find some element of a cycle;
 *  VISITED is a temporary state recording the
 *  visits of the search
 */
struct nodelist *mark(struct nodelist *i)
{
    struct nodelist *j;
    struct predlist *t;

    if (i->live == DEAD)
        return (NULL);
    if (i->live == VISITED)
        return (i);
    i->live = VISITED;
    for (t = i->inedges; t != NULL; t = t->nextpred) {
        j = mark(t->pred);
        if (j != NULL) {
            note(i->name, empty);
            return (j);
        }
    }
    return (NULL);
}

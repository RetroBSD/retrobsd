/*
 * save (III)   J. Gillogly
 * save user core image for restarting
 */
#include "hdr.h"
#include <unistd.h>
#include <fcntl.h>

void
save(savfile, offset)                   /* save game state to file      */
char *savfile;
unsigned offset;
{
	int fd, i;
        struct travlist *entry;

        if (offset == 0)
                fd = creat(savfile, 0644);
        else
                fd = open(savfile, O_WRONLY);
	if (fd < 0) {
	        printf("Cannot write to %s\n", savfile);
		exit(0);
	}
        if (offset != 0) {               /* use offset                   */
                lseek (fd, (off_t) offset, SEEK_SET);
        }
        write(fd, &game, sizeof game);  /* write all game data          */

        for (i=0; i<LOCSIZ; i++) {      /* save travel lists            */
                entry = travel[i];
                if (! entry)
                        continue;
                while (entry != 0) {
                        /*printf("entry %d: next=%p conditions=%d tloc=%d tverb=%d\n",
                                i, entry->next, entry->conditions, entry->tloc, entry->tverb);*/
                        write(fd, entry, sizeof(*entry));
                        entry = entry->next;
                }
        }
        printf("Saved %u bytes to %s\n",
                (unsigned) lseek(fd, (off_t) 0, SEEK_CUR), savfile);
	close(fd);
}

int
restdat(fd, offset)                     /* restore game from dat file   */
int fd;
unsigned offset;
{
	int i;
        struct travlist **entryp;

        if (offset != 0)                /* use offset                   */
                lseek (fd, (off_t) offset, SEEK_SET);

        /* read all game data */
        if (read(fd, &game, sizeof game) != sizeof game) {
failed:         close(fd);
	        return 0;
        }

        for (i=0; i<LOCSIZ; i++) {      /* restore travel lists         */
                if (! travel[i])
                        continue;
                entryp = &travel[i];
                while (*entryp != 0) {
                        *entryp = (struct travlist*) malloc(sizeof(**entryp));
                        if (! *entryp)
                                goto failed;
                        if (read(fd, *entryp, sizeof **entryp) != sizeof **entryp)
                                goto failed;
                        /*printf("entry %d: next=%p conditions=%d tloc=%d tverb=%d\n",
                                i, (*entryp)->next, (*entryp)->conditions,
                                (*entryp)->tloc, (*entryp)->tverb);*/
                        entryp = &(*entryp)->next;
                }
        }
        return 1;
}

int
restore(savfile)                        /* restore game from user file  */
char *savfile;
{
	int fd;

	fd = open(savfile, O_RDONLY);
	if (fd < 0)
	        return 0;
	if (! restdat(fd, 0))
	        return 0;
	close(fd);
        return 1;
}

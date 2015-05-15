/* 
 *  $Id: help.c,v 1.8 2011/03/08 12:37:41 rader Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <curses.h>
#include "cnagios.h"

/*------------------------------------------------------------------*/

void
help()
{
  FILE *fp;
  int l;
  char buf[MAX_CHARS_PER_LINE];

  alarm(0);
  clear();

  if ((fp = fopen(HELP_FILE, "r")) == NULL) {
    endwin();
    fprintf(stderr,"fatal error: fopen %s ",HELP_FILE);
    perror("failed");
    exit(1);
  }

  for ( l = 0; fgets(buf,sizeof(buf),fp) != NULL;  l++ ) {
    mvaddstr(l,0,buf);
  }
  fclose(fp);

  move(LINES-1,0);
  refresh();

  getch();

}


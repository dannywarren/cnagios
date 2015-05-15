/*
 * $Id: debug.c,v 1.9 2011/03/08 12:37:41 rader Exp $
 */

#include <curses.h>
#include "cnagios.h"

extern int debugging;

/*------------------------------------------------------------------*/

void
debug(msg,a,b,c,d,e,f,g,h,i,j,k)
char *msg;
{
  if ( debugging == 1 ) {
    fprintf(stderr,msg,a,b,c,d,e,f,g,h,i,j,k);
    fprintf(stderr,"\r\n");
    fflush(stderr);
  }
}


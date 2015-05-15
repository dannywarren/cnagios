/*
 *
 * $Id: update.c,v 1.5 2011/03/08 12:37:41 rader Exp $
 *
 */

/*------------------------------------------------------------------*/

#include "cnagios.h"
#include <signal.h>
#include <unistd.h>

extern int need_swipe;
extern int polling_interval;

/*------------------------------------------------------------------*/

void update_display() 
{
  alarm(0);
  read_status();
  need_swipe = 1;
  draw_screen();
#ifdef SUN
  signal(SIGALRM,update_display);
#endif
  alarm(polling_interval);
}


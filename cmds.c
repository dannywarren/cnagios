/*
 *
 * $Id: cmds.c,v 1.54 2011/03/08 12:37:41 rader Exp $
 *
 */

/*------------------------------------------------------------------*/

#include <string.h>
#include <unistd.h>
#include "cnagios.h"
#include <curses.h>

int spacebar_direction = 0;
int need_swipe = 0;
int filter_set = NO_FILTER;
char name_filter[STRING_LENGTH];
char not_name_filter[STRING_LENGTH];
char age_filter[STRING_LENGTH];
char age_okay_filter[STRING_LENGTH];
int age_filter_secs;
int age_okay_filter_secs;

extern int object_mode;
extern int host_level;
extern int service_level;
extern int sort_mode;
extern int display_mode;
extern int cur_page;
extern int num_pages;
extern int polling_interval;
extern int pad;
extern char fkey_name_filters[13][STRING_LENGTH];
extern char fkey_not_name_filters[13][STRING_LENGTH];


/*------------------------------------------------------------------*/

void
getch_loop() 
{
  int ch, secs, page_up, page_down, i;
  char buf[STRING_LENGTH];

  for (;;) {

    page_up = 0;
    page_down = 0;

    ch = getch();


    switch(ch) {

      /*--------------------*/
      case ERR:
        /* interrupted or somesuch... no action... */
      break;


      /*--------------------*/
      case 'u':
        if ( object_mode != HOST_OBJECTS || host_level != UP ) {
          addstr("up mode");
          cur_page = 1;
          need_swipe = 1;
          refresh();
          usleep(SHORT_MSG_DELAY);
        } else {
          beep();
        } 
        host_level = UP;
        object_mode = HOST_OBJECTS;
      break;

      /*--------------------*/
      case 'd':
        if ( object_mode != HOST_OBJECTS || host_level != DOWN ) {
          addstr("down mode");
          cur_page = 1;
          need_swipe = 1;
          refresh();
          usleep(SHORT_MSG_DELAY);
        } else {
          beep();
        } 
        host_level = DOWN;
        object_mode = HOST_OBJECTS;
      break;

      /*--------------------*/
      case 'o':
        if ( object_mode != SERVICE_OBJECTS || service_level != OKAY ) { 
          addstr("okay mode");
          cur_page = 1; 
          need_swipe = 1; 
          refresh();
          usleep(SHORT_MSG_DELAY);
        } else {
          beep();
        } 
        service_level = OKAY;
        object_mode = SERVICE_OBJECTS;
      break;

      /*--------------------*/
      case 'w':
        if ( object_mode != SERVICE_OBJECTS || service_level != WARNING ) { 
          addstr("warning mode");
          cur_page = 1; 
          need_swipe = 1; 
          refresh();
          usleep(SHORT_MSG_DELAY);
        } else {
          beep();
        } 
        service_level = WARNING;
        object_mode = SERVICE_OBJECTS;
      break; 

      /*--------------------*/
      case 'c':
        if ( object_mode != SERVICE_OBJECTS || service_level != CRITICAL ) { 
          addstr("critical mode");
          cur_page = 1; 
          need_swipe = 1; 
          refresh();
          usleep(SHORT_MSG_DELAY);
        } else {
          beep();
        } 
        service_level = CRITICAL;
        object_mode = SERVICE_OBJECTS;
      break; 

      /*--------------------*/
      case 's':
      case 't':
        switch (sort_mode) {
          case SORT_BY_AGE:
            sort_mode = SORT_BY_NAME;
            addstr("sort by name");
          break;
          case SORT_BY_NAME:
            sort_mode = SORT_BY_AGE;
            addstr("sort by age");
          break;
        }
        need_swipe = 1; 
        refresh();
        usleep(SHORT_MSG_DELAY);
      break;

      /*--------------------*/
      case ' ':
        if ( cur_page == 1 && num_pages == 1 ) { break; }
        if ( spacebar_direction == 0 ) {
          if ( cur_page < num_pages ) { 
            cur_page++; 
            page_down = 1;
          } else { 
            cur_page--;  
            page_up = 1;
            spacebar_direction = 1;
          }
        } else {
          if ( cur_page > 1 ) { 
            cur_page--;
            page_up = 1;
          } else { 
            cur_page++;
            page_down = 1;
            spacebar_direction = 0;
          }
        } 
        need_swipe = 1; 
      break;

      /*--------------------*/
      case 'r':
      case 0x0c:  /* ^l */
        alarm(0);
        read_status();
        alarm(polling_interval);
        need_swipe = 1; 
      break; 

      /*--------------------*/
      case '+':
      case 0x06:  /* vi */
      case '>':   /* emacs */
        if ( cur_page < num_pages ) { 
          cur_page++; 
          page_down = 1;
          need_swipe = 1; 
        }
      break; 

      /*--------------------*/
      case '-':
      case 0x02: /* vi */
      case '<':  /* emacs */
        if ( cur_page > 1 ) { 
          cur_page--;  
          page_up = 1;
          need_swipe = 1; 
        }
      break; 

      /*--------------------*/
      case '0':
        if ( cur_page != 1 ) { 
          cur_page = 1;
          addstr("first page");
          refresh();
          usleep(SHORT_MSG_DELAY);
          need_swipe = 1;
        }
      break; 

      /*--------------------*/
      case 'G':
        if ( cur_page != num_pages ) {
          cur_page = num_pages;
          addstr("last page");
          refresh();
          usleep(SHORT_MSG_DELAY);
          need_swipe = 1;
        }
      break; 

      /*--------------------*/
      case 'q':
      case 0x04: /* ^d */
        addstr("quit");
        refresh();
        usleep(SHORT_MSG_DELAY);
        return;

      /*--------------------*/
      case '?':
      case 'h':
        addstr("help");
        refresh();
        usleep(SHORT_MSG_DELAY);
        help();
      break;

      /*--------------------*/
      case '=':
      case 'g':
      case 'f':
      case '/':
        mvaddstr(LINES-1-pad,pad,"Filter: ");
        refresh();
        echo();
        curs_set(1);
        alarm(0);
        getstr(name_filter);
        alarm(polling_interval);
        curs_set(0);
        noecho();
        if ( name_filter[0] == '\0' ) {
          if ( BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) {
            filter_set -= FILTER_BY_NAME;
          }
        } else {
          if ( ! BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) { 
            filter_set += FILTER_BY_NAME;
          }
          if ( BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) { 
            filter_set -= FILTER_BY_NOT_NAME;
            not_name_filter[0] = '\0';
          }
        }
        cur_page = 1;
        need_swipe = 1; 
      break;

      /*--------------------*/
      case '!':
      case 'F':
      case 'v':
        mvaddstr(LINES-1-pad,pad,"NegativeFilter: ");
        refresh();
        echo();
        curs_set(1);
        alarm(0);
        getstr(not_name_filter);
        alarm(polling_interval);
        curs_set(0);
        noecho();
        if ( not_name_filter[0] == '\0' ) {
          if ( BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) {
            filter_set -= FILTER_BY_NOT_NAME;
          }
        } else {
          if ( ! BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) { 
            filter_set += FILTER_BY_NOT_NAME;
          }
          if ( BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) { 
            filter_set -= FILTER_BY_NAME;
            name_filter[0] = '\0';
          }
        }
        cur_page = 1;
        need_swipe = 1;
      break;

      /*--------------------*/
      case 'a':
        mvaddstr(LINES-1-pad,pad,"AgeFilter: ");
        refresh();
        echo();
        curs_set(1);
        alarm(0);
        getstr(buf);
        alarm(polling_interval);
        curs_set(0);
        noecho();
        if ( buf[0] == '\0' ) {
          if ( BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
            filter_set -= FILTER_BY_AGE;
          }
        } else if ( secs = parse_age_filter(buf,sizeof(buf)) ) { 
          age_filter_secs = secs;
          strncpy(age_filter,buf,sizeof(age_filter)-1);
          age_filter[sizeof(age_filter)-1] = '\0';
          if ( ! BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
            filter_set += FILTER_BY_AGE;
          }
          if ( BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
            filter_set -= FILTER_BY_AGE_OKAY;
            age_okay_filter[0] = '\0';
          }
          cur_page = 1;
          need_swipe = 1;
        } else {
          beep();
          mvaddstr(LINES-1-pad,pad,"AgeFilter: ");
          addstr(buf);
          addstr(": syntax error!");
          refresh();
          usleep(ERROR_MSG_DELAY);
          need_swipe = 1;
        }
      break;

      /*--------------------*/
      case 'A':
        if ( object_mode == HOST_OBJECTS ) {
          mvaddstr(LINES-1-pad,pad,"UpAgeFilter: ");
        }
        if ( object_mode == SERVICE_OBJECTS ) {
          mvaddstr(LINES-1-pad,pad,"OkayAgeFilter: ");
        }
        refresh();
        echo();
        curs_set(1);
        alarm(0);
        getstr(buf);
        alarm(polling_interval);
        curs_set(0);
        noecho();
        if ( buf[0] == '\0' ) {
          if ( BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
            filter_set -= FILTER_BY_AGE_OKAY;
          }
        } else if ( secs = parse_age_filter(buf,sizeof(buf)) ) {
          age_okay_filter_secs = secs;
          strncpy(age_okay_filter,buf,sizeof(age_okay_filter)-1);
          age_filter[sizeof(age_okay_filter)-1] = '\0';
          if ( ! BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
            filter_set += FILTER_BY_AGE_OKAY;
          }
          if ( BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
            filter_set -= FILTER_BY_AGE;
            age_filter[0] = '\0';
          }
          cur_page = 1;
          need_swipe = 1;
        } else {
          beep();
          mvaddstr(LINES-1-pad,pad,"OkayAgeFilter: ");
          addstr(buf);
          addstr(": syntax error!");
          refresh();
          usleep(ERROR_MSG_DELAY);
          need_swipe = 1;
        }
      break;

      /*--------------------*/
      case 'x':
        if ( filter_set ) {
          addstr("filter deleted");
          refresh();
          usleep(LONG_MSG_DELAY);
          filter_set = NO_FILTER;
          cur_page = 1;
          need_swipe = 1; 
        }
      break;

      /*--------------------*/
      default:

        /*--------------------*/
        /* fuction keys */
        for (i=1; i<=12; i++) {
          if ( ch == KEY_F(i) ) { 
            if ( fkey_name_filters[i][0] != '\0' ) {
              strncpy(name_filter,fkey_name_filters[i],sizeof(name_filter)-1);
              name_filter[sizeof(name_filter)-1] = '\0';
              mvaddstr(LINES-1-pad,pad,"FkeyFilter: ");
              addstr(name_filter);
              refresh();
              usleep(LONG_MSG_DELAY);
              if ( ! BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) {
                filter_set += FILTER_BY_NAME;
              }
              if ( BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) {
                filter_set -= FILTER_BY_NOT_NAME;
                not_name_filter[0] = '\0';
              }
            } else if ( fkey_not_name_filters[i][0] != '\0' ) {
              strncpy(not_name_filter,fkey_name_filters[i],sizeof(not_name_filter)-1);
              not_name_filter[sizeof(not_name_filter)-1] = '\0';
              mvaddstr(LINES-1-pad,pad,"FkeyNegativeFilter: ");
              addstr(not_name_filter);
              refresh();
              usleep(LONG_MSG_DELAY);
              if ( ! BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) {
                filter_set += FILTER_BY_NOT_NAME;
              }
              if ( BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) {
                filter_set -= FILTER_BY_NAME;
                name_filter[0] = '\0';
              }
            } else {
              mvaddstr(LINES-1-pad,pad,"FkeyFilter: ");
              addstr("no filter defined!");
              refresh();
              beep();
              usleep(ERROR_MSG_DELAY);
            }
            cur_page = 1;
            need_swipe = 1;
            break;
          }
        }

        /*--------------------*/
        /* illegal keys */
        if ( ch > 31 && ch < 127 ) { 
          beep(); 
          break;
        }

    }

    if ( page_up ) {
      addstr("page up");
      refresh();
      usleep(SHORT_MSG_DELAY);
    }
    if ( page_down ) {
      addstr("page down");
      refresh();
      usleep(SHORT_MSG_DELAY);
    }
   
    draw_screen();

  }
}

/*------------------------------------------------------------------*/

int
parse_age_filter(p,plen)
char *p;
int plen;
{
   char l[1024];
   int d;
   if ( sscanf(p,"%d%s",&d,l) == 2 ) {
      if ( strcmp(l,"d") == 0 || strcmp(l,"day") == 0 || strcmp(l,"days") == 0 ) {
        snprintf(p,plen,"%dd",d);
        return(d*24*60*60);
      }
      if ( strcmp(l,"h") == 0 || strcmp(l,"hr") == 0 || strcmp(l,"hrs") == 0 ) {
        snprintf(p,plen,"%dhr",d);
        return(d*60*60);
      }
      if ( strcmp(l,"m") == 0 || strcmp(l,"min") == 0 || strcmp(l,"mins") == 0 ) {
        snprintf(p,plen,"%dm",d);
        return(d*60);
      }
      if ( strcmp(l,"s") == 0 || strcmp(l,"sec") == 0 || strcmp(l,"secs") == 0 ) {
        snprintf(p,plen,"%ds",d);
        return(d);
      }
      return(0);
   }
   if ( sscanf(p,"%d",&d) == 1 ) {
     snprintf(p,plen,"%ds",d);
     return(d);
   }
   return(0);
}


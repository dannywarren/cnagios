/*
 *
 * $Id: draw.c,v 1.89 2011/03/22 17:21:11 rader Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <unistd.h>
#include "cnagios.h"
#include "version.h"

/*----------------------------------------------------------------------------*/

int cur_page = 1;
int num_pages;
struct seen_item *seen_items = NULL;
struct seen_item *tail = NULL;

extern int object_mode;
extern int host_level;
extern int service_level;
extern int sort_mode;
extern int header_pad;
extern int pad;
extern int color;
extern int debugging;
extern int host_list_size;
extern char *host_list[MAX_ITEMS][STATUS_LIST_ENTRY_SIZE];
extern int host_idx_to_level[MAX_ITEMS];
extern struct obj_by_age *hosts_by_age;
extern int service_list_size;
extern char *service_list[MAX_ITEMS][STATUS_LIST_ENTRY_SIZE];
extern struct obj_by_age *services_by_age;
extern int service_idx_to_level[MAX_ITEMS];
extern int num_up, num_down;
extern int num_okay, num_warn, num_crit;
extern int need_swipe;
extern char last_update[21];
extern int last_update_int;
extern int filter_set;
extern char name_filter[STRING_LENGTH];
extern char not_name_filter[STRING_LENGTH];
extern char age_filter[STRING_LENGTH];
extern char age_okay_filter[STRING_LENGTH];
extern char site_name[STRING_LENGTH];
extern int age_filter_secs;
extern int age_okay_filter_secs;

/*----------------------------------------------------------------------------*/

void
draw_screen() 
{
  int i, j;
  int screen_line = 0;
  char buf[MAX_TERM_WIDTH]; 
  char buf2[MAX_TERM_WIDTH];
  struct obj_by_age *age_ptr;
  struct seen_item *tmp, *prev;
  int items_matched = 0, page_size, begin_item;
  int total_items_matched = 0;
  int descr_width = 0;
  int duration_int;
  int num_filters = 0;

#if _DEBUG_
  debug("draw_screen()...");
  debug("object_mode is %d",object_mode);
  debug("host_level is %d",host_level);
  debug("service_level is %d",service_level);
  debug("sort_mode is %d",sort_mode);
  debug("filter_set is %d",filter_set);
#endif

  /*--------------------------------------------------------*/
  /* prep work */

  clear();
  if ( color ) { 
    /* draw the whole screen in default color */
    attron(COLOR_PAIR(DEFAULT_COLOR));
    for(i=0; i<=LINES; i++) {
      mvaddstr(i,0," ");
      for(j=2;j<=COLS;j++) {
        addch(' ');
      }
    }
  }

  screen_line += pad;
  page_size = LINES - HEADER_SIZE - FOOTER_SIZE - pad*2;
#if _DEBUG_
  debug("page_size (items per page) is %d",page_size);
#endif
  if ( object_mode == HOST_OBJECTS ) {
    switch(host_level) {
      case UP: 
        snprintf(buf,sizeof(buf),"%d Host Objects",host_list_size);
        if ( host_list_size == 1 ) { buf[strlen(buf)-1] = '\0'; }
        num_pages = (host_list_size)/page_size;
        if ( (host_list_size%page_size) != 0 ) { num_pages++; }
      break;
      case DOWN: 
        snprintf(buf,sizeof(buf),"%d Down Hosts",num_down);
        if ( num_down == 1 ) { buf[strlen(buf)-1] = '\0'; }
        num_pages = num_down/page_size;
        if ( (num_down%page_size) != 0 ) { num_pages++; }
      break;
    }
  }
  if ( object_mode == SERVICE_OBJECTS ) {
    switch(service_level) {
      case OKAY: 
        snprintf(buf,sizeof(buf),"%d Service Objects",service_list_size);
        if ( service_list_size == 1 ) { buf[strlen(buf)-1] = '\0'; }
        num_pages = service_list_size/page_size; 
        if ( (service_list_size%page_size) != 0 ) { num_pages++; }
      break;
      case WARNING: 
        snprintf(buf,sizeof(buf),"%d Alerts",num_warn+num_crit);
        if ( num_warn+num_crit == 1 ) { buf[strlen(buf)-1] = '\0'; }
        num_pages = (num_warn+num_crit)/page_size;
        if ( ((num_warn+num_crit)%page_size) != 0 ) { num_pages++; }
      break;
      case CRITICAL: 
       snprintf(buf,sizeof(buf),"%d Alerts",num_crit);
        if ( num_crit == 1 ) { buf[strlen(buf)-1] = '\0'; }
        num_pages = (num_crit)/page_size;
        if ( (num_crit%page_size) != 0 ) { num_pages++; }
      break;
    }
  }

  /*--------------------------------------------------------*/
  /* because we haven't counted matches yet, "num_pages"    */
  /* here is really "start of current page"...              */
  /* make sure the current page is an existing page...      */
  if ( cur_page > num_pages ) { cur_page = num_pages; }

  /*--------------------------------------------------------*/
  if ( cur_page == 1 ) {
    begin_item = 0;
  } else {
    begin_item = (page_size * (cur_page - 1));
  }

#if _DEBUG_
  debug("begin_item is %d",begin_item);
#endif

  /*--------------------------------------------------------*/
  /* display blank screen for a bit to indicate screen's    */
  /* changed...                                             */

  if ( need_swipe ) {
    move(LINES-1,0);
    refresh();
    usleep(SWIPE_DELAY);
    need_swipe = 0;
  }

  /*--------------------------------------------------------*/
  /* 1st line: title */
  mvaddstr(screen_line,header_pad,site_name);
  addstr(" ");
  addstr(TITLE_SUFFIX);
  addstr(" v");
  addstr(VERSION);

  /*--------------------------------------------------------*/
  /* 1st line: num items/alerts */
  mvaddstr(screen_line,(COLS/2)-(strlen(buf)/2)-CENTERING_FUDGE,buf);

  /*--------------------------------------------------------*/
  /* 1st line: last reported time */
  mvaddstr(screen_line,COLS-19-header_pad,last_update);
  screen_line++;

  /*--------------------------------------------------------*/
  /* 2nd line: filled in later */
  screen_line++;

  /*--------------------------------------------------------*/
  /* 3rd line: filled in later */
  screen_line++;

  /*--------------------------------------------------------*/
  /* 4th line: filled in later */
  screen_line++;

  /*--------------------------------------------------------*/
  /* 5th line: header */ 
  mvaddstr(screen_line,pad,HEAD1_BEGIN);
  mvaddstr(screen_line,COLS-pad-LINE_END_LEN-sizeof(HEAD_MIDDLE_RHS),HEAD_MIDDLE_RHS);
  mvaddstr(screen_line,LINE_BEGIN_LEN+1+pad,HEAD_MIDDLE_LHS);
  mvaddstr(screen_line,COLS-pad-LINE_END_LEN,HEAD1_END);
  screen_line++;

  /*--------------------------------------------------------*/
  /* 5th line: header underline */ 
  mvaddstr(screen_line,pad,HEAD2_BEGIN);
  for (j = LINE_BEGIN_LEN+pad+1; j < COLS-pad-LINE_END_LEN-1; j++) {
    mvaddch(screen_line,j,'-');
  }
  mvaddstr(screen_line,COLS-pad-LINE_END_LEN,HEAD2_END);
  screen_line++;

  /*--------------------------------------------------------*/
  /* make a list of objects to display */

  if ( object_mode == HOST_OBJECTS ) {
    if ( sort_mode == SORT_BY_NAME ) {
      for ( i = 0; i < host_list_size;  i++ ) {
        if ( host_idx_to_level[i] >= host_level ) {
          add_seen_item(i);
        }
      }
    }
    if ( sort_mode == SORT_BY_AGE ) {
      age_ptr = hosts_by_age;
      while ( age_ptr != NULL ) {
        if ( age_ptr->level >= host_level ) {
          add_seen_item(age_ptr->index);
        }
        age_ptr = age_ptr->next;
      }
    }
  }

  if ( object_mode == SERVICE_OBJECTS ) {
    if ( sort_mode == SORT_BY_NAME ) { 
      for ( i = 0; i < service_list_size;  i++ ) {
        if ( service_idx_to_level[i] >= service_level ) {
          add_seen_item(i);
        }
      }
    }
    if ( sort_mode == SORT_BY_AGE ) { 
      age_ptr = services_by_age;
      while ( age_ptr != NULL ) {  
        if ( age_ptr->level >= service_level ) {
          add_seen_item(age_ptr->index);
        }
        age_ptr = age_ptr->next;
      }
    }
  }

  /*--------------------------------------------------------*/
  /* display objects */
  tmp = seen_items;
  for ( ; tmp != NULL; tmp = tmp->next ) {

    i = tmp->index;

    if ( object_mode == HOST_OBJECTS ) {
      /*--------------------------------------------------------*/
      /* HOST objects */

      if ( BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) {
        snprintf(buf,sizeof(buf),"%s %s",host_list[i][HOST_NAME],host_list[i][PLUGIN_OUTPUT]);
        if ( filter_doesnt_match(name_filter,buf) ) {
          continue;
        }
      }
      if ( BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) {
        snprintf(buf,sizeof(buf),"%s %s",host_list[i][HOST_NAME],host_list[i][PLUGIN_OUTPUT]);
        if ( filter_matches(not_name_filter,buf) ) { 
          continue;
        }
      }
      if ( BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
        duration_int = last_update_int - (int)host_list[i][LAST_STATE_CHANGE_INT];
        if ( duration_int > age_filter_secs ) {
          continue;
        }
      }
      if ( BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
        duration_int = last_update_int - (int)host_list[i][LAST_STATE_CHANGE_INT];
        if ( duration_int > age_okay_filter_secs && host_idx_to_level[i] == OKAY ) {
          continue;
        }
      }
      items_matched++;
      total_items_matched++;
      if ( ( items_matched > begin_item ) &&
           ( screen_line < LINES-FOOTER_SIZE-pad ) ) {
#if _DEBUG_
        debug("DISPLAY: item %2d line %2d idx %3d: host=%s",
          screen_line-HEADER_SIZE,screen_line,i,host_list[i][HOST_NAME]);
#endif
        snprintf(buf,sizeof(buf),LINE_BEGIN_SPEC,host_list[i][STATUS]);
        if ( color ) { 
          attron(COLOR_PAIR(host_idx_to_level[i]+1)); 
          /* draw the whole line in color pair... */
          mvaddstr(screen_line,0," ");
          for(j=2;j<=COLS;j++) {
            addch(' ');
          }
        }
        mvaddstr(screen_line,pad,buf);
        descr_width = COLS-(pad*2)-LINE_BEGIN_LEN-LINE_END_LEN-2;
        snprintf(buf2,sizeof(buf2),"%%%d.%ds",descr_width,descr_width);
        snprintf(buf,sizeof(buf),buf2,host_list[i][PLUGIN_OUTPUT]);
        snprintf(buf2,sizeof(buf2),"%s ",host_list[i][HOST_NAME]);
        strncpy(buf,buf2,strlen(buf2));
        mvaddstr(screen_line,LINE_BEGIN_LEN+pad+1,buf);
        snprintf(buf,sizeof(buf),LINE_END_SPEC,host_list[i][LAST_STATE_CHANGE],host_list[i][DURATION]);
        mvaddstr(screen_line,COLS-pad-LINE_END_LEN,buf);
        if ( color ) { attron(COLOR_PAIR(DEFAULT_COLOR)); }
        screen_line++;
      }
    }

    if ( object_mode == SERVICE_OBJECTS ) {
      /*--------------------------------------------------------*/
      /* SERVICE objects */

      if ( BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) {
        snprintf(buf,sizeof(buf),"%s %s %s",
          service_list[i][HOST_NAME],service_list[i][SERVICE_NAME],service_list[i][PLUGIN_OUTPUT]);
        if ( filter_doesnt_match(name_filter,buf) ) { 
          continue;
        }
      }
      if ( BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) {
        snprintf(buf,sizeof(buf),"%s %s %s",
          service_list[i][HOST_NAME],service_list[i][SERVICE_NAME],service_list[i][PLUGIN_OUTPUT]);
        if ( filter_matches(not_name_filter,buf) ) { 
          continue;
        }
      }
      if ( BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
        duration_int = last_update_int - (int)service_list[i][LAST_STATE_CHANGE_INT];
        if ( duration_int > age_filter_secs ) {
          continue;
        }
      }
      if ( BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
        duration_int = last_update_int - (int)service_list[i][LAST_STATE_CHANGE_INT];
        if ( duration_int > age_okay_filter_secs && service_idx_to_level[i] == OKAY ) {
          continue;
        }
      }
      items_matched++;
      total_items_matched++;
      if ( ( items_matched > begin_item ) &&
           ( screen_line < LINES-FOOTER_SIZE-pad ) ) { 
#if _DEBUG_
        debug("DISPLAY: item %2d line %2d idx %3d: host=%s svc=%s",
          screen_line-HEADER_SIZE,screen_line,i,service_list[i][HOST_NAME],service_list[i][SERVICE_NAME]);
#endif
        snprintf(buf,sizeof(buf), LINE_BEGIN_SPEC,service_list[i][STATUS]);
        if ( color ) { 
          attron(COLOR_PAIR(service_idx_to_level[i]+1));
          mvaddstr(screen_line,0," ");
          for(j=2;j<=COLS;j++) {
            addch(' ');
          }
        }
        mvaddstr(screen_line,pad,buf);
        descr_width = COLS-(pad*2)-LINE_BEGIN_LEN-LINE_END_LEN-2;
        snprintf(buf2,sizeof(buf2),"%%%d.%ds",descr_width,descr_width);
        snprintf(buf,sizeof(buf),buf2,service_list[i][PLUGIN_OUTPUT]);
        strncpy(buf,service_list[i][SERVICE_NAME],strlen(service_list[i][SERVICE_NAME]));
        mvaddstr(screen_line,LINE_BEGIN_LEN+pad+1,buf);
        snprintf(buf,sizeof(buf),LINE_END_SPEC,service_list[i][LAST_STATE_CHANGE],service_list[i][DURATION]);
        mvaddstr(screen_line,COLS-pad-LINE_END_LEN,buf);
        if ( color ) { attron(COLOR_PAIR(DEFAULT_COLOR)); }
        screen_line++;
      }
    } 
  } 

#if _DEBUG_
  debug("items_matched is %d",items_matched);
  debug("total_items_matched is %d",total_items_matched);
#endif

  /*--------------------------------------------------------*/
  /* update 2nd line: Age */
  num_filters = 0;
  if ( BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
    snprintf(buf,sizeof(buf),"Age: all < %s",age_filter);
    num_filters++; 
  } else if ( BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
    if ( object_mode == HOST_OBJECTS ) {
      snprintf(buf,sizeof(buf),"Age: up < %s",age_okay_filter);
    }
    if ( object_mode == SERVICE_OBJECTS ) {
      snprintf(buf,sizeof(buf),"Age: ok < %s",age_okay_filter);
    }
    num_filters++; 
  } else {
    snprintf(buf,sizeof(buf),"Age: any");
  }
  mvaddstr(1+pad,0+header_pad,buf);

  /*--------------------------------------------------------*/
  /* update 2nd line: filter matches */
  if ( filter_set ) { 
    if ( total_items_matched == 1 ) {
      snprintf(buf,sizeof(buf)," 1 matches the filter");
    } else {
      snprintf(buf,sizeof(buf)," %d match the filter", total_items_matched);
    }
    mvaddstr(1+pad,(COLS/2)-(strlen(buf)/2)-CENTERING_FUDGE,buf);
  }

  if ( sort_mode == SORT_BY_NAME ) {
    snprintf(buf,sizeof(buf),"Sort: by name");
    mvaddstr(1+pad,COLS-header_pad-strlen(buf),buf);
  }
  if ( sort_mode == SORT_BY_AGE ) {
    snprintf(buf,sizeof(buf),"Sort: by age");
    mvaddstr(1+pad,COLS-header_pad-strlen(buf),buf);
  }

  /*--------------------------------------------------------*/
  /* update 3rd line: Filter */
  if ( BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) {
    snprintf(buf,sizeof(buf),"Filter: text =~ /%s/",name_filter);
    num_filters++;
  } else if ( BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) {
    snprintf(buf,sizeof(buf),"Filter: text !~ /%s/",not_name_filter);
    num_filters++;
  } else {
    snprintf(buf,sizeof(buf),"Filter: none");
  }
  mvaddstr(2+pad,0+header_pad,buf);

  /*--------------------------------------------------------*/
  /* update 3rd line: page X of Y */
  num_pages = total_items_matched/page_size;
  if ( (total_items_matched%page_size) != 0 ) { num_pages++; }
  snprintf(buf,sizeof(buf),"page %d of %d", cur_page, num_pages);
  mvaddstr(2+pad,COLS-header_pad-strlen(buf),buf);

#if 0
there is a subtle bug here: if you go to page N and then
increase the height of the window such that page N
is no longer valid, you will (still) be on the orphaned
and blank page N...  i doubt i will ever fix this bug:
doing so would require pre-processing the big display
objects block above
#endif

#if _DEBUG_
  debug("cur_page is %d",cur_page);
  debug("num_pages is %d",num_pages);
  debug("done with draw_screen()");
  debug("");
#endif

  /*--------------------------------------------------------*/
  /* footer... */
  curs_set(0);
  mvaddstr(LINES-1-pad,pad,"Command: ");


  /*--------------------------------------------------------*/
  /* free up seen items... */
  while ( seen_items != NULL ) {
    tmp = seen_items;
    prev = seen_items;
    while ( tmp->next != NULL ) {
     prev = tmp;
     tmp = tmp->next;
    }
    free(tmp);
    if ( prev == tmp ) {
      seen_items = NULL;
    } else {
      prev->next = NULL;
    }
  } 

  /*--------------------------------------------------------*/
  refresh();

}

/*----------------------------------------------------------------------------*/

int filter_doesnt_match(filter, str)
char *filter, *str;
{
  return(perl_regex_hook(str,filter,0));
}

/*----------------------------------------------------------------------------*/

int filter_matches(filter, str)
char *filter, *str;
{
  return(perl_regex_hook(str,filter,1));
}

/*----------------------------------------------------------------------------*/

void
add_seen_item(idx)
int idx;
{
  struct seen_item *si;

  si = (struct seen_item *)malloc(sizeof(struct seen_item));
  si->index = idx;
  si->level = host_idx_to_level[idx];
  si->next = NULL;
  if ( seen_items == NULL ) {
    seen_items = si;
  } else {
    tail->next = si;
  }
  tail = si;
}


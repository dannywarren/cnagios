/* 
 *  $Id: read.c,v 1.73 2013/03/28 21:04:29 rader Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <curses.h>
#include "cnagios.h"

/*------------------------------------------------------------------*/

int host_list_size = 0;
char *host_list[MAX_ITEMS][STATUS_LIST_ENTRY_SIZE];
struct obj_by_age *hosts_by_age;
int host_idx_to_level[MAX_ITEMS];
int service_list_size = 0;
char *service_list[MAX_ITEMS][STATUS_LIST_ENTRY_SIZE];
struct obj_by_age *services_by_age;
int service_idx_to_level[MAX_ITEMS];
int num_up, num_down;
int num_okay, num_warn, num_crit;
char last_update[21];
time_t last_update_int;

extern int object_mode;
extern int sort_mode;
extern int pad;
extern int color;

/*------------------------------------------------------------------*/

void
read_status()
{
  int i, j;
  struct obj_by_age *tmp, *prev, *new;

#if _DEBUG_
debug("read_status()...");
#endif


  /*--------------------------------------------------------*/
  /* free() up old status data... */

  while ( hosts_by_age != NULL ) {
    tmp = hosts_by_age;
    prev = hosts_by_age;
    while ( tmp->next != NULL ) {
     prev = tmp;
     tmp = tmp->next;
    }
    free(tmp);
    if ( prev == tmp ) {
      hosts_by_age = NULL;
    } else {
      prev->next = NULL;
    }
  }
  while ( services_by_age != NULL ) {
    tmp = services_by_age;
    prev = services_by_age;
    while ( tmp->next != NULL ) {
     prev = tmp;
     tmp = tmp->next;
    }
    free(tmp);
    if ( prev == tmp ) {
      services_by_age = NULL;
    } else {
      prev->next = NULL;
    }
  }
  for ( i = 0; i < host_list_size; i++ ) {
    for ( j = 0; j < STATUS_LIST_ENTRY_SIZE; j++ ) {
      if ( j == LAST_UPDATE ) { continue; }
      if ( j == LAST_STATE_CHANGE_INT ) { continue; }
#if _DEBUG_
debug("freeing host: [%d,%d] %s", i, j, host_list[i][j]);
#endif
      free(host_list[i][j]);
      host_list[i][j] = NULL;
    }
  }   
  for ( i = 0; i < service_list_size; i++ ) {
    for ( j = 0; j < STATUS_LIST_ENTRY_SIZE; j++ ) {
      if ( j == LAST_UPDATE ) { continue; } 
      if ( j == LAST_STATE_CHANGE_INT ) { continue; }
#if _DEBUG_
debug("freeing service: [%d,%d] %s", i, j, service_list[i][j]);
#endif
      free(service_list[i][j]);
      service_list[i][j] = NULL;
    }
  }

  /*--------------------------------------------------------*/
  /* parse status file... */
  switch(NAGIOS_VERSION) {
    case 1:
      read_v1_status();
    break;
    case 2:
    case 3:
      read_v23_status();
    break;
    default: 
      endwin();
      fprintf(stderr,"fatal error: unknown Nagios version: %d\n",NAGIOS_VERSION);
    exit(1);
  }

  /*--------------------------------------------------------*/
  /* make linked list of hosts by age... */
  hosts_by_age = NULL;
  for ( i = 0; i < host_list_size;  i++ ) {
    new = (struct obj_by_age *)malloc(sizeof(struct obj_by_age));
    new->index = i;
    new->age = (int)host_list[i][LAST_STATE_CHANGE_INT];
    new->level = convert_level(host_list[i][STATUS]);
    new->next = NULL;
    prev = NULL;
    for ( tmp = hosts_by_age; tmp != NULL; tmp = tmp->next ) {
      if ( (int)host_list[i][LAST_STATE_CHANGE_INT] > tmp->age ) {
        break;
      }
      prev = tmp;
    }
    if ( tmp == NULL ) {
      if ( prev == NULL ) {
        hosts_by_age = new;
        /*debug("adding idx %d: first on list: age is  %d ",i,new->age);*/
      } else {
        prev->next = new;
        /*debug("adding idx %d: tail of list: age is   %d ",i,new->age);*/
      }
    } else {
      if ( prev == NULL ) {
        new->next = hosts_by_age;
        hosts_by_age = new;
        /*debug("adding idx %d: head of list: age is   %d ",i,new->age);*/
      } else {
        new->next = prev->next;
        prev->next = new;
        /*debug("adding idx %d: middle of list: age is %d ",i,new->age);*/
      }
    }
  }
#if _DEBUG_
  {
    struct obj_by_age *t;
    t = hosts_by_age;
    while (t != NULL) {
      debug("SERVICES BY AGE: idx=%d -> age=%d host=%s",
        t->index, t->age,
        host_list[t->index][HOST_NAME]
      );
      t = t->next;
    }
  }
#endif

  /*--------------------------------------------------------*/
  /* make linked list of services by age... */
  services_by_age = NULL;
  for ( i = 0; i < service_list_size;  i++ ) {
    new = (struct obj_by_age *)malloc(sizeof(struct obj_by_age));
    new->index = i;
    new->age = (int)service_list[i][LAST_STATE_CHANGE_INT];
    new->level = convert_level(service_list[i][STATUS]);
    new->next = NULL;
    prev = NULL;
    for ( tmp = services_by_age; tmp != NULL; tmp = tmp->next ) {
      if ( (int)service_list[i][LAST_STATE_CHANGE_INT] > tmp->age ) {
        break;
      }
      prev = tmp;
    }
    if ( tmp == NULL ) {
      if ( prev == NULL ) {
        services_by_age = new;
        /*debug("adding idx %d: first on list: age is  %d ",i,new->age);*/
      } else {
        prev->next = new;
        /*debug("adding idx %d: tail of list: age is   %d ",i,new->age);*/
      }
    } else {
      if ( prev == NULL ) {
        new->next = services_by_age;
        services_by_age = new;
        /*debug("adding idx %d: head of list: age is   %d ",i,new->age);*/
      } else { 
        new->next = prev->next;
        prev->next = new;
        /*debug("adding idx %d: middle of list: age is %d ",i,new->age);*/
      }
    }
  } 
#if _DEBUG_
  { 
    struct obj_by_age *t;
    t = services_by_age;
    while (t != NULL) {
      debug("SERVICES BY AGE: idx=%d -> age=%d host=%s svc=%s",
        t->index, t->age,
        service_list[t->index][HOST_NAME],
        service_list[t->index][SERVICE_NAME]
      );
      t = t->next;
    }
  }
  debug("%d host objects, %d up, %d down",host_list_size,num_up,num_down);
  debug("%d service objects, %d ok, %d warn, %d crit",
    service_list_size,num_okay,num_warn,num_crit);
  debug("done with read_status()");
  debug("");
#endif
  addstr("done");
  if ( color ) { attron(COLOR_PAIR(DEFAULT_COLOR)); }
  refresh();
  usleep(SHORT_MSG_DELAY);

}

/*------------------------------------------------------------------*/

int
convert_level(s)
char *s;
{
  if ( s == NULL )		   { return UNKNOWN; }
  if ( strcmp(s,"UP") == 0 )       { return UP; }
  if ( strcmp(s,"DOWN") == 0 )     { return DOWN; }
  if ( strcmp(s,"OKAY") == 0 )     { return OKAY; }
  if ( strcmp(s,"WARNING") == 0 )  { return WARNING; }
  if ( strcmp(s,"CRITICAL") == 0 ) { return CRITICAL; }
  if ( strcmp(s,"PENDING") == 0 )  { return PENDING; }
  if ( strcmp(s,"UNKNOWN") == 0 )  { return UNKNOWN; }
  return UNKNOWN;
}

/*------------------------------------------------------------------*/

char *
calc_duration(d)
int d;
{
  char buf[MAX_CHARS_PER_LINE];
  char *str_ptr;
  int duration;
  duration = last_update_int - d;

  if ( duration < (60 * 60) ) {
    snprintf(buf,sizeof(buf),"%dm %ds",duration/60, duration%60);
  } else if ( duration < (60 * 60 * 24) ) {
    snprintf(buf,sizeof(buf),"%dh %dm",duration/60/60,
      (duration%(60*60))/60);
  } else {
    snprintf(buf,sizeof(buf),"%dd %dh",duration/60/60/24,
      (duration%(24*60*60))/60/60);
  }
  str_ptr = malloc(strlen(buf)+1);
  strncpy(str_ptr,buf,strlen(buf));
  str_ptr[strlen(buf)] = '\0';
  return str_ptr; 
}


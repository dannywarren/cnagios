/* 
 *  $Id: readv1.c,v 1.14 2011/03/28 15:45:38 rader Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <curses.h>
#include "cnagios.h"

/*------------------------------------------------------------------*/

extern int host_list_size;
extern char *host_list[MAX_ITEMS][STATUS_LIST_ENTRY_SIZE];
extern struct obj_by_age *hosts_by_age;
extern int host_idx_to_level[MAX_ITEMS];
extern int service_list_size;
extern char *service_list[MAX_ITEMS][STATUS_LIST_ENTRY_SIZE];
extern struct obj_by_age *services_by_age;
extern int service_idx_to_level[MAX_ITEMS];
extern int num_up, num_down;
extern int num_okay, num_warn, num_crit;
extern char last_update[21];
extern int last_update_int;

extern int object_mode;
extern int sort_mode;
extern int pad;
extern int color;

/*------------------------------------------------------------------*/

void
read_v1_status()
{
  FILE *fp;
  int i, k;
  int line_num;
  int stamp;
  char buffer[MAX_CHARS_PER_LINE];
  char *str_match; 
  char entry_type[MAX_STATUS_TYPE_LEN];

#if _DEBUG_
debug("read_v1_status()...");
debug("status file is %s",STATUS_FILE);
#endif

  /*--------------------------------------------------------*/
  mvaddstr(LINES-1-pad,pad,"reading status.log...");
  refresh();

  /*--------------------------------------------------------*/
  if ((fp = fopen(STATUS_FILE, "r")) == NULL) {
    endwin();
    fprintf(stderr,"fatal error: fopen %s ",STATUS_FILE);
    perror("failed");
    exit(1);
  }

  /*--------------------------------------------------------*/
  /* ignore "Nagios 1.x Status File" line... */
  fgets(buffer,sizeof(buffer),fp);

  /*--------------------------------------------------------*/
  /* last update from PROGRAM line... */
  fgets(buffer,sizeof(buffer),fp);
  if ( sscanf(buffer,"[%d]",&last_update_int) != 1 ) {
    endwin();
    printf("fatal error: could not parse last update from 2nd line of %s\n",
      STATUS_FILE);
    exit(1);
  }
  {
    // Copy the value in case int is a different size to time_t
    time_t last_update_time = last_update_int;
    strncpy(last_update,ctime(&last_update_time),20);
  }
  last_update[21] = '\0';

  /*--------------------------------------------------------*/
  num_okay = num_warn = num_crit = 0;
  host_list_size = service_list_size = 0;
  num_up = num_down = 0;
  for ( i=0, line_num=1; fgets(buffer,sizeof(buffer),fp) != NULL; i++, line_num++ ) {

    if ( (str_match = (char *)strtok(buffer,";")) == NULL ) {
      endwin();
      fprintf(stderr,"warning: ignoring %s line %d: could not parse entry\r\n",
        STATUS_FILE,line_num);
      refresh();
      continue;
    }

    if ( sscanf(str_match,"[%ld] %s",&stamp,entry_type) != 2 ) {
      endwin();
      fprintf(stderr,"warning: ignoring %s line %d: could not parse entry\r\n",
        STATUS_FILE,line_num);
      refresh();
      continue;
    }
    host_list[i][LAST_UPDATE] = (char *)stamp;

    if (strcmp(entry_type,"HOST") == 0) {
      /*--------------------------------------------------------*/
      /* host objects... */
      for ( k=1; k < NUM_HOST_ENTRY_ATTRS+1; k++ ) {
        if ( (str_match = (char *)strtok(NULL,";")) == NULL ) {
          endwin();
          fprintf(stderr,"warning: ignoring %s line %d: could not parse entry\r\n",
            STATUS_FILE,line_num);
          refresh();
          continue;
        }
        switch(k) {
          case 1:
            host_list[host_list_size][HOST_NAME] = malloc(strlen(str_match)+1);
            strncpy(host_list[host_list_size][HOST_NAME],str_match,strlen(str_match)+1);
          break;
          case 2:
            host_list[host_list_size][STATUS] = malloc(strlen(str_match)+1);
            strncpy(host_list[host_list_size][STATUS],str_match,strlen(str_match)+1);
          break;
          case 4:
            if (strcmp(host_list[host_list_size][STATUS],"PENDING") == 0) {
              host_list[host_list_size][LAST_STATE_CHANGE_INT] = (char *)(last_update_int + 1);
              host_list[host_list_size][LAST_STATE_CHANGE] = malloc(17); /* "DOW Mon DD HH:MM\0" */
              strncpy(host_list[host_list_size][LAST_STATE_CHANGE]," not applicable ",strlen(str_match)+1);
              host_list[host_list_size][DURATION] = (char *)calc_duration(stamp);
            } else {
              sscanf(str_match,"%d",&stamp);
              host_list[host_list_size][LAST_STATE_CHANGE_INT] = (char *)stamp;
              host_list[host_list_size][LAST_STATE_CHANGE] = malloc(17); /* "DOW Mon DD HH:MM\0" */
              {
                // Copy the value in case int is a different size to time_t
                time_t stamp_time = stamp;
                strncpy(host_list[host_list_size][LAST_STATE_CHANGE],ctime(&stamp_time),16);
              }
              host_list[host_list_size][DURATION] = (char *)calc_duration(stamp);
            }
          break;
          case 20:
            host_list[host_list_size][PLUGIN_OUTPUT] = malloc(strlen(str_match)+1);
            strncpy(host_list[host_list_size][PLUGIN_OUTPUT],str_match,strlen(str_match)+1);
            host_list[host_list_size][PLUGIN_OUTPUT][strlen(str_match)-1] = '\0'; /* nix \n */
            perl_hook(HOST_PLUGIN_HOOK,host_list[host_list_size][PLUGIN_OUTPUT]);
          break;
        }
      }
      host_idx_to_level[host_list_size] = convert_level(host_list[host_list_size][STATUS]);
      switch(host_idx_to_level[host_list_size]) {
        case UP: num_up++;     break;
        case DOWN: num_down++; break;
      }
#if _DEBUG_
      debug("READ HOST: idx=%d -> NAME=%s STATUS=%s LAST_CHANGE=%s",
        host_list_size,
        host_list[host_list_size][HOST_NAME],
        host_list[host_list_size][STATUS],
        host_list[host_list_size][LAST_STATE_CHANGE]
      );
#endif
      host_list_size++;
      /* end of host entries */

    } else if (strcmp(entry_type,"SERVICE") == 0) {
      /*--------------------------------------------------------*/
      /* service objects... */
      for ( k=1; k < NUM_SERVICE_ENTRY_ATTRS+1; k++ ) {
        if ( (str_match = (char *)strtok(NULL,";")) == NULL ) {
          endwin();
          fprintf(stderr,"warning: ignoring %s line %d: could not parse entry\r\n",
            STATUS_FILE,line_num);
          refresh();
          continue;
        }
        switch(k) {
          case 1: 
            service_list[service_list_size][HOST_NAME] = malloc(strlen(str_match)+1);
            strncpy(service_list[service_list_size][HOST_NAME],str_match,strlen(str_match)+1);
          break;
          case 2:
            service_list[service_list_size][SERVICE_NAME] = 
              malloc(strlen(str_match)+1+strlen(service_list[service_list_size][HOST_NAME])+3);
            snprintf(service_list[service_list_size][SERVICE_NAME],strlen(str_match)+1+strlen(service_list[service_list_size][HOST_NAME])+3,"%s %s ",
              service_list[service_list_size][HOST_NAME],str_match);
            perl_hook(SERVICE_PLUGIN_HOOK,service_list[service_list_size][SERVICE_NAME]);
          break;
          case 3:
            if (strcmp(str_match,"OK") == 0) { 
              service_list[service_list_size][STATUS] = malloc(5);
              strncpy(service_list[service_list_size][STATUS],"OKAY",5);
            } else {
              service_list[service_list_size][STATUS] = malloc(strlen(str_match)+1);
              strncpy(service_list[service_list_size][STATUS],str_match,strlen(str_match)+1);
            }
          break;
          case 12:
            service_list[service_list_size][LAST_STATE_CHANGE] = malloc(17); 
            if (strcmp(service_list[service_list_size][STATUS],"PENDING") == 0) { 
              service_list[service_list_size][LAST_STATE_CHANGE_INT] = (char *)(last_update_int + 1);
              strncpy(service_list[service_list_size][LAST_STATE_CHANGE]," not applicable ",strlen(str_match)+1);
              service_list[service_list_size][DURATION] = malloc(1);
              service_list[service_list_size][DURATION] = (char *)calc_duration(stamp);
            } else {
              sscanf(str_match,"%d",&stamp);
              service_list[service_list_size][LAST_STATE_CHANGE_INT] = (char *)stamp;
              {
                // Copy the value in case int is a different size to time_t
                time_t stamp_time = stamp;
                strncpy(service_list[service_list_size][LAST_STATE_CHANGE],ctime(&stamp_time),16);
              }
              service_list[service_list_size][DURATION] = (char *)calc_duration(stamp);
            }
          break;
          case 31:
            service_list[service_list_size][PLUGIN_OUTPUT] = malloc(strlen(str_match)+1);
            strncpy(service_list[service_list_size][PLUGIN_OUTPUT],str_match,strlen(str_match)+1);
            service_list[service_list_size][PLUGIN_OUTPUT][strlen(str_match)-1] = '\0'; 
            perl_hook(SERVICE_PLUGIN_HOOK,service_list[service_list_size][PLUGIN_OUTPUT]);
          break;
        }
      }
      service_idx_to_level[service_list_size] = 
         convert_level(service_list[service_list_size][STATUS]);
      switch(service_idx_to_level[service_list_size]) {
        case 0: num_okay++;  break;
        case 1: num_warn++;  break;
        case 2: num_crit++;  break;
      }
#if _DEBUG_
      debug("READ SERVICE: idx=%d -> NAME=%s SERVICE=%s STATUS=%s LAST_CHANGE=%s",
        service_list_size,
        service_list[service_list_size][HOST_NAME],
        service_list[service_list_size][SERVICE_NAME],
        service_list[service_list_size][STATUS],
        service_list[service_list_size][LAST_STATE_CHANGE]
      );
#endif
      service_list_size++;
      /* end of service entries */
    } else {
      /*--------------------------------------------------------*/
      /* there are no known unknown entries */
    }
  }
  fclose(fp);

#if _DEBUG_
  debug("done with read_v1_status()");
#endif


}


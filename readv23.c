/* 
 *  $Id: readv23.c,v 1.34 2013/03/28 21:34:53 rader Exp $
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

struct status_dot_dat_entry {
 char *lhs;
 char *rhs;
};

void parse_entry(struct status_dot_dat_entry *, char *);

int line;

/*------------------------------------------------------------------*/

void
read_v23_status()
{
  FILE *fp;
  char buffer[MAX_CHARS_PER_LINE];
  struct status_dot_dat_entry ent;
  int i, host_stamp, service_stamp, eof;

#ifdef _DEBUG_
debug("read_v23_status()...");
debug("status file is %s",STATUS_DAT_FILE);
#endif

  /*--------------------------------------------------------*/
  mvaddstr(LINES-1-pad,pad,"reading status.dat...");
  refresh();

  /*--------------------------------------------------------*/
  if ((fp = fopen(STATUS_DAT_FILE, "r")) == NULL) {
    endwin();
    fprintf(stderr,"fatal error: fopen %s ",STATUS_DAT_FILE);
    perror("failed");
    exit(1);
  }
  line = 0;

  /*--------------------------------------------------------*/
  /* read last update */

  fgets(buffer,sizeof(buffer),fp); line++;
  while ( !feof(fp) && strcmp(buffer,"info {\n") != 0 ) {
    fgets(buffer,sizeof(buffer),fp); line++;
  }
  if ( strcmp(buffer,"info {\n") != 0 ) {
    endwin();
    printf("fatal error: %s: could not find \"info\" stanza\n", STATUS_DAT_FILE);
    exit(1);
  }
  fgets(buffer,sizeof(buffer),fp); line++;
  parse_entry(&ent,buffer);
  while ( !feof(fp) && strcmp(ent.lhs,"created") != 0 ) {
    fgets(buffer,sizeof(buffer),fp); line++;
    parse_entry(&ent,buffer);
  }
  if ( !strcmp(ent.lhs,"created") ) {
    last_update_int = atoi(ent.rhs);
    {
      // Copy the value in case int is a different size to time_t
      time_t last_update_time = last_update_int;
      strncpy(last_update,ctime(&last_update_time),19);
    }
    last_update[20] = '\0';
  } else {
    endwin();
    printf("fatal error: %s: could not find \"created\" entry\n", STATUS_DAT_FILE);
    exit(1);
  }
#ifdef _DEBUG_
debug("last update is %s (%d)",last_update,last_update_int);
#endif


  /*--------------------------------------------------------*/
  num_okay = num_warn = num_crit = 0;
  host_list_size = service_list_size = 0;
  num_up = num_down = 0;

  while ( fgets(buffer,sizeof(buffer),fp) != NULL ) { 
    line++;
    parse_entry(&ent,buffer);
    if ( !strcmp(ent.lhs,"fail") ) { continue; }

    /*--------------------------------------------------------*/
    /* HOST object... */
    if ( (!strcmp("host",ent.lhs)) || (!strcmp("hoststatus",ent.lhs)) ) {

      /*--------------------*/
      /* HOST_NAME... */
      fgets(buffer,sizeof(buffer),fp); line++;
      parse_entry(&ent,buffer);
      while ( !feof(fp) && strcmp(ent.lhs,"host_name") != 0 ) {
        fgets(buffer,sizeof(buffer),fp); line++;
        parse_entry(&ent,buffer);
        if ( !strcmp(ent.lhs,"fail") ) { 
          endwin();
          printf("fatal error: %s: could not find \"host_name\" for host stanza preceeding line %d\n", 
            STATUS_DAT_FILE, line);
          exit(1);
        }
      }
      if ( strcmp(ent.lhs,"host_name") ) {
        endwin();
        printf("fatal error: %s: could not find \"host_name\" for host stanza preceeding line %d\n", 
          STATUS_DAT_FILE, line);
        exit(1);
      }
      host_list[host_list_size][HOST_NAME] = malloc(strlen(ent.rhs)+1);
      strncpy(host_list[host_list_size][HOST_NAME],ent.rhs,strlen(ent.rhs)+1);
#ifdef _DEBUG_
debug("HOST NUMBER %d...", host_list_size);
debug("  host_name is \"%s\"",host_list[host_list_size][HOST_NAME]);
#endif

      /*--------------------*/
      /* STATUS... */
      fgets(buffer,sizeof(buffer),fp); line++;
      parse_entry(&ent,buffer);
      while ( !feof(fp) && strcmp(ent.lhs,"current_state") != 0 ) {
        fgets(buffer,sizeof(buffer),fp); line++;
        parse_entry(&ent,buffer);
        if ( !strcmp(ent.lhs,"fail") ) { 
          endwin();
          printf("fatal error: %s: could not find \"current_state\" for host stanza preceeding line %d\n", 
            STATUS_DAT_FILE, line);
          exit(1);
        }
      }
      if ( strcmp(ent.lhs,"current_state") ) {
        endwin();
        printf("fatal error: %s: could not find \"current_state\" for host stanza preceeding line %d\n", 
          STATUS_DAT_FILE, line);
        exit(1);
      }
      switch(ent.rhs[0]) {
        case NAGIOS_HOST_UP:
          host_list[host_list_size][STATUS] = malloc(3); 
          strncpy(host_list[host_list_size][STATUS],"UP",3);
        break;
        case NAGIOS_HOST_DOWN:
          host_list[host_list_size][STATUS] = malloc(5);
          strncpy(host_list[host_list_size][STATUS],"DOWN",5);
        break;
      }
#ifdef _DEBUG_
debug("  current_state is %s", host_list[host_list_size][STATUS]);
#endif

      /*--------------------*/
      /* PLUGIN_OUTPUT... */
      fgets(buffer,sizeof(buffer),fp); line++;
      parse_entry(&ent,buffer);
      while ( !feof(fp) && strcmp(ent.lhs,"plugin_output") != 0 ) {
        fgets(buffer,sizeof(buffer),fp); line++;
        parse_entry(&ent,buffer);
        if ( !strcmp(ent.lhs,"fail") ) { 
          endwin();
          printf("fatal error: %s: could not find \"plugin_output\" for host stanza preceeding line %d\n", 
            STATUS_DAT_FILE, line);
          exit(1);
        }
      }
      if ( strcmp(ent.lhs,"plugin_output") ) {
        endwin();
        printf("fatal error: %s: could not find \"plugin_output\" for host stanza preceeding line %d\n", 
          STATUS_DAT_FILE, line);
        exit(1);
      }
      host_list[host_list_size][PLUGIN_OUTPUT] = malloc(strlen(ent.rhs)+1);
      strncpy(host_list[host_list_size][PLUGIN_OUTPUT],ent.rhs,strlen(ent.rhs)+1);
      perl_hook(HOST_PLUGIN_HOOK,host_list[host_list_size][PLUGIN_OUTPUT]);
#ifdef _DEBUG_
debug("  raw plugin_output is \"%s\"",ent.rhs);
debug("  munged plugin_output is \"%s\"",host_list[host_list_size][PLUGIN_OUTPUT]);
#endif

      /*--------------------*/
      /* LAST_STATE_CHANGE... */
      fgets(buffer,sizeof(buffer),fp); line++;
      parse_entry(&ent,buffer);
      while ( !feof(fp) && strcmp(ent.lhs,"last_state_change") != 0 ) {
        fgets(buffer,sizeof(buffer),fp); line++;
        parse_entry(&ent,buffer);
        if ( !strcmp(ent.lhs,"fail") ) { 
          endwin();
          printf("fatal error: %s: could not find \"last_state_change\" for host stanza preceeding line %d\n", 
            STATUS_DAT_FILE, line);
          exit(1);
        }
      }
      if ( strcmp(ent.lhs,"last_state_change") ) {
        endwin();
        printf("fatal error: %s: could not find \"last_state_change\" for host stanza preceeding line %d\n", 
          STATUS_DAT_FILE, line);
        exit(1);
      }
      sscanf(ent.rhs,"%d",&host_stamp);
      if ( (host_stamp == 0) && (host_list[host_list_size][PLUGIN_OUTPUT][0] == '\0') ) {
#ifdef _DEBUG_
debug("  current_state is PENDING, inferred from empty plugin output on Jan 1 1970");
#endif
        /* PENDING... is really UP with NULL plugin output on Jan 1 1970... */
        free(host_list[host_list_size][STATUS]);
        host_list[host_list_size][STATUS] = malloc(8);
        strncpy(host_list[host_list_size][STATUS],"PENDING",8);
        free(host_list[host_list_size][PLUGIN_OUTPUT]);
        host_list[host_list_size][PLUGIN_OUTPUT] = malloc(25);
        strncpy(host_list[host_list_size][PLUGIN_OUTPUT],"(Host assumed to be up)",25);
        perl_hook(HOST_PLUGIN_HOOK,host_list[host_list_size][PLUGIN_OUTPUT]);
        host_stamp = last_update_int;
        host_list[host_list_size][LAST_STATE_CHANGE_INT] = (char *)(last_update_int);
        host_list[host_list_size][LAST_STATE_CHANGE] = malloc(17); 
        strncpy(host_list[host_list_size][LAST_STATE_CHANGE]," not applicable ",16);
      } else { 
        host_list[host_list_size][LAST_STATE_CHANGE_INT] = (char *)host_stamp;
        host_list[host_list_size][LAST_STATE_CHANGE] = malloc(17); /* "DOW Mon DD HH:MM\0" */
        {
          // Copy the value in case int is a different size to time_t
          time_t host_stamp_time = host_stamp;
          strncpy(host_list[host_list_size][LAST_STATE_CHANGE],ctime(&host_stamp_time),16);
        }
      }
      host_list[host_list_size][LAST_STATE_CHANGE][16] = '\0';
#ifdef _DEBUG_
debug("  last_state_change is \"%s\" (%d)",
  host_list[host_list_size][LAST_STATE_CHANGE],
  (int)host_list[host_list_size][LAST_STATE_CHANGE_INT]);
#endif

      /*--------------------*/
      /* DURATION... */
      host_list[host_list_size][DURATION] = (char *)calc_duration(host_stamp);
#ifdef _DEBUG_
debug("  duration is \"%s\"",host_list[host_list_size][DURATION]);
#endif
 
      /*--------------------*/
      /* Other stuff... */
      host_idx_to_level[host_list_size] = convert_level(host_list[host_list_size][STATUS]);
      switch(host_idx_to_level[host_list_size]) {
        case UP: num_up++;     break;
        case DOWN: num_down++; break;
      }
#if 0
      debug("HOST idx=%d: NAME=%s STATUS=%s LAST_CHANGE=%s",
        host_list_size,
        host_list[host_list_size][HOST_NAME],
        host_list[host_list_size][STATUS],
        host_list[host_list_size][LAST_STATE_CHANGE]
      );
#endif
      host_list_size++;
      if ( host_list_size > MAX_ITEMS ) {
        endwin();
        printf("fatal error: too many hosts!\n");  
        printf("increase MAX_ITEMS in cnagios.h\n");
        exit(1);
      }
      continue;

    }

    /*--------------------------------------------------------*/
    /* SERVICE object... */
    if ( (!strcmp("service",ent.lhs)) || (!strcmp("servicestatus",ent.lhs)) ) {

      /*--------------------*/
      /* HOST_NAME... */
      fgets(buffer,sizeof(buffer),fp); line++;
      parse_entry(&ent,buffer);
      while ( !feof(fp) && strcmp(ent.lhs,"host_name") != 0 ) {
        fgets(buffer,sizeof(buffer),fp); line++;
        parse_entry(&ent,buffer);
        if ( !strcmp(ent.lhs,"fail") ) {
          endwin();
          printf("fatal error: %s: could not find \"host_name\" for service stanza preceeding line %d\n",
            STATUS_DAT_FILE, line);
          exit(1);
        }
      }
      if ( strcmp(ent.lhs,"host_name") ) {
        endwin();
        printf("fatal error: %s: could not find \"host_name\" for service stanza preceeding line %d\n",
          STATUS_DAT_FILE, line);
        exit(1);
      }
      service_list[service_list_size][HOST_NAME] = malloc(strlen(ent.rhs)+1);
      strncpy(service_list[service_list_size][HOST_NAME],ent.rhs,strlen(ent.rhs)+1);
#ifdef _DEBUG_
debug("SERVICE NUMBER %d...",service_list_size);
debug("  host_name is \"%s\"",service_list[service_list_size][HOST_NAME]);
#endif

      /*--------------------*/
      /* SERVICE_NAME... */
      fgets(buffer,sizeof(buffer),fp); line++;
      parse_entry(&ent,buffer);
      while ( !feof(fp) && strcmp(ent.lhs,"service_description") != 0 ) {
        fgets(buffer,sizeof(buffer),fp); line++;
        parse_entry(&ent,buffer);
        if ( !strcmp(ent.lhs,"fail") ) {
          endwin();
          printf("fatal error: %s: could not find \"service_description\" for service stanza preceeding line %d\n",
            STATUS_DAT_FILE, line);
          exit(1);
        }
      }
      if ( strcmp(ent.lhs,"service_description") ) {
        endwin();
        printf("fatal error: %s: could not find \"service_description\" for service stanza preceeding line %d\n",
          STATUS_DAT_FILE, line);
        exit(1);
      }
      service_list[service_list_size][SERVICE_NAME] =
        malloc(strlen(ent.rhs)+1+strlen(service_list[service_list_size][HOST_NAME])+1);
      snprintf(service_list[service_list_size][SERVICE_NAME],(strlen(ent.rhs)+1+strlen(service_list[service_list_size][HOST_NAME])+1),"%s %s ",
        service_list[service_list_size][HOST_NAME],ent.rhs);
      perl_hook(SERVICE_PLUGIN_HOOK,service_list[service_list_size][SERVICE_NAME]);
#ifdef _DEBUG_
debug("  raw service_description is \"%s\"",ent.rhs);
debug("  munged service_description is \"%s\"",service_list[service_list_size][SERVICE_NAME]);
#endif

      /*--------------------*/
      /* STATUS... */
      fgets(buffer,sizeof(buffer),fp); line++;
      parse_entry(&ent,buffer);
      while ( !feof(fp) && strcmp(ent.lhs,"current_state") != 0 ) {
        fgets(buffer,sizeof(buffer),fp); line++;
        parse_entry(&ent,buffer);
        if ( !strcmp(ent.lhs,"fail") ) {
          endwin();
          printf("fatal error: %s: could not find \"current_state\" for service stanza preceeding line %d\n",
            STATUS_DAT_FILE, line);
          exit(1);
        }
      }
      if ( strcmp(ent.lhs,"current_state") ) {
        endwin();
        printf("fatal error: %s: could not find \"current_state\" for service stanza preceeding line %d\n",
          STATUS_DAT_FILE, line);
        exit(1);
      }
      switch(ent.rhs[0]) {
        case NAGIOS_STATE_OK:
          service_list[service_list_size][STATUS] = malloc(5);
          strncpy(service_list[service_list_size][STATUS],"OKAY",5);
        break;
        case NAGIOS_STATE_WARNING:
          service_list[service_list_size][STATUS] = malloc(8);
          strncpy(service_list[service_list_size][STATUS],"WARNING",8);
        break;
        case NAGIOS_STATE_CRITICAL:
          service_list[service_list_size][STATUS] = malloc(9);
          strncpy(service_list[service_list_size][STATUS],"CRITICAL",9);
        break;
        case NAGIOS_STATE_UNKNOWN:
          service_list[service_list_size][STATUS] = malloc(8);
          strncpy(service_list[service_list_size][STATUS],"UNKNOWN",8);
        break;
      }
#ifdef _DEBUG_
debug("  current_state is %s",service_list[service_list_size][STATUS]);
#endif

      /*--------------------*/
      /* LAST_STATE_CHANGE... */
      fgets(buffer,sizeof(buffer),fp); line++;
      parse_entry(&ent,buffer);
      while ( !feof(fp) && strcmp(ent.lhs,"last_state_change") != 0 ) {
        fgets(buffer,sizeof(buffer),fp); line++;
        parse_entry(&ent,buffer);
        if ( !strcmp(ent.lhs,"fail") ) {
          endwin();
          printf("fatal error: %s: could not find \"last_state_change\" for service stanza preceeding line %d\n",
            STATUS_DAT_FILE, line);
          exit(1);
        }
      }
      if ( strcmp(ent.lhs,"last_state_change") ) {
        endwin();
        printf("fatal error: %s: could not find \"last_state_change\" for service stanza preceeding line %d\n",
          STATUS_DAT_FILE, line);
        exit(1);
      }

      sscanf(ent.rhs,"%d",&service_stamp);
      service_list[service_list_size][LAST_STATE_CHANGE_INT] = (char *)service_stamp;
      service_list[service_list_size][LAST_STATE_CHANGE] = malloc(17); /* "DOW Mon DD HH:MM\0" */
      {
        // Copy the value in case int is a different size to time_t
        time_t service_stamp_time = service_stamp;
        strncpy(service_list[service_list_size][LAST_STATE_CHANGE],ctime(&service_stamp_time),16);
      }
      service_list[service_list_size][LAST_STATE_CHANGE][16] = '\0';
#ifdef _DEBUG_
debug("  last_state_change is \"%s\" (%d)",
  service_list[service_list_size][LAST_STATE_CHANGE],
  (int)service_list[service_list_size][LAST_STATE_CHANGE_INT]);
#endif

      /*--------------------*/
      /* DURATION... */
      service_list[service_list_size][DURATION] = (char *)calc_duration(service_stamp);
#ifdef _DEBUG_
debug("  duration is \"%s\"",service_list[service_list_size][DURATION]);
#endif

      /*--------------------*/
      /* PLUGIN_OUTPUT... */
      fgets(buffer,sizeof(buffer),fp); line++;
      parse_entry(&ent,buffer);
      while ( !feof(fp) && strcmp(ent.lhs,"plugin_output") != 0 ) {
        fgets(buffer,sizeof(buffer),fp); line++;
        parse_entry(&ent,buffer);
        if ( !strcmp(ent.lhs,"fail") ) {
          endwin();
          printf("fatal error: %s: could not find \"plugin_output\" for service stanza preceeding line %d\n",
            STATUS_DAT_FILE, line);
          exit(1);
        }
      }
      if ( strcmp(ent.lhs,"plugin_output") ) {
        endwin();
        printf("fatal error: %s: could not find \"plugin_output\" for service stanza preceeding line %d\n",
          STATUS_DAT_FILE, line);
        exit(1);
      }
      service_list[service_list_size][PLUGIN_OUTPUT] = malloc(strlen(ent.rhs)+1);
      strncpy(service_list[service_list_size][PLUGIN_OUTPUT],ent.rhs,strlen(ent.rhs)+1);
      perl_hook(SERVICE_PLUGIN_HOOK,service_list[service_list_size][PLUGIN_OUTPUT]);
#ifdef _DEBUG_
debug("  raw plugin_output is \"%s\"",ent.rhs);
debug("  munged plugin_output is \"%s\"",service_list[service_list_size][PLUGIN_OUTPUT]);
#endif
      /* PENDING is really OKAY w/ "(Service assumed to be ok)" on Jan 1 1970... */
      if ( (service_stamp == 0) && (!strcmp("(Service assumed to be ok)",ent.rhs)) ) {
#ifdef _DEBUG_
debug("  current_state is PENDING, inferred from \"(Service assumed to be ok)\" on Jan 1 1970");
#endif
        free(service_list[service_list_size][STATUS]);
        service_list[service_list_size][STATUS] = malloc(8);
        strncpy(service_list[service_list_size][STATUS],"PENDING",8);
        service_stamp = last_update_int;
        service_list[service_list_size][LAST_STATE_CHANGE_INT] = (char *)service_stamp;
        service_list[service_list_size][LAST_STATE_CHANGE] = malloc(17); /* "DOW Mon DD HH:MM\0" */
        strncpy(service_list[service_list_size][LAST_STATE_CHANGE]," not applicable ",17);
        service_list[service_list_size][DURATION] = (char *)calc_duration(service_stamp);
      }

      /*--------------------*/
      /* Other stuff... */
      service_idx_to_level[service_list_size] = 
         convert_level(service_list[service_list_size][STATUS]);
      switch(service_idx_to_level[service_list_size]) {
        case 0: num_okay++;  break;
        case 1: num_warn++;  break;
        case 2: num_crit++;  break;
      }
#if 0
      debug("SERVICE idx=%d: NAME=%s SERVICE=%s STATUS=%s LAST_CHANGE=%s",
        service_list_size,
        service_list[service_list_size][HOST_NAME],
        service_list[service_list_size][SERVICE_NAME],
        service_list[service_list_size][STATUS],
        service_list[service_list_size][LAST_STATE_CHANGE]
      );
#endif
      service_list_size++;
      if ( service_list_size > MAX_ITEMS ) { 
        endwin();
        printf("fatal error: too many services!\n");
        printf("increase MAX_ITEMS in cnagios.h\n");
        exit(1);
      }
      continue;

    }

  }

  /*--------------------------------------------------------*/

  fclose(fp);

#if _DEBUG_
  debug("done with read_v23_status()");
#endif

}

/*------------------------------------------------------------------------------*/

void
parse_entry(ent,str)
struct status_dot_dat_entry *ent;
char *str;
{
  char *p;
  p = str + strlen(str) - 1;
  *p = '\0';
  while ( *str == ' ' || *str == '\t' ) { str++; }
  p = str;
  while ( *p != ' ' && *p != '=' && *p != '\0' ) { p++; }; 
  if ( *p == '\0' ) {
    strncpy(ent->lhs,"fail",5);
    strncpy(ent->rhs,"fail",5);
    return;
  }
  *p = '\0';
  p++;
  ent->lhs = str;
  ent->rhs = p;
  return;
}


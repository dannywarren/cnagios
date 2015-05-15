/*
 *
 * $Id: dotfile.c,v 1.24 2011/03/08 12:37:41 rader Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnagios.h"

char fkey_name_filters[13][STRING_LENGTH];
char fkey_not_name_filters[13][STRING_LENGTH];
char site_name[STRING_LENGTH];

extern int filter_set;
extern char name_filter[STRING_LENGTH];
extern char not_name_filter[STRING_LENGTH];
extern char age_filter[STRING_LENGTH];
extern char age_okay_filter[STRING_LENGTH];
extern int age_filter_secs;
extern int age_okay_filter_secs;
extern int sort_mode;
extern int object_mode;
extern int service_level;
extern int host_level;

/* Using -d for debug here won't work because this code */
/* happens before curses and stderr printed before      */
/* curses get's lost.  Compile with -D_DEBUG_DOTFILE_   */
/* instead.                                             */
#if 0
#define _DEBUG_DOTFILE_
#endif 

void read_dot_file(char *);

/*------------------------------------------------------------------*/

void
read_dot_files()
{
  int i;
  char *home;
  char f[MAX_CHARS_PER_LINE];

  for(i=0;i<13;i++) {
    fkey_name_filters[i][0] = '\0';
    fkey_not_name_filters[i][0] = '\0';
  }
  read_dot_file(CONFIG_FILE);
  home = getenv("HOME");
  snprintf(f,sizeof(f),"%s/%s",home,DOT_CONFIG_FILE);
  read_dot_file(f);

}

/*------------------------------------------------------------------*/

void
read_dot_file(filename)
char *filename;
{
  char *tok1, *tok2, *tok3, *tok4;
  FILE *fp;
  char buf[MAX_CHARS_PER_LINE];
  int l, fkey_num;

  if ((fp = fopen(filename, "r")) == NULL) {
#ifdef _DEBUG_DOTFILE_
    printf("%s: file not found\n",filename);
#endif
    return;
  }
#ifdef _DEBUG_DOTFILE_
    printf("%s...\n",filename);
#endif

  for ( l = 1; fgets(buf,sizeof(buf),fp) != NULL;  l++ ) {

    /*--------------------*/
    /* parse */
    buf[strlen(buf)-1] = '\0';
    if ( buf[0] == '\0' || buf[0] == '#' ) { 
      continue; 
    }
    if ( (tok1 = (char *)strtok(buf," \t")) == NULL ) {
      printf("%s: syntax error on line %d\n",filename,l);
      continue;
    }
    if ( tok1[0] == '#' ) { continue; }
    if ( (tok2 = (char *)strtok(NULL," \t")) == NULL ) {
      printf("%s: syntax error on line %d\n",filename,l);
      continue;
    }
    if ( (tok3 = (char *)strtok(NULL," \t")) == NULL ) {
      printf("%s: syntax error on line %d\n",filename,l);
      continue;
    }
    if ( (tok4 = (char *)strtok(NULL," \t")) == NULL ) {
      printf("%s: syntax error on line %d\n",filename,l);
      continue;
    }

    /*--------------------*/
    /* default text */
    if ( (strcmp(tok1,"default") == 0) && (strcmp(tok2,"text") == 0) ) {
      if (strcmp(tok3,"=~") == 0) {
        /* tok4 should be /regex/ */
        if ( tok4[0] != '/' || tok4[strlen(tok4)-1] != '/' ) {
          printf("%s: syntax error on line %d\n",filename,l);
          continue;
        }
#ifdef _DEBUG_DOTFILE_
        printf("set default =~ %s\n",tok4);
#endif
        /* remove slashes */
        tok4++;
        tok4[strlen(tok4)-1] = '\0';
        strncpy(name_filter,tok4,sizeof(name_filter)-1);
        name_filter[sizeof(name_filter)-1] = '\0';
        filter_set += FILTER_BY_NAME;
        continue;
      }
      if (strcmp(tok3,"!~") == 0) {
        /* tok4 should be /regex/ */
        if ( tok4[0] != '/' || tok4[strlen(tok4)-1] != '/' ) {
          printf("%s: syntax error on line %d\n",filename,l);
          continue;
        }
#ifdef _DEBUG_DOTFILE_
        printf("set default !~ %s\n",tok4);
#endif
        /* remove slashes */
        tok4++;
        tok4[strlen(tok4)-1] = '\0';
        strncpy(not_name_filter,tok4,sizeof(not_name_filter)-1);
        not_name_filter[sizeof(not_name_filter)-1] = '\0';
        filter_set += FILTER_BY_NOT_NAME;
        continue;
      }
      printf("%s: syntax error on line %d\n",filename,l);
      continue;
    }

    /*--------------------*/
    /* fkey text */
    if ( sscanf(tok1,"f%d",&fkey_num) == 1 ) {
      if ( fkey_num > 12 ) { 
        printf("%s: syntax error on line %d\n",filename,l);
        continue;
      }
      if ( (strcmp(tok2,"text") == 0) && (strcmp(tok3,"=~") == 0) ) {
        /* tok4 should be /regex/ */
        if ( tok4[0] != '/' || tok4[strlen(tok4)-1] != '/' ) {
          printf("%s: syntax error on line %d\n",filename,l);
          continue;
        }
        /* remove slashes */
        tok4++;
        tok4[strlen(tok4)-1] = '\0';
        strncpy(fkey_name_filters[fkey_num],tok4,sizeof(fkey_name_filters[fkey_num])-1);
        fkey_name_filters[fkey_num][sizeof(fkey_name_filters[fkey_num])-1] = '\0';
#ifdef _DEBUG_DOTFILE_
        printf("set f%d =~ /%s/\n",fkey_num,fkey_name_filters[fkey_num]);
#endif
        continue;
      }
      if ( (strcmp(tok2,"text") == 0) && (strcmp(tok3,"!~") == 0) ) {
        /* remove slashes */
        tok4++;
        tok4[strlen(tok4)-1] = '\0';
        strncpy(fkey_not_name_filters[fkey_num],tok4,sizeof(fkey_not_name_filters[fkey_num])-1);
        fkey_not_name_filters[fkey_num][sizeof(fkey_not_name_filters[fkey_num])-1] = '\0';
#ifdef _DEBUG_DOTFILE_
        printf("set f%d !~ /%s/\n",fkey_num,fkey_not_name_filters[fkey_num]);
#endif
        continue;
      }
    }

    /*--------------------*/
    /* default age */
    if ( strcmp(tok1,"default") == 0 && strcmp(tok2,"age") == 0 && strcmp(tok3,"=") == 0) {
      if ( age_filter_secs = parse_age_filter(tok4,strlen(tok4)) ) {
        strncpy(age_filter,tok4,sizeof(age_filter)-1);
        age_filter[sizeof(age_filter)-1] = '\0';
#ifdef _DEBUG_DOTFILE_
        printf("set age = %s\n",age_filter);
#endif
        if ( ! BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
          filter_set += FILTER_BY_AGE;
        }
        if ( BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
          filter_set -= FILTER_BY_AGE_OKAY;
          age_okay_filter[0] = '\0';
        }
      } else {
        printf("%s: syntax error on line %d\n",filename,l);
      }
      continue;
    }

    /*--------------------*/
    /* default okay age */
    if ( strcmp(tok1,"default") == 0 && strcmp(tok2,"okay_age") == 0 && strcmp(tok3,"=") == 0) {
      if ( age_okay_filter_secs = parse_age_filter(tok4,strlen(tok4)) ) {
        strncpy(age_okay_filter,tok4,sizeof(age_okay_filter)-1);
        age_okay_filter[sizeof(age_okay_filter)-1] = '\0';
#ifdef _DEBUG_DOTFILE_
        printf("set okay_age = %s\n",age_okay_filter);
#endif
        if ( ! BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
          filter_set += FILTER_BY_AGE_OKAY;
        }
        if ( BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
          filter_set -= FILTER_BY_AGE;
          age_filter[0] = '\0';
        }
      } else {
        printf("%s: syntax error on line %d\n",filename,l);
      }
      continue;
    }

    /*--------------------*/
    /* default sort order */
    if ( strcmp(tok1,"default") == 0 && strcmp(tok2,"sort_order") == 0 && strcmp(tok3,"=") == 0) {
      if ( strcmp(tok4,"by_name") == 0 ) {
        sort_mode = SORT_BY_NAME;
#ifdef _DEBUG_DOTFILE_
        printf("set sort_order = by_name\n");
#endif
        continue;
      }
      if ( strcmp(tok4,"by_age") == 0 ) {
        sort_mode = SORT_BY_AGE;
#ifdef _DEBUG_DOTFILE_
        printf("set sort_order = by_age\n");
#endif
        continue;
      }
      printf("%s: syntax error on line %d\n",filename,l);
      continue;
    }

    /*--------------------*/
    /* OKAY */
    if ( strcmp(tok1,"default") == 0 && strcmp(tok2,"level") == 0 && 
         strcmp(tok3,"=") == 0 && strcmp(tok4,"OKAY") == 0 ) {
#ifdef _DEBUG_DOTFILE_
      printf("set level = OKAY\n");
#endif
      object_mode = SERVICE_OBJECTS;
      service_level = OKAY;
      continue;
    }

    /*--------------------*/
    /* WARNING */
    if ( strcmp(tok1,"default") == 0 && strcmp(tok2,"level") == 0 &&
         strcmp(tok3,"=") == 0 && strcmp(tok4,"WARNING") == 0 ) {
#ifdef _DEBUG_DOTFILE_
      printf("set level = WARNING\n");
#endif
      object_mode = SERVICE_OBJECTS;
      service_level = WARNING;
      continue;
    }

    /*--------------------*/
    /* CRITICAL */
    if ( strcmp(tok1,"default") == 0 && strcmp(tok2,"level") == 0 &&
         strcmp(tok3,"=") == 0 && strcmp(tok4,"CRITICAL") == 0 ) {
#ifdef _DEBUG_DOTFILE_
      printf("set level = CRITICAL\n");
#endif
      object_mode = SERVICE_OBJECTS;
      service_level = CRITICAL;
      continue;
    }

    /*--------------------*/
    /* UP */
    if ( strcmp(tok1,"default") == 0 && strcmp(tok2,"level") == 0 &&
         strcmp(tok3,"=") == 0 && strcmp(tok4,"UP") == 0 ) {
#ifdef _DEBUG_DOTFILE_
      printf("set level = UP\n");
#endif
      object_mode = HOST_OBJECTS;
      host_level = UP;
      continue;
    }

    /*--------------------*/
    /* DOWN */
    if ( strcmp(tok1,"default") == 0 && strcmp(tok2,"level") == 0 &&
         strcmp(tok3,"=") == 0 && strcmp(tok4,"DOWN") == 0 ) {
#ifdef _DEBUG_DOTFILE_
      printf("set level = DOWN\n");
#endif
      object_mode = HOST_OBJECTS;
      host_level = DOWN;
      continue;
    }

    /*--------------------*/
    /* site_name */
    if ( strcmp(tok1,"default") == 0 && strcmp(tok2,"site_name") == 0 && 
         strcmp(tok3,"=") == 0 ) {
      strncpy(site_name,tok4,sizeof(site_name)-1);
#ifdef _DEBUG_DOTFILE_
      printf("set site_name = %s\n",site_name);
#endif 
      continue;
    }


    /*--------------*/
    /* syntax error */
    printf("%s: syntax error on line %d\n",filename,l);

  }

  fclose(fp);

}


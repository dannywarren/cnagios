/*
 *
 * $Id: cnagios.c,v 1.62 2011/03/08 12:37:41 rader Exp $
 *
 */

/*------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include <sys/param.h>
#include <getopt.h>
#include <unistd.h>
#include "cnagios.h"
#include "version.h"

/*------------------------------------------------------------------*/

int object_mode = SERVICE_OBJECTS;
int host_level = UP;
int service_level = OKAY;
int sort_mode = SORT_BY_NAME;
int polling_interval = POLLING_INTERVAL;
int header_pad = HEADER_PAD;
int pad = PAD;
int color = 1;
int debugging = 0;
void update_display();

extern int filter_set;
extern char name_filter[STRING_LENGTH];
extern char not_name_filter[STRING_LENGTH];
extern char age_filter[STRING_LENGTH];
extern char age_okay_filter[STRING_LENGTH];
extern int age_filter_secs;
extern int age_okay_filter_secs;
extern int need_swipe;

void usage();

/*------------------------------------------------------------------*/

int
main(argc,argv)
int argc;
char *argv[];
{
  int flag;

  /*----------------------------------------*/
  /* read etc/cnagiosrc and ~/.cnagiosrc */

  read_dot_files();

  /*----------------------------------------*/
  /* do args */

  while ((flag = getopt(argc, argv, "A:a:bdg:hi:l:m:P:p:s:Vv:")) != EOF )
  switch (flag) {
    case 'V':
      printf("Cnagios-%s\n",VERSION);
      exit(0);
    case 'd':
      debugging++;
      break;
    case 'b':
      color = 0;
      break;
    case 'h':
      usage();
      exit(0);
    case 'i':
      polling_interval = atoi(optarg);
      break;
    case 'm':
      if ( strcmp(optarg,"h") == 0 ) {
        object_mode = HOST_OBJECTS;
        host_level = UP;
        break;
      }
      if ( strcmp(optarg,"s") == 0 ) {
        object_mode = SERVICE_OBJECTS;
        service_level = OKAY;
        break;
      }
      printf("unknown mode: \"%s\"\n",optarg); 
      usage();
      exit(1);
    case 'l':
      if ( strcmp(optarg,"u") == 0 ) {
        host_level = UP;
        object_mode = HOST_OBJECTS;
        break;
      }
      if ( strcmp(optarg,"d") == 0 ) {
        host_level = DOWN;
        object_mode = HOST_OBJECTS;
        break;
      }
      if ( strcmp(optarg,"o") == 0 ) {
        service_level = OKAY;
        object_mode = SERVICE_OBJECTS;
        break;
      }
      if ( strcmp(optarg,"w") == 0 ) {
        service_level = WARNING;
        object_mode = SERVICE_OBJECTS;
        break;
      }
      if ( strcmp(optarg,"c") == 0 ) {
        service_level = CRITICAL;
        object_mode = SERVICE_OBJECTS;
        break;
      }
      printf("unknown level: \"%s\"\n",optarg); 
      usage();
      exit(1);
    case 's':
      if ( strcmp(optarg,"n") == 0 ) {
        sort_mode = SORT_BY_NAME;
        break;
      }
      if ( strcmp(optarg,"a") == 0 ) {
        sort_mode = SORT_BY_AGE;
        break;
      }
      printf("unknown sort type: \"%s\"\n",optarg); 
      usage();
      exit(1);
    case 'g':
      /* /regexp/ ? */
      if ( optarg[0] != '/' || optarg[strlen(optarg)-1] != '/' ) { 
        printf("syntax error: %s: not a perl regexp (use slashes!)\n",optarg);
        exit(1);
      }
      /* remove slashes */
      optarg++;
      optarg[strlen(optarg)-1] = '\0';
      strncpy(name_filter,optarg,sizeof(name_filter)-1);
      name_filter[sizeof(name_filter)-1] = '\0';
      if ( ! BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) {
        filter_set += FILTER_BY_NAME;
      }
      if ( BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) {
        filter_set -= FILTER_BY_NOT_NAME;
        not_name_filter[0] = '\0';
      }
      break;
    case 'v':
      /* /regexp/ ? */
      if ( optarg[0] != '/' || optarg[strlen(optarg)-1] != '/' ) {
        printf("syntax error: %s: not a perl regexp (use slashes!)\n",optarg);
        exit(1);
      }
      /* remove slashes */
      optarg++;
      optarg[strlen(optarg)-1] = '\0';
      strncpy(not_name_filter,optarg,sizeof(not_name_filter)-1);
      not_name_filter[sizeof(not_name_filter)-1] = '\0';
      if ( ! BIT_SET(FILTER_BY_NOT_NAME_BIT,filter_set) ) {
        filter_set += FILTER_BY_NOT_NAME;
      }
      if ( BIT_SET(FILTER_BY_NAME_BIT,filter_set) ) {
        filter_set -= FILTER_BY_NAME;
        name_filter[0] = '\0';
      }
      break;
    case 'a':
      strncpy(age_filter,optarg,sizeof(age_filter)-1);
      age_filter[sizeof(age_filter)-1] = '\0';
      if ( age_filter_secs = parse_age_filter(age_filter,sizeof(age_filter)) ) {
        if ( ! BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
          filter_set += FILTER_BY_AGE;
        }
        if ( BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
          filter_set -= FILTER_BY_AGE_OKAY;
        }
      } else {
        printf("syntax error: can not parse age \"%s\"\n",age_filter);
        usage();
        exit(1);
      }
      break;
    case 'A':
      strncpy(age_okay_filter,optarg,sizeof(age_okay_filter)-1);
      age_okay_filter[sizeof(age_okay_filter)-1] = '\0';
      if ( age_okay_filter_secs = parse_age_filter(age_okay_filter,sizeof(age_filter)) ) {
        if ( ! BIT_SET(FILTER_BY_AGE_OKAY_BIT,filter_set) ) {
          filter_set += FILTER_BY_AGE_OKAY;
        }
        if ( BIT_SET(FILTER_BY_AGE_BIT,filter_set) ) {
          filter_set -= FILTER_BY_AGE;
        }
      } else {
        printf("syntax error: can not parse ok_age \"%s\"\n",age_okay_filter);
        usage();
        exit(1);
      }
      break;
    case 'p':
      pad = atoi(optarg);
      break;
    case 'P':
      header_pad = atoi(optarg);
      break;
    default:
      usage();
      exit(1);
  }
  if ( optind < argc ) {
    usage();
    exit(1);
  }

  /*----------------------------------------*/
  /* setup perl */

  perl_hook_init();

  /*----------------------------------------*/
  /* do curses... */

  initscr();
  keypad(stdscr,TRUE); /* for function keys etc */
  cbreak();
  noecho();
  clear();
  refresh();
  if ( color ) {
    start_color();
    init_pair(GREEN_ON_BLACK,COLOR_GREEN,COLOR_BLACK);
    init_pair(YELLOW_ON_BLACK,COLOR_YELLOW,COLOR_BLACK);
    init_pair(RED_ON_BLACK,COLOR_RED,COLOR_BLACK);
    init_pair(CYAN_ON_BLACK,COLOR_CYAN,COLOR_BLACK);
  }

#if _DEBUG_
debug("Cnagios version %s",VERSION);
debug("polling interval is %d seconds",polling_interval);
debug("pad is %d, header_pad is %d",pad,header_pad);
debug("");
#endif

  /*----------------------------------------*/
  /* do it... */

  signal(SIGALRM,update_display);
  read_status();
  draw_screen();
  alarm(polling_interval);
  getch_loop();
  endwin();

  /*----------------------------------------*/
  /* destroy perl... */

  perl_hook_free();
  return(0);

}

/*------------------------------------------------------------------*/

void usage()
{
   printf("usage: cnagios [options]\n");
   printf("  -A <age>         filter UP/OKAY objects by age less than <age>\n");
   printf("  -a <age>         filter objects by age less than <age>\n");
   printf("  -b               display in black and white\n");
   printf("  -d               print debug info to stderr\n");
   printf("  -g /perl_regex/  filter host/service/plugin-output with /perl_regex/\n");
   printf("  -h               print (this) usage info\n");
   printf("  -i <integer>     set polling interval to <integer> seconds (default is %d)\n", polling_interval);
   printf("  -l <u|d|o|w|c>   display objects with UP/DOWN/OK/WARNING/CRITICAL status\n");
   printf("  -m <h|s>         display HOST objects or SERVICE objects\n");
   printf("  -P <integer>     set header padding to <integer> (default is %d)\n",header_pad);
   printf("  -p <integer>     set row/column padding to <integer> (default is %d)\n",pad);
   printf("  -s <n|a>         sort objects by-name or by-age\n");
   printf("  -V               print version and exit\n");
   printf("  -v /perl_regex/  negative filter host/service/plugin-output with /perl_regex/\n");
}


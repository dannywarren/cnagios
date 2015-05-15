/*
 *
 * $Id: perl.c,v 1.15 2011/03/08 12:37:41 rader Exp $
 *
 */

/*------------------------------------------------------------------*/

#include <EXTERN.h>
#include <perl.h>
#include "cnagios.h"

static PerlInterpreter *my_perl;
char sub_file[MAX_CHARS_PER_LINE];
int sub_file_line;

/*------------------------------------------------------------------*/

void
perl_hook_init() {
  char *embedded_perl_file[] = {"",PERL_HOOK_FILE};
  char junk[MAX_CHARS_PER_LINE];
  int exit_status;
  FILE *fp;

  if ((fp = fopen(PERL_HOOK_FILE, "r")) == NULL) {
    fprintf(stderr,"fatal error: fopen %s ",PERL_HOOK_FILE);
    perror("failed");
    exit(1);
  }
  fclose(fp);
  my_perl = perl_alloc();
  perl_construct(my_perl);
  perl_parse(my_perl,NULL,2,embedded_perl_file,NULL);
  exit_status = perl_run(my_perl);
  if ( exit_status > 0 ) { 
    printf("fatal error: perl compile of %s failed\r\n",PERL_HOOK_FILE);
    exit(1);
  }
  strncpy(junk,"testing perl hook eval",sizeof(junk)-1);
  junk[sizeof(junk)-1] = '\0';
  perl_hook(HOST_PLUGIN_HOOK,junk);
}

/*------------------------------------------------------------------*/

void
perl_hook_free() {
  PL_perl_destruct_level = 0;
  perl_destruct(my_perl);
  perl_free(my_perl);
}

/*------------------------------------------------------------------*/

void
perl_hook(type,str)
int type;
char *str;
{
  STRLEN n_a, n_err;
  SV *rtn_sv;

  /* man perlapi re fiddling with the stack reads */
  /* "take a deep breath..."                     */
  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);
  XPUSHs(sv_2mortal(newSVpv(str,0)));
  PUTBACK;
  switch(type) {
    case HOST_PLUGIN_HOOK:    call_pv("host_plugin_hook",G_SCALAR);    break;
    case SERVICE_PLUGIN_HOOK: call_pv("service_plugin_hook",G_SCALAR); break;
  }
  SPAGAIN;
  rtn_sv = POPs;
  strncpy(str,SvPV(rtn_sv,n_a),strlen(str));
  PUTBACK;
  FREETMPS;
  LEAVE;
  if(SvTRUE(ERRSV)) {
    printf("fatal error: perl eval error: %s\r\n",SvPV(ERRSV,n_err)); 
    exit(1);
  }
}

/*------------------------------------------------------------------*/

int
perl_regex_hook(str,regex,mode)
char *str;
char *regex;
int mode;
{
  STRLEN n_err;
  SV *rtn_sv;
  int rtn = 0;

  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(SP);
  XPUSHs(sv_2mortal(newSVpv(str,0)));
  PUTBACK;
  XPUSHs(sv_2mortal(newSVpv(regex,0)));
  PUTBACK;
  XPUSHs(sv_2mortal(newSViv(mode)));
  PUTBACK;
  XPUSHs(sv_2mortal(newSViv(rtn)));
  PUTBACK;
  call_pv("regex_hook",G_SCALAR); 
  SPAGAIN;
  rtn_sv = POPs;
  rtn = SvIV(rtn_sv);
  PUTBACK;
  FREETMPS;
  LEAVE;
  if(SvTRUE(ERRSV)) {
    printf("fatal error: perl eval error: %s\r\n",SvPV(ERRSV,n_err)); 
    exit(1);
  }
  return(rtn);
}


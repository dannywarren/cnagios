#
# $Id: cnagios.pl,v 1.13 2008/03/31 14:27:25 rader Exp $
#
# See ftp://noc.hep.wisc.edu/pub/src/cnagios/cnagios.pl for more complex examples
#
# The subroutine names here are slightly misleading...
# 
#   &host_plugin_hook() munges host_name and plugin_output
#
#   &service_plugin_hook() munges host_name, service_description and plugin_output
#

use strict;

#------------------------------------------------------------------

sub host_plugin_hook {
  local($_) = $_[0];

  # check_ping v1...
  s/ping .*? - (\d+)% packet loss.*/$1% pkt loss/i;

  # check_ping v2...
  s/PING .*? - Packet loss = (\d+)%.*/$1% pkt loss/i;

  # nagios internal...
  s/\(Host assumed to be up\)/assumed up/;
  s/\(Host check timed out\)/timed out/;
  s/\(No Information Returned From Host Check\)/no plugin output/;
  s/\(Not enough data to determine host status yet\)/none/;

  return $_;

}

#------------------------------------------------------------------

sub service_plugin_hook {
  local($_) = $_[0];

  # nagios internal...
  s/\(Service Check Timed Out\)/timed out/;
  s/\(No output returned from plugin\)/no output from plugin/;
  s/Service check scheduled for.*/none/;
  s/No data yet.*/no data yet/;

  # generic... 
  s/.*no response.*/connection timed out/i;
  s/.*no route to host.*/no route to host/i;
  s/Socket timeout.*/socket timed out/;
  s/^OK\s*[-:]*\s+//i;
  s/^WARNING\s*[-:]*\s+//i;
  s/^CRITICAL\s*[-:]*\s+//i;
  s/^UNKNOWN\s*[-:]*\s+//i;
  s/.*?OK\s*[-:]*\s+//i;
  s/.*?WARNING\s*[-:]*\s+//i;
  s/.*?CRITICAL\s*[-:]*\s+//i;
  s/.*?UNKNOWN\s*[-:]*\s+//i;

  # check_ping...
  s/ping .*? - (\d+)% packet loss.*/$1% pkt loss/i;

  # check_tcp...
  s/.* (\d+\.\d+) sec[s]* response time.*/$1 sec response/;

  # check_http and check_dns...
  s/.* (\d+) second[s]* response time.*/$1 sec response/;

  # check_ntp...
  s/.* Offset ([-]*\d+\.\d+) secs.*/$1 sec offset/;
  s/.* stratum (\d+), offset ([-]*\d+\.\d+).*/stratum $1, $2 sec offset/;
  s/.*Jitter\s+too high.*/jittering/;
  s/.*desynchronized peer server.*/desynchronized peer server/i;
  s/.*probably down.*/down/;

  s/.* Offset ([-]*\d+\.\d+) secs.*/$1 sec offset/;
  s/.* stratum \d+, offset ([-]*\d+\.\d+) secs.*/$1 sec offset/;

  # check_dhcp...
  s/.* Received \d+ DHCPOFFER.*max lease time = (\d+) sec.*/$1 sec lease time/;

  # check_hpjd...
  s/.*? - \(\".*\"\)/printer okay/;
  if ( s/(.*)\s+\(\".*\"\)/$1/ ) { $_ = lc($_); }

  return $_;

}

#------------------------------------------------------------------

# this sub is used for host/service/plugin-output 
# filtering...  it should not change...

sub regex_hook {
  my($str,$regex,$mode) = @_;
  if ( $mode == 0 ) { 
    if ( $str =~ /$regex/ ) { return 0 } else { return 1 }
  }
  if ( $mode == 1 ) { 
    if ( $str !~ /$regex/ ) { return 0 } else { return 1 }
  }
  return 2;
}


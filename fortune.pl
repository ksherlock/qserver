#!/usr/bin/perl -w

use strict;

# this perl script will convert a unix fortune file (from stdin)
# to a IIgs rez file (on stdout) for use with the quote-of-the-day
# server.  Rez file will need to be compiled, naturally.
#
# use at your own risk.


my $cnt = 0;
my $line;

print '#include "Types.rez"',"\n\n";

# quotes are freetext, separated by %
while (<>)
{
  chomp;  # remove trailing linefeed
  
  $line = $_;
  if ($line =~ m/^%/)
  {
  	print "}", "\n\n" if ($cnt++);
  	
  	print "resource rTextForLETextBox2($cnt) {\n";
  }
  else
  {
  	# remove any trailing whitespace.
  	$line =~ s/\s*$//;
  	# escape any quote marks.
	$line =~ s/"/\\"/g;
	
	print '   "',$line,'\n"',"\n";
  }
}
print "};", "\n\n" if ($cnt);
exit;


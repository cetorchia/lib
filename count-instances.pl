#!/usr/bin/perl
#
# countinstances str filename counts the instances in 'filename' of 'str', 
# elucidating them by printing the line with str in parentheses like 
# "(str)"
#
# (c) 2010 Carlos Torchia
#

$line_number=0;
$n=0;		# How many lines have $str so far

#
# Check for correct # of command line arguments
#

if($#ARGV>1) {
  print STDERR "countinstances: there are too many arguments\n";
  exit 1;
}

elsif($#ARGV<1) {
  print STDERR "countinstances: there are too few arguments\n";
  exit 1;
}

#
# Get the command line arguments
#

$str=$ARGV[0];
$filename=$ARGV[1];

#
# Open file and read lines, and then close it
#

open(MYFILE,"<$filename");

#
# Process each line
#

while($_=<MYFILE>) {
  $line_number++;
  $n += s/$str/\($str\)/g;
  chop $_;
  print "Line $line_number: $_\n";
}

print "\n$n\n";

close(MYFILE);

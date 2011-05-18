#!/usr/bin/perl
use strict;

my $var=$ARGV[0] or die "Usage: $0 name";

print "// AUTOGENERATED - DO NOT EDIT\n";
print "const char $var [] = \"";
while (<STDIN>) {
	s/"/\\"/g;
	s/$/\\/;
	print
}
print "\";\n"


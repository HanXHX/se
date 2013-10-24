#!/usr/bin/perl

use strict;
use warnings;

use feature qw/say/;
use Data::Dumper;

use constant {
	'RELEASE' => '1.0git',
	'CENTER' => '""'
};

foreach my $file (glob('*.pod'))
{
	my ($locale, $section) = ($file =~ /^se_([a-z]{2})\.(\d)\.pod$/);	
	print Dumper ($locale, $section);
	my $outfile = 'se_'.$locale.'.'.$section.'.man';
	my $command = 'pod2man --release '.RELEASE.' --section '.$section.' --center '.CENTER.' -n se '.$file.' ../man/'.$outfile;
	say $command;
	`$command`;
}

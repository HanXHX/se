#!/usr/bin/perl

use strict;
use warnings;

use feature qw/say/;

use constant {
	'CENTER' => '""'
};

open('VERSION', '<', '../VERSION');
my $release = <VERSION>;
chomp $release;
close(VERSION);


foreach my $file (glob('*.pod'))
{
	my ($locale, $section) = ($file =~ /^se_([a-z]{2})\.(\d)\.pod$/);	
	my $outfile = 'se_'.$locale.'.'.$section.'.man';
	my $command = 'pod2man --release "'.$release.'" --section '.$section.' --center '.CENTER.' -n se '.$file.' ../man/'.$outfile;
	say $command;
	`$command`;
}

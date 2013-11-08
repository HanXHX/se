#!/usr/bin/perl

use strict;
use warnings;

use feature qw/say/;
use File::Path qw/make_path/;

use constant 'VERSION_FILE' => '../VERSION';

open('VERSION', '<', VERSION_FILE) or die('Can\'t open ' . VERSION_FILE);
my $release = <VERSION>;
chomp $release;
close(VERSION);


foreach my $file (glob('pod/*.pod'))
{
	my ($locale, $section) = ($file =~ /se_([a-z]{2})\.(\d)\.pod$/);	

	my $outfolder = sprintf(
		'./man/%sman%d',
		$locale ne 'en' ? $locale . '/' : '', $section
	);

	make_path($outfolder) or die("Cant't mkdir $outfolder");

	my $outfile = sprintf(
		'%s/se.%d',
		$outfolder, $section
	);

	my $command = sprintf(
		qq/pod2man --release "%s" --section "%d" --center "" -n se %s %s/,
		$release, $section, $file, $outfile
	);

	say $command;
	`$command`;
}

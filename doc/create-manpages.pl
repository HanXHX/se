#!/usr/bin/perl

# Generates doc/man/** from doc/pod/se_<locale>.<section>.pod with pod2man.
# Run from the doc/ directory (the Makefile does it).

use strict;
use warnings;

use feature qw/say/;
use File::Path qw/make_path/;

use constant 'VERSION_FILE' => '../VERSION';

open(my $fh, '<', VERSION_FILE) or die('Can\'t open ' . VERSION_FILE);
my $release = <$fh>;
chomp $release;
close($fh);

foreach my $file (glob('pod/*.pod'))
{
	my ($locale, $section) = ($file =~ /se_([a-z]{2})\.(\d)\.pod$/)
		or die("Unexpected pod file name: $file");

	my $outfolder = sprintf(
		'./man/%sman%d',
		$locale ne 'en' ? $locale . '/' : '', $section
	);

	make_path($outfolder) unless -d $outfolder;
	-d $outfolder or die("Can't mkdir $outfolder");

	my $outfile = sprintf('%s/se.%d', $outfolder, $section);

	my @command = (
		'pod2man', '--release', $release, '--section', $section,
		'--center', '', '-n', 'se', $file, $outfile
	);

	say join(' ', @command);
	system(@command) == 0 or die("pod2man failed on $file");
}

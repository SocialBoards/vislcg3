#!/usr/bin/env perl
BEGIN { $| = 1; }
use warnings;
use strict;
use File::Spec;
use Getopt::Long;
use Digest::SHA qw(sha1_hex);
Getopt::Long::Configure('no_ignore_case');

# This is updated by the update-revision.pl script.
my $revision = 13426;

# Generate list with:
# vislcg3 --help 2>&1 | perl -wpne 'if (/^ / && /-(\w), --([-\w]+)/) {print "$2|$1=s\n"} elsif (/^ / && /--([-\w]+)/) {print "$1=s\n"} s/^.*$//s;' | perl -wpne 's/^/"/; s/$/",/;'
# and then trim =s from those that don't take args
my %h = ();
GetOptions (\%h,
"help|h",
"version|V",
"min-binary-revision",
"grammar|g=s",
"grammar-out=s",
"grammar-bin=s",
"grammar-only",
"ordered",
"unsafe|u",
"sections|s=s",
"rules=s",
"rule=s",
"debug|d:s",
"verbose|v:s",
"quiet",
"vislcg-compat|2",
"stdin|I=s",
"stdout|O=s",
"stderr|E=s",
"codepage-all|C=s",
"codepage-grammar=s",
"codepage-input=s",
"codepage-output=s",
"no-mappings",
"no-corrections",
"no-before-sections",
"no-sections",
"no-after-sections",
"trace|t:s",
"trace-name-only",
"trace-no-removed",
"trace-encl",
"dry-run",
"single-run",
"max-runs=s",
"statistics|S",
"optimize-unsafe|Z",
"optimize-safe|z",
"prefix|p=s",
"unicode-tags",
"unique-tags",
"num-windows=s",
"always-span",
"soft-limit=s",
"hard-limit=s",
"dep-delimit|D:s",
"dep-original",
"dep-allow-loops",
"dep-no-crossing",
"no-magic-readings",
"no-pass-origin|o",
"split-mappings",
"show-end-tags|e",
"show-unused-sets",
"show-tags",
"show-tag-hashes",
"show-set-hashes",
"dump-ast"
);

if (defined $h{'grammar-bin'}) {
	die "Error: Cannot use --grammar-bin with the autobin wrapper !\n";
}

if (! defined $h{'grammar'}) {
	die "Error: Missing --grammar or -g !\n";
}
my $grammar = $h{'grammar'};

if (! (-r $h{'grammar'})) {
	die "Error: Cannot read file ".$h{'grammar'}." !\n";
}

my $args = " ";
while (my ($k,$v) = each(%h)) {
	if ($k ne 'grammar') {
		$args .= " --$k $v";
	}
}

my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat($h{'grammar'});

my $bn = File::Spec->tmpdir()."/".sha1_hex($args.$h{'grammar'}).".".$revision.".".$mtime.".cg3b";

my $bin = 'vislcg3';
if (-x '/usr/bin/vislcg3') {
	$bin = '/usr/bin/vislcg3';
}
if (-x '/usr/local/bin/vislcg3') {
	$bin = '/usr/local/bin/vislcg3';
}
if (-x '~/bin/vislcg3') {
	$bin = '~/bin/vislcg3';
}

if (!(-r $bn) && !(-r $bn.".lock")) {
	open(F, ">".$bn.".lock") && close F;
	`$bin $args --grammar $grammar --grammar-only --grammar-bin $bn`;
	unlink $bn.".lock";
}

my $cmd = '';
if (-r $bn && !(-r $bn.".lock")) {
#	print STDERR "CG3 AutoBin using $bn\n";
	$cmd = "cat /dev/stdin | $bin $args --grammar $bn";
}
else {
	print STDERR "CG3 AutoBin failed - falling back to normal\n";
	$cmd = "cat /dev/stdin | $bin $args --grammar $grammar";
}

if (defined $h{'stdin'}) {
	$cmd =~ s@^cat /dev/stdin | @@;
}

open CG, "$cmd|" or die $!;
while (<CG>) {
	print;
}
close CG;

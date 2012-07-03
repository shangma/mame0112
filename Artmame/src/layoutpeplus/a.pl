#!/usr/bin/perl

foreach $elt (@ARGV) {
	if ($elt =~ m/([^\.]+)\.lay$/) {
		$base = $1;
		$dst = $elt;
		$dst =~ s/\.lay$/\.lh/;
		print "../tools/file2str $elt $dst layout_$base\n";
	}
}


#!/usr/bin/perl
use strict;
use warnings;

my $app = "../bin/samiam";
my $filename = "tests.db";
my @tests = ();
my @pids = ();

if (@ARGV > 0) {
    $filename = $ARGV[0];
}

print "failed test cases:\n";
print "actual\texpected\ttest case\n";
open DB, "<$filename" or die "couldn't open $filename: $!.\n";
while (<DB>) {
    my $pid = fork;

    die "couldn't fork: $!.\n" unless (defined ($pid));
    push @pids, $pid;
    if ($pid == 0) {
	/\S/ or next;
	my ($test, $rv) = split /\s+/;
	system ($app, "-q", $test);
	if ($? == -1) {
	    print "couldn't execute $app: $!.\n";
	} elsif ($? & 127) {
	    printf "child died with signal %d.\n", ($? & 127);
	} elsif ($rv != ($? >> 8)) {
	    printf "%d\t$rv\t\t$test\n", ($? >> 8);
	}
	exit;
    }
}
for my $pid(@pids) {
    waitpid $pid, 0;
}
close DB;

#!/usr/bin/perl

use strict;
use warnings;

my $NUM_BLOCKS = 1600;
my $FORMAT = "a4 a4 S S L L L L L L L L L x16";

$| = 1;

foreach (@ARGV) {
    print qq|Checking "$_"...|;

    check_file($_);
}

exit(0);

sub check_file
{
    my ($file) = @_;

    open my $fh, "+<$file";
    binmode $fh;

    my $header;
    read $fh, $header, 64;

    my ($magic, $creator, $header_len, $version, $format, $flags, $blocks, $data_offset, $data_len, $comment_offse, $comment_len, $creator_offset, $creator_len) = unpack $FORMAT, $header;

    my $bad = 0;

    if ($blocks != $NUM_BLOCKS) {
        $blocks = $NUM_BLOCKS;
        $data_len = $blocks * 512;

        $header = pack $FORMAT, $magic, $creator, $header_len, $version, $format, $flags, $blocks, $data_offset, $data_len, $comment_offse, $comment_len, $creator_offset, $creator_len;

        seek($fh, 0, 0);
        print $fh $header;

        print "REPAIRED\n";
    }
    else {
        print "OK\n";
    }

    close($fh);
}

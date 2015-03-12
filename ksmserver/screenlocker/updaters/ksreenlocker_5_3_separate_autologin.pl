#! /usr/bin/perl


use strict;

my $key;
my $value;

my $alreadyHasAutolockKey = 0;

while (<>)
{
    chomp;
    if ( /^\[/ )
    {
        next;
    }
    ($key, $value) = ($_ =~ /([^=]+)=[ \t]*([^\n]+)/);

    if ($key eq "Autolock")
    {
        $alreadyHasAutolockKey = 1;
    }

    if ($key eq "Timeout" && $alreadyHasAutolockKey == 0)
    {
        if ($value eq "0")
        {
            print("Autolock=false\n");
        }
        else
        {
            print("Autolock=true\n");
        }
    }
    print "$_\n"
}

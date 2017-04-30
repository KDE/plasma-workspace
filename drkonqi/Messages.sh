#! /usr/bin/env bash
$EXTRACTRC ui/*.ui bugzillaintegration/ui/*.ui >> rc.cpp
$XGETTEXT *.cpp bugzillaintegration/*.cpp -o $podir/drkonqi5.pot
rm -f rc.cpp

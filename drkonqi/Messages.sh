#! /usr/bin/env bash
$EXTRACTRC ui/*.ui bugzillaintegration/ui/*.ui >> rc.cpp
$XGETTEXT *.cpp bugzillaintegration/*.cpp -o $podir/drkonqi.pot
rm -f rc.cpp

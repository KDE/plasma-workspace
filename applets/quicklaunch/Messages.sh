#! /usr/bin/env bash
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/plasma_applet_quicklaunch.pot
rm -f rc.cpp

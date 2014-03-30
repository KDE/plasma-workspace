#! /usr/bin/env bash
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/plasma_applet_system-monitor.pot
rm -f rc.cpp

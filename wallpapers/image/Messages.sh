#! /usr/bin/env bash
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/plasma_wallpaper_image.pot
rm -f rc.cpp


#! /usr/bin/env bash
$EXTRACTRC ui/*.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/drkonqi.pot
rm -f rc.cpp

#! /usr/bin/env bash
$EXTRACTRC *.ui *.kcfg >> rc.cpp
$XGETTEXT *.cpp -o $podir/klipper.pot
rm -f rc.cpp


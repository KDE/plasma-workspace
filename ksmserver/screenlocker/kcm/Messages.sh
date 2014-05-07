#!bin/sh
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.h *.cpp -o $podir/screenlocker_kcm.pot
rm -f rc.cpp

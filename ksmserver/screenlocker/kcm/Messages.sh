#!bin/sh
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT `find . -name "*.cpp" -o -name "*.qml"` -o $podir/screenlocker_kcm.pot
rm -f rc.cpp

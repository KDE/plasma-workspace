#!bin/sh
$EXTRACTRC kcfg/*.kcfg >> rc.cpp
# do not include subdirectory as it has an own Messages.sh
$XGETTEXT *.h *.cpp -o $podir/kscreenlocker.pot

#!bin/sh
$XGETTEXT `find . -name \*.cc -o -name \*.cpp -o -name \*.h` -o $podir/kscreenlocker_greet.pot
$XGETTEXT `find . -name '*.qml'` -j -L Java -o $podir/kscreenlocker_greet.pot

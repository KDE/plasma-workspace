#! /usr/bin/env bash
$XGETTEXT `find . -name \*.js -o -name \*.qml -o -name \*.cpp -name \*.h` -o $podir/plasma_applet_org.kde.plasma.digitalclock.pot

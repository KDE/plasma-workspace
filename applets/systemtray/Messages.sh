#! /usr/bin/env bash
$EXTRACTRC `find . \( -name \*.rc -o -name \*.ui -o -name \*.kcfg \) -not -path "./tests/*"` >> rc.cpp
$XGETTEXT `find . \( -name \*.js -o -name \*.qml -o -name \*.cpp \) -not -path "./tests/*"` -o $podir/plasma_applet_org.kde.plasma.systemtray.pot
rm rc.cpp

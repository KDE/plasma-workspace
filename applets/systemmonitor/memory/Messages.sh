#! /usr/bin/env bash
$XGETTEXT `find . -name \*.js -o -name \*.qml -o -name \*.cpp` `find ../common -name \*.qml` -o $podir/plasma_applet_org.kde.plasma.systemmonitor.memory.pot

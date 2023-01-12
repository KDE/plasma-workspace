#! /bin/sh
$EXTRACTRC `find . -name \*.kcfg` >>rc.cpp
$XGETTEXT  `find . -name \*.cpp -o -name \*.qml` -o $podir/plasmashellprivateplugin.pot
rm -f rc.cpp

#! /usr/bin/env bash
$XGETTEXT `find . -name \*.qml` `find ../sddm-theme -name \*.qml` -o $podir/plasma_lookandfeel_org.kde.lookandfeel.pot

#! /usr/bin/env bash
# SPDX-FileCopyrightText: None
# SPDX-License-Identifier: CC0-1.0
$XGETTEXT `find . -name \*.js -o -name \*.qml -o -name \*.cpp` -o $podir/plasma_wallpaper_org.kde.image.pot

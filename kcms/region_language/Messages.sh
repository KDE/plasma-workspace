#! /usr/bin/env bash
#Messages.sh
#SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>
#SPDX-License-Identifier: GPL-2.0-or-later

$EXTRACTRC `find . -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name "*.cpp" -o -name "*.qml"` -o $podir/kcm_regionandlang.pot


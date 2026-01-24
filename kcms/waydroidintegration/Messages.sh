#! /usr/bin/env bash

# SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

$XGETTEXT `find . -name \*.js -o -name \*.qml -o -name \*.cpp` -o $podir/kcm_waydroidintegration.pot



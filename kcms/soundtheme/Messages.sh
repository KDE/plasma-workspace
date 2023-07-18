#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause
$XGETTEXT `find . -name "*.cpp" -o -name "*.qml"` -o $podir/kcm_soundtheme.pot

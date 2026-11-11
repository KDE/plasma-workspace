#! /usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

$EXTRACTRC `find . -name \*.rc`  >> rc.cpp
$XGETTEXT rc.cpp */*.cpp */*.h apps/*/*.cpp apps/*/*.h -o $podir/kfontinst.pot
rm -f rc.cpp

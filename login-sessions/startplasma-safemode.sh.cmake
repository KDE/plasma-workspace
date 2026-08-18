#!/bin/bash
# SPDX-FileCopyrightText: 2024 David Edmundson <davidedmundson@kde.org>
# SPDX-FileCopyrightText: 2026 Jakob Petsovits <jpetso@petsovits.com>
#
# SPDX-License-Identifier: GPL-2.0-or-later

export KDE_SAFEMODE=1
export KDE_SAFEMODE_ORIG_XDG_DATA_HOME="$XDG_DATA_HOME"
export KDE_SAFEMODE_ORIG_XDG_CONFIG_HOME="$XDG_CONFIG_HOME"
export KDE_SAFEMODE_ORIG_XDG_STATE_HOME="$XDG_STATE_HOME"
export KDE_SAFEMODE_ORIG_XDG_CACHE_HOME="$XDG_CACHE_HOME"

TEMP_XDG_ROOT=/tmp/kde-safemode-$USER

export XDG_DATA_HOME="$TEMP_XDG_ROOT/local/share"
export XDG_CONFIG_HOME="$TEMP_XDG_ROOT/config"
export XDG_STATE_HOME="$TEMP_XDG_ROOT/local/state"
export XDG_CACHE_HOME="$TEMP_XDG_ROOT/cache"

rm -rf "$TEMP_XDG_ROOT" # clean up in case a previous Safe Mode didn't
cp -r "@CMAKE_INSTALL_FULL_DATADIR@/plasma-safemode-template" "$TEMP_XDG_ROOT"

TEMP_DIRS_ERROR=0
for dir in "$XDG_DATA_HOME" "$XDG_CONFIG_HOME" "$XDG_STATE_HOME" "$XDG_CACHE_HOME"; do
    mkdir -p "$dir"
    if [ $? -ne 0 ]; then
        TEMP_DIRS_ERROR=1
        break
    fi
done

if [ $TEMP_DIRS_ERROR -eq 0 ]; then
    trap 'kill -TERM $PID; wait $PID' TERM
    startplasma-wayland &
    PID=$!
    wait $PID
else
    echo "Could not create temporary XDG user directories"
fi

rm -rf "$TEMP_XDG_ROOT"

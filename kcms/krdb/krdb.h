/*
    SPDX-FileCopyrightText: 2001 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "krdb_export.h"

enum KRdbAction {
    KRdbExportQtColors = 0x0002, // Export KDE's colors to qtrc
    KRdbExportQtSettings = 0x0004, // Export all possible qtrc settings, excluding colors
    KRdbExportGtkTheme = 0x0010, // Export KDE's widget style to Gtk if possible
};

void KRDB_EXPORT runRdb(unsigned int flags);

int KRDB_EXPORT xftDpi();

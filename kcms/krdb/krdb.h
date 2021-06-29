/*
    SPDX-FileCopyrightText: 2001 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

enum KRdbAction {
    KRdbExportColors = 0x0001, // Export colors to non-(KDE/Qt) apps
    KRdbExportQtColors = 0x0002, // Export KDE's colors to qtrc
    KRdbExportQtSettings = 0x0004, // Export all possible qtrc settings, excluding colors
    KRdbExportXftSettings = 0x0008, // Export KDE's Xft (anti-alias) settings
    KRdbExportGtkTheme = 0x0010, // Export KDE's widget style to Gtk if possible
};

void Q_DECL_EXPORT runRdb(uint flags);

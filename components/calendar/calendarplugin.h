/*
    SPDX-FileCopyrightText: 2013 Mark Gaiser <markg85@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CALENDARPLUGIN_H
#define CALENDARPLUGIN_H

#include <QQmlExtensionPlugin>

class QQmlEngine;

class CalendarPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override;
};

#endif

/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef DIGITALCLOCKPLUGIN_H
#define DIGITALCLOCKPLUGIN_H

#include <QQmlExtensionPlugin>

class DigitalClockPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override;
};

#endif // DIGITALCLOCKPLUGIN_H

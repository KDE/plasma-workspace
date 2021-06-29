/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef HOLIDAYEVENTSHELPERPLUGIN_H
#define HOLIDAYEVENTSHELPERPLUGIN_H

#include <QQmlExtensionPlugin>

class HolidayEventsHelperPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char *uri) override;
};

#endif // HOLIDAYEVENTSHELPERPLUGIN_H

/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nightcolorinhibitor.h"
#include "nightcolormonitor.h"

#include <QQmlEngine>
#include <QQmlExtensionPlugin>

class Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override
    {
        qmlRegisterType<NightColorInhibitor>(uri, 1, 0, "NightColorInhibitor");
        qmlRegisterType<NightColorMonitor>(uri, 1, 0, "NightColorMonitor");
    }
};

#include "plugin.moc"

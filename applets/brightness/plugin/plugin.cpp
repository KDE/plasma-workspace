/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "keyboardbrightnesscontrol.h"
#include "nightcolorinhibitor.h"
#include "nightcolormonitor.h"
#include "screenbrightnesscontrol.h"

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
        qmlRegisterType<ScreenBrightnessControl>(uri, 1, 0, "ScreenBrightnessControl");
        qmlRegisterType<KeyboardBrightnessControl>(uri, 1, 0, "KeyboardBrightnessControl");
    }
};

#include "plugin.moc"

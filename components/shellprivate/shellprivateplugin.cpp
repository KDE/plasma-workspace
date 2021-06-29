/*
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "shellprivateplugin.h"
#include "config-shellprivate.h"

#include <QQmlEngine>

#include "widgetexplorer/widgetexplorer.h"
#include <Plasma/Containment>

#if KF5TextEditor_FOUND
#include "interactiveconsole/interactiveconsole.h"
#endif

void PlasmaShellPrivatePlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.private.shell"));

    qmlRegisterType<Plasma::Containment>();
    qmlRegisterType<WidgetExplorer>(uri, 2, 0, "WidgetExplorer");
#if KF5TextEditor_FOUND
    qmlRegisterType<InteractiveConsoleItem>(uri, 2, 0, "InteractiveConsoleWindow");
#endif
}

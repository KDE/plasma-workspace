/*
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "shellprivateplugin.h"
#include "config-shellprivate.h"

#include <QQmlEngine>

#include "widgetexplorer/widgetexplorer.h"
#include <Plasma/Containment>

void PlasmaShellPrivatePlugin::registerTypes(const char *uri)
{
    qmlRegisterAnonymousType<Plasma::Containment>("", 1);
    qmlRegisterType<WidgetExplorer>(uri, 2, 0, "WidgetExplorer");
}

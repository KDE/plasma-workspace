/*
    SPDX-FileCopyrightText: 2014 Martin Yrjölä <martin.yrjola@gmail.com>
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: MIT
*/

#include "trianglemousefilterplugin.h"
#include "trianglemousefilter.h"

#include <QQmlEngine>

void TriangleMouseFilterPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.workspace.trianglemousefilter"));

    qmlRegisterType<TriangleMouseFilter>(uri, 1, 0, "TriangleMouseFilter");
}

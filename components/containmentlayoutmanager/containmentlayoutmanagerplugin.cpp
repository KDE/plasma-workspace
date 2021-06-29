/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "containmentlayoutmanagerplugin.h"

#include <QQmlContext>
#include <QQuickItem>

#include "appletcontainer.h"
#include "appletslayout.h"
#include "configoverlay.h"
#include "itemcontainer.h"
#include "resizehandle.h"

void ContainmentLayoutManagerPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(QLatin1String(uri) == QLatin1String("org.kde.plasma.private.containmentlayoutmanager"));

    qmlRegisterType<AppletsLayout>(uri, 1, 0, "AppletsLayout");
    qmlRegisterType<AppletContainer>(uri, 1, 0, "AppletContainer");
    qmlRegisterType<ConfigOverlay>(uri, 1, 0, "ConfigOverlay");
    qmlRegisterType<ItemContainer>(uri, 1, 0, "ItemContainer");
    qmlRegisterType<ResizeHandle>(uri, 1, 0, "ResizeHandle");

    //  qmlProtectModule(uri, 1);
}

#include "moc_containmentlayoutmanagerplugin.cpp"

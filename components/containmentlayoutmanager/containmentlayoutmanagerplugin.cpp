/*
 *   Copyright 2019 by Marco Martin <mart@kde.org>

 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

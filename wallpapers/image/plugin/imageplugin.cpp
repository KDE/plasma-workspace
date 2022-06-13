/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imageplugin.h"
#include <QQmlContext>
#include <QScreen>

#include <KFileItem>

#include "imagebackend.h"
#include "provider/packageimageprovider.h"
#include "sortingmode.h"
#include "utils/maximizedwindowmonitor.h"
#include "utils/mediaproxy.h"

const auto pluginName = QByteArrayLiteral("org.kde.plasma.wallpapers.image");

void ImagePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_ASSERT(uri == pluginName);

    engine->addImageProvider(QStringLiteral("package"), new PackageImageProvider);
}

void ImagePlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == pluginName);

    qRegisterMetaType<KFileItem>(); // For image preview

    qmlRegisterType<ImageBackend>(uri, 2, 0, "ImageBackend");
    qmlRegisterType<MediaProxy>(uri, 2, 0, "MediaProxy");

    qmlRegisterType<MaximizedWindowMonitor>(uri, 2, 0, "MaximizedWindowMonitor");

    qmlRegisterAnonymousType<QAbstractItemModel>("QAbstractItemModel", 1);
    qmlRegisterUncreatableType<SortingMode>(uri, 2, 0, "SortingMode", QStringLiteral("error: only enums"));
}

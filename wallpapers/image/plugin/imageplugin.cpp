/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imageplugin.h"
#include <QQmlContext>

#include <KFileItem>

#include "imagebackend.h"
#include "finder/xmlfinder.h"
#include "provider/packageimageprovider.h"
#include "provider/wideimageprovider.h"
#include "sortingmode.h"

const auto pluginName = QByteArrayLiteral("org.kde.plasma.wallpapers.image");

void ImagePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_ASSERT(uri == pluginName);

    engine->addImageProvider(QStringLiteral("package"), new PackageImageProvider);
    engine->addImageProvider(QStringLiteral("wideimage"), new WideImageProvider);
}

void ImagePlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == pluginName);

    qRegisterMetaType<WallpaperItem>();
    qRegisterMetaType<QList<WallpaperItem>>();
    qRegisterMetaType<KFileItem>(); // For image preview

    qmlRegisterType<ImageBackend>(uri, 2, 0, "ImageBackend");
    qmlRegisterAnonymousType<QAbstractItemModel>("QAbstractItemModel", 1);
    qmlRegisterUncreatableType<SortingMode>(uri, 2, 0, "SortingMode", QStringLiteral("error: only enums"));
}

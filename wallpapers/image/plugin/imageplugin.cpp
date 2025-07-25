/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imageplugin.h"
#include <QQmlContext>

#include <KFileItem>

#include "daynightwallpaper.h"
#include "finder/mediametadatafinder.h"
#include "imagebackend.h"
#include "provider/packageimageprovider.h"
#include "provider/wallpaperpreviewimageprovider.h"
#include "sortingmode.h"
#include "utils/dynamicmode.h"
#include "utils/maximizedwindowmonitor.h"
#include "utils/mediaproxy.h"
#include "utils/wallpaperurl.h"

void ImagePlugin::initializeEngine(QQmlEngine *engine, const char *)
{
    engine->addImageProvider(QStringLiteral("package"), new PackageImageProvider);
    engine->addImageProvider(QStringLiteral("wallpaper-preview"), new WallpaperPreviewImageProvider);
}

void ImagePlugin::registerTypes(const char *uri)
{
    qRegisterMetaType<KFileItem>(); // For image preview
    qRegisterMetaType<MediaMetadata>(); // For image preview

    qmlRegisterType<ImageBackend>(uri, 2, 0, "ImageBackend");
    qmlRegisterType<MediaProxy>(uri, 2, 0, "MediaProxy");
    qmlRegisterType<DayNightWallpaper>(uri, 2, 0, "DayNightWallpaper");

    qmlRegisterType<MaximizedWindowMonitor>(uri, 2, 0, "MaximizedWindowMonitor");

    qmlRegisterAnonymousType<QAbstractItemModel>("QAbstractItemModel", 1);

    const QString reason = QStringLiteral("error: only enums");
    qmlRegisterUncreatableMetaObject(Provider::staticMetaObject, uri, 2, 0, "Provider", reason);
    qmlRegisterUncreatableMetaObject(BackgroundType::staticMetaObject, uri, 2, 0, "BackgroundType", reason);
    qmlRegisterUncreatableMetaObject(SortingMode::staticMetaObject, uri, 2, 0, "SortingMode", reason);
    qmlRegisterUncreatableMetaObject(DynamicMode::staticMetaObject, uri, 2, 0, "DynamicMode", reason);

    qmlRegisterSingletonType<WallpaperUrl>(uri, 2, 0, "WallpaperUrl", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return new WallpaperUrl;
    });
}

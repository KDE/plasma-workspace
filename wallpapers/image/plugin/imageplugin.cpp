/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imageplugin.h"
#include <QQmlContext>

#include <KFileItem>

#include "finder/mediametadatafinder.h"
#include "provider/wallpaperpreviewimageprovider.h"

void ImagePlugin::initializeEngine(QQmlEngine *engine, const char *)
{
    engine->addImageProvider(QStringLiteral("wallpaper-preview"), new WallpaperPreviewImageProvider);
}

void ImagePlugin::registerTypes(const char */*uri*/)
{
    qRegisterMetaType<KFileItem>(); // For image preview
    qRegisterMetaType<MediaMetadata>(); // For image preview
}

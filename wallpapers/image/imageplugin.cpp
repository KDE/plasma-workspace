/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imageplugin.h"
#include "imagebackend.h"
#include <QQmlContext>

#include "sortingmode.h"

void ImagePlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.wallpapers.image"));

    qmlRegisterType<ImageBackend>(uri, 2, 0, "ImageBackend");
    qmlRegisterAnonymousType<QAbstractItemModel>("QAbstractItemModel",1);
    qmlRegisterUncreatableType<SortingMode>(uri, 2, 0, "SortingMode", "error: only enums");
}

/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imageplugin.h"
#include "image.h"
#include <QQmlContext>

void ImagePlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.wallpapers.image"));

    qmlRegisterType<Image>(uri, 2, 0, "Image");
    qmlRegisterAnonymousType<QAbstractItemModel>("QAbstractItemModel",1);
}

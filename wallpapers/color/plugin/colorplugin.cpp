/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "colorplugin.h"

#include <QQmlContext>

#include "imagecolors.h"

void ColorPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.plasma.wallpapers.color"));

    qmlRegisterType<ImageColors>(uri, 0, 1, "ImageColors");
}

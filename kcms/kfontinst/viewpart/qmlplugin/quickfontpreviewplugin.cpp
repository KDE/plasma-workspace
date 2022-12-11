/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "quickfontpreviewplugin.h"

#include "quickfontpreviewbackend.h"

void FontPreviewPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.plasma.private.fontview"));

    qmlRegisterType<FontPreviewBackend>(uri, 0, 1, "FontPreviewBackend");
}

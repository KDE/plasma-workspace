/*
    SPDX-FileCopyrightText: 2022 Thiago Sueto <herzenschein@gmail.com>
    SPDX-FileCopyrightText: 2022 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "componentchooserimageviewer.h"

ComponentChooserImageViewer::ComponentChooserImageViewer(QObject *parent)
    : ComponentChooser(parent,
                       QStringLiteral("image/png"),
                       QStringLiteral("Viewer"),
                       QStringLiteral("org.kde.gwenview.desktop"),
                       i18n("Select default image viewer"))
{
}

static const QStringList imageViewerMimetypes{QStringLiteral("image/png"),
                                              QStringLiteral("image/jpeg"),
                                              QStringLiteral("image/webp"),
                                              QStringLiteral("image/avif"),
                                              QStringLiteral("image/heif"),
                                              QStringLiteral("image/bmp"),
                                              QStringLiteral("image/x-icns")};

QStringList ComponentChooserImageViewer::mimeTypes() const
{
    return imageViewerMimetypes;
}

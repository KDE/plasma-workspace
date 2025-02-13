/*
    SPDX-FileCopyrightText: 2022 Thiago Sueto <herzenschein@gmail.com>
    SPDX-FileCopyrightText: 2022 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "componentchooservideoplayer.h"

ComponentChooserVideoPlayer::ComponentChooserVideoPlayer(QObject *parent)
    : ComponentChooser(parent,
                       QStringLiteral("video/mp4"),
                       QStringLiteral("Video"),
                       QStringLiteral("org.kde.haruna.desktop"),
                       i18n("Select default video player"))
{
}

static const QStringList videoMimetypes{QStringLiteral("video/3gp"),      QStringLiteral("video/3gpp"),         QStringLiteral("video/3gpp2"),
                                        QStringLiteral("video/avi"),      QStringLiteral("video/divx"),         QStringLiteral("video/dv"),
                                        QStringLiteral("video/fli"),      QStringLiteral("video/flv"),          QStringLiteral("video/mp2t"),
                                        QStringLiteral("video/mp4"),      QStringLiteral("video/mp4v-es"),      QStringLiteral("video/mpeg"),
                                        QStringLiteral("video/msvideo"),  QStringLiteral("video/ogg"),          QStringLiteral("video/quicktime"),
                                        QStringLiteral("video/vnd.divx"), QStringLiteral("video/vnd.mpegurl"),  QStringLiteral("video/vnd.rn-realvideo"),
                                        QStringLiteral("video/webm"),     QStringLiteral("video/x-avi"),        QStringLiteral("video/x-flv"),
                                        QStringLiteral("video/x-m4v"),    QStringLiteral("video/x-matroska"),   QStringLiteral("video/x-mpeg2"),
                                        QStringLiteral("video/x-ms-asf"), QStringLiteral("video/x-msvideo"),    QStringLiteral("video/x-ms-wmv"),
                                        QStringLiteral("video/x-ms-wmx"), QStringLiteral("video/x-ogm"),        QStringLiteral("video/x-ogm+ogg"),
                                        QStringLiteral("video/x-theora"), QStringLiteral("video/x-theora+ogg"), QStringLiteral("application/x-matroska")};

QStringList ComponentChooserVideoPlayer::mimeTypes() const
{
    return videoMimetypes;
}

/*
    SPDX-FileCopyrightText: 2022 Thiago Sueto <herzenschein@gmail.com>
    SPDX-FileCopyrightText: 2022 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "componentchoosermusicplayer.h"

ComponentChooserMusicPlayer::ComponentChooserMusicPlayer(QObject *parent)
    : ComponentChooser(parent,
                       QStringLiteral("audio/mpeg"),
                       QStringLiteral("Player"),
                       QStringLiteral("org.kde.elisa.desktop"),
                       i18n("Select default music player"))
{
}

static const QStringList audioMimetypes{QStringLiteral("audio/aac"),
                                        QStringLiteral("audio/mp4"),
                                        QStringLiteral("audio/mpeg"),
                                        QStringLiteral("audio/mpegurl"),
                                        QStringLiteral("audio/ogg"),
                                        QStringLiteral("audio/vnd.rn-realaudio"),
                                        QStringLiteral("audio/vorbis"),
                                        QStringLiteral("audio/x-flac"),
                                        QStringLiteral("audio/x-mp3"),
                                        QStringLiteral("audio/x-mpegurl"),
                                        QStringLiteral("audio/x-ms-wma"),
                                        QStringLiteral("audio/x-musepack"),
                                        QStringLiteral("audio/x-oggflac"),
                                        QStringLiteral("audio/x-pn-realaudio"),
                                        QStringLiteral("audio/x-scpls"),
                                        QStringLiteral("audio/x-speex"),
                                        QStringLiteral("audio/x-vorbis"),
                                        QStringLiteral("audio/x-vorbis+ogg"),
                                        QStringLiteral("audio/x-wav")};

QStringList ComponentChooserMusicPlayer::mimeTypes() const
{
    return audioMimetypes;
}

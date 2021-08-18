/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KConfig>
#include <KConfigGroup>
#include <KWindowSystem>
#include <QProcess>

extern "C" {
Q_DECL_EXPORT void kcminit_fonts()
{
    KConfig cfg(QStringLiteral("kcmfonts"));
    KConfigGroup fontsCfg(&cfg, "General");

    QString fontDpiKey = KWindowSystem::isPlatformWayland() ? QStringLiteral("forceFontDPIWayland") : QStringLiteral("forceFontDPI");

    if (!fontsCfg.hasKey(fontDpiKey)) {
        return;
    }

    const QByteArray input = "Xft.dpi: " + QByteArray::number(fontsCfg.readEntry(fontDpiKey, 0));
    QProcess p;
    p.start(QStringLiteral("xrdb"), {QStringLiteral("-quiet"), QStringLiteral("-merge"), QStringLiteral("-nocpp")});
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.write(input);
    p.closeWriteChannel();
    p.waitForFinished(-1);
}
}

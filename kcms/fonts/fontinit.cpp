/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "krdb.h"
#include <KConfig>
#include <KConfigGroup>
#include <KWindowSystem>
#include <QProcess>

extern "C" {
Q_DECL_EXPORT void kcminit()
{
    KConfig cfg(QStringLiteral("kcmfonts"));
    KConfigGroup fontsCfg(&cfg, "General");

    const int dpi = xftDpi();
    if (dpi <= 0) {
        return;
    }

    const QByteArray input = "Xft.dpi: " + QByteArray::number(dpi);
    QProcess p;
    p.start(QStringLiteral("xrdb"), {QStringLiteral("-quiet"), QStringLiteral("-merge"), QStringLiteral("-nocpp")});
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.write(input);
    p.closeWriteChannel();
    p.waitForFinished(-1);
}
}

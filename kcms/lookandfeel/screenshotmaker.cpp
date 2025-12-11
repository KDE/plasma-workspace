/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "screenshotmaker.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QUrl>

ScreenshotMaker::ScreenshotMaker(QObject *parent)
    : QObject(parent)
{
}

void ScreenshotMaker::onTaken(const QString &fileName)
{
    m_waiting = false;

    QDBusConnection::sessionBus().disconnect(QStringLiteral("org.kde.Spectacle"),
                                             QStringLiteral("/"),
                                             QStringLiteral("org.kde.Spectacle"),
                                             QStringLiteral("ScreenshotTaken"),
                                             this,
                                             SLOT(onTaken(QString)));

    Q_EMIT accepted(QUrl::fromLocalFile(fileName));
}

void ScreenshotMaker::take()
{
    if (!m_waiting) {
        m_waiting = true;

        QDBusConnection::sessionBus().connect(QStringLiteral("org.kde.Spectacle"),
                                              QStringLiteral("/"),
                                              QStringLiteral("org.kde.Spectacle"),
                                              QStringLiteral("ScreenshotTaken"),
                                              this,
                                              SLOT(onTaken(QString)));
    }

    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.Spectacle"),
                                                          QStringLiteral("/"),
                                                          QStringLiteral("org.kde.Spectacle"),
                                                          QStringLiteral("RectangularRegion"));
    message.setArguments({0});
    QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
}

#include "moc_screenshotmaker.cpp"

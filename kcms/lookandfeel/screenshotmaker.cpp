/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "screenshotmaker.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QUrl>

ScreenshotRequest::ScreenshotRequest(const QString &objectPath, QObject *parent)
    : QObject(parent)
{
    QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.portal.Desktop"),
                                          objectPath,
                                          QStringLiteral("org.freedesktop.portal.Request"),
                                          QStringLiteral("Response"),
                                          this,
                                          SLOT(onResponse(uint, QVariantMap)));
}

void ScreenshotRequest::onResponse(uint result, const QVariantMap &data)
{
    if (result != 0) {
        Q_EMIT taken(QUrl());
    } else {
        Q_EMIT taken(QUrl::fromUserInput(data[QLatin1String("uri")].toString()));
    }

    deleteLater();
}

ScreenshotMaker::ScreenshotMaker(QObject *parent)
    : QObject(parent)
{
}

void ScreenshotMaker::take()
{
    const QString parentWindow;
    const QVariantMap options{
        {QStringLiteral("interactive"), true},
    };

    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                          QStringLiteral("/org/freedesktop/portal/desktop"),
                                                          QStringLiteral("org.freedesktop.portal.Screenshot"),
                                                          QStringLiteral("Screenshot"));
    message << parentWindow << options;

    auto watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();

        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            qWarning() << "Failed to take a screenshot:" << reply.error();
            return;
        }

        auto request = new ScreenshotRequest(reply.value().path(), this);
        connect(request, &ScreenshotRequest::taken, this, &ScreenshotMaker::accepted, Qt::SingleShotConnection);
    });
}

#include "moc_screenshotmaker.cpp"

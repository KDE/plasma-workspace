/*
    SPDX-FileCopyrightText: 2006 Lukas Tinkl <ltinkl@suse.cz>
    SPDX-FileCopyrightText: 2008 Lubos Lunak <l.lunak@suse.cz>
    SPDX-FileCopyrightText: 2009 Ivo Anjo <knuckles@gmail.com>
    SPDX-FileCopyrightText: 2020 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "freespacenotifier.h"
#include "freespacenotifier_logging.h"

#include <KNotification>
#include <KNotificationJobUiDelegate>

#include <KIO/ApplicationLauncherJob>
#include <KIO/OpenUrlJob>

#include <QStorageInfo>
#include <QtConcurrent>

#include <QCoroFuture>
#include <QCoroTask>

#include <chrono>

#include "settings.h"

FreeSpaceNotifier::FreeSpaceNotifier(const QString &path, const KLocalizedString &notificationText, QObject *parent)
    : QObject(parent)
    , m_path(path)
    , m_notificationText(notificationText)
{
    connect(&m_timer, &QTimer::timeout, this, &FreeSpaceNotifier::checkFreeDiskSpace);
    m_timer.start(std::chrono::minutes(1));
}

FreeSpaceNotifier::~FreeSpaceNotifier()
{
    if (m_notification) {
        m_notification->close();
    }
}

void FreeSpaceNotifier::checkFreeDiskSpace()
{
    if (!FreeSpaceNotifierSettings::enableNotification()) {
        // do nothing if notifying is disabled;
        // also stop the timer that probably got us here in the first place
        m_timer.stop();
        return;
    }

    if (m_checking) {
        qCWarning(FSN) << "Obtaining storage info is taking a long while for" << m_path;
        return;
    }
    m_checking = true;

    // Load the QStorageInfo in a co-routine in case the filesystem is having performance issues.
    auto future = QtConcurrent::run([path = m_path]() -> std::optional<QStorageInfo> {
        QStorageInfo info(path);
        if (!info.isValid()) {
            qCWarning(FSN) << "Failed to obtain storage info for" << path;
            return {};
        }
        if (!info.isReady()) {
            qCWarning(FSN) << "Storage info is not ready for" << path;
            return {};
        }
        return info;
    });
    QCoro::connect(std::move(future), this, [this](const auto &optionalInfo) {
        m_checking = false;
        if (!optionalInfo.has_value()) {
            qCDebug(FSN) << "Empty QStorageInfo for" << m_path;
            return;
        }
        const QStorageInfo &info = optionalInfo.value();
        if (info.isReadOnly()) {
            qCDebug(FSN) << "Not checking for free space for read only mount point" << m_path;
            return;
        }

        const int limit = FreeSpaceNotifierSettings::minimumSpace(); // MiB
        const qint64 avail = info.bytesAvailable() / (1024 * 1024); // to MiB
        qCDebug(FSN) << "Available MiB for" << m_path << ":" << avail;

        if (avail >= limit) {
            if (m_notification) {
                m_notification->close();
            }
            return;
        }

        const int availPercent = int(100 * info.bytesAvailable() / info.bytesTotal());
        const QString text = m_notificationText.subs(avail).subs(availPercent).toString();
        qCDebug(FSN) << "Available percentage for" << m_path << ":" << availPercent;

        // Make sure the notification text is always up to date whenever we checked free space
        if (m_notification) {
            m_notification->setText(text);
        }

        // User freed some space, warn if it goes low again
        if (m_lastAvail > -1 && avail > m_lastAvail) {
            m_lastAvail = avail;
            return;
        }

        // Always warn the first time or when available space dropped to half of the previous time
        const bool warn = (m_lastAvail < 0 || avail < m_lastAvail / 2);
        if (!warn) {
            return;
        }

        m_lastAvail = avail;

        if (!m_notification) {
            m_notification = new KNotification(QStringLiteral("freespacenotif"));
            m_notification->setComponentName(QStringLiteral("freespacenotifier"));
            m_notification->setText(text);

            auto filelight = filelightService();
            if (filelight) {
                auto filelightAction = m_notification->addAction(i18n("Open in Filelight"));
                connect(filelightAction, &KNotificationAction::activated, this, [this] {
                    exploreDrive();
                });
            } else {
                // Do we really want the user opening Root in a file manager?
                auto fileManagerAction = m_notification->addAction(i18n("Open in File Manager"));
                connect(fileManagerAction, &KNotificationAction::activated, this, [this] {
                    exploreDrive();
                });
            }

            // TODO once we have "configure" action support in KNotification, wire it up instead of a button
            auto configureAction = m_notification->addAction(i18n("Configure Warning…"));
            connect(configureAction, &KNotificationAction::activated, this, [this] {
                Q_EMIT configureRequested();
            });

            connect(m_notification, &KNotification::closed, this, &FreeSpaceNotifier::onNotificationClosed);
            m_notification->sendEvent();
        }
    });
}

KService::Ptr FreeSpaceNotifier::filelightService() const
{
    return KService::serviceByDesktopName(QStringLiteral("org.kde.filelight"));
}

void FreeSpaceNotifier::exploreDrive()
{
    auto service = filelightService();
    if (!service) {
        auto *job = new KIO::OpenUrlJob({QUrl::fromLocalFile(m_path)});
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
        job->start();
        return;
    }

    auto *job = new KIO::ApplicationLauncherJob(service);
    job->setUrls({QUrl::fromLocalFile(m_path)});
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->start();
}

void FreeSpaceNotifier::onNotificationClosed()
{
    // warn again if constantly below limit for too long
    if (!m_lastAvailTimer) {
        m_lastAvailTimer = new QTimer(this);
        connect(m_lastAvailTimer, &QTimer::timeout, this, &FreeSpaceNotifier::resetLastAvailable);
    }
    m_lastAvailTimer->start(std::chrono::hours(1));
}

void FreeSpaceNotifier::resetLastAvailable()
{
    m_lastAvail = -1;
    m_lastAvailTimer->deleteLater();
    m_lastAvailTimer = nullptr;
}

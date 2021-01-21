/* This file is part of the KDE Project
   Copyright (c) 2006 Lukas Tinkl <ltinkl@suse.cz>
   Copyright (c) 2008 Lubos Lunak <l.lunak@suse.cz>
   Copyright (c) 2009 Ivo Anjo <knuckles@gmail.com>
   Copyright (c) 2020 Kai Uwe Broulik <kde@broulik.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "freespacenotifier.h"

#include <KNotification>
#include <KNotificationJobUiDelegate>

#include <KIO/ApplicationLauncherJob>
#include <KIO/FileSystemFreeSpaceJob>
#include <KIO/OpenUrlJob>

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

    auto *job = KIO::fileSystemFreeSpace(QUrl::fromLocalFile(m_path));
    connect(job, &KIO::FileSystemFreeSpaceJob::result, this, [this](KIO::Job *job, KIO::filesize_t size, KIO::filesize_t available) {
        if (job->error()) {
            return;
        }

        const int limit = FreeSpaceNotifierSettings::minimumSpace(); // MiB
        const qint64 avail = available / (1024 * 1024); // to MiB

        if (avail >= limit) {
            if (m_notification) {
                m_notification->close();
            }
            return;
        }

        const int availPercent = int(100 * available / size);
        const QString text = m_notificationText.subs(avail).subs(availPercent).toString();

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

            QStringList actions = {i18n("Configure Warning...")};

            auto filelight = filelightService();
            if (filelight) {
                actions.prepend(i18n("Open in Filelight"));
            } else {
                // Do we really want the user opening Root in a file manager?
                actions.prepend(i18n("Open in File Manager"));
            }

            m_notification->setActions(actions);

            connect(m_notification, QOverload<uint>::of(&KNotification::activated), this, [this](uint actionId) {
                if (actionId == 1) {
                    exploreDrive();
                    // TODO once we have "configure" action support in KNotification, wire it up instead of a button
                } else if (actionId == 2) {
                    emit configureRequested();
                }
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

// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "applicationintegration.h"
#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>

ApplicationIntegration::ApplicationIntegration(QObject *parent)
    : QObject(parent)
{
    const auto services = KApplicationTrader::queryByMimeType(QStringLiteral("text/calendar"));

    if (!services.isEmpty()) {
        const KService::Ptr app = services.first();

        if (app->desktopEntryName() == QLatin1String("org.kde.korganizer") || app->desktopEntryName() == QLatin1String("org.kde.kalendar")) {
            m_calendarService = app;
        }
    }
}

bool ApplicationIntegration::calendarInstalled() const
{
    return m_calendarService != nullptr;
}

void ApplicationIntegration::launchCalendar() const
{
    Q_ASSERT(m_calendarService);

    auto job = new KIO::ApplicationLauncherJob(m_calendarService);
    job->start();
}

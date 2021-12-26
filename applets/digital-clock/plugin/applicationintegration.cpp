// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "applicationintegration.h"
#include <KIO/ApplicationLauncherJob>

ApplicationIntegration::ApplicationIntegration(QObject *parent)
    : QObject(parent)
    , m_calendarService(KService::serviceByDesktopName(QStringLiteral("org.kde.korganizer")))
{
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

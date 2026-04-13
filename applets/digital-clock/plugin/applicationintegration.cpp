// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "applicationintegration.h"
#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KSycoca>

ApplicationIntegration::ApplicationIntegration(QObject *parent)
    : QObject(parent)
    , m_calendarService(KApplicationTrader::preferredService(QStringLiteral("text/calendar")))
{
    connect(KSycoca::self(), &KSycoca::databaseChanged, this, &ApplicationIntegration::refresh);
}

bool ApplicationIntegration::calendarInstalled() const
{
    return m_calendarService != nullptr;
}

QString ApplicationIntegration::calendarApplicationName() const
{
    if (!m_calendarService) {
        return {};
    }
    return m_calendarService->name();
}

void ApplicationIntegration::launchCalendar() const
{
    if (!m_calendarService) {
        return;
    }

    auto job = new KIO::ApplicationLauncherJob(m_calendarService);
    job->start();
}

void ApplicationIntegration::refresh()
{
    KService::Ptr newService = KApplicationTrader::preferredService(QStringLiteral("text/calendar"));

    bool wasInstalled = calendarInstalled();
    QString oldName = calendarApplicationName();

    m_calendarService = newService;

    if (wasInstalled != calendarInstalled()) {
        Q_EMIT calendarInstalledChanged();
    }
    if (oldName != calendarApplicationName()) {
        Q_EMIT calendarApplicationNameChanged();
    }
}

#include "moc_applicationintegration.cpp"

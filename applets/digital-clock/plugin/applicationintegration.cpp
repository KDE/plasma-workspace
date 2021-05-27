// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "applicationintegration.h"
#include <KIO/ApplicationLauncherJob>

ApplicationIntegration::ApplicationIntegration(QObject *parent)
    : QObject(parent)
    , m_korganizerService(KService::serviceByDesktopName(QStringLiteral("org.kde.korganizer")))
{
}

bool ApplicationIntegration::korganizerInstalled() const
{
    return m_korganizerService != nullptr;
}

void ApplicationIntegration::launchKorganizer() const
{
    Q_ASSERT(m_korganizerService);

    auto job = new KIO::ApplicationLauncherJob(m_korganizerService);
    job->start();
}

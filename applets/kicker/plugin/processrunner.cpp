/*
    SPDX-FileCopyrightText: 2013 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "processrunner.h"

#include <KIO/ApplicationLauncherJob>
#include <KNotificationJobUiDelegate>
#include <QDebug>

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent)
{
}

ProcessRunner::~ProcessRunner()
{
}

void ProcessRunner::runMenuEditor()
{
    const auto service = KService::serviceByDesktopName(QStringLiteral("org.kde.kmenuedit"));

    if (!service) {
        qWarning() << "Could not find kmenuedit";
        return;
    }

    auto *job = new KIO::ApplicationLauncherJob(service);
    auto *delegate = new KNotificationJobUiDelegate;
    delegate->setAutoErrorHandlingEnabled(true);
    job->setUiDelegate(delegate);
    job->start();
}

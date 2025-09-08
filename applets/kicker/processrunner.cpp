/*
    SPDX-FileCopyrightText: 2013 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "processrunner.h"

#include <KIO/CommandLauncherJob>
#include <KNotificationJobUiDelegate>
#include <KService>

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent)
{
}

ProcessRunner::~ProcessRunner() = default;

void ProcessRunner::runMenuEditor(QString arg)
{
    const auto service = KService::serviceByDesktopName(QStringLiteral("org.kde.kmenuedit"));

    if (!service) {
        qWarning() << "Could not find kmenuedit";
        return;
    }

    if (arg.isEmpty()) {
        arg = QStringLiteral("/"); // If already open, will collapse editor tree
    }

    auto *job = new KIO::CommandLauncherJob(service->exec(), {arg});
    job->setDesktopName(service->desktopEntryName());
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->start();
}

#include "moc_processrunner.cpp"

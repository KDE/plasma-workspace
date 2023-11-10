/*
 SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
 SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QDebug>
#include <QGuiApplication>

#include <KConfigGroup>
#include <KIO/ApplicationLauncherJob>
#include <KService>
#include <KSharedConfig>

#include "debug.h"

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    a.setDesktopSettingsAware(false);
    KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("plasmasessionrestore"), KConfig::NoGlobals);

    QList<KJob *> jobs;
    QEventLoop e;

    for (const QString &groupName : config->groupList()) {
        const QString appId = config->group(groupName).readEntry("appId");
        auto service = KService::serviceByDesktopName(appId);
        if (!service || !service->isValid()) {
            qCDebug(FALLBACK_SESSION_RESTORE) << "skipping " << appId << "no service found.";
            continue;
        }

        if (service->noDisplay()) {
            continue;
        }

        qCDebug(FALLBACK_SESSION_RESTORE) << "launching " << service->name();
        auto job = new KIO::ApplicationLauncherJob(service);

        QObject::connect(job, &KJob::finished, &e, [job, &jobs, &e]() {
            jobs.removeOne(job);
            if (jobs.isEmpty()) {
                e.quit();
            }
        });
        jobs << job;
        job->start();
    }
    if (!jobs.isEmpty()) {
        e.exec();
    }

    return 0;
}

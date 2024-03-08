/*
 SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
 SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>

SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <QDir>
#include <QGuiApplication>

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KIO/ApplicationLauncherJob>
#include <KIO/DesktopExecParser>
#include <KService>
#include <KSharedConfig>

#include "debug.h"
#include "smserversettings.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    a.setDesktopSettingsAware(false);
    a.setApplicationName(u"plasmasessionrestore"_s);

    SMServerSettings ksmserverSettings;
    if (ksmserverSettings.loginMode() == SMServerSettings::emptySession) {
        return EXIT_SUCCESS;
    }

    KSharedConfig::Ptr config = KSharedConfig::openStateConfig();

    QList<KJob *> jobs;
    QEventLoop e;

    // Make a list of all autostart .desktop files for subsequent filtering
    QSet<QString> autoStartFiles;
    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericConfigLocation, QStringLiteral("autostart"), QStandardPaths::LocateDirectory);
    for (const QString &dir : dirs) {
        const QDir d(dir);
        const QFileInfoList files = d.entryInfoList(QStringList() << QStringLiteral("*.desktop"));
        for (const QFileInfo &file : files) {
            autoStartFiles.insert(file.completeBaseName());
        }
    }

    QSet<QString> ksmserverAutostart;
    // make a list of all .desktop files that ksmserver is going to launch
    // then list

    KConfigGroup sessionGroup;

    if (ksmserverSettings.loginMode() == SMServerSettings::restorePreviousLogout) {
        sessionGroup = ksmserverSettings.config()->group(u"Session: saved at previous logout"_s);
    } else if (ksmserverSettings.loginMode() == SMServerSettings::restoreSavedSession) {
        sessionGroup = ksmserverSettings.config()->group(u"Session: saved by user"_s);
    }

    int count = sessionGroup.readEntry("count", 0);
    if (count < 0) {
        qFatal() << "invalid config";
    }
    for (int i = 0; i < count; i++) {
        const QString app = sessionGroup.readEntry(QStringLiteral("program%1").arg(QString::number(i + 1)), QString());

        auto apps = KApplicationTrader::query([&app](const KService::Ptr &service) {
            const QString binary = KIO::DesktopExecParser::executablePath(service->exec());
            return !service->noDisplay() && !binary.isEmpty() && app.endsWith(binary);
        });
        if (!apps.isEmpty()) {
            ksmserverAutostart.insert(apps.first()->desktopEntryName());
        }
    }

    // finally look through our list and do the actual launching
    for (const QString &groupName : config->groupList()) {
        const QString appId = config->group(groupName).readEntry("appId");
        auto service = KService::serviceByMenuId(appId);
        if (!service || !service->isValid()) {
            qCDebug(FALLBACK_SESSION_RESTORE) << "skipping " << appId << "no service found.";
            continue;
        }

        if (service->noDisplay()) {
            continue;
        }
        if (autoStartFiles.contains(service->desktopEntryName())) {
            qCDebug(FALLBACK_SESSION_RESTORE) << "skipping " << appId << "already started by autostart.";
            continue;
        }
        if (ksmserverAutostart.contains(service->desktopEntryName())) {
            qCDebug(FALLBACK_SESSION_RESTORE) << "skipping " << appId << "already started by ksmserver.";
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

    return EXIT_SUCCESS;
}

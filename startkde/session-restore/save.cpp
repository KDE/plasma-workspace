/*
 SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
 SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>

SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <chrono>

#include <QEventLoop>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QThreadPool>
#include <QTimer>
#include <cstdlib>

#include <qpa/qplatformnativeinterface.h>

#include <wayland-client-core.h>

#include <abstracttasksmodel.h>
#include <windowtasksmodel.h>

#include <KConfigGroup>
#include <KSharedConfig>

#include "debug.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    QGuiApplication::setDesktopFileName(u"plasma-fallback-session-save"_s);
    QGuiApplication a(argc, argv);
    a.setApplicationName(u"plasmasessionrestore"_s);

    a.setDesktopSettingsAware(false);

    TaskManager::WindowTasksModel tasksModel(&a);

    //
    auto native = qGuiApp->platformNativeInterface();
    auto display = static_cast<struct ::wl_display *>(native->nativeResourceForIntegration("wl_display"));
    if (display) {
        // fetch all window IDs
        wl_display_roundtrip(display);
        // fetch all window properties
        wl_display_roundtrip(display);
    }

    KSharedConfig::Ptr config = KSharedConfig::openStateConfig();

    const QStringList groupList = config->groupList();
    for (const QString &group : groupList) {
        config->deleteGroup(group);
    }

    QSet<QString> seenAppIds;
    for (int i = 0; i < tasksModel.rowCount(); ++i) {
        const QModelIndex index = tasksModel.index(i, 0);
        QString appId = tasksModel.data(index, TaskManager::AbstractTasksModel::AppId).toString();

        if (seenAppIds.contains(appId)) {
            qCDebug(FALLBACK_SESSION_RESTORE) << "Skipping duplicate" << appId;
            continue;
        }

        static const auto excludeApps = []() -> QStringList {
            KSharedConfig::Ptr config = KSharedConfig::openConfig(u"ksmserverrc"_s);
            KConfigGroup generalGroup(config, u"General"_s);
            return generalGroup.readEntry("excludeApps").split(QRegularExpression(u"[,:]"_s), Qt::SkipEmptyParts);
        }();
        if (excludeApps.contains(appId)) {
            qCDebug(FALLBACK_SESSION_RESTORE) << "Excluding" << appId;
            continue;
        }

        auto group = config->group(QString::number(i));
        qCDebug(FALLBACK_SESSION_RESTORE) << "Saving" << group.name() << appId;
        group.writeEntry("appId", appId);
        seenAppIds.insert(appId);
    }

    config->sync();

    // WaylandtasksModels runs threads to fetch icons, prevent crashes when they construct QPixmaps but the QGuiApp is already gone
    QThreadPool::globalInstance()->waitForDone();

    return EXIT_SUCCESS;
}

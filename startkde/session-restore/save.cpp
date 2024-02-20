/*
 SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
 SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>

SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <chrono>

#include <QEventLoop>
#include <QGuiApplication>
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

    for (int i = 0; i < tasksModel.rowCount(); ++i) {
        const QModelIndex index = tasksModel.index(i, 0);
        QString appId = tasksModel.data(index, TaskManager::AbstractTasksModel::AppId).toString();
        auto group = config->group(QString::number(i));
        qCDebug(FALLBACK_SESSION_RESTORE) << "Saving" << group.name() << appId;
        group.writeEntry("appId", appId);
    }

    config->sync();
    return EXIT_SUCCESS;
}

#include "save.moc"

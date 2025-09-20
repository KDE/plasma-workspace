/*
    SPDX-FileCopyrightText: Ⓒ 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "libnotificationmanager-config.h"

#include "notifyrcpaths.h"

#include <QFile>
#include <QStandardPaths>
#include <QStringList>

#if HAVE_FLATPAK
#include <flatpak.h>
#endif

using namespace Qt::Literals;

#if HAVE_FLATPAK
static void appendFlatpakDirs(FlatpakInstallation *install, QStringList &dirs)
{
    if (!install) {
        return;
    }

    g_autoptr(GPtrArray) installedApps = flatpak_installation_list_installed_refs(install, nullptr, nullptr);
    if (!installedApps) {
        return;
    }
    for (uint i = 0; i < installedApps->len; i++) {
        auto ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(installedApps, i));
        dirs.push_back(QString::fromLocal8Bit(flatpak_installed_ref_get_deploy_dir(ref)) + "/files/share/knotifications6/"_L1);
    }
}
#endif

QStringList NotifyRcPaths::allSearchPaths()
{
    // Search for notifyrc files in `/knotifications6` folders first, and in `/knotifications5` for compatibility with KF5 applications last, after Flatpaks
    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, u"knotifications6"_s, QStandardPaths::LocateDirectory);

#if HAVE_FLATPAK
    g_autoptr(GPtrArray) sysInstalls = flatpak_get_system_installations(nullptr, nullptr);
    if (sysInstalls) {
        for (uint i = 0; i < sysInstalls->len; ++i) {
            auto sysInstall = FLATPAK_INSTALLATION(g_ptr_array_index(sysInstalls, i));
            appendFlatpakDirs(sysInstall, dirs);
        }
    }
    g_autoptr(FlatpakInstallation) userInstall = flatpak_installation_new_user(nullptr, nullptr);
    appendFlatpakDirs(userInstall, dirs);
#endif

    dirs += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, u"knotifications5"_s, QStandardPaths::LocateDirectory);
    return dirs;
}

QString NotifyRcPaths::locate(QStringView desktopName)
{
    const auto dirs = NotifyRcPaths::allSearchPaths();
    for (const auto &dir : dirs) {
        if (const QString fileName = dir + '/'_L1 + desktopName + ".notifyrc"_L1; QFile::exists(fileName)) {
            return fileName;
        }
    }

    return {};
}

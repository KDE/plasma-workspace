/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QUrl>

#include <KConfigGroup>
#include <KDesktopFile>
#include <KService>

namespace JumpList
{

inline static QUrl tryDecodeApplicationsUrl(const QUrl &launcherUrl)
{
    if (launcherUrl.isValid() && launcherUrl.scheme() == QLatin1String("applications")) {
        const KService::Ptr service = KService::serviceByMenuId(launcherUrl.path());

        if (service) {
            return QUrl::fromLocalFile(service->entryPath());
        }
    }

    return launcherUrl;
}

inline static QStringList applicationCategories(const QUrl &launcherUrl)
{
    const QUrl desktopEntryUrl = tryDecodeApplicationsUrl(launcherUrl);

    if (!desktopEntryUrl.isValid() || !desktopEntryUrl.isLocalFile() || !KDesktopFile::isDesktopFile(desktopEntryUrl.toLocalFile())) {
        return QStringList();
    }

    KDesktopFile desktopFile(desktopEntryUrl.toLocalFile());

    // Since we can't have dynamic jump list actions, at least add the user's "Places" for file managers.
    return desktopFile.desktopGroup().readXdgListEntry(QStringLiteral("Categories"));
}
}

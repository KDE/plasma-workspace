/*
    SPDX-FileCopyrightText: 2016 Ivan Cukic <ivan.cukic@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDebug>
#include <QStringList>
#include <QUrl>

#include "tasktools.h"

namespace TaskManager
{
#define NULL_UUID "00000000-0000-0000-0000-000000000000"

inline static bool isValidLauncherUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isValid()) {
        return false;
    }

    if (url.scheme() == QLatin1String("preferred")) {
        return !defaultApplication(url).isEmpty();
    }

    if (!url.isLocalFile() && url.scheme() != QLatin1String("applications") && url.scheme() != QLatin1String("preferred")) {
        return false;
    }

    return true;
}

inline static std::pair<QUrl, QStringList> deserializeLauncher(const QString &serializedLauncher)
{
    QStringList activities;
    QUrl url(serializedLauncher);

    // The storage format is: [list of activity ids]\nURL
    // The activity IDs list can not be empty, it at least needs
    // to contain the nulluuid.
    // If parsing fails, we are considering the serialized launcher
    // to not have the activities array -- to have the old format
    if (serializedLauncher.startsWith('[')) {
        // It seems we have the activity specifier in the launcher
        const auto activitiesBlockEnd = serializedLauncher.indexOf("]\n");

        if (activitiesBlockEnd != -1) {
            activities = serializedLauncher.mid(1, activitiesBlockEnd - 1).split(",", Qt::SkipEmptyParts);

            if (!activities.isEmpty()) {
                url = QUrl(serializedLauncher.mid(activitiesBlockEnd + 2));
            }
        }
    }

    // If the activities array is empty, this means that this launcher
    // needs to be on all activities
    if (activities.isEmpty()) {
        activities = QStringList({NULL_UUID});
    }

    return std::make_pair(url, activities);
}

} // namespace TaskManager

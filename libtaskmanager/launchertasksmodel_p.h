/********************************************************************
Copyright 2016  Ivan Cukic <ivan.cukic@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#ifndef LAUNCHERTASKSMODEL_P_H
#define LAUNCHERTASKSMODEL_P_H

#include <QDebug>
#include <QStringList>
#include <QUrl>

namespace TaskManager
{
#define NULL_UUID "00000000-0000-0000-0000-000000000000"

inline static bool isValidLauncherUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isValid()) {
        return false;
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

#endif // LAUNCHERTASKSMODEL_P_H

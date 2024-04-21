/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "applicationdata_p.h"

#include <QIcon>

#include <KService>

QHash<QString, QPair<QString, QString>> ApplicationData::m_applicationInfo;

void ApplicationData::populateApplicationData(const QString &name, QString *prettyName, QString *icon)
{
    if (auto it = m_applicationInfo.constFind(name); it != m_applicationInfo.cend()) {
        const auto &info = it.value();
        *prettyName = info.first;
        *icon = info.second;
    } else {
        KService::Ptr service = KService::serviceByStorageId(name + QLatin1String(".desktop"));
        if (service) {
            *prettyName = service->name(); // cannot be null
            *icon = service->icon();

            m_applicationInfo.insert(name, std::pair(*prettyName, *icon));
        } else {
            *prettyName = name;
            *icon = name.section(QLatin1Char('/'), -1).toLower();
            if (!QIcon::hasThemeIcon(*icon)) {
                icon->clear();
            }
        }
    }
}

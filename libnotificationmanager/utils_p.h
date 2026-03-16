/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QModelIndex>
#include <QString>

class QAbstractItemModel;

namespace NotificationManager::Utils
{
QHash<int, QByteArray> roleNames();

QString processNameFromPid(uint pid);

QString desktopEntryFromPid(uint pid);

/**
 * Map a given index to a given source model
 * @param The index
 * @param sourceModel The source model to map to. When nullptr, will map until it finds a model that isn't a proxy model.
 * @return The mapped index
 */
QModelIndex mapToModel(const QModelIndex &idx, const QAbstractItemModel *sourceModel);

bool isDBusMaster();

} // namespace NotificationManager::Utils

/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QModelIndex>
#include <QString>

class QAbstractItemModel;
class QDBusConnection;

namespace NotificationManager
{
namespace Utils
{
QHash<int, QByteArray> roleNames();

QString processNameFromPid(uint pid);

QString desktopEntryFromPid(uint pid);

QModelIndex mapToModel(const QModelIndex &idx, const QAbstractItemModel *sourceModel);

bool isDBusMaster();

} // namespace Utils

} // namespace NotificationManager

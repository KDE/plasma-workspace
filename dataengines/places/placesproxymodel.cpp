/*
 * Copyright 2014 Marco Martin <mart@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "placesproxymodel.h"

#include <QDebug>
#include <QIcon>

#include <KDiskFreeSpaceInfo>

PlacesProxyModel::PlacesProxyModel(QObject *parent, KFilePlacesModel *model)
    : QIdentityProxyModel(parent)
    , m_placesModel(model)
{
    setSourceModel(model);
}

QHash<int, QByteArray> PlacesProxyModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(Qt::DisplayRole, "display");
    roles.insert(Qt::DecorationRole, "decoration");
    roles.insert(KFilePlacesModel::UrlRole, "url");
    roles.insert(KFilePlacesModel::HiddenRole, "hidden");
    roles.insert(KFilePlacesModel::SetupNeededRole, "setupNeeded");
    roles.insert(KFilePlacesModel::FixedDeviceRole, "fixedDevice");
    roles.insert(KFilePlacesModel::CapacityBarRecommendedRole, "capacityBarRecommended");
    roles.insert(PlaceIndexRole, "placeIndex");
    roles.insert(IsDeviceRole, "isDevice");
    roles.insert(PathRole, "path");
    roles.insert(SizeRole, "size");
    roles.insert(UsedRole, "used");
    roles.insert(AvailableRole, "available");
    return roles;
}

QVariant PlacesProxyModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case PlaceIndexRole:
        return index.row();
    case IsDeviceRole:
        return m_placesModel->deviceForIndex(index).isValid();
    case PathRole:
        return m_placesModel->url(index).path();

    case SizeRole: {
        const QString path = m_placesModel->url(index).path();
        KDiskFreeSpaceInfo info = KDiskFreeSpaceInfo::freeSpaceInfo(path);
        return info.size();
    }
    case UsedRole: {
        const QString path = m_placesModel->url(index).path();
        KDiskFreeSpaceInfo info = KDiskFreeSpaceInfo::freeSpaceInfo(path);
        return info.used();
    }
    case AvailableRole: {
        const QString path = m_placesModel->url(index).path();
        KDiskFreeSpaceInfo info = KDiskFreeSpaceInfo::freeSpaceInfo(path);
        return info.used();
    }
    default:
        return QIdentityProxyModel::data(index, role);
    }
}
